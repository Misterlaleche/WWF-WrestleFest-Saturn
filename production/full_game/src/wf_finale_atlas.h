#ifndef WF_FINALE_ATLAS_H
#define WF_FINALE_ATLAS_H
#include "wf_post_atlas.h"
#include "wf_r18_metasprite.h"

static inline uint16_t wf_finale_atlas_word(const uint8_t*p){return(uint16_t)((p[0]<<8)|p[1]);}
static inline uint32_t wf_finale_atlas_long(const uint8_t*p){return((uint32_t)wf_finale_atlas_word(p)<<16)|wf_finale_atlas_word(p+2);}
static inline bool wf_finale_atlas_overlap(const void*a,size_t an,const void*b,size_t bn){
 uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return x<=y?y-x<an:x-y<bn;
}
static inline bool wf_finale_atlas_add(wf_post_atlas_t*p,uint16_t code){
 if(wf_post_atlas_find(p->fixed,p->fixed_count,code)>=0)return true;
 unsigned at=0;while(at<p->count&&p->ids[at]<code)at++;
 if(at<p->count&&p->ids[at]==code)return true;
 if(p->count==WF_POST_ATLAS_EXTRA_MAX)return false;
 memmove(p->ids+at+1u,p->ids+at,(p->count-at)*sizeof *p->ids);
 p->ids[at]=code;p->count++;return true;
}
static inline bool wf_finale_atlas_decode(wf_post_atlas_t*p,const uint8_t*actor,
 uint16_t partner,const uint8_t*palette,wf_r18_read_rom_fn read_rom,void*user,
 wf_r18_sprite_record_t*scratch,unsigned capacity){
 wf_r18_metasprite_ctx_t decoder={.read_rom=read_rom,.user=user,.palette_map=palette,
  .records=scratch,.capacity=capacity};
 /* Resource coordinates, never actor writes: after the decoder's offsets,
  * x is16..271 and y80..335 for every signed-byte row, including mirrors.
  * Thus its screen clip retains every row. Chain tiles below are not clipped. */
 if(wf_r18_metasprite_emit(&decoder,wf_finale_atlas_word(actor+2),
    wf_finale_atlas_word(actor+4),128,64,partner)!=WF_OK)return false;
 for(size_t i=0;i<decoder.count;i++){
  const uint8_t*r=scratch[i].bytes;unsigned chain=(r[3]>>5)&7u;
  unsigned code=((unsigned)r[7]<<8)|r[5],mask=chain;
  mask|=mask>>1;mask|=mask>>2;code&=~mask;
  for(unsigned j=0;j<=chain;j++)if(!wf_finale_atlas_add(p,(uint16_t)(code+j)))return false;
 }
 return true;
}

/* Load-time resource union for original $1B4E, before its first portrait.
 * The caller supplies canonical family50 IDs and scratch records. No RAM,
 * original register, VRAM, CD or palette writes occur. The original $250E
 * scans all9 actors, although $1CAE initializes only the first6. Those last3
 * actors retain family/pose: $2208 moves, $247C projects and $27B8 queues.
 * $2836 may clear their partner link; cover that future variant as well.
 * The temporary decoder is not a scene callback or a replacement game state.
 * A successful plan is still unready until wf_post_atlas_load verifies it.
 * Failed plans are invalidated; the caller must not continue after false. */
static inline bool wf_finale_atlas_plan_mapped(wf_post_atlas_t*p,const uint16_t*fixed,
 unsigned fixed_count,const uint16_t*mapping,unsigned mapping_count,
 const uint16_t*finale_ids,unsigned finale_count,
 const uint8_t*ram,size_t ram_bytes,wf_r18_read_rom_fn read_rom,void*user,
 wf_r18_sprite_record_t*scratch,unsigned capacity){
 if(!p)return false;
 if(ram&&wf_finale_atlas_overlap(p,sizeof *p,ram,ram_bytes))return false;
 if(!wf_post_atlas_plan_mapped(p,fixed,fixed_count,mapping,mapping_count,NULL,0))return false;
 if(!ram||ram_bytes<0x4000u||!read_rom||(!finale_ids&&finale_count)||
    !scratch||!capacity||capacity>512u||
    wf_finale_atlas_overlap(scratch,capacity*sizeof *scratch,ram,ram_bytes))goto failed;
 for(unsigned i=0;i<finale_count;i++)if(!wf_finale_atlas_add(p,finale_ids[i]))goto failed;
 uint32_t pointers[9];uint8_t value[4];
 for(unsigned i=0;i<9;i++){
  if(read_rom(user,0x5122u+i*4u,value,4)!=WF_OK)goto failed;
  pointers[i]=wf_finale_atlas_long(value)&0xffffffu;
  if(pointers[i]<0x1c0000u||pointers[i]>0x1c3f00u)goto failed;
 }
 for(unsigned i=6;i<9;i++){
  const uint8_t*a=ram+pointers[i]-0x1c0000u;
  if(!(a[0]&0x80u)||(wf_finale_atlas_word(a+4)&0x7fffu)==0x7fffu||a[0x21]==0xffu)continue;
  uint32_t link=wf_finale_atlas_long(a+0x26)&0xffffffu;uint16_t partner=0xffffu;
  if(link>=0x1c0000u&&link<=0x1c3ffcu){
   partner=wf_finale_atlas_word(ram+link-0x1c0000u+2u);
   for(unsigned j=0;j<6;j++)if(link==pointers[j]){partner=0x50u;break;}
  }
  if(!wf_finale_atlas_decode(p,a,partner,ram+0x1610,read_rom,user,scratch,capacity))goto failed;
  if((a[0x26]&0x80u)&&!wf_finale_atlas_word(a+0xfe)&&partner!=0xffffu&&
     !wf_finale_atlas_decode(p,a,0xffffu,ram+0x1610,read_rom,user,scratch,capacity))goto failed;
 }
 return true;
failed:
 memset(p,0,sizeof *p);return false;
}
static inline bool wf_finale_atlas_plan(wf_post_atlas_t*p,const uint16_t*fixed,
 unsigned fixed_count,const uint16_t*finale_ids,unsigned finale_count,
 const uint8_t*ram,size_t ram_bytes,wf_r18_read_rom_fn read_rom,void*user,
 wf_r18_sprite_record_t*scratch,unsigned capacity){
 return wf_finale_atlas_plan_mapped(p,fixed,fixed_count,NULL,0,finale_ids,finale_count,
  ram,ram_bytes,read_rom,user,scratch,capacity);
}
#endif
