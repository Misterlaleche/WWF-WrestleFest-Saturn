#include "wf_r18_metasprite.h"
#include <string.h>
#ifdef WF_R18_PROFILE_START
volatile uint32_t wf_r18_meta_cache_hits, wf_r18_meta_cache_misses;
#endif
typedef struct parser { wf_r18_metasprite_ctx_t *c; uint16_t x,y; uint8_t mirror,dx,dy; wf_status_t status; } parser_t;
static uint16_t be16(const uint8_t *p){return (uint16_t)((p[0]<<8)|p[1]);}
static uint32_t be32(const uint8_t *p){return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|(p[2]<<8)|p[3];}
static int get(parser_t *p,uint32_t a,void *v,size_t n){if(p->status!=WF_OK)return 0;if(a>0x80000u||n>0x80000u-a){p->status=WF_BLOCKED;return 0;}p->status=p->c->read_rom(p->c->user,a,v,n);return p->status==WF_OK;}
static uint16_t word(parser_t *p,uint32_t a){
 wf_r18_metasprite_cache_t *cache=p->c->cache;
 unsigned slot=(a>>1)&(WF_R18_METASPRITE_WORD_CACHE_ENTRIES-1u);
 if(p->status==WF_OK&&cache&&cache->word_tag[slot]==a+1u)
  return cache->word_data[slot];
 uint8_t v[2]={0};get(p,a,v,2);uint16_t result=be16(v);
 if(p->status==WF_OK&&cache){cache->word_tag[slot]=a+1u;cache->word_data[slot]=result;}
 return result;
}
static uint32_t lng(parser_t *p,uint32_t a){uint8_t v[4]={0};get(p,a,v,4);return be32(v);}
static void put16(uint8_t *p,uint16_t v){p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v;}
static uint32_t entry(parser_t *p,unsigned set,unsigned pose,uint32_t *base){
 if(set>=82){p->status=WF_BLOCKED;return 0;}
 wf_r18_metasprite_set_cache_entry_t *meta=(p->c->cache&&set<12u)?
     &p->c->cache->wrestler_sets[set]:NULL;
 uint16_t rel=(meta&&meta->valid)?meta->rel:word(p,0x38f14+set*2);
 if(!rel)return 0;
 uint16_t pose_index=(uint16_t)(pose&0x7fffu),off;
 wf_r18_metasprite_pose_cache_entry_t *cached=NULL;
 if(meta){
  unsigned slot=(pose_index*17u+set*7u)&(WF_R18_METASPRITE_POSE_CACHE_ENTRIES-1u);
  cached=&p->c->cache->pose_offsets[slot];
 }
 if(cached&&cached->valid&&cached->set==set&&cached->pose==pose_index)off=cached->offset;
 else{
  off=word(p,(uint32_t)(0x38f14+(int16_t)rel)+((pose<<1)&0xffff));
  if(cached&&p->status==WF_OK){cached->pose=pose_index;cached->offset=off;
   cached->set=(uint8_t)set;cached->valid=1;}
 }
 if(off==0xfffe)return 0;
 if(meta&&meta->valid)*base=meta->base;
 else{
  *base=lng(p,0x38fb8+set*4);
  if(meta&&p->status==WF_OK){meta->rel=rel;meta->base=*base;meta->valid=1;}
 }
 return *base+off;
}
/* Decode just the leaf's own body, as D43A/D540 do; no recursive rows.
 * The caller supplies clipping/shadow behavior for the actual routine. */
