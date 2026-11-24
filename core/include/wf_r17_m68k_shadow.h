#ifndef WF_R17_M68K_SHADOW_H
#define WF_R17_M68K_SHADOW_H
#include <stdint.h>

typedef struct wf_r17_m68k_shadow {
    uint32_t d[8];
    uint8_t valid;
} wf_r17_m68k_shadow_t;

void wf_r17_m68k_software_reset(wf_r17_m68k_shadow_t *r);
/* Constant register indexes and write widths are visible in every caller.
 * GNU extern-inline retains the external ABI supplied by the .c file (also
 * for function pointers), while avoiding a generic call on the SH-2 path. */
#if defined(__GNUC__) && !defined(WF_R17_M68K_SHADOW_IMPLEMENTATION)
#define WF_R17_SHADOW_INLINE extern inline __attribute__((__gnu_inline__,__always_inline__))
WF_R17_SHADOW_INLINE void wf_r17_m68k_move_b(wf_r17_m68k_shadow_t *r,unsigned reg,uint8_t value){if(r&&reg<8u)r->d[reg]=(r->d[reg]&0xFFFFFF00u)|(uint32_t)value;}
WF_R17_SHADOW_INLINE void wf_r17_m68k_move_w(wf_r17_m68k_shadow_t *r,unsigned reg,uint16_t value){if(r&&reg<8u)r->d[reg]=(r->d[reg]&0xFFFF0000u)|(uint32_t)value;}
WF_R17_SHADOW_INLINE void wf_r17_m68k_move_l(wf_r17_m68k_shadow_t *r,unsigned reg,uint32_t value){if(r&&reg<8u)r->d[reg]=value;}
WF_R17_SHADOW_INLINE void wf_r17_m68k_moveq(wf_r17_m68k_shadow_t *r,unsigned reg,int8_t value){if(r&&reg<8u)r->d[reg]=(uint32_t)(int32_t)value;}
WF_R17_SHADOW_INLINE void wf_r17_m68k_clr_l(wf_r17_m68k_shadow_t *r,unsigned reg){if(r&&reg<8u)r->d[reg]=0u;}
#undef WF_R17_SHADOW_INLINE
#else
void wf_r17_m68k_move_b(wf_r17_m68k_shadow_t *r,unsigned reg,uint8_t value);
void wf_r17_m68k_move_w(wf_r17_m68k_shadow_t *r,unsigned reg,uint16_t value);
void wf_r17_m68k_move_l(wf_r17_m68k_shadow_t *r,unsigned reg,uint32_t value);
void wf_r17_m68k_moveq(wf_r17_m68k_shadow_t *r,unsigned reg,int8_t value);
void wf_r17_m68k_clr_l(wf_r17_m68k_shadow_t *r,unsigned reg);

#endif

/* ROM $21B4. Mutates D0 and state_1c005c; D1..D7 are preserved. */
uint32_t wf_r17_rng21b4_step(uint32_t *state_1c005c,uint32_t global_1c0080,uint32_t d[8]);
uint32_t wf_r17_rng21b4_live(uint32_t *state_1c005c,uint32_t global_1c0080,wf_r17_m68k_shadow_t *r);

#endif
