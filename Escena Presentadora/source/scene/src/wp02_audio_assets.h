/* Generated ROM-derived PCM16BE; reduced PCM8 verified against A16. */
#ifndef WP02_AUDIO_ASSETS_H
#define WP02_AUDIO_ASSETS_H
#include <stdint.h>
typedef struct { uint8_t bank, sample, volume; uint16_t sector, samples, ticks; uint32_t bytes; } wp02_pcm_asset_t;
static const wp02_pcm_asset_t wp02_pcm_assets[] = {
 {0u,17u,5u,0u,31978u,884u,63956u},
 {0u,18u,1u,32u,32018u,885u,64036u},
 {1u,23u,2u,64u,10682u,296u,21364u},
 {1u,24u,3u,75u,16418u,454u,32836u},
 {1u,25u,3u,92u,16478u,456u,32956u},
 {1u,26u,3u,109u,16798u,465u,33596u},
 {1u,27u,3u,126u,14340u,397u,28680u},
 {1u,28u,3u,141u,10744u,297u,21488u},
 {1u,29u,3u,152u,13380u,370u,26760u},
 {1u,30u,3u,166u,14556u,403u,29112u},
 {1u,31u,3u,181u,32920u,910u,65840u},
 {1u,37u,2u,214u,24904u,689u,49808u},
 {1u,38u,2u,239u,2824u,79u,5648u},
 {1u,39u,2u,242u,6356u,176u,12712u},
};
#define WP02_PCM_ASSET_COUNT 14u
#define WP02_PCM_PACK_BYTES 509952u
#endif
