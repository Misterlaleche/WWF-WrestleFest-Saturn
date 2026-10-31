#include "wf_r18_frontend_fade.h"

/* Original bytes $266B2..$26731, indexed by channel*8 + phase.
 * The intermediate levels are deliberately not replaced by arithmetic. */
static const uint8_t fade_channel[16][8]={
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,1,1,1,1},
    {0,0,0,1,1,1,2,2},
    {0,0,1,1,2,2,3,3},
    {0,0,1,2,3,3,4,4},
    {0,1,1,2,3,3,4,5},
    {0,1,2,3,3,4,5,6},
    {0,1,2,3,4,5,6,7},
    {0,1,2,3,4,5,6,8},
    {0,2,3,4,5,6,7,9},
    {0,1,2,4,5,7,8,10},
    {0,1,3,4,5,6,8,11},
    {0,2,3,4,5,7,9,12},
    {0,2,4,5,7,8,10,13},
    {0,1,3,5,7,9,11,14},
    {0,2,4,6,8,10,12,15}
};

wf_status_t wf_r18_frontend_fade_palette_26642(const uint16_t source[256],
                                             uint16_t output[256],
                                             unsigned phase)
{
    if(!source||!output||phase>7u)return WF_BAD_STATE;
    for(unsigned color=0;color<256u;color++){
        uint16_t input=source[color];
        output[color]=(uint16_t)(((uint16_t)fade_channel[(input>>8)&15u][phase]<<8)|
                                ((uint16_t)fade_channel[(input>>4)&15u][phase]<<4)|
                                fade_channel[input&15u][phase]);
    }
    return WF_OK;
}
