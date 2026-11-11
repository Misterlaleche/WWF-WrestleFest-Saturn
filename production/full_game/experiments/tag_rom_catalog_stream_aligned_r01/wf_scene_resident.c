#pragma GCC optimize ("Os")
#include "wf_scene_resident.h"
#include "wp1980_inflate_dynamic.h"
#include <string.h>
/* Raw same-width copies from byte-backed storage; preserves pixel byte order. */
typedef uint32_t scene_word_t __attribute__((may_alias));
static uint16_t r16(const uint8_t*p){return (uint16_t)(((unsigned)p[0]<<8)|p[1]);}
static __attribute__((cold,noinline)) uint32_t r32(const uint8_t*p){return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3];}
static __attribute__((cold,noinline)) uint32_t checksum(const uint8_t*p,size_t n){uint32_t h=2166136261u;while(n--)h=(h^*p++)*16777619u;return h;}
void wf_scene_resident_close(wf_scene_resident_t*c){if(c)memset(c,0,sizeof *c);}
static int fail(wf_scene_resident_t*c){wf_scene_resident_close(c);return -1;}
static __attribute__((cold,noinline)) int index_read(const void*index,size_t n,uint32_t file_bytes,unsigned mask,unsigned key,
                      wf_scene_resident_location_t*scene,wf_scene_profile_location_t*profile){
 if(!index||n!=WF_SCENE_RESIDENT_INDEX||file_bytes<WF_SCENE_RESIDENT_ROM_PREFIX+WF_SCENE_RESIDENT_INDEX)return -1;
 const uint8_t*p=index;static const uint8_t rom_sha[32]={0x26,0x5a,0xc8,0x9d,0xee,0xb7,0xe7,0x08,0xac,0x6f,0xda,0x72,0x43,0x56,0xbf,0xd6,0xaf,0x1c,0x90,0x12,0x92,0x8a,0x4e,0x55,0x1e,0x6e,0x2f,0xaa,0x42,0x4c,0x23,0x06};
 static const unsigned masks[4]={5u,96u,3u,7u},keys[4]={0x87u,0x243u,0xc03u,0x381u};
 unsigned version=r16(p+4);
 if(memcmp(p,"WRSX",4)||(version<1u||version>3u)||r16(p+6)!=4u||r32(p+8)!=WF_SCENE_RESIDENT_ROM_PREFIX||r32(p+12)!=WF_SCENE_RESIDENT_INDEX||r32(p+16)!=file_bytes||r32(p+20)!=0x3619de1eu||r32(p+28)||memcmp(p+32,rom_sha,sizeof rom_sha))return -1;
 uint32_t h=2166136261u;for(unsigned i=0;i<WF_SCENE_RESIDENT_INDEX;i++)h=(h^((i>=24u&&i<28u)?0u:p[i]))*16777619u;
 if(h!=r32(p+24))return -1;
 unsigned profiles=version==3u?127u:4u,stride=version==3u?12u:64u;
 unsigned reserved=version==1u?160u:192u+profiles*stride;
 for(unsigned i=reserved;i<WF_SCENE_RESIDENT_INDEX;i++)if(p[i])return -1;
 uint32_t scene_end=file_bytes;
 if(version>=2u){
  if(memcmp(p+160,version==3u?"WTP2":"WTP1",4)||r16(p+164)!=version-1u||r16(p+166)!=profiles||r32(p+172)||r32(p+176)!=stride||r32(p+180)||r32(p+184)||r32(p+188))return -1;
  scene_end=r32(p+168);if(scene_end<526336u||scene_end>file_bytes)return -1;
 }
 uint32_t cursor=WF_SCENE_RESIDENT_ROM_PREFIX+WF_SCENE_RESIDENT_INDEX;wf_scene_resident_location_t selected={0};wf_scene_profile_location_t chosen={0};
 for(unsigned i=0;i<4u;i++){
  const uint8_t*e=p+64u+i*24u;uint32_t off=r32(e+4),bytes=r32(e+8);
  if(r32(e)!=masks[i]||off!=cursor||off>scene_end||bytes<64u||bytes>WF_SCENE_RESIDENT_LEASE-WF_SCENE_RESIDENT_ARCHIVE||bytes>scene_end-off||r32(e+16)||r32(e+20))return -1;
  uint32_t end=off+bytes,pad=(0u-end)&2047u;if(pad>scene_end-end)return -1;cursor=end+pad;
  if(mask==masks[i])selected=(wf_scene_resident_location_t){off,bytes,r32(e+12)};
 }
 if(cursor!=scene_end)return -1;
 if(version>=2u){
  unsigned previous=0;
  for(unsigned i=0;i<profiles;i++){
   const uint8_t*e=p+192u+i*stride;unsigned current=r32(e);
   if(version==2u){
    if(current!=keys[i]||r32(e+4))return -1;
    for(unsigned k=32u;k<64u;k++)if(e[k])return -1;
   }else if(current<=previous||current>0xfffu)return -1;
   previous=current;
   for(unsigned j=0;j<3u;j++){
    uint32_t pad=(0u-cursor)&32767u;if(pad>file_bytes-cursor)return -1;cursor+=pad;
    uint32_t off=cursor,bytes;
    if(version==3u)bytes=j==1u?12964u:r32(e+(j?8u:4u));
    else{off=r32(e+8u+j*8u);bytes=r32(e+12u+j*8u);}
    if(off!=cursor||off>file_bytes||!bytes||bytes>file_bytes-off||
       (j==0u?(bytes<68192u||bytes>710112u):j==1u?bytes!=12964u:(bytes<=27804u||bytes>1080764u)))return -1;
    if(key==current){chosen.members[j].offset=off;chosen.members[j].bytes=bytes;}
    cursor=off+bytes;
   }
  }
  uint32_t pad=(0u-cursor)&32767u;if(pad>file_bytes-cursor||cursor+pad!=file_bytes)return -1;
 }
 if(scene){if(!selected.bytes)return -1;*scene=selected;}
 if(profile)*profile=chosen;
 return 0;
}
static __attribute__((cold,noinline)) int index_destination(const void*index,size_t n,const void*out,size_t bytes){
 uintptr_t a=(uintptr_t)index,b=(uintptr_t)out;
#ifdef __sh__
 a&=0x1fffffffu;b&=0x1fffffffu;
#endif
 return out&&n<=UINTPTR_MAX-a&&bytes<=UINTPTR_MAX-b&&!(a<b+bytes&&b<a+n);
}
int wf_scene_resident_locate(const void*index,size_t n,uint32_t file_bytes,unsigned mask,wf_scene_resident_location_t*out){
 if(!index_destination(index,n,out,sizeof *out))return -1;memset(out,0,sizeof *out);
 return index_read(index,n,file_bytes,mask,0,out,NULL);
}
int wf_scene_resident_profile(const void*index,size_t n,uint32_t file_bytes,unsigned key,wf_scene_profile_location_t*out){
 if(!index_destination(index,n,out,sizeof *out))return -1;memset(out,0,sizeof *out);
 if(index_read(index,n,file_bytes,0,key,NULL,out))return -1;
 return out->members[0].bytes?1:0;
}
static const uint8_t *entry(const wf_scene_resident_t*c,unsigned scene){
 if(!c||!c->ready)return NULL;
 for(unsigned i=0;i<c->scenes;i++){const uint8_t*e=c->archive+64u+i*24u;if(r16(e)==scene)return e;}
 return NULL;
}
static int expand(wf_scene_resident_t*c,const uint8_t*e,uint8_t*out){
 uint32_t off=r32(e),n=r32(e+4),raw=r32(e+8);
 ++c->decodes;
 return wp1980_inflate_dynamic(c->archive+off,n,out,raw);
}
static int metadata(wf_scene_resident_t*c,const uint8_t*e){
 uint8_t*p=c->lease;unsigned scene=r16(e),count=r16(e+2);uint32_t raw=r32(e+20);
 if(expand(c,e+12,p)||raw<544u||memcmp(p,"R18R",4)||r16(p+4)!=2u||r16(p+6)!=scene||r16(p+8)!=scene||r16(p+10)!=count||r32(p+12)!=32u||r32(p+24)!=160u||r32(p+28)!=128u)return -1;
 uint32_t t=r32(p+16),pal=r32(p+20);
 if(!count||count>1024u||t<544u||t>raw||raw-t!=count*4u+1024u||pal!=t+count*130u||r32(e+4)!=pal+1024u)return -1;
 for(unsigned page=0;page<128u;page++){
  uint32_t off=r32(p+32u+page*4u);
  if(off<544u||off>t||t-off<1280u)return -1;
  for(unsigned j=0;j<320u;j++)if(r16(p+off+j*4u+2u)>=count)return -1;
 }
 const uint8_t*ids=p+t;const uint8_t*map=ids+count*2u+1024u;
 for(unsigned i=0;i<count;i++)if((i&&r16(ids+(i-1u)*2u)>=r16(ids+i*2u))||r16(map+i*2u)>=c->unique)return -1;
 c->metadata_bytes=raw;c->tiles_offset=t;c->ids_offset=t;c->palette_offset=t+count*2u;c->map_offset=t+count*2u+1024u;c->tiles=(uint16_t)count;c->scene=(uint16_t)scene;
 return 0;
}
/* Layout is canonical and contiguous, with only zero alignment padding.
 * Subtraction bounds precede all pointer additions into the input. */
