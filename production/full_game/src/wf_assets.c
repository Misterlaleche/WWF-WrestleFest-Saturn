#include "wf_assets.h"
#include <string.h>
uint32_t wf_assets_crc32(const void*data,size_t size){
 const uint8_t*p=data;uint32_t crc=0xffffffffu;
 while(size--){crc^=*p++;for(unsigned b=0;b<8;b++)crc=(crc>>1)^((0u-(crc&1u))&0xedb88320u);}
 return ~crc;
}
bool wf_assets_load(const wf_asset_t*m,unsigned count,void*storage,uint32_t capacity,void**out,const wf_asset_io_t*io,wf_asset_result_t*r){
 uint32_t offsets[64],used=0;
 if(!r)return false;
 memset(r,0,sizeof *r);r->error=WF_ASSET_ARGUMENT;
 if(!out||!count||count>64)return false;
 for(unsigned i=0;i<count;i++)out[i]=NULL;
 if(!m||!storage||((uintptr_t)storage&31u)||!io||!io->size||!io->read)return false;
 for(unsigned i=0;i<count;i++){
  uint32_t actual=0,pad=(0u-used)&31u;r->index=i;
  if(!m[i].name||!m[i].bytes)return false;
  if(pad>capacity-used||m[i].bytes>capacity-used-pad){r->error=WF_ASSET_CAPACITY;return false;}
  used+=pad;offsets[i]=used;used+=m[i].bytes;
  if(io->size(io->user,m[i].name,&actual)||actual!=m[i].bytes){r->error=WF_ASSET_SIZE;return false;}
 }
 for(unsigned i=0;i<count;i++){
  uint8_t*p=(uint8_t*)storage+offsets[i];r->index=i;
  if(io->read(io->user,m[i].name,0,p,m[i].bytes)){r->error=WF_ASSET_READ;return false;}
  if(wf_assets_crc32(p,m[i].bytes)!=m[i].crc32){r->error=WF_ASSET_CHECKSUM;return false;}
 }
 for(unsigned i=0;i<count;i++)out[i]=(uint8_t*)storage+offsets[i];
 r->used=used;r->error=WF_ASSET_OK;return true;
}
