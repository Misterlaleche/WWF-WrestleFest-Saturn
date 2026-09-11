#ifndef WF24_RUNTIME_AUDIO_H
#define WF24_RUNTIME_AUDIO_H

#include <stdbool.h>
#include <stdint.h>

#include "wf24_command_bus.h"

typedef struct {
    uint32_t producer_pc;
    uint8_t command;
} wf24_rom_audio_producer_t;

/* Raw ROM-backed producers only. */
uint16_t wf24_stage_music_raw_word(unsigned stage_index);

bool wf24_ringout_producer_command(uint32_t producer_pc,
                                   uint16_t raw54,
                                   uint8_t *command_out);

bool wf24_finisher_producer_command(uint32_t producer_pc,
                                    uint8_t *command_out);

bool wf24_low_stamina_producer_command(uint32_t producer_pc,
                                       uint8_t *command_out);

wf24_dispatch_result_t wf24_runtime_dispatch_stage(wf24_command_bus_t *bus,
                                                   unsigned stage_index);

wf24_dispatch_result_t wf24_runtime_dispatch_ringout(wf24_command_bus_t *bus,
                                                     uint32_t producer_pc,
                                                     uint16_t raw54);

wf24_dispatch_result_t wf24_runtime_dispatch_producer(wf24_command_bus_t *bus,
                                                      uint32_t producer_pc);

#endif
