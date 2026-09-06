#ifndef WWF_ATTRACT_PORTRAIT_ASSETS_H
#define WWF_ATTRACT_PORTRAIT_ASSETS_H
#define ATTRACT_PORTRAIT_CHARACTERS 12U
#define ATTRACT_PORTRAIT_POSES 3U
#define ATTRACT_PORTRAIT_FRAMES 36U
#define ATTRACT_PORTRAIT_BYTES 79409U
#define ATTRACT_PORTRAIT_HASH 0x1facf6afU
#define ATTRACT_PORTRAIT_PALETTE_BYTES 416U
static inline unsigned attract_portrait_frame(unsigned ch,unsigned pose){return ch*3U+pose;}
static inline unsigned attract_portrait_set(unsigned ch){return 0x30U+ch;}
static inline unsigned attract_portrait_character_palette(unsigned ch){return 0x10U+ch;}
#define ATTRACT_PORTRAIT_COMMON_PALETTE 0x1eU
#endif
