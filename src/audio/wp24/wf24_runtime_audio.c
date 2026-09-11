#include "wf24_runtime_audio.h"

static const uint16_t stage_words[10] = {
    0x3104,0x310A,0x310C,0x310F,0x3106,
    0x3104,0x310A,0x310C,0x310F,0x3106
};

static const wf24_rom_audio_call_t finisher_calls[] = {
    {0x0152BAu,0x32u}, /* Perfect Plex */
    {0x016DBEu,0x32u}, /* Gorilla Press */
    {0x01755Au,0x32u}, /* DDT */
    {0x017874u,0x2Bu}, /* Hogan Legdrop layer 1 */
    {0x01787Eu,0x32u}, /* Hogan Legdrop layer 2 */
    {0x0164B8u,0x2Bu}, /* Smash Suplex layer 1 */
    {0x0164C2u,0x32u}  /* Smash Suplex layer 2 */
};

uint16_t
wf24_stage_music_raw_word(unsigned stage_index)
{
    return stage_index < 10u ? stage_words[stage_index] : 0u;
}

bool
wf24_ringout_raw54_command(uint16_t raw54, uint8_t *command_out)
{
    if (!command_out || raw54 < 0x3165u || raw54 > 0x3168u)
        return false;
    *command_out = (uint8_t)(raw54 & 0x00FFu);
    return true;
}

bool
wf24_finisher_callsite_command(uint32_t call_pc, uint8_t *command_out)
{
    unsigned i;

    if (!command_out)
        return false;

    for (i = 0; i < sizeof(finisher_calls) / sizeof(finisher_calls[0]); ++i) {
        if (finisher_calls[i].call_pc == call_pc) {
            *command_out = finisher_calls[i].command;
            return true;
        }
    }
    return false;
}

bool
wf24_low_stamina_callsite(uint32_t call_pc, uint8_t *command_out)
{
    if (!command_out || call_pc != 0x0263CCu)
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
wf24_runtime_dispatch_raw54(wf24_command_bus_t *bus, uint16_t raw54)
{
    uint8_t command;

    if (!wf24_ringout_raw54_command(raw54, &command))
        return WF24_DISPATCH_INVALID;
    return wf24_command_dispatch(bus, command);
}

wf24_dispatch_result_t
wf24_runtime_dispatch_callsite(wf24_command_bus_t *bus, uint32_t call_pc)
{
    uint8_t command;

    if (wf24_low_stamina_callsite(call_pc, &command) ||
        wf24_finisher_callsite_command(call_pc, &command))
        return wf24_command_dispatch(bus, command);

    return WF24_DISPATCH_INVALID;
}
