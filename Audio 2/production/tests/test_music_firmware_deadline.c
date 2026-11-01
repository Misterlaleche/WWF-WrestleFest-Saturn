/* Exercise the real firmware service body. Only memory-mapped transport and
 * the DSP hardware entry points are supplied by this host fixture. */
#define _GNU_SOURCE
#include "wf_scsp_sfx.h"
#include "wf_scsp_ym.h"
#include "wf_scsp_dsp.h"
#include "wf_native_driver.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

static wf_native_mail_t test_mail;
static uint16_t test_ring[16], test_events[256][4], test_clock;
static unsigned test_record_reads;
static volatile uint16_t *test_record(unsigned index)
{
    ++test_record_reads;return test_events[index&255u];
}
/* Hardware timing is independent of host CPU speed. Probe05 measured a full
 * initial decode at294 Fs; reproducing that cost exposes deadline crossing. */
static bool timed_prepare_step(const wf_scsp_sfx_t *s,wf_sfx_prepared_t *p,unsigned bank,unsigned id,uint16_t *filled,unsigned budget)
{
    uint16_t before=*filled;
    bool ok=wf_scsp_sfx_prepare_step(s,p,bank,id,filled,budget);
    if(ok)test_clock=(uint16_t)(test_clock+(294u*(*filled-before)+255u)/256u);
    return ok;
}
static void timed_work(wf_scsp_sfx_t *s)
{
    uint32_t before=0,after=0;
    for(unsigned v=0;v<4;v++)before+=s->pcm[v].write_at;
    wf_scsp_sfx_work(s);
    for(unsigned v=0;v<4;v++)after+=s->pcm[v].write_at;
    test_clock=(uint16_t)(test_clock+(148u*(after-before)+127u)/128u);
}
#define WF_NATIVE_MAIL_OFFSET ((uintptr_t)&test_mail)
#define WF_MUSIC_RING_HEADER ((uintptr_t)test_ring)
#define WF_MUSIC_RING ((uintptr_t)test_events)
#define WF_MUSIC_RING_EVENT(index) test_record(index)
#define WF_DSP_CLOCK ((uintptr_t)&test_clock)
#define wf_scsp_sfx_prepare_step timed_prepare_step
#define wf_scsp_sfx_work timed_work
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
#include "../src/wf_music_firmware.c"
#pragma GCC diagnostic pop
#undef wf_scsp_sfx_prepare_step
#undef wf_scsp_sfx_work

bool wf_scsp_dsp_load(wf_sfx_write16_fn write, void *user) { (void)write; (void)user; return true; }
void wf_scsp_dsp_clock_run(void) {}
bool wf_scsp_dsp_voice(unsigned voice, bool start, unsigned volume, void *user)
{ (void)voice; (void)start; (void)volume; (void)user; return true; }

static void reset_fixture(unsigned kind, uint16_t when, uint16_t now)
{
    memset(&test_mail, 0, sizeof test_mail);
    memset(test_ring, 0, sizeof test_ring);
    memset(test_events, 0, sizeof test_events);
    memset(&wf_sfx_engine, 0, sizeof wf_sfx_engine);
    memset(&wf_ym_engine, 0, sizeof wf_ym_engine);
    memset((void *)wf_music_timing, 0, sizeof wf_music_timing);
    wf_sfx_engine.write16 = write16;
    wf_sfx_engine.pcm_control = wf_scsp_dsp_voice;
    wf_ym_engine.write16 = write16;
    generation = 7;
    test_ring[0] = 1;
    test_ring[2] = test_ring[5] = generation;
    test_ring[3] = WF_NATIVE_READY;
    test_mail.state = WF_NATIVE_READY;
    test_mail.fm_active = 0x55;
    test_events[0][0] = when;
    test_events[0][1] = (uint16_t)(kind << 8);
    test_events[0][2] = kind == 1 ? 0x80 : (kind == 2 ? 1 : 7);
    test_events[0][3] = generation;
    test_clock = now;
    clock_frames = last_poll = 0;
    last_dsp = now;
    oki_pending = -1;
    music_bank = 0;
    prepared_music_valid = false;prepared_music_frames=0;
    test_record_reads=0;
}

