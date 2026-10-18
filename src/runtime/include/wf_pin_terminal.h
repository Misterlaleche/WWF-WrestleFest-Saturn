#ifndef WF_PIN_TERMINAL_H
#define WF_PIN_TERMINAL_H
#include <stdint.h>
typedef enum { WF_PIN_MODE_TAG=0, WF_PIN_MODE_ROYAL=1 } wf_pin_mode_t;
typedef enum { WF_PIN_TERM_TAG_RESULT=1, WF_PIN_TERM_ROYAL_ELIMINATION=2 } wf_pin_terminal_route_t;
/* Phase7 split only. Detailed result choreography remains owned by WP11. */
static inline wf_pin_terminal_route_t wf_pin_phase7_route(wf_pin_mode_t m){return m==WF_PIN_MODE_TAG?WF_PIN_TERM_TAG_RESULT:WF_PIN_TERM_ROYAL_ELIMINATION;}
#endif
