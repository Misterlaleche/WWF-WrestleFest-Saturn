/* Backward-compatible portable presentation ABI; every Saturn operation is Jo.
 * The legacy symbol names are retained so the ROM-derived presenter is unchanged. */
#include "wf_saturn_yaul_backend.h"
#include "wf_jo_video.h"
#include "wf_jo_command_words.h"
#include <limits.h>
#include <string.h>

#ifdef WF_PLATFORM_SATURN
static wf_jo_command_t *commands;
#else
static wf_jo_command_t commands[WF_VDP1_COMMANDS_MAX];
#endif
static unsigned command_count,current_bank,pending_count,pending_capacity;
static uint16_t pending_slots[WF_JO_TEXTURE_STAGE_BYTES/128u];
static wf_saturn_yaul_backend_t *active_backend,*version_owner;
static wf_saturn_yaul_backend_t *preparation_owner;
static uint32_t preparation_generation;
static unsigned preparation_hits=UINT_MAX;
static unsigned mask(unsigned bit){
    static const uint8_t bits[8]={1,2,4,8,16,32,64,128};
    return bits[bit&7u];
}

static wf_status_t begin_frame(void *context,unsigned tiles,unsigned palettes,unsigned count)
{
    wf_saturn_yaul_backend_t *b=context;
    if(!b||b->active||active_backend)return WF_BAD_STATE;
    if(tiles>WF_VDP1_STREAM_TILES_MAX||palettes>16u||count>WF_VDP1_COMMANDS_MAX)
        return WF_BLOCKED;
    if(tiles&&(!b->hwr_stage||b->hwr_stage_bytes<128u))return WF_BLOCKED;
    wf_status_t status;
#ifdef WF_PLATFORM_SATURN
    status=wf_jo_video_acquire_bank(&current_bank);
    if(status!=WF_OK)return status;
    commands=wf_jo_video_commands_begin(count);
    if(!commands){wf_jo_video_release_bank(current_bank);return WF_BAD_STATE;}
#else
    current_bank=0;
#endif
    if(version_owner!=b){
        status=wf_jo_video_texture_invalidate();
        if(status!=WF_OK)goto failed;
        version_owner=b;
    }
    status=wf_jo_video_texture_begin(tiles);if(status!=WF_OK)goto failed;
    b->expected_tiles=tiles;b->expected_palettes=palettes;b->expected_cmds=count;
    b->emitted_tiles=b->emitted_palettes=b->emitted_cmds=0;
    b->palette_mask=0;b->active=1;active_backend=b;command_count=pending_count=0;
    preparation_owner=NULL;preparation_hits=UINT_MAX;
    pending_capacity=(unsigned)(b->hwr_stage_bytes/128u);
    if(pending_capacity>WF_JO_TEXTURE_STAGE_BYTES/128u)pending_capacity=WF_JO_TEXTURE_STAGE_BYTES/128u;
    memset(b->dirty,0,sizeof b->dirty);memset(b->resident,0,sizeof b->resident);
    memset(b->rom_identity,0,sizeof b->rom_identity);
    return WF_OK;
failed:
#ifdef WF_PLATFORM_SATURN
    wf_jo_video_commands_abort();wf_jo_video_release_bank(current_bank);
#endif
    return status;
}

wf_status_t wf_jo_backend_protect_rom_tiles(wf_saturn_yaul_backend_t *b,
    const uint16_t *sources,const uint8_t *touched,unsigned epoch,unsigned count)
{
    if(!b||active_backend!=b||!b->active||!sources||!touched||!epoch||epoch>255u||
       count!=b->expected_tiles||b->emitted_tiles)return WF_BAD_STATE;
    preparation_owner=NULL;preparation_hits=UINT_MAX;
    for(unsigned i=0;i<count;++i)if(touched[i]==epoch){
        wf_jo_texture_ref_t ref;unsigned hit;
        wf_status_t status=wf_jo_video_texture_protect(i,sources[i],&ref,&hit);
        if(status!=WF_OK)return status;
        if(hit){
            b->crop_meta[i]=ref.meta;b->crop_height[i]=ref.height;b->rom_source[i]=sources[i];
            b->resident[i>>3]|=(uint8_t)mask(i);
            b->rom_identity[i>>3]|=(uint8_t)mask(i);
        }
    }
    return WF_OK;
}

