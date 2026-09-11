#ifndef WWF_PRESENTATION_ROM_EXACT_H
#define WWF_PRESENTATION_ROM_EXACT_H
#include <stdint.h>
enum {
    WF_ARCADE_BOOT_WAIT_VBLANKS = 128,
    WF_ARCADE_BOOT_FADE_STEPS = 8,
    WF_ARCADE_BOOT_FADE_FRAMES_PER_STEP = 3,
    WF_ARCADE_BOOT_FADE_FRAMES = 24,
    WF_ARCADE_ATTRACT_STATE_COUNT = 14
};
static const uint16_t wf_arcade_attract_state_frames[WF_ARCADE_ATTRACT_STATE_COUNT] = {
    0x0350,0x02E0,0x0410,0x03A0,0x0290,0x03A0,0x04B0,
    0x03B0,0x0650,0x0480,0x0320,0x0330,0x0340,0x0360
};
static const uint8_t wf_arcade_fade_4bit[16][WF_ARCADE_BOOT_FADE_STEPS] = {
    {0,0,0,0,0,0,0,0},{0,0,0,0,1,1,1,1},{0,0,0,1,1,1,2,2},{0,0,1,1,2,2,3,3},
    {0,0,1,2,3,3,4,4},{0,1,1,2,3,3,4,5},{0,1,2,3,3,4,5,6},{0,1,2,3,4,5,6,7},
    {0,1,2,3,4,5,6,8},{0,2,3,4,5,6,7,9},{0,1,2,4,5,7,8,10},{0,1,3,4,5,6,8,11},
    {0,2,3,4,5,7,9,12},{0,2,4,5,7,8,10,13},{0,1,3,5,7,9,11,14},{0,2,4,6,8,10,12,15}
};
#endif
