#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "wf_r17_referee.h"
#include "wf_r17_contract.h"
#include "wf_gameplay_bindings.h"

static unsigned display_calls,clear_calls,audio_calls;static uint16_t audio_log[8];
static wf_status_t display_ok(wf_runtime_t*rt,void*u){(void)rt;(void)u;display_calls++;return WF_OK;}
static wf_status_t clear_ok(wf_runtime_t*rt,void*u){(void)rt;(void)u;clear_calls++;return WF_OK;}
static wf_status_t audio_ok(wf_runtime_t*rt,uint16_t cmd,void*u){(void)rt;(void)u;if(audio_calls<8u)audio_log[audio_calls]=cmd;audio_calls++;return WF_OK;}
static wf_status_t resolver_no_contact(wf_runtime_t*rt,wf_actor_raw_t*a,void*u){(void)rt;(void)u;wf_set_be16(a,WF_A_CONTACT,0u);return WF_OK;}
static wf_status_t direction_arrive(wf_runtime_t*rt,wf_actor_raw_t*a,void*u){(void)rt;(void)u;wf_set_be16(a,WF_A_X,wf_be16(a,0x60u));wf_set_be16(a,WF_A_Y,wf_be16(a,0x62u));return WF_OK;}
static void active(wf_actor_raw_t*a){wf_set_be16(a,WF_A_ACTIVE,0x8000u);}
static wf_r17_referee_ctx_t ctx0(void){wf_r17_referee_ctx_t c;memset(&c,0,sizeof(c));c.display_2067c=display_ok;c.clear_206fe=clear_ok;c.audio_2052=audio_ok;c.resolver_280dc=resolver_no_contact;c.direction_20c8=direction_arrive;return c;}
static void link_pair(wf_runtime_t*rt,unsigned a,unsigned b){wf_set_be32(&rt->actors[a],WF_A_LINK,wf_actor_rom_base(b));}

