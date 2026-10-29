#ifndef WP0_TEST_YAUL_H
#define WP0_TEST_YAUL_H
#include <stdint.h>
#include <stddef.h>
typedef struct {
    uint16_t tvmr,fbcr,ptmr,ewdr,ewlr,ewrr,endr,reserved;
    uint16_t edsr,lopr,copr,modr;
} vdp1_ioregs_t;
typedef struct { uint16_t tvmd; } vdp2_ioregs_t;
extern volatile vdp1_ioregs_t test_vdp1;
extern volatile vdp2_ioregs_t test_vdp2;
#define VDP1_IOREG_BASE ((uintptr_t)&test_vdp1)
#define VDP2_IOREG_BASE ((uintptr_t)&test_vdp2)
#define VDP1_VRAM(n) ((uintptr_t)0x25C00000UL+(n))
#define SCU_IC_INTERRUPT_SPRITE_END 0x4D
#define SCU_IC_MASK_ALL 0xBFFFU
#define SCU_IC_MASK_SPRITE_END 0x2000U
#define SCU_IC_IST_SPRITE_END 0x2000U
typedef uint32_t scu_ic_status_reg_t;
void scu_ic_ihr_set(unsigned vector,void (*handler)(void));
void scu_ic_mask_chg(uint32_t and_mask,uint32_t or_mask);
void scu_ic_status_set(scu_ic_status_reg_t value);
uint32_t cpu_intc_mask_get(void);
void cpu_intc_mask_set(uint32_t value);
uint16_t cpu_frt_count_get(void);
void cpu_cache_purge(void);
void scu_dma_transfer(unsigned level,void *dst,const void *src,size_t bytes);
void scu_dma_transfer_wait(unsigned level);
void vdp2_tvmd_display_clear(void);
#endif
