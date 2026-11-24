#ifndef WF_JO_ASSETS_H
#define WF_JO_ASSETS_H

#include <stddef.h>
#include <stdint.h>

#define WF_JO_ROM_BYTES       524288u
#define WF_JO_SPRITE_BYTES    8388608u
/* Maximum independent block and minimum scratch lease. Sprite packs may use
 * 4 or 8 KiB blocks; immutable scene packs retain their 8 KiB block format. */
#define WF_JO_SPRITE_BLOCK    8192u
#define WF_JO_SPRITE_MIN_BLOCK 4096u
#define WF_JO_CART_BYTES      4194304u
#define WF_JO_CACHE_MAX_SLOTS 256u

enum wf_jo_asset_status {
    WF_JO_ASSET_OK = 0,
    WF_JO_ASSET_ARGUMENT = -1,
    WF_JO_ASSET_FORMAT = -2,
    WF_JO_ASSET_IO = -3,
    WF_JO_ASSET_DECODE = -4,
    WF_JO_ASSET_UNLOADED = -5,
    WF_JO_ASSET_CAPACITY = -6,
    WF_JO_ASSET_SERVICE = -7
};

enum wf_jo_asset_detail {
    WF_JO_ASSET_DETAIL_NONE = 0,
    WF_JO_ASSET_DETAIL_ARGUMENT = 1,
    WF_JO_ASSET_DETAIL_CAPACITY = 2,
    WF_JO_ASSET_DETAIL_ADLER_SELFTEST = 3,
    WF_JO_ASSET_DETAIL_BLOCK_SIZE = 4,
    WF_JO_ASSET_DETAIL_HEADER = 5,
    WF_JO_ASSET_DETAIL_INDEX_LAYOUT = 6,
    WF_JO_ASSET_DETAIL_INDEX_CHECKSUM = 7,
    WF_JO_ASSET_DETAIL_OFFSET = 8,
    WF_JO_ASSET_DETAIL_OVERLAP = 9,
    WF_JO_ASSET_DETAIL_INFLATER = 10,
    WF_JO_ASSET_DETAIL_FILE_SIZE = 11,
    WF_JO_ASSET_DETAIL_ROM_SIZE = 12,
    WF_JO_ASSET_DETAIL_DISC_READ = 13
};
uint32_t wf_jo_assets_last_detail(void);

typedef struct wf_jo_asset_stats {
    uint32_t block_hits;
    uint32_t block_misses;
    uint32_t decode_failures;
    uint32_t cd_reads;
    uint32_t cd_bytes;
    uint32_t scene_block_decodes;
    uint32_t scene_reads;
} wf_jo_asset_stats_t;

/* Cartridge detection/enable belongs to the application. It passes its sole
 * owned 4 MB arena here after jo_core_init(). ROM and compressed
 * graphics remain in that arena; cache is a separate caller-owned LWRAM lease.
 * The cache needs at least 8 KB and keeps at most 256 independent blocks.
 * No heap allocation or CD access occurs for ROM/sprite reads after load. */
int wf_jo_assets_load(void *cart, size_t cart_bytes,
                      void *cache, size_t cache_bytes);
/* Preload SCENES.WFP containing all seven immutable scene packs in a separate
 * caller-owned LWRAM arena (282624 bytes reserved, 268356 bytes used).
 * Full scene reads decode directly into scene staging without displacing
 * any sprite cache blocks. Transitions perform no disc reads. */
int wf_jo_assets_preload_scenes(void *arena, size_t bytes);

/* Bind already loaded immutable assets. Useful for host verification and for
 * an application that owns its own disc preload; no copies are made. */
int wf_jo_assets_bind(const void *rom, size_t rom_bytes,
                     const void *pack, size_t pack_bytes,
                     void *cache, size_t cache_bytes);
int wf_jo_assets_ready(void);
/* Immutable binding token for presentation preparation. Failed binds advance
 * it too. Zero disables reuse permanently after its first uint32 wrap. */
