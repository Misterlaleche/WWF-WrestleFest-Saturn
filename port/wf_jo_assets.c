#include "wf_jo_assets.h"
#include "zlib/zlib.h"
#include <limits.h>
#include <string.h>
#ifdef WF_JO_ASSETS_HOST
#include <stdio.h>
#else
#include <jo/jo.h>
#endif

#if defined(WF_JO_ASSETS_TEST_WORKER) || \
    (defined(JO_COMPILE_WITH_DUAL_CPU_SUPPORT) && !defined(WF_JO_ASSETS_HOST))
#define WF_ASSETS_WORKER 1
#endif
#ifdef WF_JO_ASSETS_TEST_WORKER
#ifndef WF_JO_ASSETS_HOST
#error "Fake asset worker is only available to host tests"
#endif
#endif

#define WF_BLOCKS (WF_JO_SPRITE_BYTES / WF_JO_SPRITE_MIN_BLOCK)
#define WF_NO_SLOT 0xffffu
#define WF_PACK_HEADER 32u

typedef struct wf_scene_asset {
    const char *name;
    uint32_t bytes;
    unsigned first_block;
} wf_scene_asset_t;

static wf_scene_asset_t scenes[] = {
    {"R18RING.BIN",114918u,0}, {"R18MENU.BIN",124938u,0},
    {"R18MAP1.BIN",127138u,0}, {"R18MAP2.BIN", 95978u,0},
    {"R18MAP4.BIN",129998u,0}, {"R18MAP5.BIN",115568u,0},
    {"R18MAP6.BIN", 95198u,0}
};
static const uint8_t *scene_pack;
static size_t scene_pack_bytes;
#define WF_SCENES (sizeof scenes/sizeof scenes[0])
#define WF_SCENE_OFFSETS (WF_PACK_HEADER+WF_SCENES*16u)
static const uint8_t *rom_image, *sprite_pack;
static size_t sprite_pack_bytes;
static uint8_t *cache_data, *cart_base;
static size_t cart_capacity, cart_used, cache_bytes_used;
static unsigned cache_slots, cache_max_slots;
static void *cache_loan;
static unsigned sprite_block_bytes, sprite_block_shift, sprite_tile_shift;
static uint16_t block_slot[WF_BLOCKS];
static uint16_t slot_block[WF_JO_CACHE_MAX_SLOTS];
static uint32_t slot_age[WF_JO_CACHE_MAX_SLOTS], cache_clock;
/* A protected age sorts after every probation age, so the existing unsigned
 * minimum-age victim search also implements segmented LRU. Packed stamps live
 * after the maximum binding span, never in a tail that can be lent/returned. */
#define WF_CACHE_PROTECTED UINT32_C(0x80000000)
#define WF_CACHE_PROBATION 16u
static unsigned cache_protected_count,cache_batch_epoch;
static wf_jo_asset_stats_t stats;
static int (*service_hook)(void *);
static void *service_user;
static unsigned servicing;
static uint32_t binding_generation,last_detail;
static unsigned preparation_generation_exhausted;
static void demand_join(unsigned discard);
static void (*loading_observer)(const char *, size_t, size_t);

static int cache_policy_enabled(void) {
    return cache_max_slots&&cache_max_slots+(cache_max_slots+1u)/2u<=WF_JO_CACHE_MAX_SLOTS;
}
static unsigned cache_stamp(unsigned slot) {
    uint32_t pair=slot_age[cache_max_slots+(slot>>1)];
    return slot&1u?pair>>16:pair&65535u;
}
static void cache_stamp_set(unsigned slot,unsigned epoch) {
    uint32_t *pair=&slot_age[cache_max_slots+(slot>>1)];
    if(slot&1u)*pair=(*pair&65535u)|((uint32_t)epoch<<16);
    else *pair=(*pair&UINT32_C(0xffff0000))|epoch;
}
static void cache_slot_forget(unsigned slot) {
    /* Legacy replacement/scene invalidation retains stale ages until touch.
     * A service failure can leave a newly published block at that age. */
    if(!cache_policy_enabled())return;
    if(slot_age[slot]&WF_CACHE_PROTECTED)--cache_protected_count;
    cache_stamp_set(slot,0);
    slot_age[slot]=0;
}
static void cache_reconcile(void) {
    unsigned limit=cache_slots>WF_CACHE_PROBATION?cache_slots-WF_CACHE_PROBATION:0u;
    while(cache_protected_count>limit){
        unsigned victim=cache_slots;uint32_t oldest=UINT32_MAX;
        for(unsigned i=0;i<cache_slots;i++){
            uint32_t age=slot_age[i];
            if((age&WF_CACHE_PROTECTED)&&(victim==cache_slots||age<oldest)){
                oldest=age;victim=i;
            }
        }
        if(victim==cache_slots){cache_protected_count=0;return;}
        slot_age[victim]=cache_clock;--cache_protected_count;
    }
}
static void cache_next_batch(void) {
    if(cache_policy_enabled()&&++cache_batch_epoch>65535u){
        memset(slot_age+cache_max_slots,0,((cache_max_slots+1u)/2u)*sizeof slot_age[0]);
        cache_batch_epoch=1u;
    }
}
static void cache_touch(unsigned slot,unsigned hit) {
    if(!cache_policy_enabled()){
        if(++cache_clock==0u){memset(slot_age,0,sizeof slot_age);cache_clock=1u;}
        slot_age[slot]=cache_clock;return;
    }
    unsigned stamp=cache_stamp(slot);
    uint32_t protected=slot_age[slot]&WF_CACHE_PROTECTED;
    if(++cache_clock&WF_CACHE_PROTECTED){
        /* Only replacement metadata wraps; payload identities stay resident. */
        memset(slot_age,0,sizeof slot_age);cache_clock=1u;
        cache_protected_count=0;protected=0;stamp=0;
    }
    unsigned promote=hit&&!protected&&stamp!=cache_batch_epoch;
    if(promote){protected=WF_CACHE_PROTECTED;++cache_protected_count;}
    slot_age[slot]=cache_clock|protected;
    if(stamp!=cache_batch_epoch)cache_stamp_set(slot,cache_batch_epoch);
    if(promote)cache_reconcile();
}
#if defined(JO_COMPILE_WITH_PROFILER_SUPPORT) && !defined(WF_JO_ASSETS_HOST)
/* Millisecond clock is extended in VBlank, so cold decodes exceeding the
 * profiler's 16-bit FRT period remain measurable. count/bytes/total/max/last. */