wf_status_t wf_jo_backend_prepare_rom_tiles(wf_saturn_yaul_backend_t *b,
    const uint16_t *sources,const uint8_t *touched,unsigned epoch,unsigned count,
    unsigned unchanged,uint32_t generation)
{
    if(!b||active_backend!=b||!b->active||!sources||!touched||!epoch||epoch>255u||
       count!=b->expected_tiles||b->emitted_tiles)return WF_BAD_STATE;
    preparation_hits=UINT_MAX;
    if(unchanged&&wf_jo_video_texture_preparation_available(b,generation,count)){
        uint8_t active[WF_VDP1_STREAM_TILES_MAX/8u]={0};
        unsigned hits=0;
        for(unsigned i=0;i<count;++i)if(touched[i]==epoch){active[i>>3]|=(uint8_t)mask(i);++hits;}
        if(wf_jo_video_texture_restore_preparation(b,generation,active)){
            memcpy(b->resident,active,sizeof b->resident);
            memcpy(b->rom_identity,active,sizeof b->rom_identity);
            preparation_owner=b;preparation_generation=generation;preparation_hits=hits;return WF_OK;
        }
    }
    wf_status_t status=wf_jo_backend_protect_rom_tiles(b,sources,touched,epoch,count);
    if(status==WF_OK){preparation_owner=b;preparation_generation=generation;}
    return status;
}
int wf_jo_backend_consume_prepared_rom_tiles(wf_saturn_yaul_backend_t *b)
{
    if(!b||active_backend!=b||!b->active||preparation_owner!=b||
       preparation_hits==UINT_MAX||b->emitted_tiles||pending_count||
       !wf_jo_video_texture_consume_preparation(b,preparation_generation,b->expected_tiles))return 0;
    /* The private caller has not yielded or mutated its proof. This is the
     * exact all-resident result of scalar consumption, including wrap. */
    memcpy(b->dirty,b->resident,sizeof b->dirty);b->emitted_tiles=b->expected_tiles;
    b->texture_reused+=preparation_hits;preparation_hits=UINT_MAX;return 1;
}

