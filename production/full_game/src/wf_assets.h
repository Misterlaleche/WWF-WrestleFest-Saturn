#ifndef WF_ASSETS_H
#define WF_ASSETS_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef struct {const char *name;uint32_t bytes,crc32;} wf_asset_t;
typedef struct {
 int (*size)(void*,const char*,uint32_t*);
 int (*read)(void*,const char*,uint32_t,void*,size_t);
 void *user;
} wf_asset_io_t;
typedef enum {WF_ASSET_OK,WF_ASSET_ARGUMENT,WF_ASSET_CAPACITY,WF_ASSET_SIZE,WF_ASSET_READ,WF_ASSET_CHECKSUM} wf_asset_error_t;
typedef struct {wf_asset_error_t error;uint32_t index,used;} wf_asset_result_t;
/* Boot/transition-only.32-byte aligned caller storage, at most64 assets.
 * Output has count slots and is published only after every checksum passes.
 * On failure output is cleared, storage may contain incomplete bytes.
 * CRC checks accidental disc corruption; build provenance uses SHA256. */
bool wf_assets_load(const wf_asset_t*,unsigned,void*,uint32_t,void**,const wf_asset_io_t*,wf_asset_result_t*);
uint32_t wf_assets_crc32(const void*,size_t);
/* Raw incremental state: begin with 0xffffffffu and complement after the
 * final chunk. A zero-byte chunk preserves its incoming state. */
uint32_t wf_assets_crc32_update(uint32_t,const void*,size_t);
#endif