static void test_init(void){wf_runtime_t rt;memset(&rt,0,sizeof(rt));wf_r17_referee_init_10718(&rt);assert(wf_be16(&rt.referee,0)==0x8000u);assert(wf_be16(&rt.referee,2)==0x000cu);assert(wf_be16(&rt.referee,WF_A_X)==0x0280u);assert(wf_be16(&rt.referee,WF_A_Y)==0x0198u);assert(wf_be16(&rt.referee,WF_A_Z)==0x0140u);assert(wf_be16(&rt.referee,WF_A_STATE)==0x8000u);assert(rt.g.global_1697==1u);puts("PASS referee fixed actor $10718/$1C11F4 init");}
static void test_tables(void){static const uint32_t p[11]={0x1f97e,0x1f9e0,0x1f9f8,0x1fa82,0x1fa40,0x1f99e,0x1f9d8,0x1f9f8,0x1fa40,0x1faec,0x1faec};static const uint32_t a[11]={0x1fb46,0x1fc22,0x1fc22,0x1fd5e,0x1fc22,0x1ff52,0x20022,0x202ba,0x20354,0x203ee,0x1ff52};for(unsigned i=0;i<11;i++){assert(wf_r17_ref_primary_table_1f952[i]==p[i]);assert(wf_r17_ref_action_table_1fb1a[i]==a[i]);}puts("PASS referee primary/action dispatch tables");}
static void test_raw48_acquire(void){wf_runtime_t rt;int found=0;memset(&rt,0,sizeof(rt));rt.g.credited_route=1u;active(&rt.actors[2]);wf_r17_raw48_pin_trigger_133a2(&rt.actors[2]);assert((wf_u8(&rt.actors[2],WF_A_PINFLAGS)&1u)!=0u);assert(wf_r17_ref_acquire_20556(&rt,&rt.referee,&found)==WF_OK);assert(found==1);assert(wf_be16(&rt.referee,WF_A_STATE)==0x8001u);assert(wf_be32(&rt.referee,WF_A_ROLE)==wf_actor_rom_base(2));puts("PASS raw48 $133A2 trigger -> $20556 acquisition");}
static void test_approach_link(void){wf_runtime_t rt;wf_r17_referee_ctx_t c=ctx0();memset(&rt,0,sizeof(rt));active(&rt.actors[0]);wf_r17_raw48_pin_trigger_133a2(&rt.actors[0]);wf_set_be16(&rt.actors[0],WF_A_X,0x0200u);wf_set_be16(&rt.actors[0],WF_A_Y,0x0180u);wf_set_be32(&rt.referee,WF_A_ROLE,wf_actor_rom_base(0));wf_set_be16(&rt.referee,WF_A_STATE,0x8001u);wf_set_be16(&rt.referee,WF_A_X,0x0200u);wf_set_be16(&rt.referee,WF_A_Y,0x0180u);assert(wf_r17_referee_logic_1f914(&rt,&rt.referee,&c)==WF_OK);assert(wf_be32(&rt.referee,WF_A_ROLE)==wf_actor_rom_base(0));assert(wf_be16(&rt.referee,WF_A_STATE)==0x8006u);assert(wf_be16(&rt.referee,WF_A_Y)==0x0181u);puts("PASS $1FC22 approach/link preservation and state6 handoff");}
static void prep_count(wf_runtime_t*rt,uint8_t phase,uint8_t timer,int royal){memset(rt,0,sizeof(*rt));active(&rt->actors[0]);active(&rt->actors[1]);wf_r17_raw48_pin_trigger_133a2(&rt->actors[0]);link_pair(rt,0,1);wf_set_be32(&rt->referee,WF_A_ROLE,wf_actor_rom_base(0));wf_set_be16(&rt->referee,WF_A_STATE,6u);wf_set_be16(&rt->referee,WF_A_LATCHED,6u);wf_set_u8(&rt->referee,WF_A_ACTION_PHASE,phase);wf_set_u8(&rt->referee,WF_A_ANIM_COUNTER+1u,timer);if(royal)rt->g.mode_0161=1u;}
static void test_count_entry_clears_word24(void){wf_runtime_t rt;wf_r17_referee_ctx_t c=ctx0();prep_count(&rt,3u,0u,0);wf_set_u8(&rt.referee,WF_A_LATCHED,(uint8_t)(wf_u8(&rt.referee,WF_A_LATCHED)&0x7fu));assert(wf_r17_referee_logic_1f914(&rt,&rt.referee,&c)==WF_OK);assert(wf_be16(&rt.referee,WF_A_ANIM_INDEX)==0u);assert(wf_u8(&rt.referee,WF_A_ACTION_PHASE)==0u);assert(wf_u8(&rt.referee,WF_A_ANIM_COUNTER+1u)==19u);assert(wf_be16(&rt.referee,0x54u)==0x3162u);puts("PASS $20022 ROM CLR.W +$24 clears +$24/+25");}
static void test_even_phase(void){wf_runtime_t rt;wf_r17_referee_ctx_t c=ctx0();display_calls=audio_calls=0;prep_count(&rt,1u,1u,0);wf_set_u8(&rt.referee,WF_A_LATCHED,(uint8_t)(wf_u8(&rt.referee,WF_A_LATCHED)|0x80u));wf_set_be16(&rt.referee,0x54u,0x3162u);wf_set_u8(&rt.referee,WF_A_MIRROR+1u,4u);assert(wf_r17_referee_logic_1f914(&rt,&rt.referee,&c)==WF_OK);assert(wf_u8(&rt.referee,WF_A_ACTION_PHASE)==2u);assert(rt.g.global_169a==0x0100u);assert(rt.g.global_169b==0u);assert(display_calls==1u);assert(audio_calls==1u&&audio_log[0]==0x3162u);assert(wf_be16(&rt.referee,0x54u)==0x3163u);puts("PASS even count phase display/audio producer sequence");}
static void test_cancel(void){wf_runtime_t rt;wf_r17_referee_ctx_t c=ctx0();clear_calls=0;prep_count(&rt,5u,10u,0);wf_set_u8(&rt.actors[0],WF_A_PINFLAGS,0u);assert(wf_r17_referee_logic_1f914(&rt,&rt.referee,&c)==WF_OK);assert(clear_calls==1u);assert((wf_be16(&rt.referee,WF_A_STATE)&0x7fffu)==5u);puts("PASS pre-lock cancel +$25<6");prep_count(&rt,6u,10u,0);wf_set_u8(&rt.actors[0],WF_A_PINFLAGS,0u);clear_calls=0;assert(wf_r17_referee_logic_1f914(&rt,&rt.referee,&c)==WF_OK);assert(clear_calls==0u);assert((wf_be16(&rt.referee,WF_A_STATE)&0x7fffu)==6u);puts("PASS no false cancel +$25>=6");}
static void test_terminals(void){wf_runtime_t rt;wf_r17_referee_ctx_t c=ctx0();clear_calls=0;prep_count(&rt,6u,1u,0);wf_set_u8(&rt.referee,WF_A_LATCHED,(uint8_t)(wf_u8(&rt.referee,WF_A_LATCHED)|0x80u));assert(wf_r17_referee_logic_1f914(&rt,&rt.referee,&c)==WF_OK);assert((wf_be16(&rt.referee,WF_A_STATE)&0x7fffu)==9u);assert(wf_be16(&rt.actors[0],0x4au)==4u);assert(clear_calls==1u);puts("PASS TAG phase7 terminal");prep_count(&rt,6u,1u,1);wf_set_u8(&rt.referee,WF_A_LATCHED,(uint8_t)(wf_u8(&rt.referee,WF_A_LATCHED)|0x80u));clear_calls=0;assert(wf_r17_referee_logic_1f914(&rt,&rt.referee,&c)==WF_OK);assert((wf_be16(&rt.referee,WF_A_STATE)&0x7fffu)==5u);assert(wf_be16(&rt.actors[0],0x4au)==0u);assert(clear_calls==1u);puts("PASS Royal terminal divergence");}
static void test_result_rejects_invalid_pair(void){
 wf_runtime_t rt;wf_r17_referee_ctx_t c=ctx0();memset(&rt,0,sizeof rt);
 for(unsigned i=0;i<4u;i++)active(&rt.actors[i]);
 wf_set_be32(&rt.actors[0],WF_A_TEAM_LINK,wf_actor_rom_base(1));
 wf_set_be32(&rt.actors[1],WF_A_TEAM_LINK,wf_actor_rom_base(0));
 wf_set_be32(&rt.actors[2],WF_A_TEAM_LINK,wf_actor_rom_base(3));
 wf_set_be32(&rt.actors[3],WF_A_TEAM_LINK,wf_actor_rom_base(2));
 wf_set_be32(&rt.actors[0],WF_A_TARGET,wf_actor_rom_base(2));
 wf_set_be32(&rt.referee,WF_A_ROLE,wf_actor_rom_base(0));
 wf_set_be16(&rt.referee,WF_A_STATE,9u);
 wf_set_be16(&rt.referee,WF_A_LATCHED,0x8009u);
 wf_set_be16(&rt.referee,WF_A_MIRROR,17u);
 wf_set_be16(&rt.referee,WF_A_ANIM_COUNTER,1u);
 assert(wf_r17_referee_logic_1f914(&rt,&rt.referee,&c)==WF_BLOCKED);
 assert(c.blocked_dependency_rom==0x20482u);
 assert(!(wf_u8(&rt.actors[2],WF_A_LOCKOUT)&0x80u));
 puts("PASS result requires the original grapple pair; no target substitution");
}
static void test_idle_reacquires_pin(void){
 for(unsigned locked=0;locked<2;locked++){
  wf_runtime_t rt;wf_r17_referee_ctx_t c=ctx0();memset(&rt,0,sizeof rt);
  rt.g.mode_0161=1u;rt.g.credited_route=1u;
  active(&rt.actors[0]);active(&rt.actors[2]);
  wf_set_be16(&rt.actors[0],WF_A_LOCKOUT,locked?0x8000u:0u);
  wf_set_u8(&rt.actors[2],WF_A_PINFLAGS,1u);
  wf_set_be32(&rt.referee,WF_A_ROLE,wf_actor_rom_base(0));
  wf_set_be16(&rt.referee,WF_A_LATCHED,0x8000u);
  assert(wf_r17_referee_logic_1f914(&rt,&rt.referee,&c)==WF_OK);
  assert(wf_be32(&rt.referee,WF_A_ROLE)==wf_actor_rom_base(locked?0u:2u));
  assert(wf_be16(&rt.referee,WF_A_STATE)==(locked?0u:0x8006u));
 }
 puts("PASS referee reacquires a pin only while previous actor is unlocked");
}
static void test_render_and_scheduler(void){wf_runtime_t rt;wf_gameplay_binding_context_t g;wf_phase_binding_t b[40];size_t n;memset(&rt,0,sizeof(rt));n=wf_gameplay_bindings_build(&g,b,40);assert(n>=29u);wf_runtime_init(&rt,b,n);rt.g.rng_state_valid=1u;rt.g.rng_state=0x13579bdfu;g.camera_valid=1u;g.camera.scroll0_x=0x0140;g.camera.scroll0_y=0x0200;g.cabinet_words_valid=1u;g.cabinet_word_140020=g.cabinet_word_140022=0xffffu;rt.g.mode_0161=1u;wf_r17_referee_init_10718(&rt);assert(wf_phase_referee_1f914(&rt,0x1f914u,&g)==WF_OK);assert(g.render_queue.count[0]==1u&&g.render_queue.list[0][0]==&rt.referee);assert(wf_be16(&rt.referee,WF_A_SCREEN_X)==0x0140u);puts("PASS $247C/$27B8 referee render registration");wf_r17_render_queue_clear(&g.render_queue);assert(wf_runtime_frame(&rt)==WF_BLOCKED);assert(rt.trace_count==23u);assert(rt.trace[22]==0x008cf8u);assert(rt.trace[8]==0x01c150u&&rt.trace[9]==0x01f914u&&rt.trace[10]==0x00f4c2u&&rt.trace[11]==0x024062u&&rt.trace[12]==0x024e58u&&rt.trace[13]==0x00fdeeu&&rt.trace[14]==0x010120u&&rt.trace[15]==0x010e6au);puts("PASS scheduler crosses referee/F4C2/$24062/$24E58/$FDEE and crosses $10E6A and crosses $7A00 and blocks at $8CF8");}
static void test_fail_closed(void){wf_runtime_t rt;wf_r17_referee_ctx_t c;memset(&c,0,sizeof(c));prep_count(&rt,1u,1u,0);wf_set_u8(&rt.referee,WF_A_LATCHED,(uint8_t)(wf_u8(&rt.referee,WF_A_LATCHED)|0x80u));wf_set_be16(&rt.referee,0x54u,0x3162u);assert(wf_r17_referee_logic_1f914(&rt,&rt.referee,&c)==WF_BLOCKED);assert(c.blocked_dependency_rom==0x0002067cu);puts("PASS unresolved count graphics remain fail-closed; no fabricated kickout/submission/rope-break");}
static void test_visibility(void){wf_actor_raw_t a;memset(&a,0,sizeof(a));wf_set_be16(&a,WF_A_SCREEN_X,(uint16_t)-63);wf_set_be16(&a,WF_A_SCREEN_Y,(uint16_t)-63);assert(wf_r17_visibility_2188_exact(&a,0)==WF_R17_YES);wf_set_be16(&a,WF_A_SCREEN_X,383u);assert(wf_r17_visibility_2188_exact(&a,0)==WF_R17_NO);wf_set_be16(&a,WF_A_SCREEN_X,0u);wf_set_be16(&a,WF_A_SCREEN_Y,319u);assert(wf_r17_visibility_2188_exact(&a,0)==WF_R17_NO);puts("PASS $2188 signed visibility bounds");}
int main(void){test_init();test_tables();test_raw48_acquire();test_approach_link();test_count_entry_clears_word24();test_even_phase();test_cancel();test_terminals();test_result_rejects_invalid_pair();test_idle_reacquires_pin();test_render_and_scheduler();test_fail_closed();test_visibility();puts("PASS test_runtime17_referee");return 0;}
