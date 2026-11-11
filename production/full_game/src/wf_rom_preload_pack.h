#ifndef WF_ROM_PRELOAD_PACK_H
#define WF_ROM_PRELOAD_PACK_H
#include "wf_assets.h"
#include "wf_load_window.h"

/* Startup only: borrow an already-open R-file window, never its own buffer.
 * Zero-initialize the descriptor. Keep the original P bytes immutable and
 * disjoint from descriptor/window/scratch/destinations until finish/close.
 * The caller retains the SAME preload APIs and activates routing only around
 * them. An error revokes the window; it must never fall back to another ROM.
 * Finish (all pages AND payload CRC) must succeed before publishing preload.
 * Source errors may leave partial destinations; no gameplay may use them.
 * Header, BE: R18K,u8 version/shift,u16 key/main/fast,u32 payload/P-CRC/
 * payload-CRC/source-ROM-CRC/header-CRC. Payload starts at sector 1 (2048).
 * profile_key=(royal<<12)|mask. Source SHA is verified by the offline tool. */
enum {WF_ROM_PRELOAD_PACK_ARGUMENT=-941,WF_ROM_PRELOAD_PACK_STATE=-942,
 WF_ROM_PRELOAD_PACK_FORMAT=-943,WF_ROM_PRELOAD_PACK_IDENTITY=-944,
 WF_ROM_PRELOAD_PACK_CRC=-945,WF_ROM_PRELOAD_PACK_SEQUENCE=-946};
