#include "wp12_prematch.h"

const uint16_t wp12_tag_primary_xy_machine[4][2]={{0x0240,0x0150},{0x0220,0x0160},{0x02C0,0x0150},{0x02E0,0x0160}};
const uint16_t wp12_royal_xy_machine[6][2]={{0x0240,0x0120},{0x02D0,0x0120},{0x0230,0x0150},{0x02C0,0x0150},{0x0240,0x0180},{0x02B0,0x0180}};
const uint16_t wp12_royal_orientation[6]={0x8000,0,0x8000,0,0x8000,0};

static const uint16_t fixed_updates[24]={129,50,161,33,0,64,21,1,0,64,1,41,1,0,64,21,1,0,64,33,50,94,48,48};
static const uint8_t id_states[4]={0x04,0x08,0x0D,0x11},act_states[4]={0x05,0x09,0x0E,0x12};
static const uint8_t primary[4]={0,1,2,3},fallback[4]={4,5,4,5};
static const uint8_t voices[12]={0x6D,0x6E,0x6F,0x74,0x73,0x73,0x71,0x70,0x72,0x75,0x73,0x73};
static const uint16_t windows[12]={65,129,129,129,129,129,129,129,65,257,65,65};
static const uint16_t audio[24]={0,0,0x317B,0,0,0,0x317C,0,0,0,0,0x317D,0,0,0,0x317C,0,0,0,0,0,0,0,0};

static void zero_mem(void *v,unsigned n){unsigned char *p=v;while(n--)*p++=0;}
static void copy_mem(void *d,const void *v,unsigned n){unsigned char *a=d;const unsigned char *b=v;while(n--)*a++=*b++;}
static wp12_event_t none(void){wp12_event_t e={WP12_EVENT_NONE,0,0xFF,0xFF};return e;}
static wp12_event_t ev(wp12_event_type_t t,uint16_t v,uint8_t l,uint8_t p){wp12_event_t e={t,v,l,p};return e;}

uint16_t wp12_tag_identity_window_updates(uint8_t v){return v<12?windows[v]:0;}
uint16_t wp12_tag_identity_voice_command(uint8_t v){return v<12?(uint16_t)(0x3100U|voices[v]):0;}
uint16_t wp12_tag_activation_command(uint8_t v){return v<=5?0x3031:0x3032;}
int wp12_tag_resolve_physical_slot(const wp12_prematch_t *s,uint8_t l){if(!s||l>=4)return -1;if(s->actors[primary[l]].active)return primary[l];return s->actors[fallback[l]].active?fallback[l]:-1;}
static int logical(uint8_t st,const uint8_t states[4]){for(int i=0;i<4;i++)if(states[i]==st)return i;return -1;}
static uint16_t duration(const wp12_prematch_t *s,uint8_t st){int l=logical(st,id_states);if(l>=0){int p=wp12_tag_resolve_physical_slot(s,(uint8_t)l);return p>=0?wp12_tag_identity_window_updates(s->actors[p].plus56_identity_program):0;}return st<24?fixed_updates[st]:0;}

void wp12_init_tag(wp12_prematch_t *s,const wp12_actor_t t[6]){
 zero_mem(s,sizeof(*s));s->mode=WP12_MODE_TAG;s->phase=WP12_PHASE_VISIBLE;s->presenter_active=true;s->entry_audio_pending=true;
 if(t)copy_mem(s->actors,t,sizeof(s->actors));
 for(unsigned i=0;i<4;i++){s->actors[i].x_machine=wp12_tag_primary_xy_machine[i][0];s->actors[i].y_machine=wp12_tag_primary_xy_machine[i][1];s->actors[i].z_machine=0x0140;}
 s->presenter_x_machine=0x0280;s->presenter_y_half_machine=0x0178U*2U;s->presenter_z_machine=0x0140;s->presenter_visual_selector=2;
}
void wp12_init_royal(wp12_prematch_t *s,const wp12_actor_t t[6]){
 zero_mem(s,sizeof(*s));s->mode=WP12_MODE_ROYAL;s->phase=WP12_PHASE_VISIBLE;s->entry_audio_pending=true;if(t)copy_mem(s->actors,t,sizeof(s->actors));
 for(unsigned i=0;i<6;i++){s->actors[i].x_machine=wp12_royal_xy_machine[i][0];s->actors[i].y_machine=wp12_royal_xy_machine[i][1];s->actors[i].orientation_word=wp12_royal_orientation[i];}
}
uint16_t wp12_presenter_y_machine_floor(const wp12_prematch_t *s){return s?(uint16_t)(s->presenter_y_half_machine/2U):0;}