static int span(const uint8_t*p,size_t n,const uint8_t*e,uint32_t raw_max,uint32_t*next){
 uint32_t off=r32(e),packed=r32(e+4),raw=r32(e+8);
 if(off!=*next||off>n||!packed||packed>65536u||packed>n-off||!raw||raw>raw_max)return -1;
 uint32_t end=off+packed,pad=(0u-end)&3u;
 if(pad>n-end)return -1;
 for(unsigned i=0;i<pad;i++)if(p[end+i])return -1;
 *next=end+pad;return 0;
}
int __attribute__((cold,noinline)) wf_scene_resident_open(wf_scene_resident_t*c,void*lease,size_t capacity,size_t n,unsigned mask){
 if(!c)return -1;
 /* Do not permit context clearing to overwrite the caller-owned lease. */
 uintptr_t cp=(uintptr_t)c,lp=(uintptr_t)lease;
 if(lease&&((cp>=lp&&cp-lp<capacity)||(lp>=cp&&lp-cp<sizeof *c)))return -1;
 wf_scene_resident_close(c);
 if(!lease||lp>UINTPTR_MAX-WF_SCENE_RESIDENT_LEASE||capacity!=WF_SCENE_RESIDENT_LEASE||n<64u||n>capacity-WF_SCENE_RESIDENT_ARCHIVE)return -1;
 const uint8_t*p=(const uint8_t*)lease+WF_SCENE_RESIDENT_ARCHIVE;
 unsigned scenes=r16(p+6);uint32_t unique=r32(p+12),blocks=r32(p+16),bdir=64u+scenes*24u;
 if(memcmp(p,"WRS1",4)||r16(p+4)!=1u||scenes<2u||scenes>3u||r32(p+8)!=n||!unique||unique>3072u||blocks!=(unique+31u)/32u||r32(p+20)!=64u||r32(p+24)!=bdir||r32(p+32)!=WF_SCENE_RESIDENT_META||r32(p+36)!=WF_SCENE_RESIDENT_BLOCK||r32(p+40)!=WF_SCENE_RESIDENT_ARCHIVE)return -1;
 for(unsigned i=44;i<64;i++)if(p[i])return -1;
 uint32_t next=bdir+blocks*12u;unsigned got_mask=0,last=0;
 if(next>n||checksum(p+64u,n-64u)!=r32(p+28))return -1;
 for(unsigned i=0;i<scenes;i++){
  const uint8_t*e=p+64u+i*24u;unsigned scene=r16(e);
  if((scene!=0u&&scene!=1u&&scene!=2u&&scene!=5u&&scene!=6u)||(i&&scene<=last)||!r16(e+2)||r16(e+2)>1024u||r32(e+8)||span(p,n,e+12,WF_SCENE_RESIDENT_META,&next))return -1;
  got_mask|=1u<<scene;last=scene;
 }
 if(got_mask!=mask||(mask!=5u&&mask!=96u&&mask!=3u&&mask!=7u))return -1;
 for(unsigned i=0;i<blocks;i++){
  const uint8_t*e=p+bdir+i*12u;uint32_t raw=(i+1u==blocks)?(unique-i*32u)*128u:WF_SCENE_RESIDENT_BLOCK;
  if(r32(e+8)!=raw||span(p,n,e,WF_SCENE_RESIDENT_BLOCK,&next))return -1;
 }
 if(next!=n)return -1;
 c->lease=lease;c->archive=p;c->bytes=(uint32_t)n;c->unique=unique;c->blocks=blocks;c->block_dir=bdir;c->scenes=(uint16_t)scenes;c->cached_block=-1;
 /* Validate output semantics before ready is externally observable. */
 for(unsigned i=0;i<scenes;i++)if(metadata(c,p+64u+i*24u))return fail(c);
 for(unsigned i=0;i<blocks;i++)if(expand(c,p+bdir+i*12u,c->lease+WF_SCENE_RESIDENT_META))return fail(c);
 c->selected=0;c->cached_block=-1;c->ready=1;return 0;
}
int wf_scene_resident_size(const wf_scene_resident_t*c,unsigned scene,uint32_t*n){
 const uint8_t*e=entry(c,scene);if(!e||!n)return -1;*n=r32(e+20);return 0;
}
int wf_scene_resident_select(wf_scene_resident_t*c,unsigned scene,void*dest,size_t n){
 const uint8_t*e=entry(c,scene);
 if(!e||dest!=c->lease||n!=r32(e+20))return fail(c);
 c->selected=0;c->cached_block=-1;if(metadata(c,e))return fail(c);
 c->selected=1;return 0;
}
int wf_scene_resident_view(const wf_scene_resident_t*c,const void*p,size_t n){
 return c&&c->ready&&c->selected&&p==c->lease&&n==c->metadata_bytes;
}
static int block(wf_scene_resident_t*c,unsigned i){
 if(i>=c->blocks)return fail(c);
 if(c->cached_block!=(int)i){
  if(expand(c,c->archive+c->block_dir+i*12u,c->lease+WF_SCENE_RESIDENT_META))return fail(c);
  c->cached_block=(int16_t)i;
 }
 return 0;
}
const uint8_t *wf_scene_resident_tile(wf_scene_resident_t*c,unsigned tile){
 if(!c||!c->ready||!c->selected||tile>=c->tiles){fail(c);return NULL;}
 unsigned global=r16(c->lease+c->map_offset+tile*2u);
 if(global>=c->unique||block(c,global/32u))return NULL;
 return c->lease+WF_SCENE_RESIDENT_META+(global%32u)*128u;
}
static int character_destination(wf_scene_resident_t*c,volatile uint8_t*dest,size_t n){
 uintptr_t dp=(uintptr_t)dest,lp=(uintptr_t)c->lease,cp=(uintptr_t)c;
 return dest&&dp<=UINTPTR_MAX-n&&
  !(dp>=lp?dp-lp<WF_SCENE_RESIDENT_LEASE:lp-dp<n)&&
  !(dp>=cp?dp-cp<sizeof *c:cp-dp<n);
}
static int characters_upload(wf_scene_resident_t*c,volatile uint8_t*dest,
                             volatile uint8_t*second,size_t capacity){
 if(!c||!c->ready||!c->selected||!c->blocks||c->blocks>96u||capacity<(size_t)c->tiles*128u)return fail(c);
 size_t n=(size_t)c->tiles*128u;
 if(!character_destination(c,dest,n)||(second&&!character_destination(c,second,n)))return fail(c);
 if(second){uintptr_t a=(uintptr_t)dest,b=(uintptr_t)second;if(a>=b?a-b<n:b-a<n)return fail(c);}
 const int word_aligned=(((uintptr_t)c->lease|(uintptr_t)dest|(uintptr_t)second)&3u)==0;
 uint32_t used[3]={0,0,0};
 for(unsigned t=0;t<c->tiles;t++){
  unsigned global=r16(c->lease+c->map_offset+t*2u),b=global/32u;
  if(global>=c->unique||b>=c->blocks)return fail(c);
  used[b/32u]|=1u<<(b%32u);
 }
 /* Expand only blocks referenced by this scene, once for both CPD banks.
  * Local IDs, duplicate tiles and pixel order are unchanged.
  * VDP2 VRAM accepts longwords; SH2 requires every operand aligned to 4. */
 for(unsigned b=0;b<c->blocks;b++){
  if(!(used[b/32u]&(1u<<(b%32u))))continue;
  if(block(c,b))return -1;
  for(unsigned t=0;t<c->tiles;t++){
   unsigned global=r16(c->lease+c->map_offset+t*2u);if(global/32u!=b)continue;
   const uint8_t*p=c->lease+WF_SCENE_RESIDENT_META+(global%32u)*128u;
   for(unsigned q=0;q<4u;q++)for(unsigned y=0;y<8u;y++){
    unsigned at=t*128u+q*32u+y*4u;
    const uint8_t *row=p+((q/2u)*8u+y)*8u+(q%2u)*4u;
    if(word_aligned){
     scene_word_t pixels=*(const scene_word_t *)row;
     *(volatile scene_word_t *)(dest+at)=pixels;
     if(second)*(volatile scene_word_t *)(second+at)=pixels;
    }else for(unsigned x=0;x<4u;x++){
     uint8_t pixel=row[x];dest[at+x]=pixel;if(second)second[at+x]=pixel;
    }
   }
  }
 }
 return 0;
}
int wf_scene_resident_characters(wf_scene_resident_t*c,volatile uint8_t*dest,size_t capacity){
 return characters_upload(c,dest,NULL,capacity);
}
int wf_scene_resident_characters_pair(wf_scene_resident_t*c,volatile uint8_t*first,
                                      volatile uint8_t*second,size_t capacity){
 if(!second)return fail(c);
 return characters_upload(c,first,second,capacity);
}
