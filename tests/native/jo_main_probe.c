/* Execute the application's actual frontend input, prepare_gameplay and
 * game_frame code. Only console hardware/physical time is replaced. The
 * terminal observer stops at the first real post-ACK scene handoff, before
 * the post-match owner changes state. No actors, RNG or combat input are
 * synthesized by this harness. Host field counts are not Saturn timings. */
#define _GNU_SOURCE
#include <assert.h>
#include <inttypes.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include "assets_fixture.h"
#include "wf_r18_runtime_handoff.h"
#include "wf_saturn_yaul_backend.h"

#ifndef WF_JO_PROBE_MAIN
#define WF_JO_PROBE_MAIN "../../port/main.c"
#endif
#include WF_JO_PROBE_MAIN

static unsigned host_fields,host_queued,host_commits,host_sounds;
static unsigned host_entry_seen,host_terminal_seen,host_limit,host_stride=1;
static unsigned host_snapshot_frames,host_last_snapshot;
static uint32_t host_terminal_owner;
static const char *host_output;
static jmp_buf host_terminal_exit;
volatile uint32_t wf_host_jo_registers[4];
volatile wf_jo_audio_diagnostic_t wf_jo_audio_diagnostic={.initialized=1};

unsigned jo_core_get_vblank_count(void){return host_fields;}
int jo_get_player_input_port(int player){return player>=0&&player<4?player:-1;}
bool jo_is_input_key_pressed(unsigned port,unsigned key){(void)port;(void)key;return false;}
bool jo_is_pad1_key_down(unsigned key){(void)key;return false;}
void jo_printf(int x,int y,const char *format,...)
{(void)x;(void)y;va_list args;va_start(args,format);vfprintf(stderr,format,args);va_end(args);fputc('\n',stderr);}
void jo_goto_boot_menu(void){assert(!"Unexpected boot menu");}
void jo_core_init(unsigned color){(void)color;assert(!"Use host entry, not console boot");}
void jo_core_add_callback(void (*callback)(void)){(void)callback;assert(!"Use host entry");}
void jo_core_run(void){assert(!"Use host entry");}
void __jo_core_synch(void)
{host_queued=0;host_fields+=host_stride;}
#ifndef WF_JO_FOCAL_CLOCK_SHIM
/* Fatal video closure must stop the oracle instead of spinning forever. */
void __jo_core_pause(void){assert(!"Unexpected fatal drain pause in native oracle");}
#endif
void wf_jo_audio_emergency_stop(void){/* Hardware muted in the separate emergency SDK test. */}
bool wf_r18_scsp_service(void){return true;}
bool wf_jo_audio_service_cooperate(void){return true;}
wf_status_t wf_r18_scsp_audio(wf_runtime_t *r,uint16_t command,void *user)
{(void)r;(void)user;(void)command;++host_sounds;return WF_OK;}
/* load_resources remains linked through game_frame but is never entered.
 * Any accidental use of its hardware/audio bootstrap fails immediately. */
bool wf_jo_audio_init(const void *p,size_t n){(void)p;(void)n;assert(!"Unexpected audio bootstrap");return false;}
bool wf_jo_audio_set_prefix_workspace(void *p,size_t n){(void)p;(void)n;assert(!"Unexpected audio bootstrap");return false;}
uint32_t wf_jo_audio_prefetch_music(void *p,size_t n){(void)p;(void)n;assert(!"Unexpected audio bootstrap");return 0;}
bool wf_jo_audio_prime_music(uint8_t command){(void)command;assert(!"Unexpected audio bootstrap");return false;}
int wf_jo_video_frame_queued(void){return (int)host_queued;}
int wf_jo_video_pending_changes(void){return 1;}
wf_status_t wf_jo_video_commit(void){host_queued=1;++host_commits;return WF_OK;}
wf_status_t wf_jo_video_repeat(void){return wf_jo_video_commit();}
void wf_jo_video_gate(void){}
void wf_jo_video_diagnostics(void){}
void wf_jo_video_layers(unsigned layers,unsigned opaque,unsigned sprites)
{assert(!(layers&~7u)&&!(opaque&~layers)&&sprites<=7u);}