static wf_status_t flush_tiles(wf_saturn_yaul_backend_t *b)
{
    const uint16_t *physical=wf_jo_video_texture_frame_slots();
    for(unsigned first=0;first<pending_count;){
        unsigned end=first+1u,at=physical[pending_slots[first]];
        while(end<pending_count&&physical[pending_slots[end]]==at+end-first)++end;
        wf_status_t status=wf_jo_video_texture_transfer(at,b->hwr_stage+first*128u,end-first);
        if(status!=WF_OK)return status;
        for(unsigned i=first;i<end;++i){
            unsigned slot=pending_slots[i];
            status=wf_jo_video_texture_ready(slot,b->rom_source[slot],
                !!(b->rom_identity[slot>>3]&mask(slot)),b->crop_meta[slot],b->crop_height[slot]);
            if(status!=WF_OK)return status;
        }
        first=end;
    }
    pending_count=0;return WF_OK;
}
static wf_status_t stage_tile(wf_saturn_yaul_backend_t *b,uint16_t slot,
                               const uint8_t *pixels,uint16_t source,unsigned immutable)
{
    if(pending_count==pending_capacity){
        wf_status_t status=flush_tiles(b);if(status!=WF_OK)return status;
    }
    uint8_t *dest=b->hwr_stage+pending_count*128u;
    memcpy(dest,pixels,128u);
    wf_jo_crop_t crop;wf_jo_crop_tile(dest,&crop);
    b->crop_meta[slot]=(uint8_t)(crop.top|(crop.width==8?0x20u:0u)|(crop.left?0x40u:0u));
    b->crop_height[slot]=crop.height;b->rom_source[slot]=source;
    b->dirty[slot>>3]|=(uint8_t)mask(slot);
    if(immutable)b->rom_identity[slot>>3]|=(uint8_t)mask(slot);
    pending_slots[pending_count++]=slot;++b->emitted_tiles;
    return WF_OK;
}
static wf_status_t upload_tile(void *context,uint16_t slot,const uint8_t *source,size_t bytes)
{
    wf_saturn_yaul_backend_t *b=context;
    if(!b||active_backend!=b||!b->active||!source||bytes!=128u||
       slot!=b->emitted_tiles||slot>=b->expected_tiles)return WF_BAD_STATE;
    preparation_hits=UINT_MAX;
    wf_jo_texture_ref_t ref;unsigned hit;
    wf_status_t status=wf_jo_video_texture_resolve(slot,0,0,&ref,&hit);
    if(status!=WF_OK)return status;
    return stage_tile(b,slot,source,0,0);
}
wf_status_t wf_saturn_yaul_upload_rom_tile(wf_saturn_yaul_backend_t *b,uint16_t slot,
                                         uint16_t source,wf_status_t (*read_tile)(uint16_t,const uint8_t **))
{
    if(!b||active_backend!=b||!b->active||!read_tile||slot!=b->emitted_tiles||slot>=b->expected_tiles)
        return WF_BAD_STATE;
    preparation_hits=UINT_MAX;
    /* The complete-frame prepass already checked this identity, pinned its
     * physical bytes and copied its crop. Consumption is separate: an unused
     * prepared slot must not become an emitted sprite merely by being pinned. */
    if(b->resident[slot>>3]&mask(slot)){
        if(b->rom_source[slot]!=source)return WF_BAD_STATE;
        b->dirty[slot>>3]|=(uint8_t)mask(slot);
        ++b->emitted_tiles;++b->texture_reused;return WF_OK;
    }
    wf_jo_texture_ref_t ref;unsigned hit;
    /* Probe without reserving a miss across the read callback. A callback may
     * abort/reinitialize this backend; the post-read guards and fresh resolve
     * preserve the scalar ABI's validity checks before writing any pixels. */
    wf_status_t status=wf_jo_video_texture_protect(slot,source,&ref,&hit);
    if(status!=WF_OK)return status;
    if(!hit){
        const uint8_t *pixels=NULL;
        status=read_tile(source,&pixels);if(status!=WF_OK)return status;
        if(active_backend!=b||!b->active||!pixels||slot!=b->emitted_tiles||
           slot>=b->expected_tiles)return WF_BAD_STATE;
        status=wf_jo_video_texture_resolve(slot,source,1,&ref,&hit);
        if(status!=WF_OK)return status;
        if(!hit)return stage_tile(b,slot,pixels,source,1);
    }
    b->crop_meta[slot]=ref.meta;b->crop_height[slot]=ref.height;b->rom_source[slot]=source;
    b->rom_identity[slot>>3]|=(uint8_t)mask(slot);
    b->resident[slot>>3]|=(uint8_t)mask(slot);b->dirty[slot>>3]|=(uint8_t)mask(slot);
    ++b->emitted_tiles;++b->texture_reused;return WF_OK;
}
wf_status_t wf_jo_backend_skip_tile(wf_saturn_yaul_backend_t *b,uint16_t slot)
{
    if(!b||active_backend!=b||!b->active||slot!=b->emitted_tiles||slot>=b->expected_tiles)
        return WF_BAD_STATE;
    preparation_hits=UINT_MAX;
    ++b->emitted_tiles;return WF_OK;
}
static int overlaps_backend(const wf_saturn_yaul_backend_t *b,const void *p,size_t bytes)
{
    uintptr_t base=(uintptr_t)b,at=(uintptr_t)p;
#ifdef WF_PLATFORM_SATURN
    /* Conservatively use the scalar path outside canonical high work RAM.
     * Cache-through and other hardware mirrors can alias despite different
     * pointer values. The private presenter arrays are all in this window. */
    if(bytes&&(base<UINT32_C(0x06000000)||
       base>UINT32_C(0x06100000)-sizeof *b||bytes>UINT32_C(0x00100000)||
       at<UINT32_C(0x06000000)||at>UINT32_C(0x06100000)-bytes))return 1;
#endif
    return bytes&&(at>=base?at-base<sizeof *b:base-at<bytes);
}
/* No callback can observe a partly consumed prefix without first receiving
 * its counters. Keep them local between callbacks when the input arrays do
 * not inspect the backend bytes changed by that prefix. */
