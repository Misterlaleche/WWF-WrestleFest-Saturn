#include "wf_runtime.h"
void wf_for_each_active(wf_runtime_t *rt, void (*fn)(wf_runtime_t*,unsigned,wf_actor_raw_t*,void*), void *user){
 for(unsigned i=0;i<WF_ACTOR_SLOTS;i++) if((wf_u8(&rt->actors[i],WF_A_ACTIVE)&0x80u)!=0) fn(rt,i,&rt->actors[i],user);
}
wf_actor_raw_t *wf_actor_from_rom_ptr(wf_runtime_t *rt,uint32_t ptr,unsigned *slot_out){
 /* The 68000 exposes a 24-bit address bus; +$26.bit7 also tags links. */
 ptr &= 0x00ffffffu;
 if(!rt || ptr < WF_ARCADE_ACTOR_BASE0) return 0;
 uint32_t d=ptr-WF_ARCADE_ACTOR_BASE0;
 if((d%WF_ARCADE_ACTOR_STRIDE)!=0u) return 0;
 unsigned slot=(unsigned)(d/WF_ARCADE_ACTOR_STRIDE);
 if(slot>=WF_ACTOR_SLOTS) return 0;
 if(slot_out) *slot_out=slot;
 return &rt->actors[slot];
}
const wf_actor_raw_t *wf_actor_from_rom_ptr_const(const wf_runtime_t *rt,uint32_t ptr,unsigned *slot_out){
 return wf_actor_from_rom_ptr((wf_runtime_t*)rt,ptr,slot_out);
}

/* $115D2 uses the 68000's 24-bit bus. A tagged null link reads the
 * canonical ROM reset-vector bytes $26..$29 = FFFFFFFF, never an actor. */
wf_status_t wf_actor_unlink_115d2(wf_runtime_t *rt,wf_actor_raw_t *a){
 if(!rt||!a)return WF_BAD_STATE;
 uint32_t link=wf_be32(a,WF_A_LINK);if(wf_be16(a,0xfeu)||!link)return WF_OK;
 wf_actor_raw_t *other=wf_actor_from_rom_ptr(rt,link,NULL);
 uint32_t back;
 if(other)back=wf_be32(other,WF_A_LINK);
 else if(!(link&0x00ffffffu))back=0xffffffffu;
 else return WF_BLOCKED;
 if(back!=wf_actor_rom_base((unsigned)(a-rt->actors))){wf_set_be32(a,WF_A_LINK,0u);wf_set_be32(a,WF_A_SECONDARY_LINK,0u);}
 return WF_OK;
}
