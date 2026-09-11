#ifndef WF24_GAME_BINDINGS_H
#define WF24_GAME_BINDINGS_H

#include <stdbool.h>
#include <stdint.h>

/* ROM-backed mappings only. No synthetic event semantics live here. */
uint8_t wf24_stage_music_command(unsigned stage_index);
bool wf24_low_stamina_command(uint16_t stamina, uint8_t *command_out);
bool wf24_countout_command(unsigned displayed_count, uint8_t *command_out);
uint8_t wf24_finisher_heavy_command(void);
uint8_t wf24_heavy_short_layer_command(void);

#endif
