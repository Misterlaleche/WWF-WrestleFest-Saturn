#ifndef WF_TITLE_VISUAL_EXACT_H
#define WF_TITLE_VISUAL_EXACT_H

#include <stdbool.h>
#include <stdint.h>

enum {
    WF_TITLE_PREPHASE_VBLANKS = 128,
    WF_TITLE_CARD_COUNT = 12,
    WF_TITLE_CARD_SHORT_PHASES = 4,
    WF_TITLE_CARD_SHORT_VBLANKS = 4,
    WF_TITLE_CARD_LONG_VBLANKS = 26,
    WF_TITLE_CARD_VBLANKS = 42,
    WF_TITLE_CARD_BUILD_VBLANKS = 504,
    WF_TITLE_MAIN_VBLANKS = 1480,
    WF_TITLE_TOTAL_VBLANKS = 2112
};

typedef enum wf_title_card_id {
    WF_TITLE_CARD_HOGAN = 0,
    WF_TITLE_CARD_WARRIOR,
    WF_TITLE_CARD_HAWK,
    WF_TITLE_CARD_ANIMAL,
    WF_TITLE_CARD_BOSSMAN,
    WF_TITLE_CARD_JAKE,
    WF_TITLE_CARD_SLAUGHTER,
    WF_TITLE_CARD_DIBIASE,
    WF_TITLE_CARD_EARTHQUAKE,
    WF_TITLE_CARD_SMASH,
    WF_TITLE_CARD_PERFECT,
    WF_TITLE_CARD_CRUSH
} wf_title_card_id_t;

typedef struct wf_title_card_slot {
    wf_title_card_id_t card;
    uint16_t x;
    uint16_t y;
} wf_title_card_slot_t;

/*
 * Order and positions are capture-correlated against the independently
 * reconstructed original arcade title state. Timing is ROM-exact:
 * 42 synchronized iterations per object.
 */
extern const wf_title_card_slot_t wf_title_card_sequence[WF_TITLE_CARD_COUNT];

typedef enum wf_title_visual_phase {
    WF_TITLE_VISUAL_PREPHASE = 0,
    WF_TITLE_VISUAL_CARDS,
    WF_TITLE_VISUAL_MAIN,
    WF_TITLE_VISUAL_DONE,
    WF_TITLE_VISUAL_FAULT
} wf_title_visual_phase_t;

typedef struct wf_title_visual_state {
    wf_title_visual_phase_t phase;
    uint16_t phase_vblank;
    uint8_t card_index;
    uint8_t card_subphase;
    uint8_t card_subphase_vblank;
    uint16_t total_vblank;
} wf_title_visual_state_t;

typedef enum wf_title_visual_event_type {
    WF_TITLE_VISUAL_EVENT_NONE = 0,
    WF_TITLE_VISUAL_EVENT_BACKGROUND_TICK,
    WF_TITLE_VISUAL_EVENT_CARD_PHASE,
    WF_TITLE_VISUAL_EVENT_MAIN_TICK,
    WF_TITLE_VISUAL_EVENT_DONE,
    WF_TITLE_VISUAL_EVENT_FAULT
} wf_title_visual_event_type_t;

typedef struct wf_title_visual_event {
    wf_title_visual_event_type_t type;
    uint16_t title_vblank;
    uint16_t phase_vblank;
    uint8_t card_index;
    uint8_t card_subphase;
} wf_title_visual_event_t;

void wf_title_visual_init(wf_title_visual_state_t *state);
wf_title_visual_event_t wf_title_visual_vblank(wf_title_visual_state_t *state);

#endif
