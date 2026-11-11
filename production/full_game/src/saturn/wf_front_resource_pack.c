#include "wf_front_resource_pack.h"
#include "../wf_front_resource_pack.h"
#include "wf_front_resource_pack_config.h"
#include "wf_cd.h"
#include "wf_workspace.h"
#include "wf_platform.h"
#if !defined(WF_FRONT_RESOURCE_PACK) || defined(WF_COMBAT_MODULE)
#error Frontend resource bundle must be linked only into the opt-in frontend
#endif
static wf_front_pack_t pack;
static bool attempted;
volatile uint32_t wf_front_pack_reads,wf_front_pack_sectors,wf_front_pack_decoded_bytes,
 wf_front_pack_decode_calls,wf_front_pack_cache_hits,wf_front_pack_partial_reads,
 wf_front_pack_read_fields,wf_front_pack_decode_fields,wf_front_pack_error;
static uint32_t clock_now(void*u){(void)u;return wf_fields;}
static void publish(int error){
 wf_front_pack_reads=pack.stats.reads;wf_front_pack_sectors=pack.stats.sectors;
 wf_front_pack_decoded_bytes=pack.stats.decoded_bytes;wf_front_pack_decode_calls=pack.stats.decode_calls;
 wf_front_pack_cache_hits=pack.stats.cache_hits;wf_front_pack_partial_reads=pack.stats.partial_reads;
 wf_front_pack_read_fields=pack.stats.read_fields;wf_front_pack_decode_fields=pack.stats.decode_fields;
 if(error)wf_front_pack_error=(uint32_t)-error;
}
static int ensure(void){
 if(attempted)return pack.ready?0:-2;
 attempted=true;
 int rc=wf_front_pack_open(&pack,&wf_cd_asset_io,wf_workspace.front.resource_input,
  sizeof wf_workspace.front.resource_input,wf_workspace.front.resource_header,
  sizeof wf_workspace.front.resource_header,WF_FRONT_PACK_FILE_BYTES,WF_FRONT_PACK_FIRST_BYTES,
  WF_FRONT_PACK_HEADER_CRC,clock_now,NULL);
 publish(rc);return rc;
}
static int size_of(void*u,const char*name,uint32_t*bytes){
 (void)u;if(!wf_front_pack_known(name))return wf_cd_asset_io.size(wf_cd_asset_io.user,name,bytes);
 int rc=ensure();if(!rc)rc=wf_front_pack_size(&pack,name,bytes);publish(rc);return rc;
}
static int read_data(void*u,const char*name,uint32_t at,void*dst,size_t bytes){
 (void)u;if(!wf_front_pack_known(name))return wf_cd_asset_io.read(wf_cd_asset_io.user,name,at,dst,bytes);
 int rc=ensure();if(!rc)rc=wf_front_pack_read(&pack,name,at,dst,bytes);publish(rc);return rc;
}
const wf_asset_io_t wf_front_asset_io={size_of,read_data,NULL};

void wf_front_resource_pack_invalidate(void){pack=(wf_front_pack_t){0};attempted=false;}
