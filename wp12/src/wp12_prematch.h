#ifndef WP12_PREMATCH_H
#define WP12_PREMATCH_H
#include <stdint.h>
#include <stdbool.h>

typedef enum { WP12_MODE_TAG=0, WP12_MODE_ROYAL=1 } wp12_mode_t;
typedef enum { WP12_PHASE_INACTIVE=0, WP12_PHASE_VISIBLE=1, WP12_PHASE_TEARDOWN=2, WP12_PHASE_HANDOFF=3 } wp12_phase_t;
typedef enum { WP12_EVENT_NONE=0, WP12_EVENT_AUDIO_RAW=1, WP12_EVENT_TAG_ACTIVATE=2, WP12_EVENT_TAG_IDENTITY=3, WP12_EVENT_PRESENTER_CLEAR=4, WP12_EVENT_HANDOFF_GAMEPLAY=5 } wp12_event_type_t;

typedef struct {
 bool active; uint8_t plus56_identity_program,plus57_voice_identity;
 uint16_t local_state,phase_counter_plus22,visual_selector_plus04;
 uint16_t x_machine,y_machine,z_machine,orientation_word;
} wp12_actor_t;

typedef struct { wp12_event_type_t type; uint16_t value; uint8_t logical_slot,physical_slot; } wp12_event_t;

typedef struct {
 wp12_mode_t mode; wp12_phase_t phase; uint8_t tag_controller_state; uint16_t state_tick; uint32_t total_updates;
 bool skipped,entry_audio_pending; uint8_t teardown_stage; bool presenter_active;
 uint16_t presenter_x_machine,presenter_y_half_machine,presenter_z_machine,presenter_visual_selector;
 wp12_actor_t actors[6];
} wp12_prematch_t;

typedef struct { bool action_mask_0030_any_port; } wp12_input_t;

void wp12_init_tag(wp12_prematch_t *s,const wp12_actor_t topology[6]);
void wp12_init_royal(wp12_prematch_t *s,const wp12_actor_t topology[6]);
wp12_event_t wp12_step(wp12_prematch_t *s,wp12_input_t input);
uint16_t wp12_tag_identity_window_updates(uint8_t plus56_low);
int wp12_tag_resolve_physical_slot(const wp12_prematch_t *s,uint8_t logical_slot);
uint16_t wp12_tag_identity_voice_command(uint8_t plus57_low);
uint16_t wp12_tag_activation_command(uint8_t plus56_low);
uint16_t wp12_presenter_y_machine_floor(const wp12_prematch_t *s);
extern const uint16_t wp12_tag_primary_xy_machine[4][2];
extern const uint16_t wp12_royal_xy_machine[6][2];
extern const uint16_t wp12_royal_orientation[6];
#endif
