#ifndef WP2015_SECONDARY_H
#define WP2015_SECONDARY_H
#include <stddef.h>
#include <stdint.h>

#define WP2015_SECONDARY_FILE "TAGEX.BIN"
#define WP2015_SECONDARY_TILES 14352u
#define WP2015_SECONDARY_BLOCKS 1794u
#define WP2015_SECONDARY_BLOCK_BYTES 1024u
#define WP2015_SECONDARY_CACHE_BLOCKS 4u
#define WP2015_SECONDARY_LWRAM_BYTES 8192u
#define WP2015_SECONDARY_WORKSPACE_BYTES 49152u
#define WP2015_SECONDARY_INPUT_BYTES 704u
#define WP2015_SECONDARY_SCRATCH_BYTES 4704u
#define WP2015_SECONDARY_HEADER_BYTES 32u
#define WP2015_SECONDARY_MODEL_BYTES 632u
#define WP2015_SECONDARY_METADATA_BYTES (8192u+512u+(WP2015_SECONDARY_BLOCKS*2u+1u)*4u)

/* Exact shared table type. Only wp2015_rans_tables may construct it. After a
 * successful construction it is immutable until the workspace is released.
 * Source, destination, and table storage must be complete and disjoint.
 * Arbitrary or corrupted lookup tables are outside the decoder contract. */
typedef struct {
    uint16_t slots[1152];
    uint8_t frequency[316];
} wp2015_rans_compact_t;

typedef struct {
    uint32_t magic,ready,last_error,tile_count,block_count,payload_bytes,pool_capacity,cd_base;
    uint32_t cd_bytes,requests,cache_hits,block_decodes,output_hash_errors,source_read_errors,absent_tiles,cd_reads;
    uint32_t vram_reads,hw_reads,lw_reads,last_tile,last_block,max_decode_ticks,max_read_ticks,max_request_ticks;
    uint32_t reserved[4];
    uint16_t cache_tags[WP2015_SECONDARY_CACHE_BLOCKS];
    uint32_t next_slot,generation;
} wp2015_secondary_state_t;

/* This is an actual alternative member of bootstrap's union, not a cast of
 * a participant machine or uint32_t array to an unrelated struct. Each member
 * is initialized before use. Temporary model data dies before output begins. */
typedef struct {
    wp2015_secondary_state_t state;
    uint8_t membership[8192];
    uint16_t prefix[256];
    uint32_t offsets[WP2015_SECONDARY_BLOCKS+1u];
    uint32_t hashes[WP2015_SECONDARY_BLOCKS];
    uint8_t metadata_padding[12];
    wp2015_rans_compact_t tables;
    uint8_t table_padding[4];
    /* The CD FIFO helpers access complete32-bit words; byte views below
     * may inspect any object's representation, without inverse aliasing. */
    uint32_t cd_scratch[WP2015_SECONDARY_SCRATCH_BYTES/4u];
    uint8_t input[WP2015_SECONDARY_INPUT_BYTES];
    union {
        struct {uint8_t bytes[WP2015_SECONDARY_BLOCK_BYTES];uint32_t guard[8];}
            cache[WP2015_SECONDARY_CACHE_BLOCKS];
        struct {uint16_t literal[286],distance[30];} model;
    } output;
    uint8_t resident[13664];
    uint32_t end_guard[8];
} wp2015_secondary_workspace_t;

_Static_assert(sizeof(wp2015_secondary_state_t)==128u,"Secondary state layout");
_Static_assert(sizeof(wp2015_rans_compact_t)==2620u,"Exact compact table size");
_Static_assert(sizeof(wp2015_secondary_workspace_t)==WP2015_SECONDARY_WORKSPACE_BYTES,"Bootstrap lease budget");
_Static_assert(offsetof(wp2015_secondary_workspace_t,tables)==23200u,"Metadata/table separation");
_Static_assert(offsetof(wp2015_secondary_workspace_t,cd_scratch)==25824u,"Table/CD separation");
_Static_assert(offsetof(wp2015_secondary_workspace_t,input)==30528u,"CD/input separation");
_Static_assert(offsetof(wp2015_secondary_workspace_t,output)==31232u,"Input/output separation");
_Static_assert(offsetof(wp2015_secondary_workspace_t,resident)==35456u,"Output/resident separation");
_Static_assert(offsetof(wp2015_secondary_workspace_t,end_guard)==49120u,"Workspace end guard");

typedef int (*wp2015_secondary_file_read_fn)(const char *,uint32_t,void *,size_t);

/* Called only before combat, after the completed direct boot releases its
 * typed workspace and presentation grants the disjoint LWRAM tail. A load
 * publishes nothing until every header/model/span/hash and CD seal passes.
 * Failure leaves the pool unavailable. No mechanical reads occur at lookup. */
int wp2015_secondary_load(wp2015_secondary_workspace_t *workspace,
                         void *lwram,size_t lwram_bytes,uint32_t file_bytes,
                         wp2015_secondary_file_read_fn read_file);
/* 1: exactly128 original bytes copied; 0: not loaded or ID absent;
 * -1: resident-block/transport/guard failure. Never hide a resident error
 * with the original mechanical-CD fallback. dst is consumed synchronously
 * and must not overlap any leased storage. No pointer to dst is retained. */
int wp2015_secondary_tile_read(uint16_t tile,uint8_t dst[128]);
/* Reset only invalidates publication. It issues no CD commands and writes no
 * leased bytes, so call it before changing the active bootstrap union member
 * or releasing/changing presentation storage. */
void wp2015_secondary_reset(void);
int wp2015_secondary_in_use(void);
extern wp2015_secondary_workspace_t *wp2015_secondary_active;

int wp2015_rans_tables(uint32_t *,unsigned,const uint16_t [286],const uint16_t [30]);
int wp2015_rans_decode(void *,unsigned *,const void *,unsigned,const uint32_t *);
#endif
