#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "wf_r18_ring.h"

static uint8_t ring[114918];
int main(int argc, char **argv)
{
    static const int xs[] = {-301, -1, 0, 15, 319, 2549, 2559};
    static const int ys[] = {-17, 0, 15, 16, 127, 2047, 2051};
    uint8_t line[320];
    if (argc != 2) return 2;
    FILE *f = fopen(argv[1], "rb");
    if (!f || fread(ring, 1, sizeof ring, f) != sizeof ring ||
        wf_r18_ring_bind(ring, sizeof ring) != 0) return 2;
    fclose(f);
    for (unsigned layer=0; layer<2; ++layer)
        for (unsigned transparent=0; transparent<2; ++transparent)
            for (unsigned xi=0; xi<sizeof xs/sizeof xs[0]; ++xi)
                for (unsigned yi=0; yi<sizeof ys/sizeof ys[0]; ++yi) {
                    if (wf_r18_ring_scanline(layer, xs[xi], ys[yi],
                                             transparent, line) != 0) return 1;
                    for (unsigned x=0; x<320; ++x) {
                        uint8_t expected;
                        if (wf_r18_ring_pixel(layer,xs[xi]+(int)x,ys[yi],
                                              &expected) != 0) return 1;
                        if (transparent && !(expected&15u)) expected=0;
                        if (line[x] != expected) {
                            fprintf(stderr,"Ring differs layer=%u x=%d y=%d pixel=%u\n",
                                    layer,xs[xi],ys[yi],x);
                            return 1;
                        }
                    }
                }
    unsigned tiles_off=((unsigned)ring[16]<<24)|((unsigned)ring[17]<<16)|
                       ((unsigned)ring[18]<<8)|ring[19];
    unsigned count=((unsigned)ring[10]<<8)|ring[11];
    unsigned source=((unsigned)ring[tiles_off+count*128u]<<8)|
                    ring[tiles_off+count*128u+1u];
    if (wf_r18_ring_tile_write(0,0,15u,(uint16_t)source) != 0 ||
        wf_r18_ring_scanline(0,0,0,0,line) != 0) return 1;
    for (unsigned x=0; x<16u; ++x) {
        uint8_t expected;
        if (wf_r18_ring_pixel(0,(int)x,0,&expected) != 0 ||
            line[x] != expected) return 1;
    }
    wf_r18_ring_tile_restore(0,0);
    puts("PASS RUNTIME-18 ring scanlines match ROM-derived pixels");
    return 0;
}
