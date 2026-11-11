#include "wf_vdp2_resident.h"
#include <stdbool.h>
/* Same holes as wp2015_secondary_memory.h, with character-bank starts
 * derived from the successfully bound ring's actual tile count. All names,
 * all font bytes, and the back-color guard remain outside these intervals. */
typedef struct {uint32_t at,bytes;} region_t;
static const region_t names[]={
 {0x20800,0x1800},{0x22800,0x1800},{0x26800,0x1800},
 {0x28800,0x1800},{0x2c800,0x1800},{0x2e800,0x1800},{0x32000,0xdfe0}
};
static unsigned characters,generation;
static size_t published;
static bool bound,busy;
#ifdef WF_VDP2_RESIDENT_HOST_TEST
extern volatile uint16_t *wf_vdp2_resident_test_address(uint32_t);
#define VRAM(at) wf_vdp2_resident_test_address(at)
#else
#define VRAM(at) ((volatile uint16_t*)(uintptr_t)(0x25e00000u+(at)))
#endif
static region_t region(unsigned i){
 if(i<2)return (region_t){(i?0x40000u:0u)+characters,0x20000u-characters};
 return names[i-2];
}
void wf_vdp2_resident_reset(void){++generation;published=0;bound=false;}
int wf_vdp2_resident_bind(unsigned tiles){
 wf_vdp2_resident_reset();if(!tiles||tiles>1024)return -1;
 characters=tiles*128u;bound=true;return 0;
}
size_t wf_vdp2_resident_capacity(void){return bound?2u*(0x20000u-characters)+94176u:0u;}
static bool destination_ok(const void *p,size_t n){
 if(!p||n>UINTPTR_MAX-(uintptr_t)p)return false;
#if defined(__sh__)
 uintptr_t at=(uintptr_t)p&0x1fffffffu;
 /* Callers use disjoint HWRAM staging; never permit B-bus alias writes. */
 return at>=0x06000000u&&at<=0x06100000u&&n<=0x06100000u-at;
#else
 return true;
#endif
}
int wf_vdp2_resident_load(wf_vdp2_resident_reader_t read,const char *name,uint32_t file_at,
                         size_t bytes,void *scratch,size_t scratch_bytes){
 if(busy)return -1;
 published=0;
 if(!bound||!read||!name||!bytes||bytes>wf_vdp2_resident_capacity()||bytes>UINT32_MAX-file_at||
    scratch_bytes<2||scratch_bytes&1u||!destination_ok(scratch,scratch_bytes))return -1;
 busy=true;unsigned lease=generation;size_t copied=0;int result=-1;
 for(unsigned i=0;i<9&&copied<bytes;i++){
  region_t r=region(i);size_t at=0,available=r.bytes;
  if(available>bytes-copied)available=bytes-copied;
  while(at<available){
   size_t take=available-at;if(take>scratch_bytes)take=scratch_bytes;
   if(read(name,file_at+(uint32_t)copied,scratch,take)||generation!=lease)goto done;
   const uint8_t *p=scratch;volatile uint16_t *out=VRAM(r.at+(uint32_t)at);
   for(size_t k=0;k<take;k+=2)*out++=(uint16_t)((unsigned)p[k]<<8|(k+1<take?p[k+1]:0u));
   copied+=take;at+=take;
  }
 }
 if(copied==bytes&&generation==lease){published=bytes;result=0;}
done:busy=false;return result;
}
int wf_vdp2_resident_read(uint32_t offset,void *out,size_t bytes){
 if(busy||!bound||!published||offset>published||bytes>published-offset||!destination_ok(out,bytes))return -1;
 uint8_t *dest=out;size_t position=offset;
 for(unsigned i=0;i<9&&bytes;i++){
  region_t r=region(i);if(position>=r.bytes){position-=r.bytes;continue;}
  size_t take=r.bytes-position;if(take>bytes)take=bytes;
  volatile uint16_t *source=VRAM(r.at+(uint32_t)(position&~(size_t)1u));size_t n=take;
  if(position&1u){*dest++=(uint8_t)*source++;--n;}
  while(n>=2){uint16_t value=*source++;*dest++=(uint8_t)(value>>8);*dest++=(uint8_t)value;n-=2;}
  if(n)*dest++=(uint8_t)(*source>>8);
  bytes-=take;position=0;
 }
 return bytes?-1:0;
}
