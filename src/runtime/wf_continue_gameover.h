#ifndef WF_CONTINUE_GAMEOVER_H
#define WF_CONTINUE_GAMEOVER_H

#include <stdbool.h>
#include <stdint.h>

#define WF_CONTINUE_INITIAL_DIGIT 9
#define WF_CONTINUE_DIGIT_QUANTUM 64u
#define WF_CONTINUE_TOTAL_ITERATIONS 640u
#define WF_CONTINUE_RESOURCE_SELECTOR 0x0014u
#define WF_CONTINUE_RESOURCE_ROM 0x000C1118u
#define WF_CONTINUE_NO_CREDIT_TEXT_ID 0x001Cu
#define WF_CONTINUE_PAIR_A_PROMPT_BASE 0x001Du
#define WF_CONTINUE_PAIR_B_TEXT_OFFSET 0x0002u
#define WF_CONTINUE_DIGIT_RENDERER_ROM 0x0002067Cu
#define WF_CONTINUE_ACTOR_CLEAR_BYTES 0x010Cu
#define WF_CONTINUE_PRESERVE_WORD_OFFSET 0x0056u
#define WF_CONTINUE_MARKER_OFFSET 0x0104u
#define WF_CONTINUE_MARKER_VALUE 0x8000u
#define WF_CONTINUE_PREMATCH_RETURN_ROM 0x00000AC0u
#define WF_GAMEOVER_NORMAL_WAIT_EACH 65u
#define WF_GAMEOVER_NORMAL_WAIT_TOTAL 130u
#define WF_GAMEOVER_ROYAL_WAIT 97u

typedef enum wf_continue_pair { WF_CONTINUE_PAIR_A=0,WF_CONTINUE_PAIR_B=1,WF_CONTINUE_PAIR_NONE=2 } wf_continue_pair_t;
typedef bool (*wf_continue_credit_available_fn)(uint8_t request_code,void *user);
typedef bool (*wf_continue_credit_spend_fn)(uint8_t request_code,void *user);
typedef struct wf_continue_credit_ops { wf_continue_credit_available_fn available;wf_continue_credit_spend_fn spend;void *user; } wf_continue_credit_ops_t;
typedef struct wf_continue_pair_inputs { uint16_t player_a_active_low;uint16_t player_b_active_low; } wf_continue_pair_inputs_t;

typedef struct wf_continue_runtime {
    wf_continue_pair_t pair;uint8_t participation_mask;int16_t digit;uint16_t subcounter;uint16_t iterations;
    bool active;bool accepted;bool expired;
} wf_continue_runtime_t;

typedef struct wf_continue_event {
    bool render_digit;uint16_t render_word;
    bool credit_available;uint16_t no_credit_text_id;bool no_credit_visible;
    uint16_t player_prompt_a_id;uint16_t player_prompt_b_id;
    bool player_prompt_a_visible;bool player_prompt_b_visible;
    bool start_level_seen;bool spend_attempted;bool accepted;
    bool set_global_markers;bool set_pair_actor_markers;
    bool expired;bool clear_pair_actor_domains;uint16_t clear_bytes;uint16_t preserve_word_offset;
    bool retry_same_campaign_position;
} wf_continue_event_t;

typedef struct wf_continue_session {
    uint8_t pair_mask_a;uint8_t pair_mask_b;bool pair_eligible_a;bool pair_eligible_b;
    wf_continue_pair_t next_pair;wf_continue_runtime_t current;bool finished;
} wf_continue_session_t;

uint8_t wf_continue_request_code(uint8_t participation_mask);
uint32_t wf_continue_input_word_a_rom(wf_continue_pair_t pair,uint8_t participation_mask);
uint32_t wf_continue_input_word_b_rom(wf_continue_pair_t pair,uint8_t participation_mask);
bool wf_continue_start_level(uint8_t participation_mask,const wf_continue_pair_inputs_t *inputs);
bool wf_continue_begin(wf_continue_runtime_t *rt,wf_continue_pair_t pair,uint8_t participation_mask);
void wf_continue_step(wf_continue_runtime_t *rt,const wf_continue_pair_inputs_t *inputs,const wf_continue_credit_ops_t *credit,wf_continue_event_t *event);
void wf_continue_session_begin(wf_continue_session_t *session,uint8_t pair_mask_a,bool pair_eligible_a,uint8_t pair_mask_b,bool pair_eligible_b);
bool wf_continue_session_next(wf_continue_session_t *session);
void wf_continue_session_complete_current(wf_continue_session_t *session);
#endif
