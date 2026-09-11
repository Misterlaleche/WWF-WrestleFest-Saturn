#ifndef WF24_COMMAND_BUS_H
#define WF24_COMMAND_BUS_H

#include <stdbool.h>
#include <stdint.h>
#include "wf24_audio_contract.h"

typedef struct {
    bool (*stop_music)(void *user);
    bool (*play_music)(uint8_t command, void *user);
    bool (*special_fade_1f)(void *user);
    bool (*play_sfx)(uint8_t command, void *user);
    void *user;
} wf24_command_bus_ops_t;

typedef enum {
    WF24_DISPATCH_OK = 0,
    WF24_DISPATCH_NO_PRODUCER,
    WF24_DISPATCH_INVALID,
    WF24_DISPATCH_BACKEND_ERROR
} wf24_dispatch_result_t;

typedef struct {
    wf24_command_bus_ops_t ops;
    uint8_t last_command;
    wf24_route_t last_route;
    wf24_dispatch_result_t last_result;
    uint32_t dispatch_count;
    uint32_t rejected_count;
} wf24_command_bus_t;

bool wf24_command_bus_init(wf24_command_bus_t *bus,
                           const wf24_command_bus_ops_t *ops);
wf24_dispatch_result_t wf24_command_dispatch(wf24_command_bus_t *bus,
                                              uint8_t command);

#endif
