/* Host-only observation of the untouched scalar original Z80 core.
 * Build z80.c with -Dz80_step=wf_profile_core_step; this wrapper changes no
 * state or bus callback. Native control batching is deliberately not linked.
 * The production idle-skip stays enabled and omitted instructions are counted
 * separately. Emulated cycles here are not SH2 performance measurements. */
#include "wf_music_seq.h"
#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CLOCK UINT64_C(3579545)
#define SECONDS 14u

extern void wf_profile_core_step(z80 *);
typedef struct {uint64_t hits, cycles;} count_t;
typedef struct {uint64_t cycle; unsigned kind,reg,value;} event_t;
static count_t pc_count[SECONDS][65536];
static uint64_t event_kinds[SECONDS][4], tl_writes[SECONDS], non_timer_ym[SECONDS];
static uint64_t histogram_cycles[SECONDS], scalar_steps[SECONDS];
static wf_music_seq_t profiled, control;
static uint8_t rom[65536];
static event_t *events;
static size_t event_count,event_capacity,compare_index;
static uint64_t origin,fade_cycle,semantic_checkpoints;
static bool profiling,compare_events;

void z80_step(z80 *cpu)
{
    uint16_t pc=cpu->pc;
    uint64_t before=cpu->cyc;
    wf_profile_core_step(cpu);
    if(profiling&&before>=origin&&before<origin+SECONDS*CLOCK){
        unsigned second=(unsigned)((before-origin)/CLOCK);
        ++pc_count[second][pc].hits;
        pc_count[second][pc].cycles+=cpu->cyc-before;
        histogram_cycles[second]+=cpu->cyc-before;
        ++scalar_steps[second];
    }
}

static bool emit(unsigned kind,unsigned reg,unsigned value,uint64_t cycle,void *unused)
{
    (void)unused;
    if(compare_events){
        assert(compare_index<event_count);
        event_t e=events[compare_index++];
        if(e.cycle!=cycle||e.kind!=kind||e.reg!=reg||e.value!=value){
            fprintf(stderr,"event mismatch index=%zu cycle=%"PRIu64"/%"PRIu64"\n",compare_index-1,e.cycle,cycle);
            abort();
        }
    }else{
        if(event_count==event_capacity){
            event_capacity=event_capacity?event_capacity*2:4096;
            events=realloc(events,event_capacity*sizeof *events);assert(events);
        }
        events[event_count++]=(event_t){cycle,kind,reg,value};
        if(profiling&&cycle>=origin&&cycle<origin+SECONDS*CLOCK){
            unsigned second=(unsigned)((cycle-origin)/CLOCK);
            assert(kind<4);++event_kinds[second][kind];
            if(kind==WF_MUSIC_YM&&reg!=0x14)++non_timer_ym[second];
            if(kind==WF_MUSIC_YM&&reg>=0x60&&reg<=0x7f)++tl_writes[second];
        }
    }
    return true;
}

static void normalize(z80 *cpu)
{
    cpu->userdata=NULL;cpu->read_byte=NULL;cpu->write_byte=NULL;
    cpu->port_in=NULL;cpu->port_out=NULL;
}

static void equal_state(void)
{
    z80 a=profiled.cpu,b=control.cpu;normalize(&a);normalize(&b);
    assert(!memcmp(&a,&b,sizeof a));
    assert(!memcmp(profiled.ram,control.ram,sizeof profiled.ram));
    assert(profiled.cycles==control.cycles&&profiled.last_cpu==control.last_cpu);
    assert(profiled.next_tb==control.next_tb&&profiled.ym_addr==control.ym_addr);
    assert(profiled.status==control.status&&profiled.tb==control.tb&&profiled.ctl==control.ctl);
    assert(profiled.latch==control.latch&&profiled.bank==control.bank);
    assert(profiled.command_pending==control.command_pending&&profiled.error==control.error);
    assert(profiled.io_epoch==control.io_epoch);
    assert(compare_index==event_count);
    ++semantic_checkpoints;
}

static void run(wf_music_seq_t *seq,uint64_t target,unsigned budget)
{
    int status;
    do{status=wf_music_seq_run(seq,target,budget);assert(status>=0);}while(status);
}