static wf_status_t consume_distinct(wf_saturn_yaul_backend_t *b,
    const uint16_t *sources,const uint8_t *touched,unsigned epoch,unsigned count,
    wf_status_t (*read_tile)(uint16_t,const uint8_t **))
{
    unsigned limit=count,emitted=0;
    uint32_t reused=b->texture_reused;
    for(unsigned slot=0;slot<limit;++slot){
        if(touched[slot]!=epoch){++emitted;continue;}
        unsigned bit=mask(slot);
        if(b->resident[slot>>3]&bit){
            if(b->rom_source[slot]!=sources[slot]){
                b->emitted_tiles=emitted;b->texture_reused=reused;
                return WF_BAD_STATE;
            }
            b->dirty[slot>>3]|=(uint8_t)bit;
            ++emitted;++reused;
        }else{
            b->emitted_tiles=emitted;b->texture_reused=reused;
            wf_status_t status=wf_saturn_yaul_upload_rom_tile(b,(uint16_t)slot,sources[slot],read_tile);
            /* A failed callback owns its final observable state too. */
            if(status!=WF_OK)return status;
            emitted=b->emitted_tiles;reused=b->texture_reused;
            unsigned next=slot+1u;
            if(next<count){
                if(active_backend!=b||!b->active||emitted!=next||
                   next>=b->expected_tiles)return WF_BAD_STATE;
                limit=b->expected_tiles<count?b->expected_tiles:count;
            }
        }
    }
    b->emitted_tiles=emitted;b->texture_reused=reused;
    return limit==count?WF_OK:WF_BAD_STATE;
}
wf_status_t wf_jo_backend_consume_rom_tiles(wf_saturn_yaul_backend_t *b,
    const uint16_t *sources,const uint8_t *touched,unsigned epoch,unsigned count,
    wf_status_t (*read_tile)(uint16_t,const uint8_t **))
{
    if(!b||active_backend!=b||!b->active||(!sources&&count)||(!touched&&count)||
       !epoch||epoch>255u||count!=b->expected_tiles||b->emitted_tiles)return WF_BAD_STATE;
    preparation_hits=UINT_MAX;
    if(!overlaps_backend(b,sources,(size_t)count*sizeof *sources)&&
       !overlaps_backend(b,touched,count))
        return consume_distinct(b,sources,touched,epoch,count,read_tile);
    unsigned limit=count;
    for(unsigned slot=0;slot<limit;++slot){
        if(touched[slot]!=epoch){++b->emitted_tiles;continue;}
        unsigned bit=mask(slot);
        if(b->resident[slot>>3]&bit){
            if(b->rom_source[slot]!=sources[slot])return WF_BAD_STATE;
            b->dirty[slot>>3]|=(uint8_t)bit;
            ++b->emitted_tiles;++b->texture_reused;
        }else{
            wf_status_t status=wf_saturn_yaul_upload_rom_tile(b,(uint16_t)slot,sources[slot],read_tile);
            if(status!=WF_OK)return status;
            unsigned next=slot+1u;
            if(next<count){
                /* A read callback may change backend state. Match the next
                 * scalar operation's rejection and its consumed prefix. */
                if(active_backend!=b||!b->active||b->emitted_tiles!=next||
                   next>=b->expected_tiles)return WF_BAD_STATE;
                limit=b->expected_tiles<count?b->expected_tiles:count;
            }
        }
    }
    return limit==count?WF_OK:WF_BAD_STATE;
}
static wf_status_t upload_palette(void *context,uint8_t bank,const uint8_t *source,size_t bytes)
{
    wf_saturn_yaul_backend_t *b=context;
    if(!b||active_backend!=b||!b->active||!source||bytes!=32u||bank>=16||
       b->emitted_palettes>=b->expected_palettes||(b->palette_mask&(1u<<bank)))return WF_BAD_STATE;
    memcpy(b->palette_stage[bank],source,32);
    b->palette_mask|=(uint16_t)(1u<<bank);++b->emitted_palettes;
    return WF_OK;
}
static wf_status_t emit_sprite(void *context,const wf_vdp1_sprite_cmd_t *sprite)
{
    wf_saturn_yaul_backend_t *b=context;
    if(!b||active_backend!=b||!b->active||!sprite||b->emitted_tiles!=b->expected_tiles||
       sprite->stream_tile>=b->expected_tiles||b->emitted_cmds>=b->expected_cmds||
       !(b->dirty[sprite->stream_tile>>3]&mask(sprite->stream_tile)))return WF_BAD_STATE;
    unsigned slot=sprite->stream_tile,meta=b->crop_meta[slot];
    wf_jo_crop_t crop={(uint8_t)(meta&31u),b->crop_height[slot],
                       (uint8_t)((meta&64u)?8u:0u),(uint8_t)((meta&32u)?8u:16u)};
    unsigned physical=wf_jo_video_texture_frame_slots()[slot];
    if(physical>=WF_JO_GPU_TILES)return WF_BAD_STATE;
    if(sprite->cram_slot>=16)return WF_BLOCKED;
    if(crop.height){
        if(command_count>=WF_VDP1_COMMANDS_MAX)return WF_BLOCKED;
        wf_status_t status=wf_jo_sprite_build(sprite,&crop,WF_JO_TEXTURE_FIRST+physical*128u,
                                             &commands[command_count]);
        if(status!=WF_OK)return status;
        ++command_count;
    }
    ++b->emitted_cmds;return WF_OK;
}
wf_status_t wf_jo_backend_emit_sprites(wf_saturn_yaul_backend_t *b,
                                      const wf_vdp1_sprite_cmd_t *sprites,unsigned count)
{
    if(!b||active_backend!=b||!b->active||(!sprites&&count)||
       b->emitted_tiles!=b->expected_tiles||b->emitted_cmds>b->expected_cmds||
       count>b->expected_cmds-b->emitted_cmds)return WF_BAD_STATE;
    const uint16_t *physical=wf_jo_video_texture_frame_slots();
    wf_jo_command_t *out=commands+command_count;
    for(unsigned i=0;i<count;++i){
        const wf_vdp1_sprite_cmd_t *sprite=sprites+i;
        unsigned slot=sprite->stream_tile;
        if(slot>=b->expected_tiles||!(b->dirty[slot>>3]&mask(slot))||
           physical[slot]>=WF_JO_GPU_TILES)return WF_BAD_STATE;
        if(sprite->cram_slot>=16u)return WF_BLOCKED;
        unsigned height=b->crop_height[slot];if(!height)continue;
        unsigned meta=b->crop_meta[slot],top=meta&31u;
        unsigned size=0x200u,address=WF_JO_TEXTURE_FIRST/8u+physical[slot]*16u;
        int x=sprite->x,y=sprite->y-WF_JO_VISIBLE_Y;
        if(meta&32u){
            size=0x100u;
            unsigned left=(meta&64u)?8u:0u;
            x+=(int)(sprite->flip_x?8u-left:left);
        }else address+=top;
        y+=(int)(sprite->flip_y?16u-top-height:top);
        wf_jo_command_write(out,(sprite->flip_x?0x10u:0u)|(sprite->flip_y?0x20u:0u),
            WF_SATURN_CRAM_SPRITE_FIRST+sprite->cram_slot*16u,address,size|height,x,y);
        ++out;
    }
    command_count=(unsigned)(out-commands);b->emitted_cmds+=count;
    return WF_OK;
}
static void abort_frame(void *context)
{
    wf_saturn_yaul_backend_t *b=context;
    if(!b||active_backend!=b)return;
#ifdef WF_PLATFORM_SATURN
    wf_jo_video_commands_abort();wf_jo_video_release_bank(current_bank);
#else
    wf_jo_video_texture_abort();
#endif
    b->active=0;active_backend=NULL;command_count=pending_count=0;preparation_owner=NULL;
    b->expected_tiles=b->expected_palettes=b->expected_cmds=0;
    b->emitted_tiles=b->emitted_palettes=b->emitted_cmds=0;
}
static wf_status_t end_frame(void *context)
{
    wf_saturn_yaul_backend_t *b=context;
    if(!b||active_backend!=b||!b->active)return WF_BAD_STATE;
    if(b->emitted_tiles!=b->expected_tiles||b->emitted_palettes!=b->expected_palettes||
       b->emitted_cmds!=b->expected_cmds){abort_frame(b);return WF_BAD_STATE;}
    wf_status_t status=flush_tiles(b);
    if(status==WF_OK)status=wf_jo_video_submit(current_bank,commands,command_count);
    if(status!=WF_OK){abort_frame(b);return status;}
    if(preparation_owner==b&&!memcmp(b->dirty,b->rom_identity,sizeof b->dirty))
        wf_jo_video_texture_remember_preparation(b,preparation_generation,b->dirty);
    preparation_owner=NULL;
    b->texture_bytes+=wf_jo_video_stats.last_texture_bytes;
    b->texture_transfers=wf_jo_video_stats.texture_transfers;
    if(b->palette_mask)for(unsigned bank=0,bit=1u;bank<16;++bank,bit<<=1)
        if(b->palette_mask&bit)
            wf_saturn_cram_write_bank((uint16_t)(WF_SATURN_CRAM_SPRITE_FIRST+bank*16u),b->palette_stage[bank]);
    b->active=0;active_backend=NULL;
    /* The caller publishes all VDP2 state with wf_jo_video_commit().
     * jo_core_run() is the sole synchronizer, including repeat frames. */
    return WF_OK;
}

