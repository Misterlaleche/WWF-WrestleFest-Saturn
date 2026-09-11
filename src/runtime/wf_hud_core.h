#ifndef WF_HUD_CORE_H
#define WF_HUD_CORE_H
#include <stdbool.h>
#include <stdint.h>
#define WF_HUD_SLOT_COUNT 4u
#define WF_HUD_BAR_COLUMNS 6u
#define WF_HUD_PORTRAIT_COUNT 12u
#define WF_HUD_PLAYER_INDICATOR_COUNT 4u
#define WF_HUD_TIMER_SEED_MINUTES_BCD 0x30u
#define WF_HUD_TIMER_SEED_SECONDS_BCD 0x01u
#define WF_HUD_TIMER_SUBCOUNTER_SEED 1u
#define WF_HUD_TIMER_SUBCOUNTER_RELOAD 4u
#define WF_HUD_TIMER_FIVE_MINUTES_BCD 0x05u
#define WF_HUD_TIMER_FIVE_MINUTE_COMMAND 0x0Bu
#define WF_HUD_TIMER_TIMEUP_TRANSITION 3u
typedef struct wf_hud_slot_meta {uint16_t fg_offset,indicator_offset,portrait_offset,bar_upper_offset,bar_lower_offset;uint8_t map_row,map_col;} wf_hud_slot_meta_t;
typedef struct wf_hud_bar_cell {uint16_t upper_tile,lower_tile;} wf_hud_bar_cell_t;
typedef struct wf_hud_stamina {int16_t queued_delta;int16_t accumulator;int16_t displayed;uint16_t maximum;} wf_hud_stamina_t;
typedef struct wf_hud_timer {uint8_t minutes_bcd,seconds_bcd;uint16_t subcounter;bool initialized,five_minute_event_seen,time_up;} wf_hud_timer_t;
typedef struct wf_hud_timer_event {bool redraw;bool five_minute_event;uint8_t command_byte;bool time_up;uint8_t transition_gate_value;} wf_hud_timer_event_t;
extern const wf_hud_slot_meta_t wf_hud_slots[WF_HUD_SLOT_COUNT];
extern const uint16_t wf_hud_player_indicator_tiles[WF_HUD_PLAYER_INDICATOR_COUNT];
extern const uint16_t wf_hud_portrait_descriptors[WF_HUD_PORTRAIT_COUNT];
void wf_hud_stamina_init(wf_hud_stamina_t *s,uint16_t live,uint16_t maximum);
void wf_hud_stamina_queue(wf_hud_stamina_t *s,int16_t delta);
void wf_hud_stamina_update(wf_hud_stamina_t *s);
void wf_hud_stamina_cells(uint8_t displayed_low_byte,unsigned slot,wf_hud_bar_cell_t out[WF_HUD_BAR_COLUMNS]);
void wf_hud_timer_reset(wf_hud_timer_t *timer);
void wf_hud_timer_step(wf_hud_timer_t *timer,bool gameplay_display_gate,bool timer_enabled,wf_hud_timer_event_t *event);
#endif
