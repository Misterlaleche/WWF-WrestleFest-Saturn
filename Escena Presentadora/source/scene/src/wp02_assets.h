#ifndef WP02_ASSETS_H
#define WP02_ASSETS_H
#include <stdint.h>
typedef struct { uint16_t tile; int16_t x,y; uint16_t palette; uint8_t flip,flags; uint16_t width,height; } wp02_piece_t;
typedef struct { uint16_t first,count; } wp02_frame_t;
extern const wp02_piece_t wp02_pieces[];
extern const wp02_frame_t wp02_frames[];
const wp02_frame_t *wp02_pose_find(unsigned raw_gfx_set,unsigned raw_selector);
const wp02_frame_t *wp02_name_find(unsigned raw_identity,unsigned logical_slot);
/* Pose X mirror = -piece.x, flip XOR1. Pose Y = raster anchor +piece.y. */
/* Textures4bpp, row-major native width/height; address=atlas base+tile*128. */
/* All actors16x16; exact static text composed into variable-size textures. */
/* Width is a multiple of8; height1..255. No scaling. */
/* palette is the absolute CRAM entry base; flags bit0 means opaque. */
#define WP02_TEXTURE_BYTES 381056U
#define WP02_RING_BYTES 131072U
#define WP02_RING_PALETTE_BASE 768U
#define WP02_FOREGROUND_FRAME 94U
#define WP02_CREDIT_FRAME 95U
#define WP02_CAPTURE_NAMES_FRAME 96U
#define WP02_VS_FRAME 145U
#define WP02_PIECE_COUNT 2536U
#define WP02_FRAME_COUNT 147U
#endif
