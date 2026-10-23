#include "wf_r17_referee.h"
#include "wf_r18_referee.h"
#include "wf_r18_match_result.h"
#include <string.h>
static uint32_t b(const uint8_t*p,unsigned n){uint32_t v=0;while(n--)v=(v<<8)|*p++;return v;}
static void w(uint8_t*p,uint32_t v,unsigned n){for(unsigned i=0;i<n;i++)p[i]=(uint8_t)(v>>(8*(n-i-1)));}
static wf_status_t rd(void*u,uint32_t at,void*d,size_t n){if(at>0x80000u||n>0x80000u-at)return WF_BLOCKED;memcpy(d,(uint8_t*)u+at,n);return WF_OK;}
static wf_status_t audio(wf_runtime_t*r,uint16_t cmd,void*u){(void)r;(void)cmd;(void)u;return WF_OK;}
int referee_ringside_oracle(uint8_t*mem,uint32_t*regs){
 wf_runtime_t r={0};wf_r18_video_t v={0};wf_r17_referee_ctx_t c={0};uint8_t*g=mem+0x1c0000;
 memcpy(r.actors,g+0x5b0,sizeof r.actors);memcpy(r.referee.b,g+0x11f4,0x64);memcpy(r.m68k_regs.d,regs,32);r.m68k_regs.valid=1;
#define G(at,field,n) r.g.field=b(g+at,n)
 G(0x66,config_0066,2);G(0x7c,credited_route,2);G(0x161,mode_0161,1);G(0x162,progression_0162,2);G(0x1682,global_1682,2);G(0x169a,global_169a,2);G(0x169b,global_169b,1);G(0x169e,global_169e,2);
#undef G
 memcpy(v.fg0,mem+0xc0000,sizeof v.fg0);v.read_rom=rd;v.read_user=mem;c.user=&v;c.display_2067c=wf_r18_referee_display_2067c;c.audio_2052=audio;c.tag_phase6_90d6=wf_r18_tag_bonus_90d6;c.result_20156=wf_r18_result_sound_20156;c.number_2067c=wf_r18_referee_number_2067c;c.text_2503c=wf_r18_text_2503c;
 wf_status_t status=wf_r17_referee_logic_1f914(&r,&r.referee,&c);
 memcpy(g+0x5b0,r.actors,sizeof r.actors);memcpy(g+0x11f4,r.referee.b,0x64);memcpy(regs,r.m68k_regs.d,32);memcpy(mem+0xc0000,v.fg0,sizeof v.fg0);
#define G(at,field,n) w(g+at,r.g.field,n)
 G(0x1682,global_1682,2);G(0x169a,global_169a,2);G(0x169e,global_169e,2);
#undef G
 return status?-(int)(c.blocked_dependency_rom?c.blocked_dependency_rom:status):0;
}
