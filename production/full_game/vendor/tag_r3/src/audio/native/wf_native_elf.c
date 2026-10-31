#include "wf_native_elf.h"
static uint32_t word32(const uint8_t *p)
{
    return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3];
}
static uint16_t word16(const uint8_t *p){return (uint16_t)((p[0]<<8)|p[1]);}
bool wf_native_elf_image(const void *data,size_t bytes,wf_native_segment_t s[3],
                         wf_native_image_t *image)
{
    const uint8_t *p=data;
    uint32_t table;
    if(!p||!s||!image||bytes<52||p[0]!=127||p[1]!='E'||p[2]!='L'||p[3]!='F'||
       p[4]!=1||p[5]!=2||p[6]!=1||word16(p+16)!=2||word16(p+18)!=4||
       word32(p+36)!=0x1000000u||word16(p+40)!=52||
       word16(p+42)!=32||word16(p+44)!=3)return false;
    table=word32(p+28);
    if(table>bytes||96u>bytes-table)return false;
    for(unsigned i=0;i<3u;i++){
        const uint8_t *h=p+table+i*32u;
        uint32_t off=word32(h+4),n=word32(h+16);
        if(word32(h)!=1||off>bytes||n>bytes-off||word32(h+8)!=word32(h+12))
            return false;
        s[i].address=word32(h+8);s[i].file_bytes=n;
        s[i].memory_bytes=word32(h+20);s[i].data_be=n?p+off:NULL;
    }
    image->segments=s;image->count=3;
    return wf_native_image_valid(image)&&s[0].address==0&&
           word32(p+24)==word32(s[0].data_be+4);
}