static FILE *host_file(const char *stage,const char *part,const char *mode)
{
    char path[1024];int n=snprintf(path,sizeof path,"%s/%s-%s",host_output,stage,part);
    assert(n>0&&(size_t)n<sizeof path);FILE *out=fopen(path,mode);
    if(!out){perror(path);abort();}return out;
}
static void host_bytes(const char *stage,const char *part,const void *bytes,size_t count)
{FILE *out=host_file(stage,part,"wb");assert(fwrite(bytes,1,count,out)==count);assert(!fclose(out));}
static void host_snapshot(const char *stage)
{
    uint8_t canonical[16384]={0};
    assert(wf_r18_runtime_export(&rt,canonical,sizeof canonical)==WF_OK);
    host_bytes(stage,"arcade-ram.bin",canonical,sizeof canonical);
    host_bytes(stage,"actors.bin",rt.actors,sizeof rt.actors);
    host_bytes(stage,"globals-host.bin",&rt.g,sizeof rt.g);
    /* Pointers, host-size binding counters, trace diagnostics and outer ABI
     * padding are excluded. The JSON records every concatenated member. */
    FILE *core=host_file(stage,"core-host.bin","wb");
    FILE *json=host_file(stage,"state.json","w");size_t offset=0;
    fprintf(json,"{\"frame\":%u,\"rng\":%u,\"mode\":%u,\"credited_route\":%u,\"frame_stage\":%u,\"frame_ready\":%u,\"vblank_ack\":%u,\"host_fields\":%u,\"terminal_owner\":%u,\"actors\":[",
        rt.g.frame_counter,rt.g.rng_state,rt.g.mode_0161,rt.g.credited_route,rt.frame_stage,rt.frame_ready,rt.vblank_ack,host_fields,host_terminal_owner);
    for(unsigned i=0;i<WF_ACTOR_SLOTS;i++)fprintf(json,"%s{\"slot\":%u,\"active\":%u,\"id\":%u,\"x\":%u,\"y\":%u,\"role\":%u,\"health\":%u}",i?",":"",i,wf_u8(&rt.actors[i],WF_A_ACTIVE),wf_be16(&rt.actors[i],2),wf_be16(&rt.actors[i],WF_A_X),wf_be16(&rt.actors[i],WF_A_Y),wf_u8(&rt.actors[i],0x56u),wf_be16(&rt.actors[i],WF_A_STAMINA));
    fprintf(json,"],\"d\":[");for(unsigned i=0;i<8;i++)fprintf(json,"%s%u",i?",":"",rt.m68k_regs.d[i]);
    fprintf(json,"],\"registers_valid\":%u,\"sections\":[",rt.m68k_regs.valid);
#define MEMBER(name) do{if(offset)fputc(',',json);fprintf(json,"{\"name\":\"" #name "\",\"offset\":%zu,\"bytes\":%zu}",offset,sizeof rt.name);assert(fwrite(&rt.name,1,sizeof rt.name,core)==sizeof rt.name);offset+=sizeof rt.name;}while(0)
    MEMBER(g);MEMBER(actors);MEMBER(fixed_object0);MEMBER(fixed_object1);MEMBER(fixed_four_1134);
    MEMBER(referee_aux);MEMBER(fixed_four_1194);MEMBER(fixed_four_11c4);MEMBER(referee);
    MEMBER(transient_1258);MEMBER(branchb_list_182a);MEMBER(branchb_prev_185a);MEMBER(branchb_objs_188a);
    MEMBER(effect_1450);MEMBER(compact_14ce);MEMBER(m68k_regs.d);MEMBER(m68k_regs.valid);
    MEMBER(frame_ready);MEMBER(vblank_ack);MEMBER(frame_stage);
#undef MEMBER
    fprintf(json,"]");
#ifdef WF_JO_FRONTEND
    fprintf(json,",\"frontend_ticks\":%u,\"frontend_irq2\":%u,\"frontend_irq3\":%u,\"combat_coin_irqs\":%u,\"front_generation\":%u",frontend.arcade_ticks,frontend.raster_irqs,frontend.vblank_irqs,frontend.combat_coin_irqs,frontend.driver.generation);
    host_bytes(stage,"retained-ram.bin",frontend.driver.ram,frontend.driver.ram_bytes);
#endif
    fprintf(json,"}\n");assert(!fclose(json));assert(!fclose(core));
    printf("SNAPSHOT %s mode=%u frame=%u rng=%08x owner=%x fields=%u\n",stage,rt.g.mode_0161,rt.g.frame_counter,rt.g.rng_state,host_terminal_owner,host_fields);fflush(stdout);
}

