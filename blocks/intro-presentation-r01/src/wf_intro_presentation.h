#ifndef WF_INTRO_PRESENTATION_H
#define WF_INTRO_PRESENTATION_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "presentation_rom_exact.h"
typedef enum wf_intro_scene {
    WF_INTRO_SCENE_FBI=0, WF_INTRO_SCENE_LEGAL=1, WF_INTRO_SCENE_TECHNOS=2,
    WF_INTRO_SCENE_TITLE_HANDOFF=3
} wf_intro_scene_t;
typedef enum wf_intro_phase {
    WF_INTRO_PHASE_HOLD=0, WF_INTRO_PHASE_FADE=1, WF_INTRO_PHASE_HANDOFF=2,
    WF_INTRO_PHASE_FAULT=3
} wf_intro_phase_t;
typedef enum wf_intro_event_type {
    WF_INTRO_EVENT_NONE=0, WF_INTRO_EVENT_PUBLISH_SCENE=1, WF_INTRO_EVENT_APPLY_FADE=2,
    WF_INTRO_EVENT_TITLE_HANDOFF=3, WF_INTRO_EVENT_FAULT=4
} wf_intro_event_type_t;
typedef struct wf_intro_event { wf_intro_event_type_t type; wf_intro_scene_t scene; uint8_t fade_level; } wf_intro_event_t;
typedef struct wf_intro_state {
    wf_intro_scene_t scene; wf_intro_phase_t phase; uint16_t hold_vblanks;
    uint8_t fade_level; uint8_t fade_frames; bool fbi_enabled;
} wf_intro_state_t;
enum {
    WF_INTRO_SCENE_COUNT=3, WF_INTRO_BITMAP_BYTES=512*240,
    WF_INTRO_VRAM_PAGE_BYTES=512*256, WF_INTRO_PALETTE_BYTES=512,
    WF_INTRO_ASSET_BYTES=67104
};
typedef struct wf_intro_asset {
    uint32_t offset,size,palette_offset,bitmap_fnv1a,palette_fnv1a;
} wf_intro_asset_t;
extern const wf_intro_asset_t wf_intro_assets[WF_INTRO_SCENE_COUNT];
wf_intro_event_t wf_intro_init(wf_intro_state_t *state,bool fbi_enabled);
wf_intro_event_t wf_intro_vblank(wf_intro_state_t *state);
bool wf_intro_unpack(const uint8_t *src,size_t size,uint8_t *dst,size_t capacity);
uint32_t wf_intro_fnv1a(const uint8_t *data,size_t size);
uint16_t wf_intro_fade_rgb1555(uint16_t color,unsigned level);
void wf_intro_build_fade_palette(const uint8_t source_be[WF_INTRO_PALETTE_BYTES],unsigned level,uint8_t output_be[WF_INTRO_PALETTE_BYTES]);
#endif
