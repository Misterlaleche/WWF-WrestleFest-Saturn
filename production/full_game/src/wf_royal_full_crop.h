#ifndef WF_ROYAL_FULL_CROP_H
#define WF_ROYAL_FULL_CROP_H
#include "wf_full_crop_lz4.h"
/* Dedicated Royal leases. ROM, hints, scene and video remain immutable. */
enum { WF_ROYAL_FULL_HOT_SLOTS=5, WF_ROYAL_FULL_COLD_SLOTS=5,
 WF_ROYAL_FULL_TAIL_SLOTS=7, WF_ROYAL_FULL_SLOTS=17,
 WF_ROYAL_FULL_COLD=0x00241000u, WF_ROYAL_FULL_COLD_END=0x00255000u,
 WF_ROYAL_FULL_ROM=0x00261400u, WF_ROYAL_FULL_ROM_END=0x002e1400u,
 WF_ROYAL_FULL_TAIL=0x002e1400u, WF_ROYAL_FULL_TAIL_END=0x002fd400u };
_Static_assert(WF_ROYAL_FULL_COLD_END-WF_ROYAL_FULL_COLD==5u*WF_FULL_CROP_BLOCK,"Five cold blocks");
_Static_assert(WF_ROYAL_FULL_ROM_END-WF_ROYAL_FULL_ROM==0x80000u,"Full immutable ROM");
_Static_assert(WF_ROYAL_FULL_TAIL_END-WF_ROYAL_FULL_TAIL==7u*WF_FULL_CROP_BLOCK,"Seven tail blocks");
_Static_assert(WF_ROYAL_FULL_COLD_END<=0x00255400u&&WF_ROYAL_FULL_TAIL_END<=0x00300000u,"LWRAM lease boundaries");
_Static_assert(((WF_ROYAL_FULL_COLD|WF_ROYAL_FULL_TAIL)&31u)==0u,"Aligned decode slots");
extern wf_full_crop_lz4_t wf_royal_full_crop;
const uint8_t*wf_combat_full_crop_read_alias(void *destination,size_t bytes);
void wf_r18_full_crop_reset(void);
bool wf_r18_full_crop_configure(uint8_t *cold,size_t cold_bytes,uint8_t *tail,size_t tail_bytes);
void wf_combat_full_crop_failure(uint32_t error,uint32_t tile,uint32_t block);
#endif
