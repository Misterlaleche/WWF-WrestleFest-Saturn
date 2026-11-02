#ifndef WF_AUDIO2_GAME_QUEUE_H
#define WF_AUDIO2_GAME_QUEUE_H
#include "wf_audio2.h"
typedef struct {
    uint16_t words[64], last_unimplemented;
    unsigned head, tail, used;
    uint32_t submitted, completed, unsupported, overflows, invalid;
    bool fault;
} wf_audio2_game_queue_t;
bool wf_audio2_game_queue_push(wf_audio2_game_queue_t *, uint16_t);
bool wf_audio2_game_queue_service(wf_audio2_game_queue_t *, wf_audio2_t *);
#endif
