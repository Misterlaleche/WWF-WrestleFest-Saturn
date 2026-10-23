#include <string.h>
#include "wf_gameplay_bindings.h"
static uint16_t b16(const uint8_t *p){return (uint16_t)((p[0]<<8)|p[1]);}
static void w16(uint8_t *p,uint16_t v){p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v;}
int wf_camera_oracle(unsigned op,uint8_t *mem,uint32_t *regs,uint16_t *cache){
 wf_runtime_t rt={0};wf_gameplay_binding_context_t c={0};uint8_t*g=mem+0x1c0000;
 memcpy(rt.actors,g+0x5b0,sizeof rt.actors);memcpy(rt.m68k_regs.d,regs,32);rt.m68k_regs.valid=1;
 rt.g.global_17e6=b16(g+0x17e6);rt.g.global_17ea=b16(g+0x17ea);rt.g.global_17ee=b16(g+0x17ee);rt.g.global_17f2_word=b16(g+0x17f2);
 rt.g.global_1804=b16(g+0x1804);rt.g.global_1806=b16(g+0x1806);rt.g.global_007e=b16(g+0x7e);rt.g.mode_0161=g[0x161];
 c.camera_valid=1;c.camera.scroll0_x=111;c.camera.scroll1_x=222;c.camera.scroll0_y=333;c.camera.scroll1_y=444;c.camera.scene_index=6;c.camera.target_x=555;c.camera.target_y=666;
 wf_status_t s=WF_OK;
 if(op!=1)s=wf_phase_camera_26936(&rt,0x26936,&c);
 if(s==WF_OK&&op!=0)s=wf_phase_camera_2983c(&rt,0x2983c,&c);
 w16(g+0x17e6,rt.g.global_17e6);w16(g+0x17ea,rt.g.global_17ea);w16(g+0x17ee,rt.g.global_17ee);w16(g+0x17f2,rt.g.global_17f2_word);
 cache[0]=(uint16_t)c.camera.scroll0_x;cache[1]=(uint16_t)c.camera.scroll1_x;cache[2]=(uint16_t)c.camera.scroll0_y;cache[3]=(uint16_t)c.camera.scroll1_y;
 memcpy(regs,rt.m68k_regs.d,32);return s;
}
