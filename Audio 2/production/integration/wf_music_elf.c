#include "wf_music_elf.h"
static uint16_t be16(const uint8_t *p){return (uint16_t)(p[0]*256u+p[1]);}
static uint32_t be32(const uint8_t *p){return (uint32_t)be16(p)*65536u+be16(p+2);}
static bool owned_window(uint32_t base,uint32_t bytes)
{
#if defined(WF_AUDIO2_WORKSPACE_VDP1) && WF_AUDIO2_WORKSPACE_VDP1
 return bytes==WF_MUSIC_HOT_WINDOW_BYTES && base==0x05c78020u;
#else
 return bytes==WF_MUSIC_HOT_WINDOW_BYTES && !(base&3u) &&
        base>=0x06000000u && base<=0x06100000u-WF_MUSIC_HOT_WINDOW_BYTES;
#endif
}
bool wf_music_elf_image(const void *data,size_t bytes,uint32_t hot_base,
                        uint32_t hot_bytes,wf_music_image_t *out)
{
 const uint8_t *p=data;
 wf_music_image_t m={0};
 uint32_t offsets[4],table;
 if(!p||!out||!owned_window(hot_base,hot_bytes)||bytes<52||
    be32(p)!=0x7f454c46u||p[4]!=1||p[5]!=2||p[6]!=1||
    be16(p+16)!=2||be16(p+18)!=42||be32(p+20)!=1||be32(p+36)!=2||
    be16(p+40)!=52||be16(p+42)!=32||be16(p+44)!=4)return false;
 table=be32(p+28);
 if(table<52||table>bytes||128>bytes-table)return false;
 for(unsigned i=0;i<4;i++){
  const uint8_t *h=p+table+i*32u;
  uint32_t off=be32(h+4),address=be32(h+8),file=be32(h+16);
  uint32_t mem=be32(h+20),flags=be32(h+24),align=be32(h+28);
  if(be32(h)!=1||address!=be32(h+12)||off>bytes||file>bytes-off||
     file>mem||((address|off|file|mem)&3u)||!mem||
     (align>1&&((align&(align-1u))||((address^off)&(align-1u)))))return false;
  if(file&&(size_t)off<(size_t)table+128u)return false;
  if(i==0){
   if(address!=WF_MUSIC_MODULE_BASE||file!=mem||file<32||
      mem>WF_MUSIC_MODULE_STATE-WF_MUSIC_MODULE_BASE||flags!=5)return false;
  }else if(i==1){
   if(address!=WF_MUSIC_MODULE_STATE||file||
      mem>WF_MUSIC_MODULE_SHARED_ADDRESS-WF_MUSIC_MODULE_STATE||flags!=6)return false;
  }else if(i==2){
   if(address!=WF_MUSIC_MODULE_SHARED_ADDRESS||file||mem!=4096||flags!=6)return false;
  }else if(address!=hot_base||mem>hot_bytes||!file||flags!=7)return false;
  for(unsigned j=0;j<i;j++){
   if(file&&m.segments[j].file_bytes&&(size_t)off<(size_t)offsets[j]+m.segments[j].file_bytes&&
      (size_t)offsets[j]<(size_t)off+file)return false;
  }
  offsets[i]=off;
  m.segments[i]=(wf_native_segment_t){address,file?p+off:NULL,file,mem};
 }
 const uint8_t *h=m.segments[0].data_be;
 uint32_t end=WF_MUSIC_MODULE_BASE+m.segments[0].file_bytes;
 if(be32(h)!=WF_MUSIC_MODULE_MAGIC||be16(h+4)!=WF_MUSIC_MODULE_ABI||
    be16(h+6)!=32||be32(h+8)!=WF_MUSIC_MODULE_BASE||
    be32(h+12)!=WF_MUSIC_MODULE_END||be32(h+24)!=WF_MUSIC_MODULE_SHARED||
    be32(h+28)!=hot_base)return false;
 m.prepare=be32(h+16);m.run=be32(h+20);
 if(m.prepare<WF_MUSIC_MODULE_BASE+32u||m.prepare>=end||
    m.run<WF_MUSIC_MODULE_BASE+32u||m.run>=end||
    ((m.prepare|m.run)&1u)||be32(p+24)!=m.prepare)return false;
 m.elf_data=data;m.elf_bytes=bytes;
 *out=m;
 return true;
}
bool wf_music_elf_copy(const wf_music_image_t *m,uint32_t hot_base,
                       uint32_t hot_bytes,void (*write16)(uint32_t,uint16_t,void *),void *u)
{
 wf_music_image_t checked;
 if(!m||!write16||!wf_music_elf_image(m->elf_data,m->elf_bytes,hot_base,hot_bytes,&checked)||
    m->prepare!=checked.prepare||m->run!=checked.run)return false;
 /* Revalidate all four loads before the first write, including a changed file. */
 for(unsigned i=0;i<4;i++){
  const wf_native_segment_t *a=&m->segments[i],*b=&checked.segments[i];
  if(a->address!=b->address||a->data_be!=b->data_be||
     a->file_bytes!=b->file_bytes||a->memory_bytes!=b->memory_bytes)return false;
 }
 for(unsigned i=0;i<4;i++){
  const wf_native_segment_t *s=&checked.segments[i];
  for(uint32_t n=0;n<s->memory_bytes;n+=2)
   write16((s->address|0x20000000u)+n,n<s->file_bytes?be16(s->data_be+n):0,u);
 }
 return true;
}