typedef struct {
 wf_load_window_t *window;
 const uint8_t *profile;
 uint32_t crc,expected_crc;
 int32_t error;
 uint16_t main_count,total_count,next;
} wf_rom_preload_pack_t;
static inline unsigned wf_rom_preload_pack_u16(const uint8_t *p){return ((unsigned)p[0]<<8)|p[1];}
static inline uint32_t wf_rom_preload_pack_u32(const uint8_t *p){
 return ((uint32_t)wf_rom_preload_pack_u16(p)<<16)|wf_rom_preload_pack_u16(p+2);
}
static inline int wf_rom_preload_pack_active(const wf_rom_preload_pack_t *p){return p&&p->window!=NULL;}
static inline void wf_rom_preload_pack_close(wf_rom_preload_pack_t *p){
 if(!p)return;
 if(p->window)wf_load_window_close(p->window);
 int32_t error=p->error;memset(p,0,sizeof *p);p->error=error;
}
static inline int wf_rom_preload_pack_fail(wf_rom_preload_pack_t *p,int error){
 if(!p)return error;
 if(!p->error)p->error=error;
 wf_rom_preload_pack_close(p);return p->error;
}
static inline int wf_rom_preload_pack_open(wf_rom_preload_pack_t *p,wf_load_window_t *w,
 unsigned key,const void *profile,size_t bytes,unsigned main_capacity,unsigned fast_capacity){
 if(!p||!wf_load_window_span(p,sizeof *p))return WF_ROM_PRELOAD_PACK_ARGUMENT;
 if(p->window)return wf_rom_preload_pack_fail(p,WF_ROM_PRELOAD_PACK_STATE);
 memset(p,0,sizeof *p);
 if(!w||!wf_load_window_span(w,sizeof *w))return wf_rom_preload_pack_fail(p,WF_ROM_PRELOAD_PACK_ARGUMENT);
 p->window=w;
 if(!w->active||w->busy)return wf_rom_preload_pack_fail(p,WF_ROM_PRELOAD_PACK_STATE);
 if(!profile||!wf_load_window_span(profile,bytes)||!(key&0xfffu)||(key&~0x1fffu)||
    wf_load_window_overlap(p,sizeof *p,w,sizeof *w)||
    wf_load_window_overlap(p,sizeof *p,w->buffer,WF_LOAD_WINDOW_BYTES)||
    wf_load_window_overlap(profile,bytes,p,sizeof *p)||
    wf_load_window_overlap(profile,bytes,w,sizeof *w)||
    wf_load_window_overlap(profile,bytes,w->buffer,WF_LOAD_WINDOW_BYTES))
  return wf_rom_preload_pack_fail(p,WF_ROM_PRELOAD_PACK_ARGUMENT);
 const uint8_t *q=profile;
 if(bytes<8||memcmp(q,"R18Q",4))return wf_rom_preload_pack_fail(p,WF_ROM_PRELOAD_PACK_FORMAT);
 unsigned main=wf_rom_preload_pack_u16(q+4),fast=wf_rom_preload_pack_u16(q+6),total=main+fast;
 if(!main||main>256||main>main_capacity||fast>fast_capacity||total>264||bytes!=8u+total*2u)
  return wf_rom_preload_pack_fail(p,WF_ROM_PRELOAD_PACK_FORMAT);
 unsigned j=0;
 for(unsigned i=0;i<total;i++){
  unsigned page=wf_rom_preload_pack_u16(q+8+i*2);
  if(page>=512||(i&&i!=main&&page<=wf_rom_preload_pack_u16(q+6+i*2)))
   return wf_rom_preload_pack_fail(p,WF_ROM_PRELOAD_PACK_FORMAT);
  if(i>=main){
   while(j<main&&wf_rom_preload_pack_u16(q+8+j*2)<page)++j;
   if(j<main&&wf_rom_preload_pack_u16(q+8+j*2)==page)
    return wf_rom_preload_pack_fail(p,WF_ROM_PRELOAD_PACK_FORMAT);
  }
 }
 if(w->file_bytes!=2048u+total*1024u)return wf_rom_preload_pack_fail(p,WF_ROM_PRELOAD_PACK_FORMAT);
 p->profile=q;p->main_count=(uint16_t)main;p->total_count=(uint16_t)total;p->crc=0xffffffffu;
 uint8_t header[32];int rc=wf_load_window_read(w,w->name,0,header,sizeof header);
 if(rc)return wf_rom_preload_pack_fail(p,rc);
 if(wf_assets_crc32(header,28)!=wf_rom_preload_pack_u32(header+28))return wf_rom_preload_pack_fail(p,WF_ROM_PRELOAD_PACK_CRC);
 if(memcmp(header,"R18K",4)||header[4]!=1||header[5]!=10||wf_rom_preload_pack_u32(header+12)!=total*1024u)
  return wf_rom_preload_pack_fail(p,WF_ROM_PRELOAD_PACK_FORMAT);
 if(wf_rom_preload_pack_u16(header+6)!=key||wf_rom_preload_pack_u16(header+8)!=main||
    wf_rom_preload_pack_u16(header+10)!=fast||wf_rom_preload_pack_u32(header+16)!=wf_assets_crc32(q,bytes)||
    wf_rom_preload_pack_u32(header+24)!=0x3619de1eu)return wf_rom_preload_pack_fail(p,WF_ROM_PRELOAD_PACK_IDENTITY);
 p->expected_crc=wf_rom_preload_pack_u32(header+20);return 0;
}
static inline int wf_rom_preload_pack_read(wf_rom_preload_pack_t *p,uint32_t offset,void *out,size_t bytes){
 if(!p||!p->window)return wf_rom_preload_pack_fail(p,WF_ROM_PRELOAD_PACK_STATE);
 unsigned next=p->next,limit=next<p->main_count?p->main_count:p->total_count;
 if(!bytes||(bytes&1023u)||(offset&1023u)||offset>=512u*1024u||bytes/1024u>limit-next)
  return wf_rom_preload_pack_fail(p,WF_ROM_PRELOAD_PACK_SEQUENCE);
 unsigned count=(unsigned)(bytes/1024u),page=offset/1024u;
 for(unsigned i=0;i<count;i++)if(wf_rom_preload_pack_u16(p->profile+8+(next+i)*2)!=page+i)
  return wf_rom_preload_pack_fail(p,WF_ROM_PRELOAD_PACK_SEQUENCE);
 wf_load_window_t *w=p->window;
 if(!out||!wf_load_window_span(out,bytes)||wf_load_window_overlap(out,bytes,p,sizeof *p)||
    wf_load_window_overlap(out,bytes,p->profile,8u+p->total_count*2u)||
    wf_load_window_overlap(out,bytes,w,sizeof *w)||wf_load_window_overlap(out,bytes,w->buffer,WF_LOAD_WINDOW_BYTES))
  return wf_rom_preload_pack_fail(p,WF_ROM_PRELOAD_PACK_ARGUMENT);
 int rc=wf_load_window_read(w,w->name,2048u+next*1024u,out,bytes);
 if(rc)return wf_rom_preload_pack_fail(p,rc);
 p->crc=wf_assets_crc32_update(p->crc,out,bytes);p->next=(uint16_t)(next+count);return 0;
}
static inline int wf_rom_preload_pack_finish(wf_rom_preload_pack_t *p){
 if(!p||!p->window)return wf_rom_preload_pack_fail(p,WF_ROM_PRELOAD_PACK_STATE);
 if(p->next!=p->total_count)return wf_rom_preload_pack_fail(p,WF_ROM_PRELOAD_PACK_SEQUENCE);
 if(~p->crc!=p->expected_crc)return wf_rom_preload_pack_fail(p,WF_ROM_PRELOAD_PACK_CRC);
 wf_rom_preload_pack_close(p);return 0;
}
#endif
