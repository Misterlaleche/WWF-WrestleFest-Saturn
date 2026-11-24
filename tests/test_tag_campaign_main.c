/* Continuous real application campaign probe. Reuses the existing hardware
 * replacement, but calls the real post-match adapter and keeps playing.
 * Inputs are the production autorun tape; no result, progression, actor, RNG,
 * timer or scene state is written by this observer. Host fields are not Saturn
 * performance. A bounded run reports its actual coverage, never ten wins by
 * assumption. */
#define WF_JO_PROBE_MAIN "../test_tag_campaign_game.c"
#define main first_entry_main
#define __wrap_wf_jo_frontend_run first_entry_wrapper
#define __wrap_wf_jo_frontend_after_combat_at_level first_terminal_wrapper
#include "native/jo_main_probe.c"
#undef main
#undef __wrap_wf_jo_frontend_run
#undef __wrap_wf_jo_frontend_after_combat_at_level
#include "wf_r18_post_tag.h"

/* Optional driver defines unsigned campaign_pad_buttons(unsigned port).
 * It receives only physical key queries. Keep all policy state test-local. */
#ifdef WF_CAMPAIGN_INPUT_DRIVER
#include WF_CAMPAIGN_INPUT_DRIVER
#else
static unsigned campaign_pad_buttons(unsigned port){(void)port;return 0;}
#endif
static FILE *campaign_pad_tape;
static unsigned campaign_tape_initialized,campaign_tape_frame=~0u,campaign_tape_stage=~0u;
static bool campaign_pad_read(unsigned port,unsigned key)
{
    unsigned keys=campaign_pad_buttons(port);
    if(!port&&running&&!frontend_state&&frontend.handed_off){
        if(!campaign_tape_initialized){
            const char *path=getenv("WF_POLICY_TAPE");
            if(path){campaign_pad_tape=fopen(path,"w");assert(campaign_pad_tape);fputs("stage,frame,bus_active_low\n",campaign_pad_tape);}
            campaign_tape_initialized=1;
        }
        if(campaign_pad_tape&&(campaign_tape_frame!=rt.g.frame_counter||campaign_tape_stage!=rt.g.progression_0162)){
            static const unsigned physical[]={JO_KEY_RIGHT,JO_KEY_LEFT,JO_KEY_UP,JO_KEY_DOWN,JO_KEY_A,JO_KEY_B,JO_KEY_C,JO_KEY_START};
            unsigned level=255u;
            for(unsigned bit=0;bit<8u;bit++)if(keys&physical[bit])level&=~(1u<<bit);
            fprintf(campaign_pad_tape,"%u,%u,%u\n",rt.g.progression_0162,rt.g.frame_counter,level);
            campaign_tape_frame=rt.g.frame_counter;campaign_tape_stage=rt.g.progression_0162;
        }
    }
    return (keys&key)!=0;
}

static unsigned campaign_entries,campaign_results,campaign_scenes,campaign_stages;
static unsigned campaign_credited_entries,campaign_game_over;
static unsigned campaign_required_owner,campaign_owner_completed;
static unsigned campaign_credits_entered,campaign_credits_completed;
static unsigned campaign_last_entry;
static unsigned campaign_world_stage=~0u,campaign_world_id=~0u;
static FILE *campaign_events;
wf_status_t __real_wf_jo_frontend_after_combat_at_level(wf_jo_frontend_t *,uint32_t,unsigned);
int __real_wf_r18_ring_bind(const void *,size_t);
int __real_wf_r18_post_tag_step(wf_r18_post_tag_t *,wf_r18_post_tag_context_t *);

static void campaign_observe_world(void)
{
    if(!running||frontend_state||!frontend.handed_off||!rt.g.credited_route)return;
    unsigned stage=rt.g.progression_0162,world=rt.g.global_007e;
    if(stage==campaign_world_stage&&world==campaign_world_id)return;
    fprintf(campaign_events,"WORLD stage=%u id=%u tick=%u fields=%u mode=%u\n",stage,world,rt.g.frame_counter,host_fields,rt.g.mode_0161&1u);fflush(campaign_events);
    campaign_world_stage=stage;campaign_world_id=world;
}

int __wrap_wf_r18_post_tag_step(wf_r18_post_tag_t *cursor,wf_r18_post_tag_context_t *context)
{
    unsigned pc=cursor->pc;
    if(pc==0x1b4eu){
        ++campaign_credits_entered;
        fprintf(campaign_events,"CREDITS_ENTER stage=%u fields=%u\n",rt.g.progression_0162,host_fields);fflush(campaign_events);
    }
    int result=__real_wf_r18_post_tag_step(cursor,context);
    if(pc==0x1dcau&&cursor->pc==0x6fcu&&result==WF_R18_POST_MORE&&campaign_credits_entered){
        ++campaign_credits_completed;
        fprintf(campaign_events,"CREDITS_COMPLETE stage=%u fields=%u\n",rt.g.progression_0162,host_fields);fflush(campaign_events);
    }
    return result;
}

