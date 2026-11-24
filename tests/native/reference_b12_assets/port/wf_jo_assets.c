#include "wf_jo_assets.h"
#include "zlib/zlib.h"
#include <limits.h>
#include <string.h>
#ifdef WF_JO_ASSETS_HOST
#include <stdio.h>
#else
#include <jo/jo.h>
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
static wf_jo_asset_stats_t stats;
static int (*service_hook)(void *);
static void *service_user;
static unsigned servicing;
static void (*loading_observer)(const char *, size_t, size_t);
#if defined(JO_COMPILE_WITH_PROFILER_SUPPORT) && !defined(WF_JO_ASSETS_HOST)
/* Millisecond clock is extended in VBlank, so cold decodes exceeding the
 * profiler's 16-bit FRT period remain measurable. count/bytes/total/max/last. */
volatile uint32_t wf_jo_asset_profile[5];
#endif

void wf_jo_assets_set_service_hook(int (*service)(void *), void *user) {
    service_hook=service;service_user=user;
}
void wf_jo_assets_set_loading_observer(
        void (*observer)(const char *, size_t, size_t)) {
    loading_observer=observer;
}
static int service_after_decode(void) {
    if(!service_hook||servicing)return WF_JO_ASSET_OK;
    servicing=1;int result=service_hook(service_user);servicing=0;
    return result?WF_JO_ASSET_SERVICE:WF_JO_ASSET_OK;
}

/* A one-shot Z_FINISH call has its entire dictionary in the destination
 * block. zlib never needs a separate sliding-window allocation on that path.
 * Reject any extra allocation, including malformed streams needing a resume.
 * The state is reused between blocks, so no allocator runs during gameplay. */
static union {
    uint64_t alignment;
    void *pointer_alignment;
    uint8_t bytes[8192];
} inflate_storage;
/* One-shot output/history of sprite and scene streams live in HWRAM.
 * Publication to the selected LWRAM slot occurs only after stream validation.
 * No service callback runs until that copy and the slot map are complete. */
static uint8_t block_decode_stage[WF_JO_SPRITE_BLOCK] __attribute__((aligned(32)));
static z_stream inflater;
static unsigned inflate_storage_used, inflater_ready;

