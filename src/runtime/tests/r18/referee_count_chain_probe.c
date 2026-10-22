#include <string.h>
#include "wf_gameplay_bindings.h"
#include "wf_r18_referee.h"
#include "wf_r18_match_result.h"
static uint16_t word(const uint8_t*b,unsigned a){return (uint16_t)((b[a]<<8)|b[a+1]);}
static uint32_t lng(const uint8_t*b,unsigned a){return ((uint32_t)word(b,a)<<16)|word(b,a+2);}
static void putw(uint8_t*b,unsigned a,uint16_t v){b[a]=(uint8_t)(v>>8);b[a+1]=(uint8_t)v;}
static void putl(uint8_t*b,unsigned a,uint32_t v){putw(b,a,(uint16_t)(v>>16));putw(b,a+2,(uint16_t)v);}
static wf_status_t read_rom(void *user,uint32_t at,void *dst,size_t n){if(at>0x80000u||n>0x80000u-at)return WF_BLOCKED;memcpy(dst,(const uint8_t*)user+at,n);return WF_OK;}
/* The real $2052 preserves all D registers and emits only a sound-device word.
 * The production build deliberately uses this same silent device boundary. */
static wf_status_t silent_audio(wf_runtime_t*r,uint16_t command,void*u){(void)r;(void)command;(void)u;return WF_OK;}
int referee_count_chain(uint8_t *ram,uint8_t *fg,uint32_t *regs,const uint8_t *rom,uint32_t *after){
 wf_runtime_t rt={0};wf_r18_video_t video={0};wf_gameplay_binding_context_t game={0};
 memcpy(rt.actors,ram+0x5b0,sizeof rt.actors);memcpy(rt.referee.b,ram+0x11f4,WF_ACTOR_BYTES);memcpy(rt.m68k_regs.d,regs,32);rt.m68k_regs.valid=1;
 rt.g.config_0066=word(ram,0x66);rt.g.credited_route=word(ram,0x7c);rt.g.mode_0161=ram[0x161];rt.g.progression_0162=word(ram,0x162);
 rt.g.global_169a=word(ram,0x169a);rt.g.global_169b=ram[0x169b];rt.g.global_169e=word(ram,0x169e);
 rt.g.rng_state=lng(ram,0x5c);rt.g.rng_state_valid=1;rt.g.frame_counter=lng(ram,0x80);
 memcpy(video.fg0,fg,sizeof video.fg0);video.read_rom=read_rom;video.read_user=(void*)rom;
 game.camera_valid=1;game.camera.scroll0_x=word(ram,0x17e6);game.camera.scroll0_y=word(ram,0x17ee);
 game.referee_direction_20c8=wf_r18_referee_direction_20c8;game.referee_display_2067c=wf_r18_referee_display_2067c;game.referee_clear_206fe=wf_r18_referee_clear_206fe;
 game.referee_tag_phase6_90d6=wf_r18_tag_bonus_90d6;game.referee_result_20156=wf_r18_result_sound_20156;game.audio_command=silent_audio;game.r17_owner_user=&video;
 wf_status_t result=wf_phase_referee_1f914(&rt,0x1f914,&game);
 /* Normalize the real native render queue to its original RAM representation. */
 putw(ram,0x19b8,game.render_queue.count[0]);
 for(unsigned i=0;i<game.render_queue.count[0];i++){
  if(game.render_queue.list[0][i]!=&rt.referee)return WF_BAD_STATE;
  putl(ram,0x19d0+i*4u,0x1c11f4u);
 }
 memcpy(ram+0x5b0,rt.actors,sizeof rt.actors);memcpy(ram+0x11f4,rt.referee.b,WF_ACTOR_BYTES);putw(ram,0x169a,rt.g.global_169a);putw(ram,0x169e,rt.g.global_169e);
 memcpy(fg,video.fg0,sizeof video.fg0);memcpy(regs,rt.m68k_regs.d,32);
 wf_r17_rng21b4_live(&rt.g.rng_state,rt.g.frame_counter,&rt.m68k_regs);putl(ram,0x5c,rt.g.rng_state);memcpy(after,rt.m68k_regs.d,32);
 return result;
}
