#include "wf_music_elf.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#if defined(WF_AUDIO2_WORKSPACE_VDP1) && WF_AUDIO2_WORKSPACE_VDP1
enum{OWNED_BASE=0x05c78020u};
#else
enum{OWNED_BASE=0x06088000u};
#endif
enum{OWNED_BYTES=18432u,TEXT_BASE=WF_MUSIC_MODULE_BASE};
static unsigned writes;
static uint16_t text[32],state[6144],shared[2048],hot[OWNED_BYTES/2];
static void put16(uint8_t*p,unsigned x){p[0]=(uint8_t)(x>>8);p[1]=(uint8_t)x;}
static void put32(uint8_t*p,unsigned x){put16(p,x>>16);put16(p+2,x);}
static void ph(uint8_t*p,unsigned off,unsigned address,unsigned file,unsigned mem,unsigned flags){
 put32(p,1);put32(p+4,off);put32(p+8,address);put32(p+12,address);
 put32(p+16,file);put32(p+20,mem);put32(p+24,flags);put32(p+28,4);
}
static void fixture(uint8_t*p){
 memset(p,0,512);put32(p,0x7f454c46);p[4]=1;p[5]=2;p[6]=1;
 put16(p+16,2);put16(p+18,42);put32(p+20,1);put32(p+24,TEXT_BASE+32);
 put32(p+28,52);put32(p+36,2);put16(p+40,52);put16(p+42,32);put16(p+44,4);
 ph(p+52,192,TEXT_BASE,64,64,5);ph(p+84,320,WF_MUSIC_MODULE_STATE,0,256,6);
 ph(p+116,320,WF_MUSIC_MODULE_SHARED_ADDRESS,0,4096,6);ph(p+148,256,OWNED_BASE,64,3284,7);
 uint8_t*h=p+192;put32(h,WF_MUSIC_MODULE_MAGIC);put16(h+4,WF_MUSIC_MODULE_ABI);put16(h+6,32);
 put32(h+8,WF_MUSIC_MODULE_BASE);put32(h+12,WF_MUSIC_MODULE_END);
 put32(h+16,TEXT_BASE+32);put32(h+20,TEXT_BASE+34);put32(h+24,WF_MUSIC_MODULE_SHARED);
 put32(h+28,OWNED_BASE);
 for(unsigned i=224;i<320;i++)p[i]=(uint8_t)(i*13u+7u);
}
static int parse(const void*p,size_t n,wf_music_image_t*m){
#ifdef WF_TEST_LEGACY
 return wf_music_elf_image(p,n,m);
#else
 return wf_music_elf_image(p,n,OWNED_BASE,OWNED_BYTES,m);
#endif
}
static void store(uint32_t address,uint16_t value,void*u){
 (void)u;assert(address&0x20000000u);address&=0x1fffffffu;++writes;
 if(address>=TEXT_BASE&&address<TEXT_BASE+64)text[(address-TEXT_BASE)/2]=value;
 else if(address>=WF_MUSIC_MODULE_STATE&&address<WF_MUSIC_MODULE_SHARED_ADDRESS)state[(address-WF_MUSIC_MODULE_STATE)/2]=value;
 else if(address>=WF_MUSIC_MODULE_SHARED_ADDRESS&&address<WF_MUSIC_MODULE_END)shared[(address-WF_MUSIC_MODULE_SHARED_ADDRESS)/2]=value;
 else if(address>=OWNED_BASE&&address<OWNED_BASE+OWNED_BYTES)hot[(address-OWNED_BASE)/2]=value;
 else assert(!"write outside owned windows");
}
static int copy(const wf_music_image_t*m){
#ifdef WF_TEST_LEGACY
 return wf_music_elf_copy(m,store,NULL);
#else
 return wf_music_elf_copy(m,OWNED_BASE,OWNED_BYTES,store,NULL);
#endif
}
static unsigned rejects;
static void reject(const uint8_t*p,size_t n){
 wf_music_image_t m,before;memset(&m,0xa5,sizeof m);before=m;
 assert(!parse(p,n,&m));assert(!memcmp(&m,&before,sizeof m));++rejects;
}
int main(void){
 uint8_t p[512];wf_music_image_t m;
#ifndef WF_TEST_LEGACY
 uint8_t bad[512];
#endif
 fixture(p);
 if(!parse(p,320,&m)){puts("RED: four-segment music image for the selected master-owned window was rejected");return 1;}
 memset(text,0xa5,sizeof text);memset(state,0xa5,sizeof state);
 memset(shared,0xa5,sizeof shared);memset(hot,0xa5,sizeof hot);writes=0;
 assert(copy(&m)&&writes==(64+256+4096+3284)/2);
 assert(text[0]==0x5746&&text[16]==((unsigned)p[224]<<8|p[225]));
 for(unsigned i=0;i<128;i++)assert(!state[i]);
 assert(state[128]==0xa5a5);
 for(unsigned i=0;i<2048;i++)assert(!shared[i]);
 for(unsigned i=0;i<32;i++)assert(hot[i]==((unsigned)p[256+2*i]<<8|p[257+2*i]));
 for(unsigned i=32;i<3284/2;i++)assert(!hot[i]);
 assert(hot[3284/2]==0xa5a5);
#ifndef WF_TEST_LEGACY
 for(unsigned n=0;n<320;n++)reject(p,n);
 for(unsigned segment=0;segment<4;segment++){
  unsigned start=52+segment*32;
  memcpy(bad,p,512);put32(bad+start+12,0);reject(bad,320);
  memcpy(bad,p,512);put32(bad+start+20,0);reject(bad,320);
  memcpy(bad,p,512);put32(bad+start+24,0);reject(bad,320);
  memcpy(bad,p,512);put32(bad+start+4,321);reject(bad,320);
  memcpy(bad,p,512);put32(bad+start+8,0xffffffff);reject(bad,320);
  memcpy(bad,p,512);put32(bad+start+28,3);reject(bad,320);
 }
 memcpy(bad,p,512);put32(bad+148+20,OWNED_BYTES+4);reject(bad,320);
 memcpy(bad,p,512);put32(bad+148+4,192);reject(bad,320);
 memcpy(bad,p,512);put32(bad+148+4,240);reject(bad,320);
 memcpy(bad,p,512);put32(bad+148+8,OWNED_BASE+16);put32(bad+148+12,OWNED_BASE+16);reject(bad,320);
 memcpy(bad,p,512);put32(bad+192+28,0);reject(bad,320);
 memcpy(bad,p,512);put16(bad+192+4,3);reject(bad,320);
 memcpy(bad,p,512);put32(bad+24,TEXT_BASE+34);reject(bad,320);
 /* Same ABI number does not authorize a different video-memory profile. */
 unsigned other=TEXT_BASE==0x05c5c000u?0x05e26000u:0x05c5c000u;
 memcpy(bad,p,512);put32(bad+52+8,other);put32(bad+52+12,other);
 put32(bad+192+8,other);reject(bad,320);
 assert(!wf_music_elf_image(p,320,OWNED_BASE+16,OWNED_BYTES,&m));
 assert(!wf_music_elf_image(p,320,OWNED_BASE,16384,&m));
 /* Even a self-consistent ELF cannot authorize a different workspace
  * class than the master's compiled memory profile. */
 unsigned other_hot=OWNED_BASE==0x05c78020u?0x06088000u:0x05c78020u;
 memcpy(bad,p,512);put32(bad+148+8,other_hot);put32(bad+148+12,other_hot);
 put32(bad+192+28,other_hot);
 assert(!wf_music_elf_image(bad,320,other_hot,OWNED_BYTES,&m));
 assert(parse(p,320,&m));
 for(unsigned segment=0;segment<4;segment++){
  wf_music_image_t forged=m;forged.segments[segment].address+=4;writes=0;
  assert(!copy(&forged)&&!writes);
  forged=m;forged.segments[segment].file_bytes=forged.segments[segment].memory_bytes+4;writes=0;
  assert(!copy(&forged)&&!writes);
 }
 wf_music_image_t forged=m;forged.prepare=TEXT_BASE+64;writes=0;assert(!copy(&forged)&&!writes);
 put32(p+192+28,0);writes=0;assert(!copy(&m)&&!writes);
 printf("PASS owned 18KiB window, four loads, exact copy/zero-fill, %u malformed images, no partial output or writes\n",rejects);
#endif
 return 0;
}
