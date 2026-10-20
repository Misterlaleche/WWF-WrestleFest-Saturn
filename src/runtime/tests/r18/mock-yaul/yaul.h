/* Host double for the Yaul boundary. It records DMA and submitted commands;
 * this is not a replacement for the SH-2 build or emulator validation. */
#ifndef TEST_YAUL_H
#define TEST_YAUL_H
#include <stdint.h>
#include <stddef.h>
#define __aligned(n) __attribute__((aligned(n)))
typedef struct { int16_t x,y; } int16_vec2_t;
#define INT16_VEC2(x,y) ((int16_vec2_t){x,y})
typedef struct { unsigned raw,color_mode,cc_mode,trans_pixel_disable,end_code_disable; } vdp1_cmdt_draw_mode_t;
typedef struct { uint16_t raw; } vdp1_cmdt_color_bank_t;
typedef unsigned vdp1_cmdt_char_flip_t;
typedef struct { unsigned kind; vdp1_cmdt_draw_mode_t mode; } vdp1_cmdt_t;
typedef struct { vdp1_cmdt_t *cmdts; uint16_t count; } vdp1_cmdt_list_t;
typedef struct { unsigned bpp,color_mode,sprite_type,erase_color; int16_vec2_t erase_points[2]; } vdp1_env_t;
enum { VDP1_CMDT_CM_CB_16, VDP1_CMDT_CC_REPLACE, VDP1_CMDT_CHAR_FLIP_NONE, VDP1_CMDT_CHAR_FLIP_H, VDP1_CMDT_CHAR_FLIP_V, VDP1_ENV_BPP_16, VDP1_ENV_COLOR_MODE_PALETTE, VDP1_SYNC_MODE_ERASE_CHANGE };
extern uint8_t mock_vram[0x80000];extern uint32_t mock_cram[1024];
#define VDP1_VRAM(off) (mock_vram+(off))
#define VDP2_CRAM_MODE_2_OFFSET(a,b,c) (&mock_cram[(b)*16+(c)])
#define RGB1555(a,b,c,d) 0
void scu_dma_transfer(unsigned,void*,const void*,size_t);
void scu_dma_transfer_wait(unsigned);
void vdp1_sync_cmdt_list_put(vdp1_cmdt_list_t*,unsigned);
static inline void vdp1_cmdt_system_clip_coord_set(vdp1_cmdt_t*c){c->kind=1;}
static inline void vdp1_cmdt_local_coord_set(vdp1_cmdt_t*c){c->kind=2;}
static inline void vdp1_cmdt_normal_sprite_set(vdp1_cmdt_t*c){c->kind=3;}
static inline void vdp1_cmdt_end_set(vdp1_cmdt_t*c){c->kind=4;}
#define POS_STUB(name) static inline void name(vdp1_cmdt_t*c,int16_vec2_t v){(void)c;(void)v;}
POS_STUB(vdp1_cmdt_vtx_system_clip_coord_set)
POS_STUB(vdp1_cmdt_vtx_local_coord_set)
POS_STUB(vdp1_cmdt_vtx_zoom_point_set)
static inline void vdp1_cmdt_draw_mode_set(vdp1_cmdt_t*c,vdp1_cmdt_draw_mode_t d){c->mode=d;}
static inline void vdp1_cmdt_char_base_set(vdp1_cmdt_t*c,const void*p){(void)c;(void)p;}
static inline void vdp1_cmdt_char_size_set(vdp1_cmdt_t*c,unsigned x,unsigned y){(void)c;(void)x;(void)y;}
static inline void vdp1_cmdt_char_flip_set(vdp1_cmdt_t*c,unsigned f){(void)c;(void)f;}
static inline void vdp1_cmdt_color_mode0_set(vdp1_cmdt_t*c,vdp1_cmdt_color_bank_t f){(void)c;(void)f;}
static inline void vdp1_cmdt_list_init(vdp1_cmdt_list_t*l,vdp1_cmdt_t*c){l->cmdts=c;}
static inline void vdp2_cram_mode_set(unsigned x){(void)x;}
static inline void vdp1_vram_partitions_set(unsigned a,unsigned b,unsigned c,unsigned d){(void)a;(void)b;(void)c;(void)d;}
static inline void vdp1_env_default_init(vdp1_env_t*e){(void)e;}
static inline void vdp1_env_set(vdp1_env_t*e){(void)e;}
static inline void vdp1_sync_interval_set(unsigned x){(void)x;}
static inline void vdp1_sync_mode_set(unsigned x){(void)x;}
static inline void vdp2_sprite_priority_set(unsigned a,unsigned b){(void)a;(void)b;}
static inline void vdp1_sync_put_wait(void){}
static inline void vdp1_sync_render(void){}
extern unsigned mock_sync_calls;
static inline void vdp1_sync(void){mock_sync_calls++;}
static inline void vdp1_sync_wait(void){}
#endif