static void hard_deadline_is_counted_once_and_never_replayed(void)
{
    for (unsigned kind=1; kind<=4; ++kind) {
        if (kind == 3) continue;
        reset_fixture(kind, 65000, 1670); /* 2206 Fs late across clock wrap. */
        music_service();
        assert(test_ring[4] == 1 && "one late event was counted twice");
        if(WF_MUSIC_TIMING){
            assert(wf_music_timing[kind]==2206&&wf_music_timing[5+kind]==1);
            assert(wf_music_timing[17]==kind&&wf_music_timing[18]==65000&&wf_music_timing[19]==1670);
        }
        assert(wf_ym_engine.fault == 2 && "hard-late event left firmware healthy");
        assert(test_ring[1] == 0 && test_mail.fm_active == 0x55);
        assert(oki_pending == -1 && music_bank == 0);
        music_service();
        assert(test_ring[4] == 1 && wf_ym_engine.fault == 2 && test_ring[1] == 0);
        for(unsigned voice=0;voice<4;voice++)wf_sfx_engine.pcm[voice].playing=1;
        for(unsigned voice=0;voice<3;voice++)wf_sfx_engine.sequence[voice].active=1;
        fail_stop(0);
        assert(test_mail.state==WF_NATIVE_FAULT && test_ring[3]==WF_NATIVE_FAULT);
        assert(test_mail.reserved[0]==0xb040 && test_mail.fm_active==0);
        for(unsigned voice=0;voice<4;voice++)assert(!wf_sfx_engine.pcm[voice].playing);
        for(unsigned voice=0;voice<3;voice++)assert(!wf_sfx_engine.sequence[voice].active);
        wf_ym_engine.fault=1;wf_sfx_engine.fault=1;
        fail_stop(7);
        assert(test_mail.reserved[0]==0xb040 && test_ring[4]==1 && test_ring[1]==0);
    }
}

static void existing_soft_and_hard_boundaries_are_preserved(void)
{
    const uint16_t due[] = {256, 257, 2205};
    const uint16_t late[] = {0, 1, 1};
    for (unsigned i=0; i<3; ++i) {
        reset_fixture(4, 0, due[i]);
        music_service();
        assert(test_ring[4] == late[i] && test_ring[1] == 1);
        assert(wf_ym_engine.fault == 0 && test_mail.fm_active == 7);
    }
    reset_fixture(4, 100, 99);
    music_service();
    assert(test_ring[1] == 0 && test_ring[4] == 0 && !wf_ym_engine.fault);
    reset_fixture(4, 0, 2306);
    test_events[0][3] = 6;
    music_service();
    assert(test_ring[1] == 1 && test_ring[4] == 0 && !wf_ym_engine.fault);
}

static bool cache_test_write(uint32_t address,uint16_t value,void *user)
{
    (void)value;(void)user;
    return address>=WF_SFX_PCM_BASE&&address<0xb018;
}

