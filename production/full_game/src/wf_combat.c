#include "wf_combat.h"
#include "wf_r18_referee.h"
#include "wf_r18_match_result.h"
#include "wf_r18_present.h"
#include "wf_r18_rom_hot_pages.h"
#include "wp1980_royal_rom_pages.h"
#include <string.h>
wf_status_t wf_combat_preload_pages(unsigned royal){
 if(!royal){
  wf_status_t st=wf_r18_rom_pages_preload(wf_r18_rom_hot_pages,sizeof wf_r18_rom_hot_pages/2);
#ifdef WF_COMBAT_TAG_TRANSPORT
  /* Original14 pinned pages plus the9 demands observed through the
   * integrated match's real finish. Keep all original LWRAM pages. */
  static const uint16_t pages[]={48,129,156,162,163,165,227,231,241,243,249,261,283,285,286,287,289,292,294,352,376,418,419};
  return st==WF_OK?wf_r18_rom_extra_pages_preload(pages,sizeof pages/2):st;
#else
  return st==WF_OK?wf_r18_rom_extra_pages_preload(wf_r18_rom_tag_extra_pages,sizeof wf_r18_rom_tag_extra_pages/2):st;
#endif
 }
#ifdef WF_COMBAT_BASE_ROYAL
 /* Keep all198 original main pages and39 fast pages. The integrated
  * lineup adds16 pinned pages; its smaller atlas frees their HWRAM. */
 static const uint16_t added[]={129,318,323,324,325,326,327,328,329,330,331,332,334,335,337,338};
 _Static_assert(sizeof wp1980_rom_fast_pages/2+sizeof added/2<=55,"Royal fast ROM lease");
 uint16_t pages[55];unsigned i=0,j=0,n=0;
 while(i<sizeof wp1980_rom_fast_pages/2||j<sizeof added/2){
  if(j==sizeof added/2||(i<sizeof wp1980_rom_fast_pages/2&&wp1980_rom_fast_pages[i]<added[j]))pages[n++]=wp1980_rom_fast_pages[i++];
  else pages[n++]=added[j++];
 }
 wf_status_t st=wf_r18_rom_pages_preload(wp1980_rom_main_pages,sizeof wp1980_rom_main_pages/2);
 return st==WF_OK?wf_r18_rom_extra_pages_preload(pages,n):st;
#else
 /* Preserve HW1's entire237-page Royal set. The universal lease has209
  * LWRAM slots: move the last11 of39 fast pages there, leaving28 in HWRAM.
  * Only immutable residency changes; misses outside the set still stream. */
 _Static_assert(sizeof wp1980_rom_main_pages/2==198&&sizeof wp1980_rom_fast_pages/2==39,"locked HW1 Royal page sets");
 uint16_t pages[209];unsigned i=0,j=28,n=0;
 while(i<198||j<39){
  if(j==39||(i<198&&wp1980_rom_main_pages[i]<wp1980_rom_fast_pages[j]))pages[n++]=wp1980_rom_main_pages[i++];
  else pages[n++]=wp1980_rom_fast_pages[j++];
 }
 wf_status_t st=wf_r18_rom_pages_preload(pages,n);
 return st==WF_OK?wf_r18_rom_extra_pages_preload(wp1980_rom_fast_pages,28):st;
#endif
}
static uint32_t read32(const uint8_t*p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|p[2]<<8|p[3];}
wf_status_t wf_combat_read_ram(wf_runtime_t*r,uint32_t at,uint16_t*v,void*u){
 if(!r||!v)return WF_BAD_STATE;
 switch(at){
 case 0x1c0062:case 0x1c0063:*v=r->g.global_0063;return WF_OK;
 case 0x1c004e:*v=(uint16_t)(r->g.credit_remainder_004e<<8|r->g.credit_count_004f);return WF_OK;
 case 0x1c004a:*v=r->g.credit_cycle_units_004b;return WF_OK;
 default:if(u)((wf_r18_video_t*)u)->failed_address=at;return WF_BLOCKED;
 }
}
wf_status_t wf_combat_join(wf_combat_t*c,wf_session_t*s,const wf_combat_io_t*io){
 if(!c||!s||!s->initialized||s->phase!=WF_SESSION_GAMEPLAY||s->runtime.frame_stage!=3||!io||
  !io->read_rom||!io->input||!io->audio||!io->wait||!io->render||!io->reset||!io->barrier)return WF_BAD_STATE;
 memset(c,0,sizeof *c);
 if(wf_gameplay_bindings_build(&c->gameplay,c->bindings,36)!=36)return WF_BAD_STATE;
 wf_runtime_t*r=&s->runtime;wf_gameplay_binding_context_t*g=&c->gameplay;
 for(unsigned side=0;side<2;side++)for(unsigned i=0;i<16;i++){
  const uint8_t*p=s->ram+0x16d6+side*128+i*8;c->world.cache[side][i]=(wf_r18_world_transport_t){read32(p),read32(p+4)};
 }
 c->world.read_rom=io->read_rom;
 s->video.read_rom=io->read_rom;s->video.read_ram=wf_combat_read_ram;s->video.read_user=&s->video;
 g->cabinet_words_valid=1;g->cabinet_word_140020=g->cabinet_word_140022=0xffff;
 g->camera_valid=1;g->camera.scene_index=r->g.global_007e;
 g->camera.scroll0_x=r->g.global_17e6;g->camera.scroll1_x=r->g.global_17ea;
 g->camera.scroll0_y=r->g.global_17ee;g->camera.scroll1_y=r->g.global_17f2_word;
 g->input_valid_mask=0x1ff;memset(g->input_hw_low,255,sizeof g->input_hw_low);
 g->audio_command=io->audio;g->participant_input_word=io->input;g->post7_wait_21e6=io->wait;g->post7_render_2836_batch=io->render;
 g->referee_direction_20c8=wf_r18_referee_direction_20c8;g->referee_display_2067c=wf_r18_referee_display_2067c;
 g->referee_number_2067c=wf_r18_referee_number_2067c;g->referee_clear_206fe=wf_r18_referee_clear_206fe;
 g->referee_tag_phase6_90d6=wf_r18_tag_bonus_90d6;g->referee_result_20156=wf_r18_result_sound_20156;
 g->participant_commit_batch=wf_r18_video_commit;g->post7_fg0_batch=g->post7_video_batch=wf_r18_video_batch;
 g->post7_resource_2aea=wf_r18_palette_2aea;g->post7_resource_2b58=wf_r18_palette_2b58;
 g->post7_raw_2503c=wf_r18_text_2503c;g->post7_raw_9aca=wf_r18_strip_9aca;g->post7_fg0_peek_long=wf_r18_video_peek;
 g->transition_reset=io->reset;g->transition_barrier=io->barrier;g->r17_owner_user=&s->video;
 r->bindings=c->bindings;r->binding_count=36;return WF_OK;
}