#ifdef WF_JO_FRONTEND
wf_status_t __real_wf_jo_frontend_run(wf_jo_frontend_t *f);
wf_status_t __wrap_wf_jo_frontend_run(wf_jo_frontend_t *f)
{
    wf_status_t status=__real_wf_jo_frontend_run(f);
    if(status==WF_OK&&f->handed_off&&!host_entry_seen){host_snapshot("f8c");host_entry_seen=1;}
    return status;
}
wf_status_t __wrap_wf_jo_frontend_after_combat_at_level(wf_jo_frontend_t *f,uint32_t owner,unsigned high)
{
    assert(f==&frontend&&high==1u&&rt.frame_stage==2u&&rt.frame_ready&&rt.vblank_ack);
    assert((owner==0x11b6u&&wf_r18_tag_match_complete(&rt))||
           (owner==0x1afcu&&wf_r18_royal_game_over(&rt))||
           (owner==0x1b4eu&&(rt.g.mode_0161&1u)&&(rt.g.global_16c4>>8))||
           (owner==0x1120u&&wf_r18_attract_timeout(&rt)));
    host_terminal_owner=owner;host_terminal_seen=1;host_snapshot("terminal");
    /* Observation ends here. The actual game_frame chose this owner after
     * the final ACK; no result screen or next match is executed by this test. */
    longjmp(host_terminal_exit,1);
}
#endif

int main(int argc,char **argv)
{
    assert(argc>=4&&argc<=5);selected_royal=(unsigned)strtoul(argv[1],NULL,0);
    host_limit=(unsigned)strtoul(argv[2],NULL,0);host_output=argv[3];
    host_snapshot_frames=getenv("WF_JO_TEST_SNAPSHOTS")!=NULL;
    if(argc==5)host_stride=(unsigned)strtoul(argv[4],NULL,0);
    assert(selected_royal<=1u&&host_stride>=1u&&host_stride<=4u);
#ifdef WF_JO_FRONTEND
    assert(selected_royal==(WF_JO_AUTORUN_MODE==2));
#endif
    void *ram=mmap((void *)0x00200000u,0x100000u,PROT_READ|PROT_WRITE,
        MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE,-1,0);
    if(ram==MAP_FAILED){perror("Host LWRAM mapping");return 2;}
    assert(ram==(void *)0x00200000u);
    wf_host_assets_init();wf_jo_assets_set_service_hook(service_during_assets,NULL);
    selected_demo=1;loaded=1;wf_jo_clock_start(&arcade_clock,host_fields);
#ifdef WF_JO_FRONTEND
    assert(init_platform());assert(init_frontend());
    /* The real callback runs frontend_input from reset and performs F8C,
     * flush and prepare_gameplay. No manual TITLE trigger is substituted. */
    while(frontend_state&&!failed){game_frame();if(frontend_state)__jo_core_synch();}
    assert(host_entry_seen);
#else
    assert(start_native_match());host_snapshot("f8c");host_entry_seen=1;
#endif
    if(failed)return 2;
    host_snapshot("prepared");wf_host_assets_play();
    if(!host_limit)return 0;
    if(!setjmp(host_terminal_exit)){
        unsigned calls=0;
        while(!failed&&!done&&rt.g.frame_counter<host_limit){
            __jo_core_synch();game_frame();wf_host_assets_frame(rt.g.frame_counter);
            if(host_snapshot_frames&&rt.g.frame_counter!=host_last_snapshot&&
               (rt.g.frame_counter==30u||rt.g.frame_counter==120u||
                rt.g.frame_counter==600u||rt.g.frame_counter%1000u==0u)){
                char label[32];snprintf(label,sizeof label,"tick%05u",rt.g.frame_counter);
                host_snapshot(label);host_last_snapshot=rt.g.frame_counter;
            }
            assert(++calls<host_limit*8u+10000u);
        }
    }
    if(failed){host_snapshot("blocked");return 2;}
#ifndef WF_JO_FRONTEND
    if(done){host_terminal_seen=1;host_terminal_owner=selected_royal?0x1afcu:0x11b6u;host_snapshot("terminal");}
#endif
    if(!host_terminal_seen){host_snapshot("bounded");fprintf(stderr,"No terminal within%u native ticks\n",host_limit);return 3;}
    wf_host_assets_dump(rt.actors,sizeof rt.actors);
    printf("COMPLETE actual_main mode=%s native_ticks=%u owner=%x host_fields=%u commits=%u sounds=%u\n",selected_royal?"Royal":"TAG",rt.g.frame_counter,host_terminal_owner,host_fields,host_commits,host_sounds);
    return 0;
}
