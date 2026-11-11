#ifndef WF_FIXED_ATTRACT_H
#define WF_FIXED_ATTRACT_H
#include <stdbool.h>
#include <stdint.h>
typedef struct {
 uint32_t magic, error,
 boot_stage,
 video_clock,
 display_source,
 display_output,
 active,
 commit_count,
 max_io_ticks,
 io_calls,
 texture_bytes,
 loops,
 source_frame,
 output_frames,
 arcade_steps,
 sequence_offset,
 packet_bytes,
 max_packet_bytes,
 max_texture_bytes,
 packet_ticks,
 sprite_count,
 last_draw_ticks,
 max_draw_ticks,
 draw_irq_count,
 cef_fallback_count,
 framebuffer_error,
 framebuffer_flags,
 stream_available,
 stream_min_available,
 stream_produced,
 stream_consumed,
 stream_state,
 stream_error,
 stream_commands,
 video_standard,
 notice,
 exit_reason,
 notice_fields,
 missed_vblanks,
 done,
 last_work_ticks,
 max_work_ticks,
 stop_start,
 stop_fields,
 last_input_ticks,
 max_input_ticks,
 submit_vcount;
} wf_fixed_attract_debug_t;
extern volatile wf_fixed_attract_debug_t wf_attract_debug;
/* Call after finishing live VDP work. Cold notice requires validated cart. */
void wf_fixed_attract_video_begin(bool cold_notice);
/* on_field polls actual pads and services original raster IRQs; +1 exits on
 * accepted credit, -1 fails closed. All resources restored by caller. */
bool wf_fixed_attract_run(bool already_credited,int (*on_field)(void *),void *);
#endif
