#include "wf_music_elf.h"
static uint16_t be16(const uint8_t*p){return (uint16_t)(p[0]*256u+p[1]);}
static uint32_t be32(const uint8_t*p){return (uint32_t)be16(p)*65536u+be16(p+2);}
bool wf_music_elf_image(const void *data,size_t bytes,wf_music_image_t *m)
{
 const uint8_t*p=data;uint32_t table;
 if(!p||!m||bytes<52||be32(p)!=0x7f454c46u||p[4]!=1||p[5]!=2||p[6]!=1||
    be16(p+16)!=2||be16(p+18)!=42||be32(p+36)!=2||be16(p+40)!=52||be16(p+42)!=32||be16(p+44)!=3)return false;
 table=be32(p+28);if(table>bytes||96>bytes-table)return false;
 for(unsigned i=0;i<3;i++){
  const uint8_t*h=p+table+i*32u;uint32_t off=be32(h+4),address=be32(h+8),file=be32(h+16),mem=be32(h+20);
  if(be32(h)!=1||address!=be32(h+12)||off>bytes||file>bytes-off||file>mem||((address|file|mem)&3u)||!mem)return false;
  if(i==0){if(address!=0x060da000u||file!=mem||file<32||mem>0x20000u||be32(h+24)!=5)return false;}
  else if(i==1){if(address!=0x060fa000u||file||mem>0x3000u||be32(h+24)!=6)return false;}
  else if(address!=0x060fd000u||file||mem!=4096||be32(h+24)!=6)return false;
  m->segments[i]=(wf_native_segment_t){address,file?p+off:NULL,file,mem};
 }
 const uint8_t*h=m->segments[0].data_be;uint32_t end=0x060da000u+m->segments[0].file_bytes;
 if(be32(h)!=WF_MUSIC_MODULE_MAGIC||be16(h+4)!=WF_MUSIC_MODULE_ABI||be16(h+6)!=32||be32(h+8)!=WF_MUSIC_MODULE_BASE||
    be32(h+12)!=WF_MUSIC_MODULE_END||be32(h+24)!=WF_MUSIC_MODULE_SHARED||be32(h+28))return false;
 m->prepare=be32(h+16);m->run=be32(h+20);
 return m->prepare>=WF_MUSIC_MODULE_BASE+32u&&m->prepare<end&&m->run>=WF_MUSIC_MODULE_BASE+32u&&m->run<end&&
        !((m->prepare|m->run)&1u)&&be32(p+24)==m->prepare;
}
bool wf_music_elf_copy(const wf_music_image_t *m,void (*write16)(uint32_t,uint16_t,void *),void *u)
{
 if(!m||!write16)return false;
 for(unsigned i=0;i<3;i++){
  const wf_native_segment_t*s=&m->segments[i];
  /* Validated object only; repeated checks protect callers constructing one. */
  uint32_t base=i==0?0x060da000u:(i==1?0x060fa000u:0x060fd000u),capacity=i==0?0x20000u:(i==1?0x3000u:4096u);
  if(s->address!=base||s->memory_bytes>capacity||s->file_bytes>s->memory_bytes||((s->file_bytes|s->memory_bytes)&1u)||
     (s->file_bytes&&!s->data_be))return false;
 }
 for(unsigned i=0;i<3;i++){
  const wf_native_segment_t*s=&m->segments[i];
  for(uint32_t n=0;n<s->memory_bytes;n+=2)write16(s->address+0x20000000u+n,n<s->file_bytes?be16(s->data_be+n):0,u);
 }
 return true;
}