static wp12_event_t teardown(wp12_prematch_t *s){
 switch(s->teardown_stage++){case 0:return ev(WP12_EVENT_AUDIO_RAW,0x3100,0xFF,0xFF);case 1:return ev(WP12_EVENT_AUDIO_RAW,0x3120,0xFF,0xFF);case 2:return ev(WP12_EVENT_AUDIO_RAW,0x312F,0xFF,0xFF);default:s->phase=WP12_PHASE_HANDOFF;return ev(WP12_EVENT_HANDOFF_GAMEPLAY,0,0xFF,0xFF);}
}
static void presenter(wp12_prematch_t *s){
 if(s->tag_controller_state==0x15){if(s->presenter_y_half_machine<0x01A7U*2U)s->presenter_y_half_machine++;unsigned p=s->state_tick/12U;if(p>3)p=3;s->presenter_visual_selector=(uint16_t)(8U+p);}
 else if(s->tag_controller_state==0x16){unsigned p=s->state_tick/12U;if(p>3)p=3;s->presenter_visual_selector=(uint16_t)(4U+p);}
 else if(s->tag_controller_state==0x17){unsigned p=s->state_tick/12U;if(p>3)p=3;s->presenter_visual_selector=(uint16_t)(13U+p);}
}
static wp12_event_t tag(wp12_prematch_t *s){
 if(s->tag_controller_state>=0x18){s->presenter_active=false;s->phase=WP12_PHASE_TEARDOWN;s->teardown_stage=0;return ev(WP12_EVENT_PRESENTER_CLEAR,0,0xFF,0xFF);}
 uint8_t st=s->tag_controller_state;
 if(s->state_tick==0){
  if(audio[st]){s->state_tick++;s->total_updates++;return ev(WP12_EVENT_AUDIO_RAW,audio[st],0xFF,0xFF);}
  int l=logical(st,id_states);if(l>=0){int p=wp12_tag_resolve_physical_slot(s,(uint8_t)l);if(p>=0){uint16_t v=wp12_tag_identity_voice_command(s->actors[p].plus57_voice_identity);s->state_tick++;s->total_updates++;return ev(WP12_EVENT_TAG_IDENTITY,v,(uint8_t)l,(uint8_t)p);}}
  l=logical(st,act_states);if(l>=0){int p=wp12_tag_resolve_physical_slot(s,(uint8_t)l);if(p>=0){s->actors[p].local_state=1;s->actors[p].phase_counter_plus22=0;s->actors[p].visual_selector_plus04=0x000C;uint16_t v=wp12_tag_activation_command(s->actors[p].plus56_identity_program);s->state_tick++;s->total_updates++;return ev(WP12_EVENT_TAG_ACTIVATE,v,(uint8_t)l,(uint8_t)p);}}
 }
 presenter(s);uint16_t d=duration(s,st);s->state_tick++;s->total_updates++;if(d&&s->state_tick>=d){s->tag_controller_state++;s->state_tick=0;}return none();
}
static wp12_event_t royal(wp12_prematch_t *s){
 for(unsigned i=0;i<6;i++)if(s->actors[i].active&&s->actors[i].local_state==0){s->actors[i].phase_counter_plus22++;if(s->actors[i].phase_counter_plus22>=0x40){s->phase=WP12_PHASE_TEARDOWN;s->teardown_stage=0;s->total_updates++;return ev(WP12_EVENT_AUDIO_RAW,0x312F,0xFF,(uint8_t)i);}}
 s->total_updates++;return none();
}
wp12_event_t wp12_step(wp12_prematch_t *s,wp12_input_t in){
 if(!s||s->phase==WP12_PHASE_INACTIVE||s->phase==WP12_PHASE_HANDOFF)return none();
 if(s->phase==WP12_PHASE_TEARDOWN)return teardown(s);
 if(in.action_mask_0030_any_port){s->skipped=true;s->presenter_active=false;s->phase=WP12_PHASE_TEARDOWN;s->teardown_stage=0;return none();}
 if(s->entry_audio_pending){s->entry_audio_pending=false;wp12_event_t e=ev(WP12_EVENT_AUDIO_RAW,0x3132,0xFF,0xFF);if(s->mode==WP12_MODE_TAG)(void)tag(s);else (void)royal(s);return e;}
 return s->mode==WP12_MODE_TAG?tag(s):royal(s);
}
