#ifndef WF_COMBAT_SATURN_H
#define WF_COMBAT_SATURN_H
#include "wf_session.h"
#include "wf_board.h"
bool wf_combat_saturn_begin(wf_session_t *,wf_board_t *);
bool wf_combat_saturn_step(const uint8_t inputs[4]);
#endif
