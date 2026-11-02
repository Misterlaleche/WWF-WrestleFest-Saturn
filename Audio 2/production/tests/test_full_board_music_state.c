/* Private-ROM regression: the observer must leave the coupled board's CPU,
 * RAM, MMIO, synthesis, command delivery and PCM untouched. The RED build
 * substitutes only a pass-through observer over the same real board context.
 * It must fail the independently known command05 FB/end observation.
 */
#include <assert.h>
#ifdef WF_TEST_UNOBSERVED_BASELINE
#include "capture_board_context.h"
typedef struct {
    board_t board;
    uint64_t timer_b_services,checkpoints,fade_sent_cycle,fade_clear_cycle;
    uint32_t global_ends;
} full_observed_t;
static void full_observed_write(void *user,uint16_t address,uint8_t value)
{full_observed_t *o=user;board_write(&o->board,address,value);}
static void full_observe_step(full_observed_t *o){board_step(&o->board);}
#else
#define WF_FULL_BOARD_MUSIC_STATE_NO_MAIN
#include "../tools/reference/probe_full_board_music_state.c"
#endif

static board_t comparable_board(const board_t *source)
{
    board_t b=*source;
    b.original.cpu.read_byte=NULL;b.original.cpu.write_byte=NULL;
    b.original.cpu.port_in=NULL;b.original.cpu.port_out=NULL;
    b.original.cpu.userdata=NULL;b.original.log=NULL;
    b.pcm=NULL;b.deliveries=NULL;
    return b;
}

static void same_state(const board_t *plain,const full_observed_t *observed)
{
    board_t a=comparable_board(plain),b=comparable_board(&observed->board);
    assert(!memcmp(&a,&b,sizeof a));
}

static void same_stream(FILE *a,FILE *b)
{
    unsigned char left[8192],right[8192];size_t n,m;
    assert(!fflush(a)&&!fflush(b));rewind(a);rewind(b);
    do{n=fread(left,1,sizeof left,a);m=fread(right,1,sizeof right,b);
        assert(n==m&&!memcmp(left,right,n));}while(n);
    assert(!ferror(a)&&!ferror(b));
}

static void run_until(board_t *plain,full_observed_t *observed,uint64_t until)
{
    unsigned checkpoints=0;
    while(now(&plain->original)<until){
        board_step(plain);full_observe_step(observed);
        if(++checkpoints==4096){same_state(plain,observed);checkpoints=0;}
    }
    same_state(plain,observed);
}

static void deliver(board_t *b,unsigned index,unsigned word)
{
    assert(!b->latch_pending&&!b->original.cpu.nmi_pending);
    b->latch_pending=1;b->command_index=index;
    b->latch_delivery=now(&b->original);
    b->original.latch=(uint8_t)word;
    loge(&b->original,"CMD",index,word);z80_gen_nmi(&b->original.cpu);
}

static void initialize(board_t *b,const uint8_t *rom,const uint8_t *samples)
{
    memcpy(b->original.rom,rom,sizeof b->original.rom);
    b->original.log=tmpfile();b->pcm=tmpfile();b->deliveries=tmpfile();
    assert(b->original.log&&b->pcm&&b->deliveries);
    OPM_Reset(&b->opm,opm_flags_none);reference_oki_reset(&b->oki,samples,0x80000);
    z80_init(&b->original.cpu);
    b->original.cpu.read_byte=board_read;b->original.cpu.write_byte=board_write;
    b->original.cpu.port_in=pin;b->original.cpu.port_out=pout;
    b->original.cpu.userdata=b;b->profile=PROFILE_FULL;
}

int main(int argc,char **argv)
{
    if(argc!=3)return 2;
    uint8_t *rom=malloc(0x10000),*samples=malloc(0x80000);
    board_t *plain=calloc(1,sizeof *plain);
    full_observed_t *observed=calloc(1,sizeof *observed);
    assert(rom&&samples&&plain&&observed);
    assert(load(argv[1],rom,0x10000)&&load(argv[2],samples,0x80000));
    initialize(plain,rom,samples);initialize(&observed->board,rom,samples);
    observed->board.original.cpu.write_byte=full_observed_write;
    observed->board.original.cpu.userdata=observed;
#ifndef WF_TEST_UNOBSERVED_BASELINE
    observed->snapshots=tmpfile();assert(observed->snapshots);
#endif
    run_until(plain,observed,2*WF_CLK);
    plain->origin=observed->board.origin=now(&plain->original);
    plain->capturing=observed->board.capturing=1;
    loge(&plain->original,"ORIGIN",0,0);loge(&observed->board.original,"ORIGIN",0,0);
    deliver(plain,0,5);deliver(&observed->board,0,5);
    run_until(plain,observed,plain->origin+4*WF_CLK);
    assert(plain->original.ram[6]==0);
    assert(observed->global_ends==1 && "command05 must record the original FB end");
    assert(observed->timer_b_services>0&&observed->checkpoints>0);
    deliver(plain,1,0x1f);deliver(&observed->board,1,0x1f);
    observed->fade_sent_cycle=now(&plain->original)-plain->origin;
    run_until(plain,observed,plain->origin+12*WF_CLK);
    assert(observed->fade_clear_cycle&&plain->original.ram[6]==0);
    assert(plain->irq_a&&plain->irq_b&&plain->busy_ym_reads);
    same_stream(plain->original.log,observed->board.original.log);
    same_stream(plain->pcm,observed->board.pcm);
    same_stream(plain->deliveries,observed->board.deliveries);
#ifndef WF_TEST_UNOBSERVED_BASELINE
    assert(!fclose(observed->snapshots));
#endif
    assert(!fclose(plain->original.log)&&!fclose(observed->board.original.log));
    assert(!fclose(plain->pcm)&&!fclose(observed->board.pcm));
    assert(!fclose(plain->deliveries)&&!fclose(observed->board.deliveries));
    free(rom);free(samples);free(plain);free(observed);
    puts("PASS: command05 4s+1F+8s; every4096 instructions CPU/RAM/MMIO/OPM/OKI identical; complete MMIO, deliveries and PCM byte-identical; coupled FB and TimerB observations present");
    return 0;
}