static voidpf inflate_alloc(voidpf opaque, uInt items, uInt size) {
    (void)opaque;
    if(inflate_storage_used || !size || items > sizeof inflate_storage.bytes / size)
        return Z_NULL;
    inflate_storage_used=1;
    memset(inflate_storage.bytes,0,(size_t)items*size);
    return inflate_storage.bytes;
}
static void inflate_free(voidpf opaque, voidpf memory) {
    (void)opaque;
    if(memory==inflate_storage.bytes)inflate_storage_used=0;
}
static int inflater_init(void) {
    if(inflater_ready)return WF_JO_ASSET_OK;
    memset(&inflater,0,sizeof inflater);
    inflater.zalloc=inflate_alloc;inflater.zfree=inflate_free;
    if(inflateInit(&inflater)!=Z_OK)return WF_JO_ASSET_DECODE;
    inflater_ready=1;return WF_JO_ASSET_OK;
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
    service_hook=NULL;service_user=NULL;servicing=0;
    rom_image=sprite_pack=NULL;sprite_pack_bytes=0;cache_data=NULL;cache_slots=0;
    cache_max_slots=0;cache_loan=NULL;
    cart_base=NULL;cart_capacity=cart_used=cache_bytes_used=0;cache_clock=0;
    memset(&stats,0,sizeof stats);
    scene_pack=NULL;scene_pack_bytes=0;
}

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
    invalidate();
    if(!rom||!pack||!cache||rn!=WF_JO_ROM_BYTES||cn<WF_JO_SPRITE_BLOCK)
        return WF_JO_ASSET_ARGUMENT;
    if(pn<WF_PACK_HEADER || pn>WF_JO_CART_BYTES-WF_JO_ROM_BYTES)
        return WF_JO_ASSET_CAPACITY;
    const uint8_t *p=pack;
    unsigned block_bytes=read32(p+8);
    if(block_bytes!=4096u&&block_bytes!=8192u)return WF_JO_ASSET_FORMAT;
    unsigned blocks=WF_JO_SPRITE_BYTES/block_bytes;
    if(memcmp(p,"WFSZ",4)||read32(p+4)!=1u||
       read32(p+12)!=WF_JO_SPRITE_BYTES||read32(p+16)!=blocks||read32(p+24)!=pn)
        return WF_JO_ASSET_FORMAT;
    uint32_t data_at=read32(p+20);
    const size_t expected_at=aligned32(WF_PACK_HEADER+(blocks+1u)*4u);
    if(data_at!=expected_at||data_at>pn||
       (uint32_t)adler32(1,p+WF_PACK_HEADER,data_at-WF_PACK_HEADER)!=read32(p+28))
        return WF_JO_ASSET_FORMAT;
    uint32_t previous=read32(p+WF_PACK_HEADER);
    if(previous!=data_at)return WF_JO_ASSET_FORMAT;
    for(unsigned block=0;block<blocks;block++) {
        uint32_t next=read32(p+WF_PACK_HEADER+(block+1u)*4u);
        if(next<=previous||next>pn)return WF_JO_ASSET_FORMAT;
        previous=next;
    }
    if(previous!=pn)return WF_JO_ASSET_FORMAT;
    size_t requested=cn/block_bytes;
    unsigned slots=requested>WF_JO_CACHE_MAX_SLOTS?WF_JO_CACHE_MAX_SLOTS:(unsigned)requested;
    size_t used=(size_t)slots*block_bytes;
    if(overlap(rom,rn,pack,pn)||overlap(cache,used,rom,rn)||overlap(cache,used,pack,pn))
        return WF_JO_ASSET_ARGUMENT;
    int status=inflater_init();if(status)return status;
    rom_image=rom;sprite_pack=p;sprite_pack_bytes=pn;
    sprite_block_bytes=block_bytes;sprite_block_shift=block_bytes==4096u?12u:13u;
    sprite_tile_shift=sprite_block_shift-7u;
    cache_data=cache;cache_slots=cache_max_slots=slots;cache_bytes_used=used;
    memset(block_slot,0xff,sizeof block_slot);memset(slot_block,0xff,sizeof slot_block);
    memset(slot_age,0,sizeof slot_age);
    return WF_JO_ASSET_OK;
}

int wf_jo_assets_load(void *cart,size_t capacity,void *cache,size_t cn) {
    invalidate();
    if(!cart||capacity>WF_JO_CART_BYTES||capacity<WF_JO_ROM_BYTES||
       !cache||cn<WF_JO_SPRITE_BLOCK)return WF_JO_ASSET_ARGUMENT;
    uint32_t rn,pn;
#ifndef WF_JO_ASSETS_HOST
    jo_fs_set_read_sector_count(16);
#endif
    if(disc_size("ROM68K.BIN",&rn)||disc_size("SPRITES.WFP",&pn))return WF_JO_ASSET_IO;
    if(rn!=WF_JO_ROM_BYTES)return WF_JO_ASSET_FORMAT;
    if(pn>capacity-WF_JO_ROM_BYTES||aligned32(pn)>capacity-WF_JO_ROM_BYTES)
        return WF_JO_ASSET_CAPACITY;
    uint8_t *p=cart;
    size_t used=WF_JO_ROM_BYTES+aligned32(pn);
    if(overlap(cart,used,cache,cn))return WF_JO_ASSET_ARGUMENT;
    if(disc_read("ROM68K.BIN",0,p,rn)||disc_read("SPRITES.WFP",0,p+rn,pn))
        return WF_JO_ASSET_IO;
    wf_jo_asset_stats_t loading_stats=stats;
    int status=wf_jo_assets_bind(p,rn,p+rn,pn,cache,cn);
    if(status)return status;
    stats=loading_stats;cart_base=p;cart_capacity=capacity;cart_used=used;
    return WF_JO_ASSET_OK;
}