uint32_t wf_jo_assets_preparation_generation(void);
/* True only after all seven immutable scene views are resident. */
int wf_jo_assets_scenes_ready(void);
/* Temporarily lend an exact block-multiple tail of the caller-owned decoded
 * sprite cache. At least 8 KiB stays active for sprites/partial scene reads.
 * No nested loan is allowed. Mappings of all lent slots are invalidated before
 * the pointer is returned; remaining hot slots stay valid. Return requires
 * the exact live pointer and restores empty slots. Any load/bind cancels the
 * lifetime, even if it fails. Do not call these functions from a service hook.
 * The full original cache lease remains reserved against all asset overlaps.
 * The pointer retains cache_base alignment; callers storing typed structures
 * must check their required alignment (bind itself also accepts byte buffers). */
int wf_jo_assets_borrow_sprite_cache_tail(size_t bytes, void **out);
int wf_jo_assets_return_sprite_cache_tail(void *storage);
size_t wf_jo_assets_sprite_cache_active_bytes(void);

/* Read-only compressed block view into a validated, resident SCENES.WFP.
 * Only the seven known scene names and valid block indices are accepted.
 * No decode, callback or CD access occurs. Pointer lifetime ends at rebind or
 * scene preload; the caller must validate zlib END/checksum and exact lengths
 * when decoding. All output arguments are required and zeroed on failure. */
int wf_jo_assets_scene_block_view(const char *name, unsigned block,
                                 const uint8_t **source, unsigned *packed,
                                 unsigned *decoded, uint32_t *file_bytes);
size_t wf_jo_assets_cart_used(void);
void *wf_jo_assets_cart_tail(size_t *available);
void wf_jo_assets_get_stats(wf_jo_asset_stats_t *out);

/* Optional main-thread service after a complete cold block has been published.
 * Return zero on success. The hook must not read/rebind assets or synchronize
 * display fields. No callback runs for a hot sprite read. Load/bind clears it. */
void wf_jo_assets_set_service_hook(int (*service)(void *), void *user);

/* Initial-load observation only. Called before a blocking disc open and after
 * each read chunk (at most 32 KiB while attached). total==0 means a size query.
 * Runs on the caller's thread; must not read/rebind assets. Kept across load/
 * bind so it can observe the bootstrap itself. Detach with NULL before play. */
void wf_jo_assets_set_loading_observer(
    void (*observer)(const char *name, size_t done, size_t total));

/* Legacy file-reader contract: zero on success. ROM68K.BIN and SPRITES.PAK
 * are virtual resident files. Other files use checked Jo FS reads on Saturn,
 * allowing loading between scenes without changing the original callbacks. */
int wf_jo_assets_file_read(const char *name, uint32_t offset,
                           void *destination, size_t bytes);
int wf_jo_assets_file_size(const char *name, uint32_t *bytes);
int wf_jo_assets_rom_read(uint32_t offset, void *destination, size_t bytes);

/* Original 16x16 four-bit pixels: high nibble is the left pixel. The pointer
 * stays valid until another asset read evicts its block. Consume synchronously
 * or copy into HWRAM staging before submitting a hardware transfer. */
int wf_jo_assets_tile_read(uint16_t tile, const uint8_t **pixels);

/* Optional current render demand. Copy active IDs not already prepared in the
 * GPU cache; caller arrays need not survive this call. The worker may decode
 * a later cold block privately, but only a real tile read publishes it. End
 * always joins/discards that private work, including on render abort. */
void wf_jo_assets_demand_frame(const uint16_t *sources,const uint8_t *touched,
                               unsigned epoch,unsigned count,const uint8_t *prepared);
void wf_jo_assets_demand_end(void);

#ifdef WF_JO_ASSETS_TEST_WORKER
/* Host-only deterministic/threaded scheduling supplied by the regression. */
void wf_jo_assets_test_submit(void (*callback)(void));
void wf_jo_assets_test_join(void);
#endif

#endif
