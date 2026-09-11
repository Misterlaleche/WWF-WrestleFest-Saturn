#include "wf24_runtime_audio.h"

enum {
    WF24_RINGOUT_PRODUCER_PC = 0x01FE44u,
    WF24_LOW_STAMINA_PRODUCER_PC = 0x009088u
};

static const uint16_t stage_words[10] = {
    0x3104, 0x310A, 0x310C, 0x310F, 0x3106,
    0x3104, 0x310A, 0x310C, 0x310F, 0x3106
};

/*
 * producer_pc is the address of MOVE.W #command,D0, not the following
 * JSR $00002052. This distinction matters when reconciling SH-2 callbacks.
 */
static const wf24_rom_audio_producer_t finisher_producers[] = {
    {0x0152BAu, 0x32u}, /* Perfect Plex; JSR at 0x0152BE */
    {0x016DBEu, 0x32u}, /* Gorilla Press; JSR at 0x016DC2 */
    {0x01755Au, 0x32u}, /* DDT; JSR at 0x01755E */
    {0x017874u, 0x2Bu}, /* Hogan Legdrop layer 1; JSR at 0x017878 */
    {0x01787Eu, 0x32u}, /* Hogan Legdrop layer 2; JSR at 0x017882 */
    {0x0164B8u, 0x2Bu}, /* Smash Suplex layer 1; JSR at 0x0164BC */
    {0x0164C2u, 0x32u}  /* Smash Suplex layer 2; JSR at 0x0164C6 */
};

uint16_t
wf24_stage_music_raw_word(unsigned stage_index)
{
    return stage_index < 10u ? stage_words[stage_index] : 0u;
}

bool
wf24_ringout_producer_command(uint32_t producer_pc,
                              uint16_t raw54,
                              uint8_t *command_out)
{
    if (!command_out || producer_pc != WF24_RINGOUT_PRODUCER_PC)
        return false;
    if (raw54 < 0x3165u || raw54 > 0x3168u)
        return false;

    *command_out = (uint8_t)(raw54 & 0x00FFu);
    return true;
}

bool
wf24_finisher_producer_command(uint32_t producer_pc,
                               uint8_t *command_out)
{
    unsigned i;

    if (!command_out)
        return false;

    for (i = 0;
         i < sizeof(finisher_producers) / sizeof(finisher_producers[0]);
         ++i) {
        if (finisher_producers[i].producer_pc == producer_pc) {
            *command_out = finisher_producers[i].command;
            return true;
        }
    }
    return false;
}

bool
wf24_low_stamina_producer_command(uint32_t producer_pc,
                                  uint8_t *command_out)
{
    if (!command_out || producer_pc != WF24_LOW_STAMINA_PRODUCER_PC)
        return false;

    *command_out = 0x0Bu;
    return true;
}

wf24_dispatch_result_t
wf24_runtime_dispatch_stage(wf24_command_bus_t *bus, unsigned stage_index)
{
    uint16_t raw = wf24_stage_music_raw_word(stage_index);

    if (!raw)
        return WF24_DISPATCH_INVALID;

    return wf24_command_dispatch(bus, (uint8_t)(raw & 0x00FFu));
}

wf24_dispatch_result_t
wf24_runtime_dispatch_ringout(wf24_command_bus_t *bus,
                              uint32_t producer_pc,
                              uint16_t raw54)
{
    uint8_t command;

    if (!wf24_ringout_producer_command(producer_pc, raw54, &command))
        return WF24_DISPATCH_INVALID;

    return wf24_command_dispatch(bus, command);
}

wf24_dispatch_result_t
wf24_runtime_dispatch_producer(wf24_command_bus_t *bus,
                               uint32_t producer_pc)
{
    uint8_t command;

    if (wf24_low_stamina_producer_command(producer_pc, &command) ||
        wf24_finisher_producer_command(producer_pc, &command))
        return wf24_command_dispatch(bus, command);

    return WF24_DISPATCH_INVALID;
}
