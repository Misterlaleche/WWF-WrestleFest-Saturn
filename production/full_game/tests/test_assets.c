#include "wf_assets.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
/* Boundary adapter tested against exact data and failures, not mock calls. */
static unsigned fail_size,fail_read,corrupt;
static int size_of(void*u,const char*n,uint32_t*b){(void)u;(void)n;if(fail_size)return -1;*b=9;return 0;}
static int read_data(void*u,const char*n,uint32_t offset,void*d,size_t bytes){(void)u;(void)n;assert(offset==0&&bytes==9);if(fail_read)return -1;memcpy(d,"123456789",9);if(corrupt)((uint8_t*)d)[8]='0';return 0;}
int main(void){
 const wf_asset_t manifest[]={{"ONE.BIN",9,0xcbf43926u},{"TWO.BIN",9,0xcbf43926u}};
 const wf_asset_io_t io={size_of,read_data,0};
 _Alignas(32) uint8_t storage[96];void*assets[2];wf_asset_result_t r;
 memset(storage,0xa5,sizeof storage);
 assert(wf_assets_load(manifest,2,storage,41,assets,&io,&r));
 assert(assets[0]==storage&&assets[1]==storage+32&&r.used==41);
 assert(!memcmp(assets[1],"123456789",9)&&storage[41]==0xa5);
 assert(!wf_assets_load(manifest,2,storage,40,assets,&io,&r));
 assert(!assets[0]&&!assets[1]&&r.error==WF_ASSET_CAPACITY);
 assert(!wf_assets_load(manifest,2,storage+1,80,assets,&io,&r));
 assert(r.error==WF_ASSET_ARGUMENT);
 corrupt=1;assert(!wf_assets_load(manifest,2,storage,96,assets,&io,&r));
 assert(r.error==WF_ASSET_CHECKSUM&&!assets[0]&&!assets[1]);corrupt=0;
 fail_size=1;assert(!wf_assets_load(manifest,2,storage,96,assets,&io,&r));assert(r.error==WF_ASSET_SIZE);fail_size=0;
 fail_read=1;assert(!wf_assets_load(manifest,2,storage,96,assets,&io,&r));assert(r.error==WF_ASSET_READ);fail_read=0;
 assert(wf_assets_crc32("123456789",9)==0xcbf43926u);
 puts("assets: exact pack,alignment,capacity,CRC,read/size failures,no partial publication PASS");
}
