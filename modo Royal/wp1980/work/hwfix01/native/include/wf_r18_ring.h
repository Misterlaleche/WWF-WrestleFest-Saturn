#ifndef WF_R18_RING_H
#define WF_R18_RING_H
#include <stddef.h>
#include <stdint.h>
/* ROM-derived mode/palette are encoded in blob; borrowed for lifetime.
 * VDP2 NBG0/NBG1: characters in banks0/2, pattern names in bank1 (0x0000/0x2000).
 * Caller owns vblank/commit synchronization; priority follows the ROM mode.
 */
int wf_r18_ring_bind(const void *blob, size_t bytes);
int wf_r18_ring_update(int16_t sx0,int16_t sy0,int16_t sx1,int16_t sy1,uint8_t priority);
/* Frontend transition: prepare NBG maps and scroll while the caller keeps
 * the display gated, then expose the complete frame in its own commit. */
int wf_r18_ring_update_deferred(int16_t sx0,int16_t sy0,int16_t sx1,int16_t sy1,uint8_t priority);
/* Host-verifiable exact world lookup; layer0 FG/layer1 BG. */
int wf_r18_ring_pixel(unsigned layer,int wx,int wy,uint8_t *index);
int wf_r18_ring_scanline(unsigned layer,int wx,int wy,unsigned transparent,uint8_t out[320]);
/* Native16x16 characters, one-word pattern names (palette/flip/tile).
 * Upload characters once; update only map cells affected by scroll/animation. */
int wf_r18_ring_characters(volatile uint8_t *characters,size_t capacity);
int wf_r18_ring_cells_update(unsigned layer,int wx,int wy,volatile uint16_t *map,size_t *writes);
/* Scalar/bitmap reference path for HOST pixel comparisons. */
int wf_r18_ring_surface_update(unsigned layer,int wx,int wy,unsigned transparent,
                               volatile uint8_t *bitmap,size_t *writes);
int wf_r18_ring_tile_write(unsigned layer,unsigned physical_cell,uint16_t attr,uint16_t source_tile);
/* Sixteen original attr/source-tile pairs, row-major in a 4x4 patch. */
int wf_r18_ring_patch_write(unsigned layer,unsigned first_cell,const uint16_t cells[32]);
void wf_r18_ring_tile_restore(unsigned layer,unsigned physical_cell);
/* Live palette byte images: FG=$188000, BG=$186000, bank stride128. */
int wf_r18_ring_palette_set(unsigned layer,const uint8_t *strided,size_t bytes);
#endif
