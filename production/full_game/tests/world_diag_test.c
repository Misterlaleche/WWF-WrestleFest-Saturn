#include "wf_r18_world.h"
#include "wf_r18_ring.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

void wf_r18_ring_tile_restore(unsigned layer, unsigned cell) {(void)layer;(void)cell;}
int wf_r18_ring_tile_block_write(unsigned layer, unsigned first, const uint8_t *pixels)
{(void)layer;(void)first;(void)pixels;return 0;}
static wf_status_t blocked_read(void *u,uint32_t a,void *dst,size_t n)
{(void)u;(void)a;(void)dst;(void)n;return WF_BLOCKED;}
int main(void)
{
 wf_runtime_t rt={0}; wf_r18_world_ctx_t ctx={0};
 rt.g.global_007e=7;
 assert(wf_r18_world_commit(&rt,&ctx)==WF_BLOCKED);
 assert(wf_r18_world_error_info==0x01000000u);
 rt.g.global_007e=0;ctx.read_rom=blocked_read;ctx.animation_count[0]=1;
 assert(wf_r18_world_commit(&rt,&ctx)==WF_BLOCKED);
 assert(wf_r18_world_error_info==0x03000000u);
 assert(wf_r18_world_error_address==0x295a4);
 puts("PASS world diagnostic branch publication");
 return 0;
}