volatile uint32_t wf_jo_asset_profile[5];
#endif

void wf_jo_assets_set_service_hook(int (*service)(void *), void *user) {
    demand_join(0);service_hook=service;service_user=user;
}
void wf_jo_assets_set_loading_observer(
        void (*observer)(const char *, size_t, size_t)) {
    loading_observer=observer;
}
static int service_after_decode(void) {
    demand_join(0);
    if(!service_hook||servicing)return WF_JO_ASSET_OK;
    servicing=1;int result=service_hook(service_user);servicing=0;
    return result?WF_JO_ASSET_SERVICE:WF_JO_ASSET_OK;
}

/* A one-shot Z_FINISH call has its entire dictionary in the destination
 * block. zlib never needs a separate sliding-window allocation on that path.
 * Reject any extra allocation, including malformed streams needing a resume.
 * The state is reused between blocks, so no allocator runs during gameplay. */
typedef struct wf_inflater_workspace {
    union {
        uint64_t alignment;
        void *pointer_alignment;
        uint8_t bytes[8192];
    } storage;
    z_stream stream;
    unsigned storage_used,ready;
    uint8_t stage[WF_JO_SPRITE_BLOCK] __attribute__((aligned(32)));
} wf_inflater_workspace_t;
static wf_inflater_workspace_t master_decoder;

static voidpf inflate_alloc(voidpf opaque,uInt items,uInt size) {
    wf_inflater_workspace_t *w=opaque;
    if(w->storage_used||!size||items>sizeof w->storage.bytes/size)return Z_NULL;
    w->storage_used=1;memset(w->storage.bytes,0,(size_t)items*size);
    return w->storage.bytes;
}
static void inflate_free(voidpf opaque,voidpf memory) {
    wf_inflater_workspace_t *w=opaque;
    if(memory==w->storage.bytes)w->storage_used=0;
}
static int inflater_init(wf_inflater_workspace_t *w) {
    if(w->ready)return WF_JO_ASSET_OK;
    memset(&w->stream,0,sizeof w->stream);
    w->stream.zalloc=inflate_alloc;w->stream.zfree=inflate_free;w->stream.opaque=w;
    if(inflateInit(&w->stream)!=Z_OK)return WF_JO_ASSET_DECODE;
    w->ready=1;return WF_JO_ASSET_OK;
}
/* No cache, profiling, callback, clock-extension or publication side effects.
 * Each CPU owns a complete inflater and output/history allocation. */
static int inflate_private(wf_inflater_workspace_t *w,const uint8_t *source,
                           unsigned packed,void *out,unsigned capacity,unsigned bytes) {
    if(bytes>capacity||inflateReset(&w->stream)!=Z_OK)return WF_JO_ASSET_DECODE;
    w->stream.next_in=(Bytef *)(uintptr_t)source;w->stream.avail_in=packed;
    w->stream.next_out=out;w->stream.avail_out=capacity;
    int result=inflate(&w->stream,Z_FINISH);
    return result==Z_STREAM_END&&!w->stream.avail_in&&w->stream.total_out==bytes&&
        w->stream.avail_out==capacity-bytes?WF_JO_ASSET_OK:WF_JO_ASSET_DECODE;
}
static uint32_t read32(const uint8_t *p) {
    return (uint32_t)p[0]<<24 | (uint32_t)p[1]<<16 | (uint32_t)p[2]<<8 | p[3];
}
static size_t aligned32(size_t n) {return (n+31u)&~(size_t)31u;}
static int valid_range(uint32_t at,size_t n,size_t total) {
    return (size_t)at<=total && n<=total-(size_t)at;
}
static int overlap(const void *a,size_t an,const void *b,size_t bn) {
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    if(an>UINTPTR_MAX-x||bn>UINTPTR_MAX-y)return 1;
    return x<y+bn&&y<x+an;
}
static void invalidate(void) {
    wf_jo_assets_demand_end();++binding_generation;
    if(!binding_generation)preparation_generation_exhausted=1;
    service_hook=NULL;service_user=NULL;servicing=0;
    rom_image=sprite_pack=NULL;sprite_pack_bytes=0;cache_data=NULL;cache_slots=0;
    cache_max_slots=0;cache_loan=NULL;
    cache_protected_count=cache_batch_epoch=0;
    cart_base=NULL;cart_capacity=cart_used=cache_bytes_used=0;cache_clock=0;
    memset(&stats,0,sizeof stats);
    scene_pack=NULL;scene_pack_bytes=0;
}
uint32_t wf_jo_assets_preparation_generation(void)
{return preparation_generation_exhausted?0:binding_generation;}
uint32_t wf_jo_assets_last_detail(void){return last_detail;}

