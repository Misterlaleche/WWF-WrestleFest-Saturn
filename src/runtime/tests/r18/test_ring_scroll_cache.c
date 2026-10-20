#include <stdint.h>
#include <stdio.h>
#include "wf_r18_ring.h"

static uint8_t ring[114918];
static uint8_t surface[2][512u*256u];

static int check(unsigned layer,int sx,int wy,unsigned transparent,size_t *writes)
{
    if (wf_r18_ring_surface_update(layer,sx,wy,transparent,
                                   surface[layer],writes) != 0) return -1;
    for (unsigned y=0; y<240u; ++y)
        for (unsigned x=0; x<320u; ++x) {
            uint8_t expected;
            if (wf_r18_ring_pixel(layer,sx+(int)x,wy+(int)y,&expected) != 0)
                return -1;
            if (transparent && !(expected&15u)) expected=0;
            unsigned slot_x=(unsigned)(sx+(int)x)&511u;
            unsigned slot_y=(unsigned)(wy+(int)y)&255u;
            if (surface[layer][slot_y*512u+slot_x] != expected) {
                fprintf(stderr,"Ring scroll differs layer=%u sx=%d wy=%d x=%u y=%u\n",
                        layer,sx,wy,x,y);
                return -1;
            }
        }
    return 0;
}

int main(int argc,char **argv)
{
    static const int positions[][2] = {
        {480,1488},{481,1488},{482,1489},{479,1488},
        {520,1490},{2540,2035},{-5,-7},{2558,2047}
    };
    if (argc != 2) return 2;
    FILE *f=fopen(argv[1],"rb");
    if (!f || fread(ring,1,sizeof ring,f)!=sizeof ring ||
        wf_r18_ring_bind(ring,sizeof ring)!=0) return 2;
    fclose(f);
    for (unsigned layer=0; layer<2u; ++layer) {
        size_t writes=0;
        for (unsigned i=0; i<sizeof positions/sizeof positions[0]; ++i) {
            if (check(layer,positions[i][0],positions[i][1],layer==0u,&writes))
                return 1;
            if (i==0u && writes!=320u*240u) {
                fprintf(stderr,"Initial ring fill wrote %zu pixels\n",writes);
                return 1;
            }
            if (i==2u && writes>1024u) {
                fprintf(stderr,"One-pixel camera move rewrote %zu pixels\n",writes);
                return 1;
            }
        }
    }
    unsigned t=((unsigned)ring[16]<<24)|((unsigned)ring[17]<<16)|
               ((unsigned)ring[18]<<8)|ring[19];
    unsigned n=((unsigned)ring[10]<<8)|ring[11];
    unsigned tile=((unsigned)ring[t+n*128u]<<8)|ring[t+n*128u+1u];
    size_t writes;
    if (check(0,0,0,1u,&writes) != 0) return 1;
    if (wf_r18_ring_tile_write(0,0,15u,(uint16_t)tile) != 0 ||
        check(0,0,0,1u,&writes) != 0 || !writes || writes>256u) return 1;
    wf_r18_ring_tile_restore(0,0);
    if (check(0,0,0,1u,&writes) != 0 || !writes || writes>256u) return 1;
    puts("PASS RUNTIME-18 hardware-scroll surface matches ROM pixels");
    return 0;
}
