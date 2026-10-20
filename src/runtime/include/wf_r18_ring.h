#ifndef WF_R18_RING_H
#define WF_R18_RING_H
#include <stddef.h>
#include <stdint.h>
/* ROM-derived mode/palette are encoded in blob; borrowed for lifetime.
 * VDP2 NBG0/NBG1, VRAM A0/A1 (128KiB each); CRAM entries 512..1023.
 * Caller owns vblank/commit synchronization and sprite priority class 0.
 */
int wf_r18_ring_bind(const void *blob, size_t bytes);
int wf_r18_ring_update(int16_t sx0,int16_t sy0,int16_t sx1,int16_t sy1,uint8_t priority);
/* Host-verifiable exact world lookup; layer0 FG/layer1 BG. */
int wf_r18_ring_pixel(unsigned layer,int wx,int wy,uint8_t *index);
int wf_r18_ring_scanline(unsigned layer,int wx,int wy,unsigned transparent,uint8_t out[320]);
/* 512x256 VDP2 bitmap cache; returns pixels rewritten in writes. */
int wf_r18_ring_surface_update(unsigned layer,int wx,int wy,unsigned transparent,
                               volatile uint8_t *bitmap,size_t *writes);
int wf_r18_ring_tile_write(unsigned layer,unsigned physical_cell,uint16_t attr,uint16_t source_tile);
void wf_r18_ring_tile_restore(unsigned layer,unsigned physical_cell);
/* Live palette byte images: FG=$188000, BG=$186000, bank stride128. */
int wf_r18_ring_palette_set(unsigned layer,const uint8_t *strided,size_t bytes);
#endif
