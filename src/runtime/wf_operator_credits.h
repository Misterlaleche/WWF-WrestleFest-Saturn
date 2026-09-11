#ifndef WF_OPERATOR_CREDITS_H
#define WF_OPERATOR_CREDITS_H
#include <stdbool.h>
#include <stdint.h>
#define WF_OPERATOR_MAX_CREDITS 99u
#define WF_OPERATOR_PULSE_STABILITY_MIN 4u
#define WF_OPERATOR_PULSE_STABILITY_MAX 10u
#define WF_OPERATOR_PULSE_HELD_MAX_EXCLUSIVE 0x0360u
#define WF_OPERATOR_COIN_SOUND_COMMAND 0x2Au
#define WF_OPERATOR_CONFIG_BUYIN_AS_START 0x0004u
#define WF_OPERATOR_CONFIG_REGAIN_AS_START 0x0008u
#define WF_OPERATOR_CONFIG_CONTINUE_AS_START 0x0010u
#define WF_OPERATOR_CONFIG_DEMO_SOUNDS_OFF 0x0020u
#define WF_OPERATOR_CONFIG_FLIP_ON 0x0040u
#define WF_OPERATOR_CONFIG_FBI_OFF 0x0080u
#define WF_OPERATOR_CONFIG_PLAYERS_MASK 0x0C00u
#define WF_OPERATOR_CONFIG_PLAYERS_2P 0x0800u
#define WF_OPERATOR_CONFIG_PLAYERS_3P 0x0400u
#define WF_OPERATOR_CONFIG_PLAYERS_4P 0x0000u
#define WF_OPERATOR_CONFIG_PLAYERS_RAW0 0x0C00u
#define WF_OPERATOR_CONFIG_UNUSED_DSW2_BIT4 0x1000u
#define WF_OPERATOR_CONFIG_CLEAR_STAGE_MASK 0x6000u
#define WF_OPERATOR_CONFIG_CHAMPIONSHIP_4TH 0x8000u
typedef struct wf_credit_state {uint8_t coin_threshold;uint8_t credit_award;uint8_t partial;uint8_t credits;uint16_t awarded_bookkeeping;uint16_t accepted_pulse_bookkeeping;} wf_credit_state_t;
typedef struct wf_operator_pulse_filter {uint16_t held_duration;uint8_t release_stability;} wf_operator_pulse_filter_t;
typedef struct wf_coin_event {bool sound_command;uint8_t sound_command_byte;bool converted;bool capped;} wf_coin_event_t;
uint16_t wf_operator_pack_config(uint16_t port20,uint16_t port22,uint16_t port24,uint16_t port26);
void wf_credit_boot_init(wf_credit_state_t *state,uint16_t config);
void wf_credit_accept_pulse(wf_credit_state_t *state,wf_coin_event_t *event);
bool wf_operator_pulse_sample(wf_operator_pulse_filter_t *filter,bool asserted_active_low);
bool wf_credit_pay(wf_credit_state_t *state,uint16_t config,uint8_t mode);
bool wf_credit_available(const wf_credit_state_t *state,uint16_t config,uint8_t mode);
uint8_t wf_title_prompt_id(const wf_credit_state_t *state,uint16_t config,bool dedicated_2p_second_prompt_phase);
bool wf_title_prompt_blink_phase(uint16_t counter);
int16_t wf_operator_difficulty_stamina_adjust(uint16_t config);
uint8_t wf_operator_clear_stage_power(uint16_t config);
bool wf_operator_demo_sound_allowed(uint16_t config,bool presentation_context);
bool wf_operator_flip_enabled(uint16_t config);
bool wf_operator_fbi_logo_enabled(uint16_t config);
bool wf_operator_championship_is_4th(uint16_t config);
#endif