/* Read into exactly the caller's checked allocation. Do not use
 * jo_fs_read_file_ptr(), which appends a NUL beyond binary file length. */
static int disc_size(const char *name,uint32_t *bytes) {
    if(!name||!bytes)return WF_JO_ASSET_ARGUMENT;
    if(loading_observer)loading_observer(name,0,0);
#ifdef WF_JO_ASSETS_HOST
    FILE *file=fopen(name,"rb");if(!file)return WF_JO_ASSET_IO;
    int status=fseek(file,0,SEEK_END);long size=ftell(file);fclose(file);
    if(status||size<0||(unsigned long)size>UINT32_MAX)return WF_JO_ASSET_IO;
    *bytes=(uint32_t)size;
#else
#if defined(WF_JO_RAW_DISC_ASSETS) && WF_JO_RAW_DISC_ASSETS
    /* The packaging gate verifies these native WF payloads are not JoLZ.
     * Other names retain the generic decompressed-size contract below. */
    if(!strcmp(name,"ROM68K.BIN")||!strcmp(name,"SPRITES.WFP")||
       !strcmp(name,"SCENES.WFP")||!strcmp(name,"SFX.DAT")){
        int size=jo_fs_get_raw_file_size(name);
        if(size<0)return WF_JO_ASSET_IO;
        *bytes=(uint32_t)size;return WF_JO_ASSET_OK;
    }
#endif
    jo_file file;
    if(!jo_fs_open(&file,name))return WF_JO_ASSET_IO;
    int size=file.size;jo_fs_close(&file);
    if(size<0)return WF_JO_ASSET_IO;
    *bytes=(uint32_t)size;
#endif
    return WF_JO_ASSET_OK;
}
static int disc_read(const char *name,uint32_t at,void *out,size_t n) {
    if(!name||(!out&&n)||n>UINT_MAX)return WF_JO_ASSET_ARGUMENT;
    if(loading_observer)loading_observer(name,0,n);
#ifdef WF_JO_ASSETS_HOST
    FILE *file=fopen(name,"rb");if(!file)return WF_JO_ASSET_IO;
    int status=fseek(file,0,SEEK_END);long size=ftell(file);
    if(status||size<0||!valid_range(at,n,(size_t)size)||fseek(file,(long)at,SEEK_SET)) {
        fclose(file);return WF_JO_ASSET_IO;
    }
    size_t got=0;
    while(got<n) {
        size_t chunk=n-got;
        if(loading_observer&&chunk>32768u)chunk=32768u;
        size_t received=fread((char *)out+got,1,chunk,file);
        if(!received)break;
        got+=received;
        if(loading_observer)loading_observer(name,got,n);
    }
    fclose(file);
    if(got!=n)return WF_JO_ASSET_IO;
#else
    jo_file file;
    if(!jo_fs_open(&file,name))return WF_JO_ASSET_IO;
    if(file.size<0||!valid_range(at,n,(size_t)file.size) ||
       (at&&!jo_fs_seek_forward(&file,at))) {
        jo_fs_close(&file);return WF_JO_ASSET_IO;
    }
    size_t got=0;
    while(got<n) {
        size_t chunk=n-got;
        if(loading_observer&&chunk>32768u)chunk=32768u;
        int received=jo_fs_read_next_bytes(&file,(char *)out+got,(unsigned)chunk);
        if(received<=0||(size_t)received>chunk)break;
        got+=(size_t)received;
        if(loading_observer)loading_observer(name,got,n);
    }
    jo_fs_close(&file);
    if(got!=n)return WF_JO_ASSET_IO;
#endif
    if(n){stats.cd_reads++;stats.cd_bytes+=(uint32_t)n;}
    return WF_JO_ASSET_OK;
}

