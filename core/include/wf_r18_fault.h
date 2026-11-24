#ifndef WF_R18_FAULT_H
#define WF_R18_FAULT_H
#include <stdint.h>

/* Fault-only text on the existing 512x256, 8bpp NBG0 bitmap. Word accesses
 * preserve the other pixel: VDP2 VRAM must not depend on byte writes.
 * Put owner and dependency at the vertical center: on a CRT camera capture,
 * the first two scanline rows can be lost to shutter banding. The left digit
 * remains the diagnostic word number, independent of its screen row. */
static void wf_r18_fault_draw(volatile uint16_t *bitmap,
                              const volatile uint32_t values[10])
{
    static const uint8_t hex[16][7]={
        {14,17,19,21,25,17,14},{4,12,4,4,4,4,14},
        {14,17,1,2,4,8,31},{30,1,1,14,1,1,30},
        {2,6,10,18,31,2,2},{31,16,16,30,1,1,30},
        {14,16,16,30,17,17,14},{31,1,2,4,8,8,8},
        {14,17,17,14,17,17,14},{14,17,17,15,1,1,14},
        {14,17,17,31,17,17,17},{30,17,17,30,17,17,30},
        {15,16,16,16,16,16,15},{30,17,17,17,17,17,30},
        {31,16,16,30,16,16,31},{31,16,16,30,16,16,16}
    };
    static const uint8_t display_order[10]={5,6,2,3,4,0,1,7,8,9};
    for(unsigned row=0;row<10u;row++){
        unsigned word=display_order[row];
        uint32_t value=values[word];
        for(unsigned column=0;column<9u;column++){
            unsigned digit=column?((value>>(32u-column*4u))&15u):word;
            unsigned x0=32u+(column?36u+(column-1u)*18u:0u);
            unsigned y0=8u+row*23u;
            for(unsigned y=0;y<7u;y++)for(unsigned x=0;x<5u;x++){
                if(!(hex[digit][y]&(16u>>x)))continue;
                for(unsigned sy=0;sy<3u;sy++)for(unsigned sx=0;sx<3u;sx++){
                    unsigned px=x0+x*3u+sx;
                    unsigned at=(y0+y*3u+sy)*256u+px/2u;
                    uint16_t old=bitmap[at];
                    bitmap[at]=(px&1u)?(uint16_t)((old&0xff00u)|3u):
                                        (uint16_t)((old&0x00ffu)|0x0300u);
                }
            }
        }
    }
}
#endif
