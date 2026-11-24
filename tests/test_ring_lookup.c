/* Exhaustive lookup oracle independent of the residency data structure. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#ifndef WF_RING_SOURCE
#define WF_RING_SOURCE "../core/src/wf_r18_ring.c"
#endif
#include WF_RING_SOURCE
static unsigned long queries;
static void put16(uint8_t *p,unsigned v){p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v;}
static void put32(uint8_t *p,unsigned v){put16(p,v>>16);put16(p+2,v);}
static void check(const uint8_t *pack,size_t bytes){
 assert(!wf_r18_ring_bind(pack,bytes));
 unsigned count=u16(pack+10),start=u32(pack+16),next=0;
 const uint8_t *ids=pack+start+count*128u;
 for(unsigned source=0;source<65536u;source++){
  unsigned actual=0xdeadbeefu;
  int present=next<count&&u16(ids+next*2u)==source;
  int status=source_tile_index((uint16_t)source,&actual);
  assert(status==(present?0:-1));
  assert(actual==(present?next:0xdeadbeefu));
  if(present)next++;
  queries++;
 }
 assert(next==count);
 assert(wf_r18_ring_bind(NULL,0)==-1);
 unsigned actual=0xfeedu;assert(source_tile_index(0,&actual)==-1&&actual==0xfeedu);
}
int main(void){
 const char *names[]={"R18RING.BIN","R18MAP1.BIN","R18MAP2.BIN","R18MAP4.BIN","R18MAP5.BIN","R18MAP6.BIN","R18MENU.BIN"};
 for(unsigned pass=0;pass<2;pass++)for(unsigned file=0;file<7;file++){
  char path[128];snprintf(path,sizeof path,"cd/%s",names[pass?6u-file:file]);
  FILE *f=fopen(path,"rb");assert(f);assert(!fseek(f,0,SEEK_END));
  long size=ftell(f);assert(size>0);rewind(f);
  uint8_t *allocation=malloc((size_t)size+1);assert(allocation);
  uint8_t *pack=allocation+pass;assert(fread(pack,1,(size_t)size,f)==(size_t)size);fclose(f);
  check(pack,(size_t)size);free(allocation);
 }
 /* The public format accepts arbitrary sorted16-bit IDs. Exercise tables
  * wider than the sparse pool, including its46th/47th page boundary, gaps,
  * IDs outside the fast region and the last possible ID. */
 for(unsigned pattern=0;pattern<8;pattern++){
  unsigned count=pattern<2?1024u:256u;
  unsigned start=544u+1280u,palette=start+count*130u,bytes=palette+1024u;
  uint8_t *pack=calloc(1,bytes);assert(pack);memcpy(pack,"R18R",4);
  put16(pack+4,2);put16(pack+10,count);put32(pack+12,32);
  put32(pack+16,start);put32(pack+20,palette);put32(pack+24,160);put32(pack+28,128);
  for(unsigned page=0;page<128;page++)put32(pack+32+page*4,544);
  for(unsigned i=0;i<count;i++){
   unsigned id;
   switch(pattern){
    case 0:id=i;break;
    case 1:id=i*64u;break;
    case 2:id=i*32u;break;
    case 3:id=31u+i*32u;break;
    case 4:id=8192u+i*32u;break;
    case 5:id=65535u-(count-1u-i);break;
    case 6:id=i<46u?i*32u:8192u+(i-46u)*32u;break;
    default:id=i<47u?i*32u:8192u+(i-47u)*32u;break;
   }
   put16(pack+start+count*128u+i*2u,id);
  }
  check(pack,bytes);free(pack);
 }
 printf("PASS ring lookup: %lu exhaustive queries, all seven scenes, rebinding, unaligned packs, sparse overflow,16-bit boundaries\n",queries);
 return 0;
}