int wf_jo_assets_bind(const void *rom,size_t rn,const void *pack,size_t pn,
                      void *cache,size_t cn) {
    last_detail=0;invalidate();
    if(!rom||!pack||!cache||rn!=WF_JO_ROM_BYTES||cn<WF_JO_SPRITE_BLOCK){
        last_detail=WF_JO_ASSET_DETAIL_ARGUMENT;return WF_JO_ASSET_ARGUMENT;
    }
    if(pn<WF_PACK_HEADER || pn>WF_JO_CART_BYTES-WF_JO_ROM_BYTES){
        last_detail=WF_JO_ASSET_DETAIL_CAPACITY;return WF_JO_ASSET_CAPACITY;
    }
    const uint8_t *p=pack;
    unsigned block_bytes=read32(p+8);
    if(block_bytes!=4096u&&block_bytes!=8192u){
        last_detail=WF_JO_ASSET_DETAIL_BLOCK_SIZE;return WF_JO_ASSET_FORMAT;
    }
    unsigned blocks=WF_JO_SPRITE_BYTES/block_bytes;
    if(memcmp(p,"WFSZ",4)||read32(p+4)!=1u||
       read32(p+12)!=WF_JO_SPRITE_BYTES||read32(p+16)!=blocks||read32(p+24)!=pn){
        last_detail=WF_JO_ASSET_DETAIL_HEADER;return WF_JO_ASSET_FORMAT;
    }
    uint32_t data_at=read32(p+20);
    const size_t expected_at=aligned32(WF_PACK_HEADER+(blocks+1u)*4u);
    if(data_at!=expected_at||data_at>pn){
        last_detail=WF_JO_ASSET_DETAIL_INDEX_LAYOUT;return WF_JO_ASSET_FORMAT;
    }
    if((uint32_t)adler32(1,p+WF_PACK_HEADER,data_at-WF_PACK_HEADER)!=read32(p+28)){
        last_detail=WF_JO_ASSET_DETAIL_INDEX_CHECKSUM;return WF_JO_ASSET_FORMAT;
    }
    uint32_t previous=read32(p+WF_PACK_HEADER);
    if(previous!=data_at){last_detail=WF_JO_ASSET_DETAIL_OFFSET;return WF_JO_ASSET_FORMAT;}
    for(unsigned block=0;block<blocks;block++) {
        uint32_t next=read32(p+WF_PACK_HEADER+(block+1u)*4u);
        if(next<=previous||next>pn){last_detail=WF_JO_ASSET_DETAIL_OFFSET;return WF_JO_ASSET_FORMAT;}
        previous=next;
    }
    if(previous!=pn){last_detail=WF_JO_ASSET_DETAIL_OFFSET;return WF_JO_ASSET_FORMAT;}
    size_t requested=cn/block_bytes;
    unsigned slots=requested>WF_JO_CACHE_MAX_SLOTS?WF_JO_CACHE_MAX_SLOTS:(unsigned)requested;
    size_t used=(size_t)slots*block_bytes;
    if(overlap(rom,rn,pack,pn)||overlap(cache,used,rom,rn)||overlap(cache,used,pack,pn)){
        last_detail=WF_JO_ASSET_DETAIL_OVERLAP;return WF_JO_ASSET_ARGUMENT;
    }
    int status=inflater_init(&master_decoder);
    if(status){last_detail=WF_JO_ASSET_DETAIL_INFLATER;return status;}
    rom_image=rom;sprite_pack=p;sprite_pack_bytes=pn;
    sprite_block_bytes=block_bytes;sprite_block_shift=block_bytes==4096u?12u:13u;
    sprite_tile_shift=sprite_block_shift-7u;
    cache_data=cache;cache_slots=cache_max_slots=slots;cache_bytes_used=used;
    memset(block_slot,0xff,sizeof block_slot);memset(slot_block,0xff,sizeof slot_block);
    memset(slot_age,0,sizeof slot_age);
    cache_batch_epoch=cache_policy_enabled()?1u:0u;
    return WF_JO_ASSET_OK;
}

int wf_jo_assets_load(void *cart,size_t capacity,void *cache,size_t cn) {
    last_detail=0;invalidate();
    if(!cart||capacity>WF_JO_CART_BYTES||capacity<WF_JO_ROM_BYTES||
       !cache||cn<WF_JO_SPRITE_BLOCK){last_detail=WF_JO_ASSET_DETAIL_ARGUMENT;return WF_JO_ASSET_ARGUMENT;}
    uint32_t rn,pn;
#ifndef WF_JO_ASSETS_HOST
    jo_fs_set_read_sector_count(16);
#endif
    if(disc_size("ROM68K.BIN",&rn)||disc_size("SPRITES.WFP",&pn)){last_detail=WF_JO_ASSET_DETAIL_FILE_SIZE;return WF_JO_ASSET_IO;}
    if(rn!=WF_JO_ROM_BYTES){last_detail=WF_JO_ASSET_DETAIL_ROM_SIZE;return WF_JO_ASSET_FORMAT;}
    if(pn>capacity-WF_JO_ROM_BYTES||aligned32(pn)>capacity-WF_JO_ROM_BYTES){
        last_detail=WF_JO_ASSET_DETAIL_CAPACITY;return WF_JO_ASSET_CAPACITY;
    }
    uint8_t *p=cart;
    size_t used=WF_JO_ROM_BYTES+aligned32(pn);
    if(overlap(cart,used,cache,cn)){last_detail=WF_JO_ASSET_DETAIL_OVERLAP;return WF_JO_ASSET_ARGUMENT;}
    if(disc_read("ROM68K.BIN",0,p,rn)||disc_read("SPRITES.WFP",0,p+rn,pn)){
        last_detail=WF_JO_ASSET_DETAIL_DISC_READ;return WF_JO_ASSET_IO;
    }
    wf_jo_asset_stats_t loading_stats=stats;
    int status=wf_jo_assets_bind(p,rn,p+rn,pn,cache,cn);
    if(status)return status;
    stats=loading_stats;cart_base=p;cart_capacity=capacity;cart_used=used;
    return WF_JO_ASSET_OK;
}

