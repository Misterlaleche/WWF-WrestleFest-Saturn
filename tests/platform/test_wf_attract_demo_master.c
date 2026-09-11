#include <assert.h>
#include <stdint.h>
#include "../../src/runtime/wf_attract_demo_master.h"

static const uint16_t threshold[14]={848,736,1040,928,656,928,1200,944,1616,1152,800,816,832,864};
static const uint8_t actor_count[14]={4,6,4,4,4,4,4,4,4,4,4,4,4,4};
static const uint8_t mode[14]={0,1,0,0,0,0,0,0,0,0,0,0,0,0};
static const uint8_t lineup[14][6]={
 {0,1,4,5,0xff,0xff},{3,6,7,8,9,2},{0,1,6,7,0xff,0xff},{1,0,9,8,0xff,0xff},
 {2,3,10,11,0xff,0xff},{3,2,6,9,0xff,0xff},{6,7,0,1,0xff,0xff},{7,9,2,0,0xff,0xff},
 {8,6,3,2,0xff,0xff},{9,8,1,3,0xff,0xff},{10,11,2,1,0xff,0xff},{11,10,2,1,0xff,0xff},
 {4,5,0,1,0xff,0xff},{5,4,0,1,0xff,0xff}
};

static void test_table(void){
 for(unsigned s=0;s<14;s++){
  const wf_attract_demo_config_t*c=wf_attract_demo_config(s);
  assert(c&&c->state==s&&c->threshold_updates==threshold[s]);
  assert((unsigned)c->mode==mode[s]&&c->actor_count==actor_count[s]);
  for(unsigned i=0;i<6;i++)assert(c->lineup[i]==lineup[s][i]);
 }
 assert(!wf_attract_demo_config(14));
 assert(wf_attract_demo_config(0)->override_policy==WF_ATTRACT_OVERRIDE_STATE0_SPECIAL);
 assert(wf_attract_demo_config(1)->override_policy==WF_ATTRACT_OVERRIDE_DISABLED_RUMBLE);
 for(unsigned s=2;s<14;s++)assert(wf_attract_demo_config(s)->override_policy==WF_ATTRACT_OVERRIDE_CHARACTER_12);
 assert(wf_attract_demo_config(0)->bio_featured_id==WF_ATTRACT_DEMO_NO_BIO);
 assert(wf_attract_demo_config(1)->bio_featured_id==WF_ATTRACT_DEMO_NO_BIO);
 assert(wf_attract_demo_config(2)->bio_featured_id==0);
 assert(wf_attract_demo_config(12)->bio_featured_id==4);
 assert(wf_attract_demo_config(13)->bio_featured_id==5);
}

static void test_threshold_and_90_vblank(void){
 wf_attract_demo_runtime_t rt;wf_attract_demo_event_t ev;
 assert(wf_attract_demo_begin(&rt,0));
 for(unsigned i=1;i<848;i++){wf_attract_demo_gameplay_update(&rt,&ev);assert(!ev.threshold_reached&&wf_attract_demo_is_live_gameplay(&rt));}
 wf_attract_demo_gameplay_update(&rt,&ev);
 assert(ev.threshold_reached&&ev.begin_palette_fade&&rt.gameplay_updates==848&&rt.phase==WF_ATTRACT_DEMO_FADE_24);
 wf_attract_demo_gameplay_update(&rt,&ev);assert(rt.gameplay_updates==848&&!ev.threshold_reached);
 for(unsigned i=1;i<=89;i++){wf_attract_demo_transition_vblank(&rt,&ev);assert(!ev.next_attract_state&&!ev.frontend_return);}
 wf_attract_demo_transition_vblank(&rt,&ev);
 assert(rt.phase==WF_ATTRACT_DEMO_HANDOFF&&ev.next_attract_state&&ev.next_state==1);
}

static void test_last_returns(void){
 wf_attract_demo_runtime_t rt;wf_attract_demo_event_t ev;
 assert(wf_attract_demo_begin(&rt,13));
 for(unsigned i=0;i<864;i++)wf_attract_demo_gameplay_update(&rt,&ev);
 for(unsigned i=0;i<90;i++)wf_attract_demo_transition_vblank(&rt,&ev);
 assert(ev.frontend_return&&ev.frontend_return_rom==0x000006fcu);
}

int main(void){
 test_table();test_threshold_and_90_vblank();test_last_returns();
 assert(WF_ATTRACT_DEMO_TRANSITION_VBLANKS==90u);
 assert(WF_ATTRACT_DEMO_CREDITED_START_ROM==0x000052beu);
 assert(wf_attract_demo_credited_start_is_external());
 return 0;
}