static void preparation_respects_the_nearest_deadline(unsigned head_lead)
{
    static uint8_t pack[1024];
    memset(pack,0,sizeof pack);
    /* Only one trusted sample descriptor is needed for this timing fixture. */
    pack[43]=64;pack[44]=1; /* offset64,256 bytes =512 decoded samples */
    reset_fixture(1,(uint16_t)(1000+head_lead),1000);
    wf_sfx_engine.bound=1;wf_sfx_engine.pack=pack;wf_sfx_engine.samples=32;
    wf_sfx_engine.write16=cache_test_write;
    test_events[0][2]=0x81;
    test_ring[0]=2;
    test_events[1][0]=1600;test_events[1][1]=0x100;
    test_events[1][2]=0x10;test_events[1][3]=generation;
    if(head_lead==10){
        assert(!music_prepare());
        assert(test_clock==1000 && "predecode crossed an earlier queued deadline");
    }else{
        assert(music_prepare());
        assert(prepared_music_frames==64 && !prepared_music_valid);
        assert(test_clock==1074);
        while(music_prepare())assert(test_clock<=1000+head_lead);
        assert(test_clock<1000+head_lead && !prepared_music_valid);
    }
    assert(test_ring[1]==0 && oki_pending==-1);
    test_clock=(uint16_t)(1000+head_lead);
    assert(music_service());
    assert(test_ring[1]==1 && oki_pending==1);
    while(prepared_music_frames<256){
        uint16_t before=test_clock;
        uint16_t filled=prepared_music_frames;
        assert(music_prepare());
        assert((uint16_t)(test_clock-before)<=128);
        assert(prepared_music_frames>filled && prepared_music_frames-filled<=64);
        assert(prepared_music_valid==(prepared_music_frames==256));
        poll_clock();
        assert(music_service());
    }
    assert(prepared_music_valid && test_clock<1600);
    assert(test_ring[4]==0 && !wf_ym_engine.fault);
    test_clock=1600;
    assert(music_service());
    assert(test_ring[1]==2);
    if(WF_MUSIC_TIMING)assert(wf_music_timing[14]==1 && wf_music_timing[15]==0);
    assert(wf_sfx_engine.pcm[3].playing && test_ring[4]==0);
    ++test_ring[2];
    assert(music_service());
    assert(!prepared_music_valid && !prepared_music_frames && !wf_sfx_engine.pcm[3].playing);
}

static void preparation_uses_only_admissible_smaller_steps(void)
{
    static uint8_t pack[1024];
    const unsigned leads[]={63,64,95,96,127,128};
    const unsigned samples[]={0,16,16,32,32,64};
    for(unsigned i=0;i<sizeof leads/sizeof leads[0];i++){
        memset(pack,0,sizeof pack);pack[43]=64;pack[44]=1;
        reset_fixture(4,(uint16_t)(1000+leads[i]),1000);
        wf_sfx_engine.bound=1;wf_sfx_engine.pack=pack;wf_sfx_engine.samples=32;
        wf_sfx_engine.write16=cache_test_write;
        test_ring[0]=3;
        test_events[1][0]=1600;test_events[1][1]=0x100;
        test_events[1][2]=0x81;test_events[1][3]=generation;
        test_events[2][0]=1602;test_events[2][1]=0x100;
        test_events[2][2]=0x10;test_events[2][3]=generation;
        assert(music_prepare()==(samples[i]!=0));
        assert(prepared_music_frames==samples[i]);
        assert(test_clock<=test_events[0][0] && "small preparation crossed the nearest metadata deadline");
        assert(test_ring[1]==0 && !wf_sfx_engine.pcm[3].playing);
    }
}

static void preparation_finishes_in_dense_short_gaps(void)
{
    static uint8_t pack[1024];
    memset(pack,0,sizeof pack);pack[43]=64;pack[44]=1;
    reset_fixture(4,1100,1000);
    wf_sfx_engine.bound=1;wf_sfx_engine.pack=pack;wf_sfx_engine.samples=32;
    wf_sfx_engine.write16=cache_test_write;
    test_ring[0]=10;
    for(unsigned i=0;i<8;i++){
        test_events[i][0]=(uint16_t)(1100+112*i);test_events[i][1]=0x400;
        test_events[i][2]=(uint16_t)(1u<<(i&7u));test_events[i][3]=generation;
    }
    test_events[8][0]=1990;test_events[8][1]=0x100;
    test_events[8][2]=0x81;test_events[8][3]=generation;
    test_events[9][0]=1992;test_events[9][1]=0x100;
    test_events[9][2]=0x10;test_events[9][3]=generation;
    while(test_clock<1990){
        poll_clock();assert(music_service());
        uint16_t nearest=test_events[test_ring[1]][0];
        bool worked=music_prepare();
        assert(!worked||test_clock<=nearest);
        /* Fixed service overhead, independent of the decoded sample cost. */
        test_clock=(uint16_t)(test_clock+12);
    }
    assert(prepared_music_valid && prepared_music_frames==256 &&
           "64-sample-only preparation wasted all useful sub128Fs gaps");
    assert(!test_ring[4]&&!wf_ym_engine.fault);
    assert(test_ring[1]<=8 && "preparation consumed or postponed transport events");
}