int wf_jo_assets_preload_scenes(void *arena,size_t capacity) {
    wf_jo_assets_demand_end();
    if(!wf_jo_assets_ready())return WF_JO_ASSET_UNLOADED;
    if(!arena)return WF_JO_ASSET_ARGUMENT;
    uint32_t needed;
    if(disc_size("SCENES.WFP",&needed))return WF_JO_ASSET_IO;
    if(needed<WF_SCENE_OFFSETS+8u)return WF_JO_ASSET_FORMAT;
    if(needed>capacity)return WF_JO_ASSET_CAPACITY;
    if(overlap(arena,needed,cache_data,cache_bytes_used)||
       overlap(arena,needed,rom_image,WF_JO_ROM_BYTES)||
       overlap(arena,needed,sprite_pack,sprite_pack_bytes))return WF_JO_ASSET_ARGUMENT;
    scene_pack=NULL;scene_pack_bytes=0;
    if(disc_read("SCENES.WFP",0,arena,needed))return WF_JO_ASSET_IO;
    const uint8_t *p=arena;
    unsigned blocks=0;
    for(unsigned i=0;i<WF_SCENES;i++)blocks+=(scenes[i].bytes+WF_JO_SPRITE_BLOCK-1u)/WF_JO_SPRITE_BLOCK;
    uint32_t data_at=read32(p+20);
    size_t expected_at=aligned32(WF_SCENE_OFFSETS+blocks*8u);
    if(memcmp(p,"WFSC",4)||read32(p+4)!=2u||read32(p+8)!=WF_JO_SPRITE_BLOCK||
       read32(p+12)!=WF_SCENES||read32(p+16)!=blocks||read32(p+24)!=needed||
       data_at!=expected_at||data_at>needed||
       (uint32_t)adler32(1,p+WF_PACK_HEADER,data_at-WF_PACK_HEADER)!=read32(p+28))
        return WF_JO_ASSET_FORMAT;
    unsigned first=0;
    for(unsigned i=0;i<WF_SCENES;i++) {
        const uint8_t *entry=p+WF_PACK_HEADER+i*16u;
        unsigned count=(scenes[i].bytes+WF_JO_SPRITE_BLOCK-1u)/WF_JO_SPRITE_BLOCK;
        if(read32(entry)!=scenes[i].bytes||read32(entry+4)!=first||
           read32(entry+8)!=count||read32(entry+12))return WF_JO_ASSET_FORMAT;
        scenes[i].first_block=first;first+=count;
    }
    /* Physical streams occur in first-use order. Later logical blocks may
     * share an entire earlier stream, never an overlapping subrange or gap.
     * This check runs once at preload; scene reads remain constant-time. */
    uint32_t cursor=data_at;
    for(unsigned block=0;block<blocks;block++) {
        const uint8_t *span=p+WF_SCENE_OFFSETS+block*8u;
        uint32_t start=read32(span),end=read32(span+4u);
        if(start<data_at||end<=start||end>needed)return WF_JO_ASSET_FORMAT;
        if(start==cursor){cursor=end;continue;}
        if(end>cursor)return WF_JO_ASSET_FORMAT;
        unsigned previous=0;
        for(;previous<block;previous++)
            if(!memcmp(span,p+WF_SCENE_OFFSETS+previous*8u,8u))break;
        if(previous==block)return WF_JO_ASSET_FORMAT;
    }
    if(cursor!=needed)return WF_JO_ASSET_FORMAT;
    scene_pack=p;scene_pack_bytes=needed;
    return WF_JO_ASSET_OK;
}

int wf_jo_assets_ready(void){return rom_image&&sprite_pack&&cache_slots;}
int wf_jo_assets_scenes_ready(void){return wf_jo_assets_ready()&&scene_pack!=NULL;}
size_t wf_jo_assets_sprite_cache_active_bytes(void) {
    return wf_jo_assets_ready()?(size_t)cache_slots*sprite_block_bytes:0;
}
static void invalidate_cache_slots(unsigned first,unsigned end) {
    for(unsigned i=first;i<end;i++) {
        if(slot_block[i]!=WF_NO_SLOT)block_slot[slot_block[i]]=WF_NO_SLOT;
        slot_block[i]=WF_NO_SLOT;cache_slot_forget(i);slot_age[i]=0;
    }
}
int wf_jo_assets_borrow_sprite_cache_tail(size_t bytes,void **out) {
    wf_jo_assets_demand_end();
    if(!out)return WF_JO_ASSET_ARGUMENT;
    *out=NULL;
    if(!wf_jo_assets_ready())return WF_JO_ASSET_UNLOADED;
    if(servicing||cache_loan||!bytes||bytes%sprite_block_bytes)
        return WF_JO_ASSET_ARGUMENT;
    if(bytes>cache_bytes_used-WF_JO_SPRITE_BLOCK)return WF_JO_ASSET_CAPACITY;
    unsigned active=cache_max_slots-(unsigned)(bytes/sprite_block_bytes);
    /* Invalidate both directions before exposing any storage to the caller.
     * Full lease size stays unchanged for every overlap check. */
    invalidate_cache_slots(active,cache_max_slots);
    cache_slots=active;cache_reconcile();cache_loan=cache_data+(size_t)active*sprite_block_bytes;
    *out=cache_loan;
    return WF_JO_ASSET_OK;
}
int wf_jo_assets_return_sprite_cache_tail(void *storage) {
    wf_jo_assets_demand_end();
    if(!wf_jo_assets_ready())return WF_JO_ASSET_UNLOADED;
    if(servicing||!cache_loan||storage!=cache_loan)return WF_JO_ASSET_ARGUMENT;
    invalidate_cache_slots(cache_slots,cache_max_slots);
    cache_loan=NULL;cache_slots=cache_max_slots;
    return WF_JO_ASSET_OK;
}
int wf_jo_assets_scene_block_view(const char *name,unsigned block,
                                 const uint8_t **source,unsigned *packed,
                                 unsigned *decoded,uint32_t *file_bytes) {
    if(source)*source=NULL;
    if(packed)*packed=0;
    if(decoded)*decoded=0;
    if(file_bytes)*file_bytes=0;
    if(!name||!source||!packed||!decoded||!file_bytes)return WF_JO_ASSET_ARGUMENT;
    if(!wf_jo_assets_scenes_ready())return WF_JO_ASSET_UNLOADED;
    for(unsigned i=0;i<WF_SCENES;i++) {
        const wf_scene_asset_t *scene=&scenes[i];
        if(strcmp(name,scene->name))continue;
        unsigned count=(scene->bytes+WF_JO_SPRITE_BLOCK-1u)/WF_JO_SPRITE_BLOCK;
        if(block>=count)return WF_JO_ASSET_ARGUMENT;
        unsigned index=scene->first_block+block;
        uint32_t start=read32(scene_pack+WF_SCENE_OFFSETS+index*8u);
        uint32_t end=read32(scene_pack+WF_SCENE_OFFSETS+index*8u+4u);
        if(start<read32(scene_pack+20)||end<=start||end>scene_pack_bytes)
            return WF_JO_ASSET_FORMAT;
        unsigned bytes=scene->bytes-block*WF_JO_SPRITE_BLOCK;
        if(bytes>WF_JO_SPRITE_BLOCK)bytes=WF_JO_SPRITE_BLOCK;
        *source=scene_pack+start;*packed=end-start;*decoded=bytes;*file_bytes=scene->bytes;
        return WF_JO_ASSET_OK;
    }
    return WF_JO_ASSET_ARGUMENT;
}
size_t wf_jo_assets_cart_used(void){return cart_used;}
void *wf_jo_assets_cart_tail(size_t *available) {
    if(available)*available=cart_base?cart_capacity-cart_used:0;
    return cart_base?cart_base+cart_used:NULL;
}
void wf_jo_assets_get_stats(wf_jo_asset_stats_t *out){if(out)*out=stats;}