static int decode_body(parser_t *p,uint32_t a,uint16_t h,uint8_t rows[255][6],uint32_t *end,int skip_prefix){
 unsigned n=h&255;uint32_t q=a+2;
 if(skip_prefix&&(h&512)){
  uint8_t desc[4];if(!get(p,q,desc,4))return 0;q+=4;
  q+=!!(desc[1]&64)+!!(desc[1]&32)+!!(desc[1]&16);
 }
 if(!(h&0x4000)){
  if(!get(p,q,rows,n*6))return 0;
  *end=q+n*6;return 1;
 }
 q=(q+1)&~1u;uint16_t fmt=word(p,q);q+=2;
 for(unsigned col=0;col<6;col++){
  unsigned mode=(fmt>>(col*2))&3;uint8_t values[255];uint32_t at;
  if(mode==1){uint8_t v=0;if(!get(p,q++,&v,1))return 0;for(unsigned i=0;i<n;i++)rows[i][col]=v;continue;}
  if(mode==0){at=q;q+=n;}else{q=(q+1)&~1u;at=0x6b0b0+word(p,q)+(mode==3?0x8000:0);q+=2;}
  if(!get(p,at,values,n))return 0;
  for(unsigned i=0;i<n;i++)rows[i][col]=values[i];
 }
 *end=q;return p->status==WF_OK;
}
void wf_r18_metasprite_cache_reset(wf_r18_metasprite_cache_t *cache){
 if(cache)memset(cache,0,sizeof *cache);
}
static unsigned cache_bucket(uint32_t a,uint16_t h,unsigned skip_prefix){
 uint32_t mixed=(a>>1)^(a>>7)^(a>>13)^(uint32_t)h^((uint32_t)h>>6)^(skip_prefix<<5);
 return (unsigned)(mixed&(WF_R18_METASPRITE_CACHE_BUCKETS-1u));
}
static void cache_unlink(wf_r18_metasprite_cache_t *cache,unsigned slot){
 const wf_r18_metasprite_cache_entry_t *entry=&cache->entries[slot];
 unsigned bucket=cache_bucket(entry->address,entry->header,entry->skip_prefix);
 uint8_t *link=&cache->buckets[bucket];
 while(*link){
  if(*link==slot+1u){*link=cache->links[slot];break;}
  link=&cache->links[*link-1u];
 }
 cache->links[slot]=0;
}
typedef const uint8_t (*row_view_t)[6];
static row_view_t body(parser_t *p,uint32_t a,uint16_t h,uint8_t rows[255][6],uint32_t *end,int skip_prefix){
 wf_r18_metasprite_cache_t *cache=p->c->cache;unsigned n=h&255u;
 if(p->status!=WF_OK)return 0;
 if(cache&&n<=WF_R18_METASPRITE_CACHE_ROWS){
  unsigned bucket=cache_bucket(a,h,(unsigned)skip_prefix);
  for(uint8_t link=cache->buckets[bucket];link;link=cache->links[link-1u]){
   const wf_r18_metasprite_cache_entry_t *entry=&cache->entries[link-1u];
   if(entry->valid&&entry->address==a&&entry->header==h&&entry->skip_prefix==(unsigned)skip_prefix){
#ifdef WF_R18_PROFILE_START
    ++wf_r18_meta_cache_hits;
#endif
    *end=entry->end;return entry->rows;
   }
  }
 }
#ifdef WF_R18_PROFILE_START
 ++wf_r18_meta_cache_misses;
#endif
 if(!decode_body(p,a,h,rows,end,skip_prefix))return 0;
 if(cache&&n<=WF_R18_METASPRITE_CACHE_ROWS&&p->status==WF_OK){
  unsigned slot=cache->next,bucket=cache_bucket(a,h,(unsigned)skip_prefix);
  wf_r18_metasprite_cache_entry_t *entry=&cache->entries[slot];
  if(entry->valid)cache_unlink(cache,slot);
  entry->address=a;entry->header=h;entry->skip_prefix=(uint8_t)skip_prefix;entry->end=*end;
  memcpy(entry->rows,rows,n*6u);entry->valid=1;
  cache->links[slot]=cache->buckets[bucket];cache->buckets[bucket]=(uint8_t)(slot+1u);
  cache->next=(uint8_t)((cache->next+1u)%WF_R18_METASPRITE_CACHE_ENTRIES);
 }
 return rows;
}
static void emit_row(parser_t *p,const uint8_t r[6],uint8_t dx,uint8_t dy,uint8_t paloff,int plain){
 uint8_t bx=(uint8_t)(r[0]+p->dx+dx),by=(uint8_t)(r[1]+p->dy+dy);
 if(p->mirror)bx=(uint8_t)(0u-bx);
 uint16_t x=(uint16_t)(p->x+0x10+(int8_t)bx),y=(uint16_t)(p->y+0x90+(int8_t)by);
 if(plain)p->c->d6=5;
 if(x>0x15f)return;
 if(plain)p->c->d6=4;
 if(y>0x187)return;
 if(plain)p->c->d6=0;
 x=(uint16_t)(x-0x10);y=(uint16_t)(y-0x90);
 if(p->c->count>=p->c->capacity){p->status=WF_BLOCKED;return;}
 uint8_t *o=p->c->records[p->c->count++].bytes;
 /* Fixed-size clearing stays inline; the generic byte-aligned API remains
  * supported while naturally aligned Saturn records use four word stores. */
 if(!((uintptr_t)o&3u))__builtin_memset(__builtin_assume_aligned(o,4u),0,16);
 else __builtin_memset(o,0,16);
 put16(o,y);o[3]=(uint8_t)((r[2]|((x>>6)&4)|((y>>7)&2))^(p->mirror?16:0));
 o[5]=r[5];o[7]=r[4];o[9]=p->c->palette_map[(uint8_t)(r[3]+paloff)];put16(o+10,x);
}
static uint32_t reference(parser_t *p,uint32_t q,int tail){
 uint8_t desc[4],offset[3]={0};if(!get(p,q,desc,4))return q;q+=4;
 for(unsigned j=0;j<3;j++)if(desc[1]&(64u>>j)){if(!get(p,q++,&offset[j],1))return q;}
 uint32_t target=be32(desc)&0xfffff;uint16_t h=word(p,target);unsigned n=h&255,take=desc[0];uint8_t rows[255][6];uint32_t end;
 /* D43A's packed-reference prefix skip differs from the ordinary decoder.
  * No known FINAL corpus reference enters this case: fail closed. */
 if(!take||take>n||((h&0x8000)!=0)||(!tail&&(h&0x4200)==0x4200)){p->status=WF_BLOCKED;return q;}
 row_view_t decoded=body(p,target,h,rows,&end,tail);if(!decoded)return q;
 unsigned start=tail?n-take:0;
 for(unsigned i=0;i<take&&p->status==WF_OK;i++)emit_row(p,decoded[start+i],offset[0],offset[1],offset[2],!(h&0x4000));
 if(h&0x4000)p->c->d6=0xffff; /* D63C CLR.L / DBRA D6 */
 return q;
}
static void leaf(parser_t *p,uint32_t a,uint16_t h){
 uint32_t q=a+2,end=0;uint8_t rows[255][6];
 if(h&0x8000){p->status=WF_BLOCKED;return;}
 if(h&512)q=reference(p,q,0);
 row_view_t decoded=body(p,a,h,rows,&end,1);if(!decoded)return;
 unsigned n=h&255;
 if((h&0x4000)&&!n){p->status=WF_BLOCKED;return;} /* packed DBRA underflow */
 if(h&0x4000)p->c->d6=0x1c1610;
 for(unsigned i=0;i<n&&p->status==WF_OK;i++)emit_row(p,decoded[i],0,0,0,!(h&0x4000));
 if(h&256)reference(p,(end+1)&~1u,1);
 (void)q;
}
static void composition(parser_t *p,uint32_t a,uint32_t base,unsigned partner){
 uint16_t h=word(p,a);unsigned kind=h>>14,n=h&255;
 if(kind<2){leaf(p,a,h);return;}
 if(kind==3){
  if(!n){p->status=WF_BLOCKED;return;}
  for(unsigned i=0;i<n&&p->status==WF_OK;i++){
   uint32_t part=base+word(p,a+2+i*2);
   leaf(p,part,word(p,part));
  }
  return;
 }
 if(partner>=12){p->status=WF_BLOCKED;return;}
 uint32_t otherbase=0,other=entry(p,partner,word(p,a+2),&otherbase);
 if(!other){p->c->d7=6;return;} /* $DA16 +22; $DA5A -16 */
 uint32_t own=base+word(p,a+4);uint8_t dxy[2]={0};get(p,a+6+partner*2,dxy,2);
 uint32_t addr[2]={own,other},bases[2]={base,otherbase};unsigned total[2],used[2]={0,0};int multi[2];uint16_t heads[2];
 for(unsigned i=0;i<2;i++){heads[i]=word(p,addr[i]);multi[i]=(heads[i]&0x8000)!=0;total[i]=multi[i]?(heads[i]&255):1;if(!total[i]){p->status=WF_BLOCKED;return;}if(multi[i])addr[i]+=2;}
 unsigned bits=h,chosen=bits&1;bits>>=1;
 while((used[0]<total[0]||used[1]<total[1])&&p->status==WF_OK){
  if(used[chosen]>=total[chosen])chosen^=1;
  p->dx=chosen?dxy[0]:0;p->dy=chosen?dxy[1]:0;
  uint32_t part=multi[chosen]?bases[chosen]+word(p,addr[chosen]+used[chosen]*2):addr[chosen];
  leaf(p,part,multi[chosen]?word(p,part):heads[chosen]);used[chosen]++;
  if(multi[chosen]&&used[chosen]<total[chosen]){chosen=bits&1;bits>>=1;}else chosen^=1;
 }
 p->dx=p->dy=0;
}
wf_status_t wf_r18_metasprite_emit(wf_r18_metasprite_ctx_t *c,uint16_t set,uint16_t pose,uint16_t x,uint16_t y,uint16_t partner){
 if(!c||!c->read_rom||!c->palette_map||!c->records||c->count>c->capacity)return WF_BAD_STATE;
 if(c->count>=512)return WF_OK; /* $D1FC entry limit */
 size_t oldcount=c->count;uint32_t old6=c->d6,old7=c->d7,base=0;
 parser_t p={.c=c,.x=x,.y=y,.mirror=(uint8_t)((pose>>15)&1),.status=WF_OK};
 uint32_t a=entry(&p,set,pose,&base);
 if(a&&p.status==WF_OK){c->d7=0;composition(&p,a,base,partner);}
 if(p.status!=WF_OK){c->count=oldcount;c->d6=old6;c->d7=old7;}
 return p.status;
}