static void refill_respects_the_nearest_deadline(void)
{
    static uint8_t pack[1024];
    reset_fixture(4,1100,1000);
    wf_sfx_engine.bound=1;wf_sfx_engine.pack=pack;
    wf_sfx_engine.write16=cache_test_write;
    wf_scsp_sfx_pcm_t *p=&wf_sfx_engine.pcm[3];
    p->playing=1;p->write_at=256;p->write_end=1024;p->decoder.remaining=512;
    music_work();
    assert(test_clock==1000 && p->write_at==256 && "refill crossed the nearest queued deadline");
    /* When the full measured work budget fits, normal128-sample refill still
     * makes progress; no queue, ring size or refill granularity changed. */
    test_events[0][0]=1300;
    music_work();
    assert(test_clock==1148 && p->write_at==384);
    if(WF_MUSIC_TIMING)assert(wf_music_timing[24]==148);
}

static void preparation_sees_the_start_behind_a_dense_burst(unsigned records)
{
    static uint8_t pack[1024];
    memset(pack,0,sizeof pack);pack[43]=64;pack[44]=1;
    reset_fixture(4,4000,1000);
    wf_sfx_engine.bound=1;wf_sfx_engine.pack=pack;wf_sfx_engine.samples=32;
    wf_sfx_engine.write16=cache_test_write;
    test_ring[0]=(uint16_t)records;
    for(unsigned i=0;i<records;i++){
        test_events[i][0]=(uint16_t)(4000+i*20);
        test_events[i][1]=i<records-3?0x400:0x100;
        test_events[i][2]=i<records-3?(uint16_t)(i+1):(i==records-3?8:(i==records-2?0x81:0x10));
        test_events[i][3]=generation;
    }
    /* Proven misses included the14th and17th already-published records. They must
     * become visible during this existing idle gap, before the dense burst
     * leaves no128Fs slot. The transport ring and time lead are unchanged. */
    assert(music_prepare() && "bounded preview missed the already queued PCM start");
    assert(prepared_music_frames==64 && test_clock==1074);
    for(unsigned i=1;i<4;i++)assert(music_prepare());
    assert(prepared_music_valid && test_clock<4000);
    assert(test_ring[1]==0 && oki_pending==-1 && !wf_sfx_engine.pcm[3].playing);
}

static void completed_preparation_does_not_rescan_immutable_records(void)
{
    preparation_sees_the_start_behind_a_dense_burst(32);
    test_record_reads=0;
    assert(!music_prepare());
    assert(test_record_reads<=1 && "complete cache rescanned all immutable transport records");
}

int main(int argc,char **argv)
{
    void *mmio = mmap((void *)(uintptr_t)0x100000u, 4096, PROT_READ|PROT_WRITE,
                      MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE, -1, 0);
    assert(mmio != MAP_FAILED);
    hard_deadline_is_counted_once_and_never_replayed();
    existing_soft_and_hard_boundaries_are_preserved();
    if(argc==2&&argv[1][0]=='s'){
        preparation_uses_only_admissible_smaller_steps();
        preparation_finishes_in_dense_short_gaps();
        puts("PASS adaptive preparation gaps");return 0;
    }
    if(argc==2&&argv[1][0]=='c'){
        completed_preparation_does_not_rescan_immutable_records();
        puts("PASS cached preparation target");return 0;
    }
    preparation_respects_the_nearest_deadline(10);
    preparation_respects_the_nearest_deadline(200);
    refill_respects_the_nearest_deadline();
    preparation_uses_only_admissible_smaller_steps();
    preparation_finishes_in_dense_short_gaps();
    if(argc==2)preparation_sees_the_start_behind_a_dense_burst((unsigned)strtoul(argv[1],NULL,0));
    else{
        preparation_sees_the_start_behind_a_dense_burst(14);
        preparation_sees_the_start_behind_a_dense_burst(17);
        preparation_sees_the_start_behind_a_dense_burst(32);
    }
    assert(munmap(mmio, 4096) == 0);
    puts("PASS real firmware hard deadline, one count, no stale replay, wrap and bounded prepare at nearest deadline");
    return 0;
}
