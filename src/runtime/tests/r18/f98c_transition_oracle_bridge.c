#include "wf_r17_dependencies.h"
#include "wf_gameplay_bindings.h"
#include <string.h>
static unsigned barrier_entered;
static uint32_t b(const uint8_t*p,unsigned n){uint32_t v=0;while(n--)v=(v<<8)|*p++;return v;}
static void w(uint8_t*p,uint32_t v,unsigned n){for(unsigned i=0;i<n;i++)p[i]=(uint8_t)(v>>(8*(n-i-1)));}
static wf_status_t reset(wf_runtime_t*r,uint8_t mode,void*user){
 (void)mode;(void)user;
#ifdef HAVE_TRANSITION_RESET_RAM
 return wf_r17_transition_reset_ram(r);
#else
 (void)r;return WF_BLOCKED;
#endif
}
static wf_status_t barrier(wf_runtime_t*r,uint8_t mode,void*user){(void)r;(void)mode;(void)user;barrier_entered=1;return WF_BLOCKED;}
int f98c_transition_oracle(uint8_t*mem,uint32_t*regs,unsigned op){
 wf_runtime_t r={0};wf_gameplay_binding_context_t c={0};uint8_t*g=mem+0x1c0000;barrier_entered=0;
 memcpy(r.m68k_regs.d,regs,32);r.m68k_regs.valid=1;
 memcpy(r.actors,g+0x5b0,sizeof r.actors);memcpy(r.fixed_object0.b,g+0xf1c,0x10c);memcpy(r.fixed_object1.b,g+0x1028,0x10c);
 memcpy(r.g.transfer_queue_1808,g+0x1808,sizeof r.g.transfer_queue_1808);memcpy(r.branchb_list_182a,g+0x182a,48);memcpy(r.branchb_prev_185a,g+0x185a,48);memcpy(r.branchb_objs_188a,g+0x188a,192);
#define G(at,field,n) r.g.field=b(g+at,n)
 G(0x7e,global_007e,2);
 G(0x7f,global_007f,1);
 G(0x76,gameplay_vblank_enable,1);
 G(0x7c,credited_route,2);
 G(0x161,mode_0161,1);
 G(0x162,progression_0162,2);
 G(0x1670,global_1670,4);
 G(0x1674,global_1674,4);
 G(0x1678,global_1678,1);
 G(0x1679,global_1679,1);
 G(0x15f4,global_15f4,2);
 G(0x15f8,global_15f8,2);
 G(0x16d2,global_16d2,2);
 G(0x16d4,global_16d4,2);
 G(0x16e6,global_16e6,1);
 G(0x17d6,global_17d6,2);
 G(0x17d8,global_17d8,2);
 G(0x17da,global_17da,2);
 G(0x17dc,global_17dc,2);
 G(0x17de,global_17de,2);
 G(0x17e0,global_17e0,2);
 G(0x17e2,global_17e2,2);
 G(0x17e4,global_17e4,2);
 G(0x17e6,global_17e6,2);
 G(0x17ea,global_17ea,2);
 G(0x17ee,global_17ee,2);
 G(0x17f2,global_17f2_word,2);
 G(0x17f6,global_17f6,2);
 G(0x17fa,global_17fa,2);
 G(0x17fe,global_17fe,2);
 G(0x1800,global_1800,2);
 G(0x1802,global_1802,2);
 G(0x1804,global_1804,2);
 G(0x1806,global_1806,2);
 G(0x1828,transfer_count_1828,2);
#undef G
 c.transition_reset=reset;c.transition_barrier=barrier;
 wf_status_t status=op?reset(&r,0,0):wf_r17_f98c(&r,&c);
 memcpy(regs,r.m68k_regs.d,32);memcpy(g+0x5b0,r.actors,sizeof r.actors);memcpy(g+0xf1c,r.fixed_object0.b,0x10c);memcpy(g+0x1028,r.fixed_object1.b,0x10c);
 memcpy(g+0x1808,r.g.transfer_queue_1808,sizeof r.g.transfer_queue_1808);memcpy(g+0x182a,r.branchb_list_182a,48);memcpy(g+0x185a,r.branchb_prev_185a,48);memcpy(g+0x188a,r.branchb_objs_188a,192);
#define G(at,field,n) w(g+at,r.g.field,n)
 G(0x7e,global_007e,2);
 G(0x7f,global_007f,1);
 G(0x76,gameplay_vblank_enable,1);
 G(0x7c,credited_route,2);
 G(0x161,mode_0161,1);
 G(0x162,progression_0162,2);
 G(0x1670,global_1670,4);
 G(0x1674,global_1674,4);
 G(0x1678,global_1678,1);
 G(0x1679,global_1679,1);
 G(0x15f4,global_15f4,2);
 G(0x15f8,global_15f8,2);
 G(0x16d2,global_16d2,2);
 G(0x16d4,global_16d4,2);
 G(0x16e6,global_16e6,1);
 G(0x17d6,global_17d6,2);
 G(0x17d8,global_17d8,2);
 G(0x17da,global_17da,2);
 G(0x17dc,global_17dc,2);
 G(0x17de,global_17de,2);
 G(0x17e0,global_17e0,2);
 G(0x17e2,global_17e2,2);
 G(0x17e4,global_17e4,2);
 G(0x17e6,global_17e6,2);
 G(0x17ea,global_17ea,2);
 G(0x17ee,global_17ee,2);
 G(0x17f2,global_17f2_word,2);
 G(0x17f6,global_17f6,2);
 G(0x17fa,global_17fa,2);
 G(0x17fe,global_17fe,2);
 G(0x1800,global_1800,2);
 G(0x1802,global_1802,2);
 G(0x1804,global_1804,2);
 G(0x1806,global_1806,2);
 G(0x1828,transfer_count_1828,2);
#undef G
 return status?(barrier_entered?-(int)0x26e66:-(int)status):0;
}
