#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wf_r18_ring.h"
static unsigned be16(const uint8_t*p){return ((unsigned)p[0]<<8)|p[1];}
static unsigned be32(const uint8_t*p){return (be16(p)<<16)|be16(p+2);}
static unsigned pixel(const uint8_t *p,unsigned x,unsigned y)
{
    unsigned offset=((y/8)*2+x/8)*32+(y%8)*4+(x%8)/2;
    return x&1?p[offset]&15:p[offset]>>4;
}
static void check_names(const uint8_t *chars,const uint16_t map[1024],unsigned layer,int wx,int wy)
{
    for(unsigned y=0;y<240;y+=3)for(unsigned x=0;x<320;x+=3){
        unsigned px=(unsigned)(wx+(int)x)%2560,py=(unsigned)(wy+(int)y)%2048;
        unsigned name=map[((py/16)&31)*32+((px/16)&31)];
        unsigned tx=px&15,ty=py&15;
        if(name&0x400)tx=15-tx;if(name&0x800)ty=15-ty;
        uint8_t reference;
        assert(!wf_r18_ring_pixel(layer,(int)px,(int)py,&reference));
        assert(reference==((name>>12)*16+pixel(chars+(name&1023)*128,tx,ty)));
    }
}
int main(void)
{
    const char *names[]={"R18RING.BIN","R18MAP1.BIN","R18MAP2.BIN","R18MAP4.BIN","R18MAP5.BIN","R18MAP6.BIN","R18MENU.BIN"};
    uint8_t *chars=malloc(0x20002),*unaligned=malloc(0x20002);
    assert(chars&&unaligned);
    for(unsigned file=0;file<sizeof names/sizeof *names;file++){
        char path[128];snprintf(path,sizeof path,"cd/%s",names[file]);FILE *f=fopen(path,"rb");assert(f);
        assert(!fseek(f,0,SEEK_END));long length=ftell(f);assert(length>0);rewind(f);
        uint8_t *pack=malloc((size_t)length);assert(pack);assert(fread(pack,1,(size_t)length,f)==(size_t)length);fclose(f);
        /* Every aliased/unique directory entry must remain checked. A bad
         * tile near the end of its page is rejected even after earlier valid
         * aliases; restoration must permit the original complete bind. */
        for(unsigned page=0;page<128;page++){
            unsigned at=be32(pack+32+page*4)+319u*4u+2u;
            uint8_t a=pack[at],b=pack[at+1];pack[at]=pack[at+1]=255;
            assert(wf_r18_ring_bind(pack,(size_t)length)==-1);
            pack[at]=a;pack[at+1]=b;
        }
        assert(!wf_r18_ring_bind(pack,(size_t)length));
        assert(!wf_r18_ring_characters(chars,0x20000));assert(!wf_r18_ring_characters(unaligned+1,0x20000));
        unsigned tiles=be16(pack+10),start=be32(pack+16);
        assert(!memcmp(chars,unaligned+1,tiles*128));
        for(unsigned t=0;t<tiles;t++)for(unsigned y=0;y<16;y++)for(unsigned x=0;x<16;x++){
            unsigned value=pack[start+t*128+y*8+x/2];value=x&1?value&15:value>>4;
            assert(value==pixel(chars+t*128,x,y));
        }
        for(unsigned layer=0;layer<2;layer++){
            uint16_t map[1024]={0};size_t writes;int wx=1023,wy=1111;
            assert(!wf_r18_ring_cells_update(layer,wx,wy,map,&writes)&&writes);
            check_names(chars,map,layer,wx,wy);
            assert(!wf_r18_ring_cells_update(layer,wx,wy,map,&writes)&&!writes);
            unsigned cell=((unsigned)wy/16&31)*32+((unsigned)wx/16&31);
            assert(!wf_r18_ring_tile_write(layer,cell,0xc3,(uint16_t)be16(pack+start+tiles*128)));
            assert(!wf_r18_ring_cells_update(layer,wx,wy,map,&writes)&&writes==1);
            check_names(chars,map,layer,wx,wy);
        }
        free(pack);
    }
    free(chars);free(unaligned);
    puts("Jo ring: seven scene packs, aligned/unaligned tiles, quadrant pixels, dirty names and flips: PASS");
    return 0;
}
