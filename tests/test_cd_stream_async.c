/* The production asynchronous CD command sequencer, with only MMIO replaced.
 * A command is deliberately never acknowledged until the test advances the
 * device. Every poll must return while CMOK/ESEL are absent. */
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static unsigned short hirq,cr1,cr2,cr3,cr4;
#define JO_CD_HIRQ hirq
#define JO_CD_CR1 cr1
#define JO_CD_CR2 cr2
#define JO_CD_CR3 cr3
#define JO_CD_CR4 cr4
#define JO_CD_HIRQ_CMOK 0x0001
#define JO_CD_HIRQ_ESEL 0x0040
#define JO_CD_STATUS_REJECT 0xff
#define JO_CD_COMMAND_PLAY 0x10
#define JO_CD_COMMAND_SEEK 0x11
#define JO_CD_COMMAND_SET_DEVICE_CONNECTION 0x30
#define JO_CD_COMMAND_SET_FILTER_RANGE 0x40
#define JO_CD_COMMAND_SET_FILTER_CONNECTION 0x46
#define JO_CD_COMMAND_RESET_SELECTOR 0x48
#define JO_CD_CR1_VALUE(c,p) ((unsigned short)(((c)<<8)|((p)&255u)))
#define JO_CD_POSITION_HIGH(p) (((p)>>16)&255u)
#define JO_CD_POSITION_LOW(p) ((p)&65535u)
#define JO_CD_POSITION_FAD(p) (0x800000u|(p))
#define JO_CD_POSITION_PAUSE 0xffffffu
#define JO_CD_FILTER_TRUE_CONNECTION 1u
#define JO_CD_PARTITION 0u
#define JO_CD_FILTER 0u
#define JO_CD_TIMEOUT 100u

static unsigned __jo_cd_play_next_fad,__jo_cd_play_end_fad;
static bool __jo_cd_stream_owner,__jo_cd_initialized=true;

#include "internal/cd_stream_async.h"

static void reset(void)
{
    memset(&__jo_cd_async,0,sizeof __jo_cd_async);
    __jo_cd_async_errors=0;__jo_cd_play_next_fad=__jo_cd_play_end_fad=0;
    __jo_cd_stream_owner=false;hirq=JO_CD_HIRQ_CMOK;cr1=cr2=cr3=cr4=0;
}
static void acknowledge(bool selector)
{
    cr1=0x0100;hirq|=JO_CD_HIRQ_CMOK;
    if(selector)hirq|=JO_CD_HIRQ_ESEL;
}
static void expect_command(unsigned command)
{
    assert((cr1>>8)==command);
    unsigned short before[4]={cr1,cr2,cr3,cr4};
    for(unsigned i=0;i<100;++i)assert(jo_cd_stream_async_poll()==0);
    assert(cr1==before[0]&&cr2==before[1]&&cr3==before[2]&&cr4==before[3]);
}
static void complete_start(unsigned fad,unsigned count)
{
    static const unsigned commands[]={0x11,0x48,0x40,0x46,0x30,0x10};
    for(unsigned i=0;i<6;++i){
        assert(jo_cd_stream_async_poll()==0);
        expect_command(commands[i]);
        if(i==2){assert(cr2==(fad&65535u));assert(cr4==(count&65535u));}
        if(i==5){assert(cr2==(fad&65535u));assert(cr4==(count&65535u));}
        acknowledge(false);
        if(i>=1&&i<=4){
            /* CMOK alone cannot authorize the next selector command. */
            assert(jo_cd_stream_async_poll()==0);
            assert(__jo_cd_async.inflight);
            acknowledge(true);
        }
    }
    assert(jo_cd_stream_async_poll()==1);
    assert(!__jo_cd_async.active&&!__jo_cd_async.inflight);
    assert(__jo_cd_stream_owner);
    assert(__jo_cd_play_next_fad==fad&&__jo_cd_play_end_fad==fad+count);
}
int main(void)
{
    reset();assert(jo_cd_stream_async_begin(1000,5000)==0);
    complete_start(1000,5000);
    /* Stop also waits across polls, leaving the caller free to run the game. */
    jo_cd_stream_async_stop();assert(!__jo_cd_stream_owner);
    assert(jo_cd_stream_async_poll()==0);expect_command(0x11);acknowledge(false);
    assert(jo_cd_stream_async_poll()==0);expect_command(0x48);acknowledge(true);
    assert(jo_cd_stream_async_poll()==1);
    assert(!__jo_cd_stream_owner&&__jo_cd_play_end_fad==0);
    /* A new request arriving with a prior command in flight cannot overwrite
     * its command registers or consume sectors from the abandoned cue. */
    reset();assert(jo_cd_stream_async_begin(2000,100)==0);
    assert(jo_cd_stream_async_poll()==0);expect_command(0x11);
    jo_cd_stream_async_stop();assert(jo_cd_stream_async_begin(3000,900)==0);
    expect_command(0x11);acknowledge(false);
    complete_start(3000,900);
    /* A rejected command remains an explicit error and loses ownership. */
    reset();assert(jo_cd_stream_async_begin(4000,500)==0);
    assert(jo_cd_stream_async_poll()==0);cr1=0xff00;hirq|=JO_CD_HIRQ_CMOK;
    assert(jo_cd_stream_async_poll()==-1);
    assert(__jo_cd_async_errors==1&&!__jo_cd_stream_owner);
    /* Synchronous callers can drain a completed deferred job; a missing
     * acknowledgement is bounded by the original synchronous timeout. */
    reset();assert(jo_cd_stream_async_flush()==0);
    assert(jo_cd_stream_async_begin(4000,500)==0);
    assert(jo_cd_stream_async_flush()==-1);
    assert(__jo_cd_async.inflight);
    puts("PASS: nonblocking CMOK/ESEL waits, ordered commands, cue replacement, stop and explicit rejection");
}
