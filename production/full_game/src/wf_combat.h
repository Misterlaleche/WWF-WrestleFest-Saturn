#ifndef WF_COMBAT_H
#define WF_COMBAT_H
#include "wf_session.h"
#include "wf_gameplay_bindings.h"
#include "wf_r18_world.h"
typedef struct {
 wf_r18_rom_read_fn read_rom;
 wf_r17_participant_input_fn input;
 wf_r17_audio_command_fn audio;
 wf_r17_raw_word_fn wait;
 wf_r17_render_2836_batch_fn render;
 wf_r17_platform_transition_fn reset,barrier;
} wf_combat_io_t;
typedef struct {
 wf_gameplay_binding_context_t gameplay;
 wf_phase_binding_t bindings[36];
 wf_r18_world_ctx_t world;
} wf_combat_t;
wf_status_t wf_combat_join(wf_combat_t *,wf_session_t *,const wf_combat_io_t *);
wf_status_t wf_combat_read_ram(wf_runtime_t *,uint32_t,uint16_t *,void *);
/* After present_init:209 LWRAM pages plus at least28 pinned HWRAM pages. */
wf_status_t wf_combat_preload_pages(unsigned royal);
#endif
