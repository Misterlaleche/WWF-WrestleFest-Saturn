#ifndef WF_POST_ATLAS_H
#define WF_POST_ATLAS_H
#include "wf_assets.h"
#include "wf_video_plan.h"
#include <string.h>

enum {WF_POST_ATLAS_STATIC_MAX=3250,WF_POST_ATLAS_EXTRA_MAX=512,
 WF_POST_ATLAS_MAX_TILES=3762,WF_POST_ATLAS_TILE_BYTES=128,
 WF_POST_ATLAS_LOGICAL_MAX=65536,WF_POST_ATLAS_CRC_BYTES=65536*4,WF_POST_ATLAS_PACK_BYTES=65536*128};
_Static_assert(1028u*32u+WF_POST_ATLAS_MAX_TILES*128u<=0x80000u,"POST plus final picture fits VDP1");
typedef struct {
 const uint16_t *fixed;
 unsigned fixed_count,count;
 uint16_t ids[WF_POST_ATLAS_EXTRA_MAX];
 uint32_t crc[WF_POST_ATLAS_EXTRA_MAX];
 bool ready;
} wf_post_atlas_t;
typedef bool (*wf_post_atlas_upload_fn)(void *,unsigned,const uint8_t *);

static inline int wf_post_atlas_find(const uint16_t *ids,unsigned count,unsigned code){
 unsigned lo=0,hi=count;
 while(lo<hi){unsigned mid=(lo+hi)/2u;if(ids[mid]<code)lo=mid+1u;else hi=mid;}
 return lo<count&&ids[lo]==code?(int)lo:-1;
}
/* A mapped frontend keeps 3250 physical slots while retaining more logical
 * IDs. The caller owns immutable tables; no atlas-object ABI fields change. */
static inline bool wf_post_atlas_mapping_valid(const uint16_t *fixed,unsigned count,
 const uint16_t *mapping,unsigned mapping_count){
 if(!fixed||!count||count>WF_POST_ATLAS_LOGICAL_MAX)return false;
 if(mapping){if(mapping_count!=count)return false;}
 else if(mapping_count||count>WF_POST_ATLAS_STATIC_MAX)return false;
 for(unsigned i=0;i<count;i++){
  if(i&&fixed[i]<=fixed[i-1])return false;
  if(mapping&&mapping[i]>=WF_POST_ATLAS_STATIC_MAX)return false;
 }
 return true;
}
/* Big-endian serialization makes the generated checksum host-independent. */
static inline uint32_t wf_post_atlas_table_crc(const uint16_t *table,unsigned count){
 uint32_t crc=0xffffffffu;
 for(unsigned i=0;i<count;i++){
  uint8_t bytes[2]={(uint8_t)(table[i]>>8),(uint8_t)table[i]};
  crc=wf_assets_crc32_update(crc,bytes,sizeof bytes);
 }
 return crc^0xffffffffu;
}
static inline int wf_post_atlas_fixed_slot(const uint16_t *fixed,unsigned count,
 unsigned code,const uint16_t *mapping,unsigned mapping_count){
 if(!fixed||!count||count>WF_POST_ATLAS_LOGICAL_MAX||
    (mapping?mapping_count!=count:(mapping_count||count>WF_POST_ATLAS_STATIC_MAX)))return -1;
 int at=wf_post_atlas_find(fixed,count,code);
 if(at<0)return -1;
 if(!mapping)return at;
 return mapping[at]<WF_POST_ATLAS_STATIC_MAX?(int)mapping[at]:-1;
}
/* Load-time only. Sorted supplemental IDs map to slots after the untouched
 * static atlas. Failed plans invalidate the previous supplement. */
