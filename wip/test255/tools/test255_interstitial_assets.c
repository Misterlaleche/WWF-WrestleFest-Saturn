#include "interstitial_assets.h"
#include "roster_pack.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
static uint8_t *read_all(const char *p,size_t*n){FILE*f=fopen(p,"rb");assert(f);fseek(f,0,SEEK_END);long z=ftell(f);rewind(f);uint8_t*b=malloc((size_t)z);assert(b);assert(fread(b,1,(size_t)z,f)==(size_t)z);fclose(f);*n=(size_t)z;return b;}
static uint16_t be16(const uint8_t*p){return (uint16_t)((p[0]<<8)|p[1]);}
static uint16_t cv(uint16_t v){unsigned r=v&15U,g=(v>>4)&15U,b=(v>>8)&15U;r=(r<<1)|(r>>3);g=(g<<1)|(g>>3);b=(b<<1)|(b>>3);return (uint16_t)(0x8000U|(b<<10)|(g<<5)|r);}
int main(void){
 size_t ps=0,gs=0,pals=0;uint8_t*p=read_all("program.bin",&ps),*g=read_all("cd/INTER2C.PAK",&gs),*pal=read_all("assets/INTER2C.PAL",&pals);assert(ps==0x80000U);assert(gs==INTERSTITIAL2C_BYTES);assert(pals==INTERSTITIAL2C_PALETTE_BYTES);
 roster_pack_t pack;assert(roster_pack_open(&pack,g,gs,INTERSTITIAL2C_FRAMES));assert(pack.frames==32U&&pack.records==2288U&&pack.tiles==973U);
 for(unsigned j=0;j<3;j++){unsigned bank=0x31U+j;for(unsigned i=0;i<16;i++)assert(be16(pal+(j*16+i)*2)==cv(be16(p+0x2F22U+bank*32U+i*2U)));}
 for(unsigned f=0;f<32;f++){unsigned bank=interstitial2c_palette_bank(f);assert(bank==((f<=9)?0x31U:((f<=0x15U||f==0x1fU)?0x32U:0x33U)));}
 free(p);free(g);free(pal);puts("PASS Test255: gfx-set $2C exact 32-frame pack and palette resources $31/$32/$33 validate");return 0;
}
