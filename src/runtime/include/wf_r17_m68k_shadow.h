#ifndef WF_R17_M68K_SHADOW_H
#define WF_R17_M68K_SHADOW_H
#include <stdint.h>

typedef struct wf_r17_m68k_shadow {
    uint32_t d[8];
    uint8_t valid;
} wf_r17_m68k_shadow_t;

void wf_r17_m68k_software_reset(wf_r17_m68k_shadow_t *r);
void wf_r17_m68k_move_b(wf_r17_m68k_shadow_t *r,unsigned reg,uint8_t value);
void wf_r17_m68k_move_w(wf_r17_m68k_shadow_t *r,unsigned reg,uint16_t value);
void wf_r17_m68k_move_l(wf_r17_m68k_shadow_t *r,unsigned reg,uint32_t value);
void wf_r17_m68k_moveq(wf_r17_m68k_shadow_t *r,unsigned reg,int8_t value);
void wf_r17_m68k_clr_l(wf_r17_m68k_shadow_t *r,unsigned reg);

/* ROM $21B4. Mutates D0 and state_1c005c; D1..D7 are preserved. */
uint32_t wf_r17_rng21b4_step(uint32_t *state_1c005c,uint32_t global_1c0080,uint32_t d[8]);
uint32_t wf_r17_rng21b4_live(uint32_t *state_1c005c,uint32_t global_1c0080,wf_r17_m68k_shadow_t *r);

#endif
