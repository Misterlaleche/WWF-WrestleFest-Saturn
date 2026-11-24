#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wf_jo_video.h"

/* Linked from the immutable B12 source with only external symbols renamed. */
extern void wf_b12_crop_tile(uint8_t pixels[128],wf_jo_crop_t *crop);
static uint32_t random_state=0x98d4c271u;
static unsigned cases;
static uint32_t random_word(void)
{random_state^=random_state<<13;random_state^=random_state>>17;random_state^=random_state<<5;return random_state;}

static void compare(const uint8_t source[128],unsigned offset,int alias)
{
    uint8_t expected[320] __attribute__((aligned(32)));
    uint8_t actual[320] __attribute__((aligned(32)));
    memset(expected,0xa7,sizeof expected);memset(actual,0xa7,sizeof actual);
    uint8_t *a=expected+64+offset,*b=actual+64+offset;
    memcpy(a,source,128);memcpy(b,source,128);
    wf_jo_crop_t first={0},second={0};
    wf_jo_crop_t *ca=alias==999?&first:(wf_jo_crop_t *)(void *)(a+alias);
    wf_jo_crop_t *cb=alias==999?&second:(wf_jo_crop_t *)(void *)(b+alias);
    wf_b12_crop_tile(a,ca);wf_jo_crop_tile(b,cb);
    assert(!memcmp(expected,actual,sizeof expected));
    assert(!memcmp(ca,cb,sizeof *ca));++cases;
}

int main(int argc,char **argv)
{
    assert(argc==2);FILE *file=fopen(argv[1],"rb");assert(file);
    uint8_t tile[128];
    for(unsigned id=0;id<65536u;++id){
        assert(fread(tile,1,sizeof tile,file)==sizeof tile);
        for(unsigned offset=0;offset<8u;++offset)compare(tile,offset,999);
    }
    assert(fgetc(file)==EOF);assert(!fclose(file));
    for(unsigned sample=0;sample<8192u;++sample){
        for(unsigned i=0;i<128u;++i)tile[i]=(uint8_t)random_word();
        unsigned top=random_word()%17u,bottom=top+random_word()%(17u-top);
        for(unsigned y=0;y<16u;++y){
            if(y<top||y>=bottom)memset(tile+y*8u,0,8);
            else if(sample%3u)memset(tile+y*8u+(sample&1u)*4u,0,4);
        }
        for(unsigned offset=0;offset<8u;++offset)compare(tile,offset,999);
        if(sample<256u)for(int alias=-3;alias<128;++alias)
            for(unsigned offset=0;offset<4u;++offset)compare(tile,offset,alias);
    }
    printf("PASS B12 crop differential: %u cases, all128 bytes/metadata/canaries, real65536 tiles, alignment and overlap\n",cases);
    return 0;
}