#ifdef WF_ASSETS_WORKER
/* Descriptor handoff is ordered by Jo's queue. Jo purges the slave cache
 * before calling and the master cache at join; do not poll cached flags. */
static struct {
    wf_inflater_workspace_t decoder;
    const uint8_t *source;
    unsigned packed,bytes;
    uint32_t generation,completed_generation,elapsed_ms;
    int status;
} worker;
static uint16_t demand_tiles[256];
static unsigned demand_count,demand_cursor,demand_active;
static unsigned worker_pending,worker_ready,worker_block;
static uint32_t demand_generation,worker_serial,worker_expected;
static uint32_t worker_binding,worker_plan;
static void worker_decode(void) {
#if defined(JO_COMPILE_WITH_PROFILER_SUPPORT) && !defined(WF_JO_ASSETS_HOST)
    unsigned began=jo_time_get_frc();
#endif
    worker.status=inflate_private(&worker.decoder,worker.source,worker.packed,
        worker.decoder.stage,sizeof worker.decoder.stage,worker.bytes);
#if defined(JO_COMPILE_WITH_PROFILER_SUPPORT) && !defined(WF_JO_ASSETS_HOST)
    /* Read only the slave's local FRT. jo_get_ticks mutates a master clock. */
    worker.elapsed_ms=((jo_time_get_frc()-began)&65535u)/839u;
#else
    worker.elapsed_ms=0;
#endif
    __asm__ volatile ("" ::: "memory");
    worker.completed_generation=worker.generation;
    __asm__ volatile ("" ::: "memory");
}
#endif
static void demand_join(unsigned discard) {
#ifdef WF_ASSETS_WORKER
    if(worker_pending){
#ifdef WF_JO_ASSETS_TEST_WORKER
        wf_jo_assets_test_join();
#else
        jo_core_wait_for_slave();
#endif
        __asm__ volatile ("" ::: "memory");
        worker_pending=0;
        worker_ready=worker.completed_generation==worker_expected&&
            worker_binding==binding_generation&&worker_plan==demand_generation;
    }
    if(discard)worker_ready=0;
#else
    (void)discard;
#endif
}
void wf_jo_assets_demand_end(void) {
    demand_join(1);
#ifdef WF_ASSETS_WORKER
    demand_active=demand_count=demand_cursor=0;++demand_generation;
#endif
}
void wf_jo_assets_demand_frame(const uint16_t *sources,const uint8_t *touched,
                               unsigned epoch,unsigned count,const uint8_t *prepared) {
    wf_jo_assets_demand_end();
    cache_next_batch();
#ifdef WF_ASSETS_WORKER
    if(servicing||!wf_jo_assets_ready()||sprite_block_bytes!=4096u||!sources||
       !touched||!prepared||!epoch||epoch>255u||count>256u)return;
    for(unsigned first=0;first<count;first+=8u){
        unsigned missing=prepared[first>>3]^255u;
        for(unsigned i=first;missing&&i<count;++i,missing>>=1)
            if((missing&1u)&&touched[i]==epoch)
                demand_tiles[demand_count++]=sources[i];
    }
    demand_active=demand_count!=0;
#else
    (void)sources;(void)touched;(void)epoch;(void)count;(void)prepared;
#endif
}
static void demand_advance(uint16_t tile) {
#ifdef WF_ASSETS_WORKER
    if(!demand_active)return;
    if(servicing||demand_cursor>=demand_count||demand_tiles[demand_cursor]!=tile)
        wf_jo_assets_demand_end();
    else ++demand_cursor;
#else
    (void)tile;
#endif
}
static void demand_pair(unsigned current) {
#ifdef WF_ASSETS_WORKER
    if(!demand_active||worker_pending||worker_ready||servicing)return;
    for(unsigned i=demand_cursor;i<demand_count;++i){
        unsigned block=(unsigned)demand_tiles[i]>>sprite_tile_shift;
        if(block==current||block_slot[block]!=WF_NO_SLOT)continue;
        if(inflater_init(&worker.decoder)!=WF_JO_ASSET_OK)return; /* Serial fallback. */
        uint32_t start=read32(sprite_pack+WF_PACK_HEADER+block*4u);
        uint32_t end=read32(sprite_pack+WF_PACK_HEADER+(block+1u)*4u);
        worker.source=sprite_pack+start;worker.packed=end-start;worker.bytes=sprite_block_bytes;
        if(++worker_serial==0)++worker_serial;
        worker.generation=worker_expected=worker_serial;worker.completed_generation=0;
        worker.status=WF_JO_ASSET_DECODE;worker.elapsed_ms=0;
        worker_block=block;worker_binding=binding_generation;worker_plan=demand_generation;
        worker_pending=1;
        __asm__ volatile ("" ::: "memory");
#ifdef WF_JO_ASSETS_TEST_WORKER
        wf_jo_assets_test_submit(worker_decode);
#else
        jo_core_exec_on_slave(worker_decode);
#endif
        return;
    }
#else
    (void)current;
#endif
}
static void copy_decoded(void *out,const void *stage,unsigned bytes) {
#ifdef WF_JO_ASSETS_HOST
    memcpy(out,stage,bytes);
#else
    jo_memcpy(out,stage,bytes);
#endif
}
static int record_decode(int status,unsigned bytes,unsigned elapsed) {
#if defined(JO_COMPILE_WITH_PROFILER_SUPPORT) && !defined(WF_JO_ASSETS_HOST)
    ++wf_jo_asset_profile[0];wf_jo_asset_profile[1]+=bytes;
    wf_jo_asset_profile[2]+=elapsed;wf_jo_asset_profile[4]=elapsed;
    if(elapsed>wf_jo_asset_profile[3])wf_jo_asset_profile[3]=elapsed;
#else
    (void)bytes;(void)elapsed;
#endif
    if(status)++stats.decode_failures;
    return status;
}
static int decode_block(const uint8_t *source,unsigned packed,void *out,unsigned bytes,unsigned stage_output) {
#if defined(JO_COMPILE_WITH_PROFILER_SUPPORT) && !defined(WF_JO_ASSETS_HOST)
    unsigned began=jo_get_ticks();
#endif
    unsigned capacity=stage_output?sizeof master_decoder.stage:bytes;
    int status=inflate_private(&master_decoder,source,packed,
        stage_output?master_decoder.stage:out,capacity,bytes);
    if(!status&&stage_output)copy_decoded(out,master_decoder.stage,bytes);
    unsigned elapsed=0;
#if defined(JO_COMPILE_WITH_PROFILER_SUPPORT) && !defined(WF_JO_ASSETS_HOST)
    elapsed=jo_get_ticks()-began;
#endif
    return record_decode(status,bytes,elapsed);
}
static int decode_sprite(unsigned block,void *out) {
    demand_join(0);
#ifdef WF_ASSETS_WORKER
    if(worker_ready&&worker_block==block&&worker_binding==binding_generation&&
       worker_plan==demand_generation){
        worker_ready=0;
        if(!worker.status)copy_decoded(out,worker.decoder.stage,sprite_block_bytes);
        return record_decode(worker.status,sprite_block_bytes,worker.elapsed_ms);
    }
#endif
    demand_pair(block);
    uint32_t start=read32(sprite_pack+WF_PACK_HEADER+block*4u);
    uint32_t end=read32(sprite_pack+WF_PACK_HEADER+(block+1u)*4u);
    int status=decode_block(sprite_pack+start,end-start,out,sprite_block_bytes,1u);
    /* All exits, including corrupt master streams, drain the other CPU. */
    demand_join(0);
    return status;
}

