#include "wf_front_resource_pack.h"
#include "wp1980_lz4.h"
#include <string.h>
static const char *const names[]={"PROGRAM.BIN","BACK.TEX","FONT.TEX","POST.TEX","FRONT.TEX","ENTRY.TEX","RING.TEX"};
static uint32_t word(const uint8_t*p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static int resource(const char*name){if(name)for(unsigned i=0;i<7;i++)if(!strcmp(name,names[i]))return (int)i;return -1;}
bool wf_front_pack_known(const char*name){return resource(name)>=0;}
static bool overlap(const void*a,size_t n,const void*b,size_t m){
 uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
 return n&&m&&(n>UINTPTR_MAX-x||m>UINTPTR_MAX-y||(x<y+m&&y<x+n));
}
static const uint8_t *entry(const wf_front_pack_t*p,unsigned i){return p->header+64+i*64;}
static const uint8_t *block(const wf_front_pack_t*p,unsigned i){return p->header+512+i*24;}
static const uint8_t *group(const wf_front_pack_t*p,unsigned i){return p->header+512+p->blocks*24+i*8;}
static uint32_t now(wf_front_pack_t*p){return p->clock?p->clock(p->clock_user):0;}
static int read_group(wf_front_pack_t*p,uint32_t at,uint32_t bytes,unsigned index){
 uint32_t start=now(p);int rc=p->source->read(p->source->user,"PROGRAM.BIN",at,p->cache,bytes);
 p->stats.read_fields+=now(p)-start;
 if(rc){p->ready=false;p->cached=UINT32_MAX;return -3;}
 ++p->stats.reads;p->stats.sectors+=bytes/2048u;p->cached=index;return 0;
}
static int validate(wf_front_pack_t*p){
 const uint8_t*h=p->header;static const uint8_t zero[4]={0};
 uint32_t checksum=wf_assets_crc32_update(0xffffffffu,h,44);
 checksum=wf_assets_crc32_update(checksum,zero,4);checksum=~wf_assets_crc32_update(checksum,h+48,2048-48);
 uint32_t nb=word(h+20),ng=word(h+24);
 if(word(h)!=0x57465031u||word(h+4)!=1u||word(h+8)!=2048u||word(h+12)!=p->bytes||word(h+16)!=7u||
    !nb||nb>64u||!ng||ng>16u||word(h+28)!=64u||word(h+32)!=512u||word(h+36)!=512u+nb*24u||
    word(h+40)!=512u+nb*24u+ng*8u||word(h+40)>2048u||word(h+44)!=p->header_crc||checksum!=p->header_crc||
    word(h+48)!=p->first_bytes||word(h+56)||word(h+60))return -2;
 p->blocks=nb;p->groups=ng;
 uint32_t next=0;
 for(unsigned i=0;i<ng;i++){
  const uint8_t*g=group(p,i);uint32_t at=word(g),n=word(g+4);
  if(at!=next||n<32768u||n>196608u||(n&16383u)||at>p->bytes||n>p->bytes-at)return -2;
  next=at+n;
 }
 if(next!=p->bytes||word(group(p,0)+4)!=p->first_bytes)return -2;
 uint32_t block_index=0,input_end=2048u,previous_group=0,total=0,used_groups=0,post_group=0;
 for(unsigned i=0;i<7;i++){
  const uint8_t*r=entry(p,i);uint8_t expected[12]={0};memcpy(expected,names[i],strlen(names[i]));
  uint32_t n=word(r+12),first=word(r+20),count=word(r+24),split=word(r+28),decoded=0;
  if(memcmp(r,expected,12)||!n||n>UINT32_MAX-total||first!=block_index||first>nb||!count||count>nb-first||
     split!=(i==3?416000u:0u)||(i==3&&n!=678144u))return -2;
  for(unsigned j=first;j<first+count;j++){
   const uint8_t*b=block(p,j);uint32_t src=word(b),in=word(b+4),dst=word(b+8),out=word(b+12),codec=word(b+16),g=word(b+20);
   if(g>=ng||g<previous_group||!in||in>65536u||!out||out>65536u||codec>1u||(!codec&&in!=out)||
      dst!=decoded||dst>n||out>n-dst||(src&31u)||src<input_end||src>UINT32_MAX-31u||
      (i==3&&dst<416000u&&out>416000u-dst)||(i>=4&&g<=post_group))return -2;
   const uint8_t*gr=group(p,g);uint32_t base=word(gr),bytes=word(gr+4);
   if(src<base||src-base>bytes||in>bytes-(src-base))return -2;
   uint32_t pad=(in+31u)&~31u;
   if(pad>bytes-(src-base))return -2;
   input_end=src+pad;previous_group=g;decoded+=out;used_groups|=1u<<g;
   if(i==3)post_group=g;
  }
  if(decoded!=n)return -2;
  block_index+=count;total+=n;
 }
 if(block_index!=nb||total!=word(h+52)||used_groups!=(1u<<ng)-1u)return -2;
 return 0;
}
int wf_front_pack_open(wf_front_pack_t*p,const wf_asset_io_t*source,uint8_t*cache,size_t cache_bytes,
 uint8_t*header,size_t header_bytes,uint32_t bytes,uint32_t first,uint32_t checksum,uint32_t(*clock)(void*),void*clock_user){
 if(!p||!source||!source->size||!source->read||!cache||!header||cache_bytes!=196608u||header_bytes!=2048u||
    ((uintptr_t)cache&31u)||((uintptr_t)header&31u)||overlap(cache,cache_bytes,header,header_bytes)||
    overlap(p,sizeof *p,cache,cache_bytes)||overlap(p,sizeof *p,header,header_bytes)||
    first<32768u||first>196608u||(first&16383u)||bytes<first)return -1;
 memset(p,0,sizeof *p);p->source=source;p->cache=cache;p->header=header;p->bytes=bytes;p->first_bytes=first;
 p->header_crc=checksum;p->clock=clock;p->clock_user=clock_user;p->cached=UINT32_MAX;
 uint32_t actual;if(source->size(source->user,"PROGRAM.BIN",&actual)||actual!=bytes)return -2;
 if(read_group(p,0,first,0))return -3;
 memcpy(header,cache,2048u);int rc=validate(p);p->ready=rc==0;return rc;
}
int wf_front_pack_size(void*user,const char*name,uint32_t*bytes){
 wf_front_pack_t*p=user;if(!p||!p->source||!name||!bytes)return -1;
 int i=resource(name);if(i<0)return p->source->size(p->source->user,name,bytes);
 if(!p->ready)return -2;*bytes=word(entry(p,(unsigned)i)+12);return 0;
}
static int decode(wf_front_pack_t*p,const uint8_t*src,uint32_t in,uint8_t*dst,uint32_t out){
 uint32_t start=now(p);int rc=wp1980_lz4_decode(src,in,dst,out);p->stats.decode_fields+=now(p)-start;
 if(rc){p->ready=false;p->cached=UINT32_MAX;return -4;}
 ++p->stats.decode_calls;p->stats.decoded_bytes+=out;return 0;
}
int wf_front_pack_read(void*user,const char*name,uint32_t at,void*destination,size_t bytes){
 wf_front_pack_t*p=user;if(!p||!p->source||!name||(!destination&&bytes))return -1;
 int i=resource(name);if(i<0)return p->source->read(p->source->user,name,at,destination,bytes);
 if(!p->ready)return -2;
 const uint8_t*r=entry(p,(unsigned)i);uint32_t size=word(r+12),first=word(r+20),count=word(r+24);
 if(at>size||bytes>size-at||overlap(destination,bytes,p->cache,196608u)||overlap(destination,bytes,p->header,2048u)||overlap(destination,bytes,p,sizeof *p))return -5;
 uint8_t*out=destination;uint32_t end=at+(uint32_t)bytes;
 for(unsigned j=first;j<first+count&&at<end;j++){
  const uint8_t*b=block(p,j);uint32_t dst=word(b+8),n=word(b+12);
  if(dst+n<=at||dst>=end)continue;
  uint32_t g=word(b+20);const uint8_t*gr=group(p,g);uint32_t base=word(gr),in=word(b+4);
  if(p->cached!=g){if(read_group(p,base,word(gr+4),g))return -3;}else ++p->stats.cache_hits;
  const uint8_t*src=p->cache+word(b)-base;uint32_t within=at-dst,take=n-within;if(take>end-at)take=end-at;
  if(!word(b+16)){memcpy(out,src+within,take);p->stats.decoded_bytes+=take;}
  else if(!within&&take==n){if(decode(p,src,in,out,n))return -4;}
  else{
   /* The source can overlap cache[0,in): preserve it with memmove, invalidate
    * the group, then use a disjoint64KiB decode destination within the lease. */
   ++p->stats.partial_reads;p->cached=UINT32_MAX;memmove(p->cache,src,in);
   if(decode(p,p->cache,in,p->cache+65536u,n))return -4;
   memcpy(out,p->cache+65536u+within,take);
  }
  out+=take;at+=take;
 }
 return at==end?0:-5;
}
