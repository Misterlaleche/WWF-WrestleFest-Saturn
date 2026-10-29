#ifndef TEST_YAUL_H
#define TEST_YAUL_H
#include <stddef.h>
#include <stdint.h>
#define __aligned(n) __attribute__((aligned(n)))
#define CPU_FRT_CLOCK_DIV_8 0
#define CPU_FRT_NTSC_320_8_COUNT_1MS 3359u
#define VDP2_TVMD_TV_STANDARD_PAL 1u
/* Hardware boundary used by the FRT regression. Field names match Yaul. */
typedef struct {
    uint8_t tier, ftcsr, frch, frcl, ocrah, ocral, tcr, tocr;
} cpu_ioregs_t;
extern cpu_ioregs_t test_cpu_ioregs;
#define CPU_IOREG_BASE ((uintptr_t)&test_cpu_ioregs)
unsigned vdp2_tvmd_tv_standard_get(void);
int cd_block_sectors_read(uint32_t fad, void *dst, size_t bytes);
uint32_t cpu_frt_count_get(void);
void cpu_frt_count_set(uint32_t count);
void cpu_frt_init(unsigned clock);
void cpu_frt_oca_set(unsigned count, void (*handler)(void));
void cpu_frt_interrupt_priority_set(unsigned priority);
uint8_t cpu_intc_mask_get(void);
void cpu_intc_mask_set(uint8_t mask);
#endif