static int sprite_block(unsigned block,const uint8_t **out) {
    unsigned slot=block_slot[block];
    unsigned hit=slot!=WF_NO_SLOT;
    if(slot!=WF_NO_SLOT)stats.block_hits++;
    else {
        stats.block_misses++;
        slot=0;
        for(unsigned i=0;i<cache_slots;i++) {
            if(slot_block[i]==WF_NO_SLOT){slot=i;break;}
            if(slot_age[i]<slot_age[slot])slot=i;
        }
        if(slot_block[slot]!=WF_NO_SLOT)block_slot[slot_block[slot]]=WF_NO_SLOT;
        slot_block[slot]=WF_NO_SLOT;cache_slot_forget(slot);
        int status=decode_sprite(block,cache_data+(size_t)slot*sprite_block_bytes);
        if(status){wf_jo_assets_demand_end();return status;}
        block_slot[block]=(uint16_t)slot;slot_block[slot]=(uint16_t)block;
        uint32_t generation=binding_generation;
        status=service_after_decode();if(status)return status;
        /* Hooks are normally non-reentrant. A defensive rebind/eviction must
         * never return a pointer describing the earlier asset generation. */
        if(generation!=binding_generation||!wf_jo_assets_ready()||
           block_slot[block]!=slot||slot_block[slot]!=block)return WF_JO_ASSET_SERVICE;
    }
    cache_touch(slot,hit);*out=cache_data+(size_t)slot*sprite_block_bytes;
    return WF_JO_ASSET_OK;
}
int wf_jo_assets_tile_read(uint16_t tile,const uint8_t **pixels) {
    if(!pixels){wf_jo_assets_demand_end();return WF_JO_ASSET_ARGUMENT;}
    *pixels=NULL;
    if(servicing)wf_jo_assets_demand_end();
    if(!wf_jo_assets_ready())return WF_JO_ASSET_UNLOADED;
    demand_advance(tile);
    const uint8_t *block;
    int status=sprite_block((unsigned)tile>>sprite_tile_shift,&block);
    if(status)return status;
    *pixels=block+(((unsigned)tile*128u)&(sprite_block_bytes-1u));return WF_JO_ASSET_OK;
}
int wf_jo_assets_rom_read(uint32_t offset,void *destination,size_t bytes) {
    demand_join(0);
    if(!wf_jo_assets_ready())return WF_JO_ASSET_UNLOADED;
    if((!destination&&bytes)||!valid_range(offset,bytes,WF_JO_ROM_BYTES))
        return WF_JO_ASSET_ARGUMENT;
    if(bytes)memcpy(destination,rom_image+offset,bytes);
    return WF_JO_ASSET_OK;
}
static int scene_read(const wf_scene_asset_t *scene,uint32_t at,void *destination,size_t bytes) {
    if(!valid_range(at,bytes,scene->bytes))return WF_JO_ASSET_ARGUMENT;
    uint8_t *out=destination;
    stats.scene_reads++;
    while(bytes) {
        unsigned block=at/WF_JO_SPRITE_BLOCK,within=at&(WF_JO_SPRITE_BLOCK-1u);
        unsigned decoded=scene->bytes-block*WF_JO_SPRITE_BLOCK;
        if(decoded>WF_JO_SPRITE_BLOCK)decoded=WF_JO_SPRITE_BLOCK;
        size_t count=decoded-within;if(count>bytes)count=bytes;
        unsigned index=scene->first_block+block;
        uint32_t start=read32(scene_pack+WF_SCENE_OFFSETS+index*8u);
        uint32_t end=read32(scene_pack+WF_SCENE_OFFSETS+index*8u+4u);
        void *target=out;
        if(within||count!=decoded) {
            /* An 8 KiB scene fragment can span two 4 KiB sprite slots. Invalidate
             * every borrowed slot before decoding; whole scenes bypass this. */
            for(unsigned slot=0,covered=0;covered<decoded;slot++,covered+=sprite_block_bytes){
                if(slot_block[slot]!=WF_NO_SLOT)block_slot[slot_block[slot]]=WF_NO_SLOT;
                slot_block[slot]=WF_NO_SLOT;cache_slot_forget(slot);
            }
            target=cache_data;
        }
        stats.scene_block_decodes++;
        int status=decode_block(scene_pack+start,end-start,target,decoded,1u);
        if(status)return status;
        if(target!=out)memcpy(out,(const uint8_t *)target+within,count);
        status=service_after_decode();if(status)return status;
        at+=(uint32_t)count;out+=count;bytes-=count;
    }
    return WF_JO_ASSET_OK;
}
int wf_jo_assets_file_read(const char *name,uint32_t offset,void *destination,size_t bytes) {
    if(!name||(!destination&&bytes))return WF_JO_ASSET_ARGUMENT;
    if(!strcmp(name,"ROM68K.BIN"))return wf_jo_assets_rom_read(offset,destination,bytes);
    wf_jo_assets_demand_end();
    if(!strcmp(name,"SPRITES.PAK")) {
        if(!wf_jo_assets_ready())return WF_JO_ASSET_UNLOADED;
        if(!valid_range(offset,bytes,WF_JO_SPRITE_BYTES))return WF_JO_ASSET_ARGUMENT;
        uint8_t *out=destination;
        while(bytes) {
            const uint8_t *block;unsigned index=offset>>sprite_block_shift;
            unsigned within=offset&(sprite_block_bytes-1u);
            size_t count=sprite_block_bytes-within;if(count>bytes)count=bytes;
            int status=sprite_block(index,&block);if(status)return status;
            memcpy(out,block+within,count);out+=count;offset+=(uint32_t)count;bytes-=count;
        }
        return WF_JO_ASSET_OK;
    }
    for(unsigned i=0;scene_pack&&i<WF_SCENES;i++) {
        if(!strcmp(name,scenes[i].name))return scene_read(&scenes[i],offset,destination,bytes);
    }
    return disc_read(name,offset,destination,bytes);
}
int wf_jo_assets_file_size(const char *name,uint32_t *bytes) {
    if(!name||!bytes)return WF_JO_ASSET_ARGUMENT;
    if(!strcmp(name,"ROM68K.BIN")||!strcmp(name,"SPRITES.PAK")) {
        if(!wf_jo_assets_ready())return WF_JO_ASSET_UNLOADED;
        *bytes=!strcmp(name,"ROM68K.BIN")?WF_JO_ROM_BYTES:WF_JO_SPRITE_BYTES;
        return WF_JO_ASSET_OK;
    }
    for(unsigned i=0;scene_pack&&i<WF_SCENES;i++) {
        if(!strcmp(name,scenes[i].name)) {
            *bytes=scenes[i].bytes;return WF_JO_ASSET_OK;
        }
    }
    return disc_size(name,bytes);
}
