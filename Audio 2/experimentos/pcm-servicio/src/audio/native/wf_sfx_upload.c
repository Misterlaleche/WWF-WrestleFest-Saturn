#include "wf_sfx_upload.h"
static uint32_t be32(const uint8_t *p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
bool wf_sfx_upload_bulk(wf_native_host_t *h,uint32_t bytes,wf_sfx_read_fn read,
                        void *user,uint8_t *block,size_t capacity)
{
    uint8_t header[32];uint32_t at,crc=0xffffffffu,table[256];bool ok=false;
    if(!h||!h->ready||h->poisoned||!h->mail||!read||!block||capacity<2u||(capacity&1u)||
       bytes<32||bytes>0x6c000u||(bytes&1u)||
       (h->mail->reserved[2]!=WF_NATIVE_SFX_PROFILE&&h->mail->reserved[2]!=0x4d46u)||!h->ops.write16||!h->ops.enter_owner||
       !h->ops.leave_owner||!h->ops.barrier||!h->ops.now_us||!h->ops.idle)return false;
    for(unsigned i=0;i<256u;i++){
        uint32_t c=i;
        for(unsigned bit=0;bit<8u;bit++)c=(c>>1)^((0u-(c&1u))&0xedb88320u);
        table[i]=c;
    }
    if(!read(0,header,32,user)||be32(header)!=0x57465358u||be32(header+4)!=0x00010074u||
       be32(header+8)!=bytes||be32(header+12)!=32||be32(header+16)!=960||
       be32(header+20)!=1548||be32(header+24)<1548||be32(header+24)>bytes)return false;
    if(!wf_native_host_command(h,WF_NATIVE_SFX_STOP,0,NULL))return false;
    if(!h->ops.enter_owner(h->ops.user))return false;
    h->ops.write16(0x14000u,0,h->ops.user); /* invalid until every byte and CRC passes */
    h->ops.barrier(h->ops.user);
    for(at=32;at<bytes;){
        uint32_t n=bytes-at;
        if(n>capacity)n=(uint32_t)capacity;
        /* After the 32-byte header, end the first read at a sector boundary.
         * Following aligned full blocks use the CDFS direct DMA path. */
        if(at&2047u){uint32_t to_sector=2048u-(at&2047u);if(n>to_sector)n=to_sector;}
        if(!read(at,block,n,user))goto done;
        for(uint32_t i=0;i<n;i++)crc=(crc>>8)^table[(crc^block[i])&255u];
        for(uint32_t i=0;i<n;i+=2)h->ops.write16(0x14000u+at+i,(uint16_t)(block[i]*256u+block[i+1]),h->ops.user);
        at+=n;(void)h->ops.now_us(h->ops.user);
    }
    if((crc^0xffffffffu)!=be32(header+28))goto done;
    for(unsigned i=2;i<32;i+=2)h->ops.write16(0x14000u+i,(uint16_t)(header[i]*256u+header[i+1]),h->ops.user);
    h->ops.barrier(h->ops.user);
    h->ops.write16(0x14000u,(uint16_t)(header[0]*256u+header[1]),h->ops.user);
    h->ops.barrier(h->ops.user);ok=true;
done:
    h->ops.leave_owner(h->ops.user);
    return ok&&wf_native_host_command(h,WF_NATIVE_SFX_BIND,0,NULL);
}
bool wf_sfx_upload(wf_native_host_t *h,uint32_t bytes,wf_sfx_read_fn read,void *user)
{
    uint8_t block[1024];
    return wf_sfx_upload_bulk(h,bytes,read,user,block,sizeof block);
}
