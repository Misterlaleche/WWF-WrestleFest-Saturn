#include "wf_r17_participant_helpers.h"
#include <string.h>
#include "wf_r17_participant_tables.inc"
static wf_participant_workspace_t participant_workspace;
static unsigned participant_workspace_owner;
wf_status_t wf_participant_workspace_acquire(unsigned owner,wf_participant_workspace_t **out){
 if(!out||(owner!=WF_PARTICIPANT_TAG&&owner!=WF_PARTICIPANT_ROYAL)||participant_workspace_owner)return WF_BAD_STATE;
 participant_workspace_owner=owner;*out=&participant_workspace;return WF_OK;
}
wf_status_t wf_participant_workspace_release(unsigned owner,wf_participant_workspace_t *workspace){
 if(workspace!=&participant_workspace||!participant_workspace_owner||participant_workspace_owner!=owner)return WF_BAD_STATE;
 participant_workspace_owner=0u;return WF_OK;
}
void wf_participant_init(wf_participant_machine_t*m,wf_runtime_t*rt,wf_participant_read16_fn read16,void*user){
 memset(m,0,sizeof(*m));m->rt=rt;m->read16=read16;m->user=user;m->status=WF_OK;
 if(rt)memcpy(m->d,rt->m68k_regs.d,sizeof(m->d));else m->status=WF_BAD_STATE;
}
static uint8_t *actor_byte(wf_runtime_t*r,uint32_t a){
 if(a>=0x1c05b0u&&a<0x1c0f1cu){unsigned o=a-0x1c05b0u;return &r->actors[o/0x10cu].b[o%0x10cu];}
 return NULL;
}
static uint16_t *word_global(wf_runtime_t*r,uint32_t a){
 if(a>=0x1c1586u&&a<=0x1c1598u)return &r->g.royal_slots_1586[(a-0x1c1586u)/2];
 if(a>=0x1c16a8u&&a<=0x1c16beu)return &r->g.royal_occupancy[(a-0x1c16a8u)/2];
 switch(a){case 0x1c0066:return &r->g.config_0066;case 0x1c007c:return &r->g.credited_route;case 0x1c0162:return &r->g.progression_0162;case 0x1c15ce:return &r->g.global_15ce;case 0x1c169e:return &r->g.global_169e;default:return NULL;}
}
static uint8_t *byte_global(wf_runtime_t*r,uint32_t a){
 if(a>=0x1c1576u&&a<=0x1c15d1u)return &r->g.royal_selector_1576[a-0x1c1576u];
 switch(a){case 0x1c0160:return &r->g.mode_0160;case 0x1c0161:return &r->g.mode_0161;case 0x1c167b:return &r->g.global_167b;default:return NULL;}
}
static uint8_t read_byte(wf_participant_machine_t*m,uint32_t a){
 uint8_t*p=actor_byte(m->rt,a);uint16_t*w;if(p)return *p;
 w=word_global(m->rt,a&~1u);if(w)return (uint8_t)(*w>>((a&1u)?0:8));
 p=byte_global(m->rt,a);if(p)return *p;
 for(size_t i=0;i<sizeof(rom_tables)/sizeof(rom_tables[0]);i++)if(a>=rom_tables[i].address&&a<rom_tables[i].address+rom_tables[i].length)return rom_tables[i].data[a-rom_tables[i].address];
 m->status=WF_BAD_STATE;m->failed_address=a;return 0;
}
uint32_t wf_participant_read(wf_participant_machine_t*m,uint32_t a,unsigned width){
 uint32_t v=0;if(m->status!=WF_OK)return 0;
 if(a>=0x140020u&&a<=0x140026u&&width==2){uint16_t raw=0;if(!m->read16){m->status=WF_BLOCKED;m->failed_address=a;return 0;}m->status=m->read16(m->rt,a,&raw,m->user);if(m->status!=WF_OK)m->failed_address=a;return raw;}
 for(unsigned i=0;i<width;i++)v=(v<<8)|read_byte(m,a+i);
 return v;
}
void wf_participant_write(wf_participant_machine_t*m,uint32_t a,uint32_t v,unsigned width){
 if(m->status!=WF_OK)return;
 v&=width==4?0xffffffffu:width==2?65535u:255u;
 if((a>=0xc0000u&&a+width<=0xc4000u)||(a>=0x180000u&&a+width<=0x181000u)){if(m->write_count>=WF_PARTICIPANT_WRITES){m->status=WF_BAD_STATE;m->failed_address=a;return;}m->writes[m->write_count++]=(wf_r17_fg0_write_t){a,v,(uint8_t)width};return;}
 for(unsigned i=0;i<width;i++){
  uint32_t at=a+i;uint8_t b=(uint8_t)(v>>((width-1-i)*8));uint8_t*p=actor_byte(m->rt,at);uint16_t*w;
  if(p){*p=b;continue;}w=word_global(m->rt,at&~1u);if(w){if(at&1u)*w=(uint16_t)((*w&0xff00u)|b);else *w=(uint16_t)((*w&255u)|((uint16_t)b<<8));continue;}
  p=byte_global(m->rt,at);if(p){*p=b;continue;}m->status=WF_BAD_STATE;m->failed_address=at;return;
 }
}
