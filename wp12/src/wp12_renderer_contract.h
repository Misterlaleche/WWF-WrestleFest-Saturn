#ifndef WP12_RENDERER_CONTRACT_H
#define WP12_RENDERER_CONTRACT_H
#include <stdint.h>
#include <stdbool.h>
#include "wp12_prematch.h"
typedef struct { int16_t screen_x,screen_y; uint16_t priority; } wp12_screen_point_t;
typedef bool (*wp12_machine_to_screen_fn)(uint16_t,uint16_t,uint16_t,wp12_screen_point_t*,void*);
typedef struct { wp12_machine_to_screen_fn machine_to_screen; void *user; } wp12_renderer_contract_t;
bool wp12_renderer_project_actor(const wp12_renderer_contract_t *r,const wp12_actor_t *a,wp12_screen_point_t *out);
#endif
