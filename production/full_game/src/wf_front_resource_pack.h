#ifndef WF_FRONT_RESOURCE_PACK_H
#define WF_FRONT_RESOURCE_PACK_H
#include "wf_assets.h"
enum {WF_FRONT_RESOURCE_CACHE_BYTES=196608,WF_FRONT_RESOURCE_HEADER_BYTES=2048};
typedef struct {
 uint32_t reads,sectors,decoded_bytes,decode_calls,cache_hits,partial_reads,read_fields,decode_fields;
} wf_front_pack_stats_t;
typedef struct {
 const wf_asset_io_t *source;
 uint8_t *cache,*header;
 uint32_t bytes,first_bytes,header_crc,blocks,groups,cached;
 uint32_t (*clock)(void*);void *clock_user;
 wf_front_pack_stats_t stats;
 bool ready;
} wf_front_pack_t;
/* Caller owns two non-overlapping,32-byte aligned frontend leases. The expected
 * file/first-group/header identity comes from the verified generated manifest. */
int wf_front_pack_open(wf_front_pack_t*,const wf_asset_io_t*,uint8_t*,size_t,uint8_t*,size_t,
 uint32_t,uint32_t,uint32_t,uint32_t(*)(void*),void*);
bool wf_front_pack_known(const char*);
int wf_front_pack_size(void*,const char*,uint32_t*);
int wf_front_pack_read(void*,const char*,uint32_t,void*,size_t);
#endif