static const wf_saturn_render_ops_t operations={begin_frame,upload_tile,upload_palette,emit_sprite,end_frame,abort_frame};
void wf_saturn_yaul_backend_init(wf_saturn_yaul_backend_t *b,void *stage,size_t bytes)
{
    if(!b)return;
    if(active_backend==b)abort_frame(b);
    if(version_owner==b)version_owner=NULL;
    memset(b,0,sizeof *b);b->hwr_stage=stage;b->hwr_stage_bytes=bytes;
}
const wf_saturn_render_ops_t *wf_saturn_yaul_render_ops(void){return &operations;}
wf_status_t wf_saturn_yaul_present_plan(wf_saturn_yaul_backend_t *b,const wf_vdp1_frame_plan_t *p,const wf_vdp1_assets_t *a)
{return wf_saturn_render_submit(p,a,&operations,b);}
wf_status_t wf_saturn_yaul_video_init(void)
{
#ifdef WF_PLATFORM_SATURN
    return wf_jo_video_init();
#else
    return wf_saturn_memory_contract_validate();
#endif
}
#ifndef WF_PLATFORM_SATURN
uint64_t wf_jo_backend_frame_digest(void)
{
    uint64_t hash=UINT64_C(14695981039346656037);
    for(unsigned i=0;i<command_count;i++){
        const wf_jo_command_t *command=&commands[i];
        const uint16_t words[]={command->ctrl,command->pmod,command->colr,command->size,
            (uint16_t)command->xa,(uint16_t)command->ya};
        for(unsigned word=0;word<sizeof words/sizeof words[0];word++){
            hash=(hash^(words[word]>>8))*UINT64_C(1099511628211);
            hash=(hash^(words[word]&255u))*UINT64_C(1099511628211);
        }
        unsigned offset=(unsigned)command->srca*8u-WF_JO_TEXTURE_FIRST;
        unsigned bytes=((command->size>>8)&0x3fu)*4u*(command->size&255u);
        const uint8_t *pixels=wf_jo_video_texture_pixels(offset,bytes);
        if(!pixels)return 0;
        for(unsigned byte=0;byte<bytes;byte++)
            hash=(hash^pixels[byte])*UINT64_C(1099511628211);
    }
    return hash;
}
#endif