int main(int argc,char **argv)
{
    assert(argc==3);
    FILE *f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof rom,f)==sizeof rom);assert(fclose(f)==0);
    assert(wf_music_seq_init(&profiled,rom,sizeof rom,emit,NULL,true));
    assert(wf_music_seq_init(&control,rom,sizeof rom,emit,NULL,true));
    run(&profiled,2*CLOCK,1);
    compare_events=true;run(&control,2*CLOCK,4096);equal_state();
    origin=profiled.cycles;
    assert(wf_music_seq_send(&profiled,2)&&wf_music_seq_send(&control,2));
    char path[4096];
    assert(snprintf(path,sizeof path,"%s/seconds.csv",argv[2])<(int)sizeof path);
    FILE *seconds=fopen(path,"wx");assert(seconds);
    fprintf(seconds,"second,phase,scalar_steps,idle_instructions_skipped,scalar_z80_cycles,ym_writes,non_timer14_ym_writes,tl_writes,oki_writes,bank_writes,timer_b_irq_events,sequence_end_cycle\n");
    for(unsigned second=0;second<SECONDS;second++){
        if(second==6){
            fade_cycle=profiled.cycles;
            assert(wf_music_seq_send(&profiled,0x1f)&&wf_music_seq_send(&control,0x1f));
        }
        uint64_t skipped_before=profiled.skipped;
        compare_events=false;profiling=true;run(&profiled,origin+(second+1)*CLOCK,1);profiling=false;
        compare_events=true;run(&control,origin+(second+1)*CLOCK,4096);equal_state();
        fprintf(seconds,"%u,%s,%"PRIu64",%"PRIu64",%"PRIu64",%"PRIu64",%"PRIu64",%"PRIu64",%"PRIu64",%"PRIu64",%"PRIu64",%"PRIu64"\n",
            second,second<6?"pre_fade":"fade",scalar_steps[second],profiled.skipped-skipped_before,
            histogram_cycles[second],event_kinds[second][0],non_timer_ym[second],tl_writes[second],
            event_kinds[second][1],event_kinds[second][2],event_kinds[second][3],profiled.cycles);
    }
    assert(fclose(seconds)==0);
    assert(snprintf(path,sizeof path,"%s/pc-counts.csv",argv[2])<(int)sizeof path);
    FILE *pcs=fopen(path,"wx");assert(pcs);
    fprintf(pcs,"second,pc,opcode0,opcode1,opcode2,opcode3,hits,z80_cycles_including_post_instruction_irq\n");
    for(unsigned second=0;second<SECONDS;second++)for(unsigned pc=0;pc<65536;pc++)if(pc_count[second][pc].hits){
        assert(pc<0xc000);
        fprintf(pcs,"%u,%04X,%02X,%02X,%02X,%02X,%"PRIu64",%"PRIu64"\n",second,pc,
            rom[pc],rom[(pc+1)&65535],rom[(pc+2)&65535],rom[(pc+3)&65535],
            pc_count[second][pc].hits,pc_count[second][pc].cycles);
    }
    assert(fclose(pcs)==0);
    assert(snprintf(path,sizeof path,"%s/verified-events.csv",argv[2])<(int)sizeof path);
    FILE *trace=fopen(path,"wx");assert(trace);
    fprintf(trace,"cycle,kind,reg,value\n");
    for(size_t i=0;i<event_count;i++)fprintf(trace,"%"PRIu64",%u,%u,%u\n",events[i].cycle,events[i].kind,events[i].reg,events[i].value);
    assert(fclose(trace)==0);
    assert(snprintf(path,sizeof path,"%s/profile.json",argv[2])<(int)sizeof path);
    FILE *report=fopen(path,"wx");assert(report);
    fprintf(report,"{\"kind\":\"ORIGINAL_SCALAR_PC_COUNTS_NOT_NATIVE_COST\",\"complete\":true,\"command\":2,\"score_seconds\":6,\"fade_seconds\":8,\"origin_cycle\":%"PRIu64",\"fade_delivery_cycle\":%"PRIu64",\"end_cycle\":%"PRIu64",\"native_batch_admission_enabled\":false,\"optimized_idle_skip_enabled\":true,\"profile_instruction_budget\":1,\"control_instruction_budget\":4096,\"exact_callback_events_compared\":%zu,\"exact_cpu_ram_timer_state_checkpoints\":%"PRIu64",\"instruction_count_excludes_optimized_idle_skips\":true,\"pc_cycle_count_includes_interrupt_ack_after_instruction\":true,\"native_performance_claim\":false}\n",
        origin,fade_cycle,profiled.cycles,event_count,semantic_checkpoints);
    assert(fclose(report)==0);free(events);
    printf("PASS command02 6s+1F8s exact event comparisons=%zu CPU/RAM/timer checkpoints=%"PRIu64"\n",event_count,semantic_checkpoints);
    return 0;
}
