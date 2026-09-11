#include "wf24_game_bindings.h"

static const uint8_t stage_music[10] = {
    0x04, 0x0A, 0x0C, 0x0F, 0x06,
    0x04, 0x0A, 0x0C, 0x0F, 0x06
};

uint8_t
wf24_stage_music_command(unsigned stage_index)
{
    /* ROM table 0x000DEE..0x000E00. Invalid stages fail closed. */
    return stage_index < 10u ? stage_music[stage_index] : 0u;
}

bool
wf24_low_stamina_command(uint16_t stamina, uint8_t *command_out)
{
    /* Recovered binding: active-actor stamina < 0x18 -> command 0x0B. */
    if (stamina >= 0x18u || !command_out)
        return false;
    *command_out = 0x0Bu;
    return true;
}

bool
wf24_countout_command(unsigned displayed_count, uint8_t *command_out)
{
    if (!command_out || displayed_count < 17u || displayed_count > 20u)
        return false;
    *command_out = (uint8_t)(0x65u + displayed_count - 17u);
    return true;
}

uint8_t
wf24_finisher_heavy_command(void)
{
    return 0x32u;
}

uint8_t
wf24_heavy_short_layer_command(void)
{
    /* Demonstrated paired layer on specific heavy-impact callbacks only. */
    return 0x2Bu;
}