static inline bool wf_post_atlas_plan_mapped(wf_post_atlas_t *p,const uint16_t *fixed,
 unsigned fixed_count,const uint16_t *mapping,unsigned mapping_count,
 const wf_video_piece_t *pieces,unsigned count){
 if(!p)return false;
 memset(p,0,sizeof *p);
 if(!wf_post_atlas_mapping_valid(fixed,fixed_count,mapping,mapping_count)||
    count>1024u||(!pieces&&count))return false;
 p->fixed=fixed;p->fixed_count=fixed_count;
 for(unsigned i=0;i<count;i++){
  unsigned code=pieces[i].code;
  if(wf_post_atlas_find(fixed,fixed_count,code)>=0)continue;
  unsigned at=0;while(at<p->count&&p->ids[at]<code)at++;
  if(at<p->count&&p->ids[at]==code)continue;
  if(p->count==WF_POST_ATLAS_EXTRA_MAX){p->count=0;return false;}
  memmove(p->ids+at+1u,p->ids+at,(p->count-at)*sizeof *p->ids);
  p->ids[at]=(uint16_t)code;p->count++;
 }
 return true;
}
static inline bool wf_post_atlas_plan(wf_post_atlas_t *p,const uint16_t *fixed,
 unsigned fixed_count,const wf_video_piece_t *pieces,unsigned count){
 return wf_post_atlas_plan_mapped(p,fixed,fixed_count,NULL,0,pieces,count);
}
static inline int wf_post_atlas_slot_mapped(const wf_post_atlas_t *p,unsigned code,
 const uint16_t *mapping,unsigned mapping_count){
 if(!p||!p->fixed||!p->fixed_count||p->fixed_count>WF_POST_ATLAS_LOGICAL_MAX||
    p->count>WF_POST_ATLAS_EXTRA_MAX||
    (mapping?mapping_count!=p->fixed_count:(mapping_count||p->fixed_count>WF_POST_ATLAS_STATIC_MAX)))return -1;
 int slot=wf_post_atlas_fixed_slot(p->fixed,p->fixed_count,code,mapping,mapping_count);
 if(slot>=0)return slot;
 if(!p->ready)return -1;
 slot=wf_post_atlas_find(p->ids,p->count,code);
 unsigned base=mapping?WF_POST_ATLAS_STATIC_MAX:p->fixed_count;
 return slot<0?-1:(int)base+slot;
}
static inline int wf_post_atlas_slot(const wf_post_atlas_t *p,unsigned code){
 return wf_post_atlas_slot_mapped(p,code,NULL,0);
}
/* CRC catalog is a tail of POST.TEX, not another CD directory entry. Verify
 * it completely while retaining only requested CRCs; then validate each
 * packed tile before upload. Failure leaves partial VRAM hidden/unusable. */
static inline wf_asset_error_t wf_post_atlas_load_mapped(wf_post_atlas_t *p,const wf_asset_io_t *io,
 const char *catalog,uint32_t crc_offset,uint32_t expected_crc,uint8_t *scratch,size_t scratch_bytes,
 wf_post_atlas_upload_fn upload,void *user,const uint16_t *mapping,unsigned mapping_count){
 if(!p)return WF_ASSET_ARGUMENT;
 p->ready=false;
 if(!wf_post_atlas_mapping_valid(p->fixed,p->fixed_count,mapping,mapping_count)||p->count>WF_POST_ATLAS_EXTRA_MAX||
    !io||!io->size||!io->read||!catalog||!scratch||scratch_bytes<128u||(scratch_bytes&3u)||!upload||
    crc_offset>UINT32_MAX-WF_POST_ATLAS_CRC_BYTES)return WF_ASSET_ARGUMENT;
 if(mapping)for(unsigned i=0;i<p->count;i++)
  if((i&&p->ids[i]<=p->ids[i-1])||wf_post_atlas_find(p->fixed,p->fixed_count,p->ids[i])>=0)return WF_ASSET_ARGUMENT;
 if(!p->count){p->ready=true;return WF_ASSET_OK;}
 uint32_t bytes;
 if(io->size(io->user,catalog,&bytes)||bytes!=crc_offset+WF_POST_ATLAS_CRC_BYTES||
    io->size(io->user,"SPRITES.PAK",&bytes)||bytes!=WF_POST_ATLAS_PACK_BYTES)return WF_ASSET_SIZE;
 uint32_t crc=0xffffffffu;unsigned selected=0;
 for(unsigned at=0;at<WF_POST_ATLAS_CRC_BYTES;){
  size_t n=WF_POST_ATLAS_CRC_BYTES-at;if(n>scratch_bytes)n=scratch_bytes;
  if(io->read(io->user,catalog,crc_offset+at,scratch,n))return WF_ASSET_READ;
  crc=wf_assets_crc32_update(crc,scratch,n);
  while(selected<p->count&&(unsigned)p->ids[selected]*4u<at+n){
   unsigned off=(unsigned)p->ids[selected]*4u-at;
   p->crc[selected]=((uint32_t)scratch[off]<<24)|((uint32_t)scratch[off+1]<<16)|
                    ((uint32_t)scratch[off+2]<<8)|scratch[off+3];selected++;
  }
  at+=(unsigned)n;
 }
 if((crc^0xffffffffu)!=expected_crc||selected!=p->count)return WF_ASSET_CHECKSUM;
 for(unsigned i=0;i<p->count;i++){
  if(io->read(io->user,"SPRITES.PAK",(uint32_t)p->ids[i]*128u,scratch,128u))return WF_ASSET_READ;
  if(wf_assets_crc32(scratch,128u)!=p->crc[i])return WF_ASSET_CHECKSUM;
  unsigned base=mapping?WF_POST_ATLAS_STATIC_MAX:p->fixed_count;
  if(!upload(user,base+i,scratch))return WF_ASSET_READ;
 }
 p->ready=true;return WF_ASSET_OK;
}
static inline wf_asset_error_t wf_post_atlas_load(wf_post_atlas_t *p,const wf_asset_io_t *io,
 const char *catalog,uint32_t crc_offset,uint32_t expected_crc,uint8_t *scratch,size_t scratch_bytes,
 wf_post_atlas_upload_fn upload,void *user){
 return wf_post_atlas_load_mapped(p,io,catalog,crc_offset,expected_crc,scratch,scratch_bytes,upload,user,NULL,0);
}
#endif
