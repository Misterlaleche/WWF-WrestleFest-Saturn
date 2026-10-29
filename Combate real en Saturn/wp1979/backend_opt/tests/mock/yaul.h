#ifndef WP1979_TEST_YAUL_H
#define WP1979_TEST_YAUL_H
#include <stdint.h>
#include <stddef.h>
#define __aligned(n) __attribute__((aligned(n)))
typedef struct { int16_t x,y; } int16_vec2_t;
#define INT16_VEC2(x,y) ((int16_vec2_t){(x),(y)})
typedef struct {
 uint16_t cmd_ctrl,cmd_link,cmd_pmod,cmd_colr,cmd_srca,cmd_size;
 int16_t cmd_xa,cmd_ya,cmd_xb,cmd_yb,cmd_xc,cmd_yc,cmd_xd,cmd_yd;
 uint16_t cmd_grda,reserved;
} vdp1_cmdt_t;
_Static_assert(sizeof(vdp1_cmdt_t)==32,"VDP1 command must contain 16 words");
typedef struct { vdp1_cmdt_t *cmdts; uint16_t count; } vdp1_cmdt_list_t;
typedef struct { unsigned bpp,color_mode,sprite_type,erase_color; int16_vec2_t erase_points[2]; } vdp1_env_t;
enum { VDP1_ENV_BPP_16=16,VDP1_ENV_COLOR_MODE_PALETTE=0,VDP1_SYNC_MODE_ERASE_CHANGE=0 };
#define RGB1555(a,r,g,b) ((uint16_t)(((a)<<15)|((b)<<10)|((g)<<5)|(r)))
extern uint8_t test_vram[0x80000];
#define VDP1_VRAM(off) (test_vram+(off))
static inline void vdp1_cmdt_system_clip_coord_set(vdp1_cmdt_t *c){c->cmd_ctrl=9;}
static inline void vdp1_cmdt_vtx_system_clip_coord_set(vdp1_cmdt_t*c,int16_vec2_t p){c->cmd_xc=p.x;c->cmd_yc=p.y;}
static inline void vdp1_cmdt_local_coord_set(vdp1_cmdt_t*c){c->cmd_ctrl=10;}
static inline void vdp1_cmdt_vtx_local_coord_set(vdp1_cmdt_t*c,int16_vec2_t p){c->cmd_xa=p.x;c->cmd_ya=p.y;}
static inline void vdp1_cmdt_end_set(vdp1_cmdt_t*c){c->cmd_ctrl=0x8000;}
static inline void vdp1_cmdt_list_init(vdp1_cmdt_list_t*l,vdp1_cmdt_t*c){l->cmdts=c;l->count=0;}
void scu_dma_transfer(unsigned channel,void *dst,const void*src,size_t bytes);
void scu_dma_transfer_wait(unsigned channel);
void vdp1_sync_cmdt_list_put(vdp1_cmdt_list_t*l,unsigned first);
void vdp1_sync_put_wait(void);
void vdp1_sync_render(void);
void vdp1_sync(void);
void vdp1_sync_wait(void);
static inline void vdp1_vram_partitions_set(unsigned a,unsigned b,unsigned c,unsigned d){(void)a;(void)b;(void)c;(void)d;}
static inline void vdp1_env_default_init(vdp1_env_t*e){*e=(vdp1_env_t){0};}
static inline void vdp1_env_set(vdp1_env_t*e){(void)e;}
static inline void vdp1_sync_interval_set(unsigned a){(void)a;}
static inline void vdp1_sync_mode_set(unsigned a){(void)a;}
static inline void vdp2_sprite_priority_set(unsigned a,unsigned b){(void)a;(void)b;}
static inline void vdp2_cram_mode_set(unsigned a){(void)a;}
#endif