int wf_jo_assets_preload_scenes(void *arena,size_t capacity) {
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
        slot_block[i]=WF_NO_SLOT;slot_age[i]=0;
    }
}
int wf_jo_assets_borrow_sprite_cache_tail(size_t bytes,void **out) {
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
    cache_slots=active;cache_loan=cache_data+(size_t)active*sprite_block_bytes;
    *out=cache_loan;
    return WF_JO_ASSET_OK;
}
int wf_jo_assets_return_sprite_cache_tail(void *storage) {
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

static int decode_block(const uint8_t *source,unsigned packed,void *out,unsigned bytes,unsigned stage_output) {
    if(inflateReset(&inflater)!=Z_OK)return WF_JO_ASSET_DECODE;
    inflater.next_in=(Bytef *)(uintptr_t)source;inflater.avail_in=packed;
    inflater.next_out=stage_output?block_decode_stage:out;inflater.avail_out=bytes;
#if defined(JO_COMPILE_WITH_PROFILER_SUPPORT) && !defined(WF_JO_ASSETS_HOST)
    unsigned began=jo_get_ticks();
#endif
    int result=inflate(&inflater,Z_FINISH);
    int complete=result==Z_STREAM_END&&!inflater.avail_in&&!inflater.avail_out&&inflater.total_out==bytes;
    if(complete&&stage_output) {
#ifdef WF_JO_ASSETS_HOST
        memcpy(out,block_decode_stage,bytes);
#else
        jo_memcpy(out,block_decode_stage,bytes);
#endif
    }
#if defined(JO_COMPILE_WITH_PROFILER_SUPPORT) && !defined(WF_JO_ASSETS_HOST)
    unsigned elapsed=jo_get_ticks()-began;
    ++wf_jo_asset_profile[0];wf_jo_asset_profile[1]+=bytes;
    wf_jo_asset_profile[2]+=elapsed;wf_jo_asset_profile[4]=elapsed;
    if(elapsed>wf_jo_asset_profile[3])wf_jo_asset_profile[3]=elapsed;
#endif
    if(!complete) {
        stats.decode_failures++;return WF_JO_ASSET_DECODE;
    }
    return WF_JO_ASSET_OK;
}

static int sprite_block(unsigned block,const uint8_t **out) {
    unsigned slot=block_slot[block];
    if(slot!=WF_NO_SLOT)stats.block_hits++;
    else {
        stats.block_misses++;
        slot=0;
        for(unsigned i=0;i<cache_slots;i++) {
            if(slot_block[i]==WF_NO_SLOT){slot=i;break;}
            if(slot_age[i]<slot_age[slot])slot=i;
        }
        if(slot_block[slot]!=WF_NO_SLOT)block_slot[slot_block[slot]]=WF_NO_SLOT;
        slot_block[slot]=WF_NO_SLOT;
        uint32_t start=read32(sprite_pack+WF_PACK_HEADER+block*4u);
        uint32_t end=read32(sprite_pack+WF_PACK_HEADER+(block+1u)*4u);
        int status=decode_block(sprite_pack+start,end-start,
            cache_data+(size_t)slot*sprite_block_bytes,sprite_block_bytes,1u);
        if(status)return status;
        block_slot[block]=(uint16_t)slot;slot_block[slot]=(uint16_t)block;
        status=service_after_decode();if(status)return status;
    }
    if(++cache_clock==0u) {
        /* Wrap affects replacement preference only, never asset identity. */
        memset(slot_age,0,sizeof slot_age);cache_clock=1u;
    }
    slot_age[slot]=cache_clock;*out=cache_data+(size_t)slot*sprite_block_bytes;
    return WF_JO_ASSET_OK;
}
int wf_jo_assets_tile_read(uint16_t tile,const uint8_t **pixels) {
    if(!pixels)return WF_JO_ASSET_ARGUMENT;
    if(!wf_jo_assets_ready())return WF_JO_ASSET_UNLOADED;
    const uint8_t *block;
    int status=sprite_block((unsigned)tile>>sprite_tile_shift,&block);
    if(status)return status;
    *pixels=block+(((unsigned)tile*128u)&(sprite_block_bytes-1u));return WF_JO_ASSET_OK;
}
int wf_jo_assets_rom_read(uint32_t offset,void *destination,size_t bytes) {
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
                slot_block[slot]=WF_NO_SLOT;
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