int __wrap_wf_r18_ring_bind(const void *blob,size_t bytes)
{
    int status=__real_wf_r18_ring_bind(blob,bytes);
    if(!status){
        const uint8_t *p=blob;unsigned scene=((unsigned)p[6]<<8)|p[7];
        assert(scene<7u);campaign_scenes|=1u<<scene;
        if(campaign_events){fprintf(campaign_events,"SCENE id=%u fields=%u\n",scene,host_fields);fflush(campaign_events);}
    }
    return status;
}
wf_status_t __wrap_wf_jo_frontend_run(wf_jo_frontend_t *f)
{
    wf_status_t status=__real_wf_jo_frontend_run(f);
    if(status==WF_OK&&f->handed_off){
        ++campaign_entries;host_entry_seen=1;
        unsigned stage=rt.g.progression_0162;
        assert(stage<10u);if(rt.g.credited_route&&((rt.g.mode_0161&1u)==(WF_JO_AUTORUN_MODE==2))){campaign_stages|=1u<<stage;++campaign_credited_entries;}campaign_last_entry=host_fields;
        char name[48];snprintf(name,sizeof name,"match%02u-stage%u-f8c",campaign_entries,stage);host_snapshot(name);
        fprintf(campaign_events,"ENTRY number=%u credited=%u mode=%u stage=%u world=%u fields=%u\n",campaign_entries,rt.g.credited_route,rt.g.mode_0161&1u,stage,rt.g.global_007e,host_fields);fflush(campaign_events);
    }
    return status;
}
wf_status_t __wrap_wf_jo_frontend_after_combat_at_level(wf_jo_frontend_t *f,uint32_t owner,unsigned high)
{
    assert(f==&frontend&&high==1u&&rt.frame_stage==2u&&rt.frame_ready&&rt.vblank_ack);
    ++campaign_results;host_terminal_owner=owner;host_terminal_seen=1;
    char name[48];snprintf(name,sizeof name,"match%02u-terminal",campaign_entries);host_snapshot(name);
    fprintf(campaign_events,"RESULT number=%u stage=%u owner=%x tick=%u fields=%u\n",campaign_results,rt.g.progression_0162,owner,rt.g.frame_counter,host_fields);fflush(campaign_events);
    wf_status_t status=__real_wf_jo_frontend_after_combat_at_level(f,owner,high);
    if(status==WF_OK&&((owner==campaign_required_owner&&owner!=0x1b4eu)||
       (campaign_required_owner==0x1b4eu&&campaign_credits_completed)))campaign_owner_completed=campaign_required_owner;
    if(status==WF_OK&&owner==0x11b6u&&f->driver.phase==WF_R18_DRIVER_BOOT)campaign_game_over=1;
    if(WF_JO_AUTORUN_MODE==2&&(owner==0x1afcu||owner==0x1b4eu))campaign_game_over=1;
    fprintf(campaign_events,"POST status=%u pc=%x phase=%u stage=%u owner=%x dependency=%x\n",status,f->driver.pc,f->driver.phase,(unsigned)((f->driver.ram[0x162]<<8)|f->driver.ram[0x163]),f->blocked_pc,f->blocked_dependency);fflush(campaign_events);
    return status;
}
int main(int argc,char **argv)
{
    assert(argc==6||argc==7);unsigned wanted=(unsigned)strtoul(argv[1],NULL,0),limit=(unsigned)strtoul(argv[2],NULL,0);
    campaign_required_owner=argc==7?(unsigned)strtoul(argv[6],NULL,0):(WF_JO_AUTORUN_MODE==2?0x1b4eu:0u);
    unsigned required_stage=(unsigned)strtoul(argv[4],NULL,0),required_scenes=(unsigned)strtoul(argv[5],NULL,0);
    assert(required_stage<10u&&required_scenes<128u);
    assert(wanted>=1u&&wanted<=64u&&limit>=1000u);
    char path[1024];int length=snprintf(path,sizeof path,"%s/campaign-events.log",argv[3]);
    assert(length>0&&(size_t)length<sizeof path);campaign_events=fopen(path,"w");assert(campaign_events);
    char *initial[]={argv[0],WF_JO_AUTORUN_MODE==2?"1":"0","0",argv[3],NULL};
    int result=first_entry_main(4,initial);if(result)return result;
    campaign_observe_world();
    while(!failed&&!campaign_game_over&&host_fields<limit&&
          (campaign_required_owner?campaign_owner_completed!=campaign_required_owner:
           (campaign_credited_entries<wanted||!(campaign_stages&(1u<<required_stage))||
            (campaign_scenes&required_scenes)!=required_scenes))){
        __jo_core_synch();game_frame();wf_host_assets_frame(rt.g.frame_counter);
        campaign_observe_world();
        if(host_fields-campaign_last_entry>100000u)break;
    }
    fprintf(campaign_events,"SUMMARY entries=%u credited_entries=%u results=%u stages=%03x scenes=%02x failed=%u game_over=%u fields=%u required_owner=%x completed_owner=%x credits_entered=%u credits_completed=%u\n",campaign_entries,campaign_credited_entries,campaign_results,campaign_stages,campaign_scenes,failed,campaign_game_over,host_fields,campaign_required_owner,campaign_owner_completed,campaign_credits_entered,campaign_credits_completed);
    assert(!fclose(campaign_events));campaign_events=NULL;
    if(campaign_pad_tape){assert(!fclose(campaign_pad_tape));campaign_pad_tape=NULL;}
    printf("CAMPAIGN entries=%u credited_entries=%u results=%u stages=%03x scenes=%02x failed=%u game_over=%u fields=%u\n",campaign_entries,campaign_credited_entries,campaign_results,campaign_stages,campaign_scenes,failed,campaign_game_over,host_fields);
    if(failed){host_snapshot("campaign-blocked");return 2;}
    if(campaign_required_owner){
        if(campaign_owner_completed!=campaign_required_owner||!campaign_credited_entries){host_snapshot("campaign-bounded");return 3;}
        return 0;
    }
    if(campaign_credited_entries<wanted||!(campaign_stages&(1u<<required_stage))||(campaign_scenes&required_scenes)!=required_scenes){host_snapshot("campaign-bounded");return 3;}
    return 0;
}
