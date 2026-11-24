#include "wf_jo_video.h"
#include "wf_jo_command_words.h"
#include "wf_saturn_memory.h"
#include <string.h>

int wf_jo_bank_reusable(unsigned used, uint32_t last_tag,
                        unsigned shown_valid, uint32_t shown_tag)
{
    return !used || (shown_valid && (int32_t)(shown_tag-last_tag)>0);
}
wf_status_t wf_jo_texture_offset(unsigned bank, unsigned slot, uint32_t *out)
{
    if(!out)return WF_BAD_STATE;
    if(bank>=WF_JO_TEXTURE_BANKS || slot>=WF_VDP1_STREAM_TILES_MAX)
        return WF_BLOCKED;
    *out=WF_JO_TEXTURE_FIRST+bank*WF_JO_TEXTURE_BANK_BYTES+slot*128u;
    return WF_OK;
}

static unsigned bytes_nonzero(const uint8_t *p,unsigned n)
{
    unsigned result=0;
    while(n--)result|=*p++;
    return result;
}

void wf_jo_crop_tile(uint8_t pixels[128], wf_jo_crop_t *crop)
{
    unsigned top=0,bottom=16,left=0,right=0;
    while(top<16&&!bytes_nonzero(pixels+top*8,8))++top;
    while(bottom>top&&!bytes_nonzero(pixels+(bottom-1)*8,8))--bottom;
    crop->top=(uint8_t)top;crop->height=(uint8_t)(bottom-top);
    crop->left=0;crop->width=16;
    if(top==bottom)return;
    for(unsigned y=top;y<bottom;++y){
        left|=bytes_nonzero(pixels+y*8,4);
        right|=bytes_nonzero(pixels+y*8+4,4);
    }
    if(!left||!right){
        unsigned column=left?0u:4u;
        crop->width=8;crop->left=(uint8_t)(column*2u);
        /* Copy forwards: the source is never below its destination. */
        for(unsigned y=top;y<bottom;++y)
            for(unsigned x=0;x<4;++x)
                pixels[(y-top)*4+x]=pixels[y*8+column+x];
    }
}
wf_status_t wf_jo_sprite_build(const wf_vdp1_sprite_cmd_t *source,
                               const wf_jo_crop_t *crop, uint32_t texture_offset,
                               wf_jo_command_t *out)
{
    if(!source||!crop||!out)return WF_BAD_STATE;
    if(source->cram_slot>=16 || (crop->width!=8&&crop->width!=16) ||
       crop->height>16 || crop->top+crop->height>16 ||
       crop->left+crop->width>16 || (texture_offset&127u) ||
       texture_offset<WF_JO_TEXTURE_FIRST ||
       texture_offset+128u>WF_JO_TEXTURE_FIRST+WF_JO_TEXTURE_BANKS*WF_JO_TEXTURE_BANK_BYTES)
        return WF_BLOCKED;
    wf_jo_command_write(out,(source->flip_x?0x10u:0u)|(source->flip_y?0x20u:0u),
        WF_SATURN_CRAM_SPRITE_FIRST+source->cram_slot*16u,
        (texture_offset>>3)+(crop->width==16?crop->top:0u),
        (crop->width<<5)|crop->height,
        source->x+(source->flip_x?16-crop->left-crop->width:crop->left),
        source->y-WF_JO_VISIBLE_Y+(source->flip_y?16-crop->top-crop->height:crop->top));
    return WF_OK;
}

volatile wf_jo_video_stats_t wf_jo_video_stats;

#ifdef WF_PLATFORM_SATURN
#ifdef WF_JO_VIDEO_TEST
#include "wf_jo_video_test_platform.h"
#else
#include <jo/jo.h>
#include <jo/wf_video.h>
#include <internal/core.h>
#include <internal/vdp1_command_pipeline.h>
#endif
#ifdef JO_COMPILE_WITH_DUAL_CPU_SUPPORT
#error "The Jo video workspace requires the single-CPU command/DMA boundary"
#endif

_Static_assert(WF_JO_TEXTURE_BANKS==JO_WF_VIDEO_BANKS,"VDP1 and VDP2 frame bank counts agree");
_Static_assert(sizeof(wf_jo_command_t)==sizeof(jo_vdp1_command),"VDP1 command ABI");
_Static_assert(WF_JO_TEXTURE_FIRST>=JO_VDP1_TEXTURE_DEF_BASE_ADDRESS,"Jo command tables protected");
_Static_assert(WF_JO_TEXTURE_FIRST+WF_JO_TEXTURE_BANKS*WF_JO_TEXTURE_BANK_BYTES<=0x7e4f8u,"Jo Gouraud tables protected");
_Static_assert(WF_JO_TEXTURE_BANK_BYTES>=WF_VDP1_STREAM_TILES_MAX*128u,"Complete frame fits texture bank");
_Static_assert(WF_JO_MAP_GROUP_BYTES*WF_JO_TEXTURE_BANKS<WF_SATURN_VDP2_BACK_COLOR,"Back color protected");

typedef struct video_bank {
    uint32_t last_tag,write_tag,revision,palette_version,map_version[3];
    uint32_t tile_version[WF_VDP1_STREAM_TILES_MAX];
    uint8_t used,reserved;
    uint16_t scroll[4];
    uint8_t priority,layers,opaque,sprites;
    uint32_t colors[1024] __attribute__((aligned(32)));
} video_bank_t;
typedef struct video_queue {
    uint32_t tag;
    unsigned bank;
    volatile unsigned ready;
} video_queue_t;
static video_bank_t banks[WF_JO_TEXTURE_BANKS];
static video_queue_t queue[8];
static uint16_t fg_names[1024] __attribute__((aligned(32)));
static uint16_t bg_names[1024] __attribute__((aligned(32)));
static uint16_t hud_names[4096] __attribute__((aligned(32)));
static uint32_t colors[1024] __attribute__((aligned(32)));
static uint32_t revision=1,palette_version=1,map_version[3]={1,1,1};
static uint32_t displayed_palette_version;
static uint16_t scroll_state[4];
static uint8_t priority_state=0x7b,layers_state,opaque_state,sprites_state;
static wf_jo_command_t *last_commands;
static unsigned commands_borrowed,borrowed_count,last_commands_valid;
static uint32_t borrowed_tag;
static unsigned last_count;
static const void *last_tiles;
static const uint32_t *last_tile_versions;
static size_t last_tile_bytes;
static int pending_bank=-1,last_bank=-1;
static volatile uint32_t shown_tag;
static volatile unsigned shown_valid,video_running,blank_pending,resume_valid;
static uint32_t resume_tag;
static int vblank_callback;
static uint32_t enqueued_tag;
static unsigned enqueued_valid;

static void video_vblank(void)
{
    unsigned int tag;
    if(!__jo_vdp1_displayed_frame_changed(&tag)){
        if(video_running&&shown_valid)for(unsigned i=0;i<8;++i)if(queue[i].ready){
            ++wf_jo_video_stats.pipeline_stalled_fields;break;
        }
        return;
    }
    shown_tag=tag;shown_valid=1;
    if(!video_running)return;
    for(unsigned i=0;i<8;++i)if(queue[i].ready&&queue[i].tag==tag){
        video_bank_t *state=&banks[queue[i].bank];
        unsigned bank=queue[i].bank;
        queue[i].ready=0;
        ++wf_jo_video_stats.displayed_frames;
        if(blank_pending&&(!resume_valid||tag!=resume_tag))return;
        /* The maps already reside in an inactive group. Palette and register
         * shadow are switched only when Jo displays this exact sprite frame. */
        if(displayed_palette_version!=state->palette_version){
            jo_dma_copy(state->colors,(void *)JO_VDP2_CRAM,sizeof state->colors);
            displayed_palette_version=state->palette_version;
            wf_jo_video_stats.palette_bytes+=sizeof state->colors;
        }
        jo_wf_vdp2_present(bank,state->scroll,state->priority,
                          state->layers,state->opaque,state->sprites);
        blank_pending=resume_valid=0;
        return;
    }
}

wf_status_t wf_jo_video_init(void)
{
    if(video_running)return WF_OK;
    if(wf_saturn_memory_contract_validate()!=WF_OK)return WF_BAD_STATE;
    memset(banks,0,sizeof banks);memset(queue,0,sizeof queue);
    memset(fg_names,0,sizeof fg_names);memset(bg_names,0,sizeof bg_names);
    memset(hud_names,0,sizeof hud_names);memset(colors,0,sizeof colors);
    memset(scroll_state,0,sizeof scroll_state);
    memset((void *)&wf_jo_video_stats,0,sizeof wf_jo_video_stats);
    revision=palette_version=1;map_version[0]=map_version[1]=map_version[2]=1;
    displayed_palette_version=0;
    last_count=0;last_commands=NULL;last_tiles=NULL;last_tile_versions=NULL;last_tile_bytes=0;
    commands_borrowed=borrowed_count=0;last_commands_valid=1;
    pending_bank=last_bank=-1;shown_valid=0;
    layers_state=opaque_state=sprites_state=0;priority_state=0x7b;enqueued_valid=0;
    blank_pending=1;resume_valid=0;video_running=1;
    wf_saturn_cram_reset();
    jo_wf_vdp2_init();
    if(!vblank_callback)vblank_callback=jo_core_add_vblank_callback(video_vblank);
    return vblank_callback?WF_OK:WF_BLOCKED;
}

wf_status_t wf_jo_video_acquire_bank(unsigned *bank)
{
    if(!bank||!video_running||pending_bank>=0)return WF_BAD_STATE;
    unsigned start=jo_core_get_vblank_count(),waited=0;
    for(;;){
        for(unsigned i=0;i<WF_JO_TEXTURE_BANKS;++i)if(!banks[i].reserved &&
            wf_jo_bank_reusable(banks[i].used,banks[i].last_tag,shown_valid,shown_tag)){
            banks[i].reserved=1;*bank=i;
            if(waited)wf_jo_video_stats.bank_wait_fields+=jo_core_get_vblank_count()-start;
            return WF_OK;
        }
        if(!waited){++wf_jo_video_stats.bank_waits;waited=1;}
        /* No normal extra field wait. IRQs can finish older submissions if
         * all four banks are temporarily in flight; diagnose a stopped GPU. */
        if(jo_core_get_vblank_count()-start>=8u){
            wf_jo_video_stats.bank_wait_fields+=jo_core_get_vblank_count()-start;
            return WF_BLOCKED;
        }
        __jo_core_pause();
    }
}

void wf_jo_video_release_bank(unsigned bank)
{
    if(bank<WF_JO_TEXTURE_BANKS)banks[bank].reserved=0;
}

wf_jo_command_t *wf_jo_video_commands_begin(unsigned count)
{
    if(!video_running||count>WF_VDP1_COMMANDS_MAX||pending_bank>=0||commands_borrowed||
       (enqueued_valid&&enqueued_tag==__jo_vdp1_frame_tag()))return NULL;
    /* Jo's sole CPU list is reusable only after end_frame waited for its DMA.
     * It is not cleared then; that same prefix also retains our complete last
     * frame. Nothing else may append while this video adapter owns display. */
    if(jo_vdp1_frame_command_count()!=0)return NULL;
    unsigned reserved;unsigned short *keys;
    jo_vdp1_command *dest=__jo_vdp1_reserve_commands(count,&keys,&reserved);
    if(reserved!=count)return NULL;
    last_commands=(wf_jo_command_t *)(void *)dest;
    borrowed_count=count;borrowed_tag=__jo_vdp1_frame_tag();
    commands_borrowed=1;last_commands_valid=0;
    return last_commands;
}
void wf_jo_video_commands_abort(void)
{commands_borrowed=borrowed_count=0;}
void wf_jo_video_forget_tile(unsigned slot)
{
    if(slot<WF_VDP1_STREAM_TILES_MAX)
        for(unsigned i=0;i<WF_JO_TEXTURE_BANKS;i++)banks[i].tile_version[slot]=0;
}

static wf_status_t append_commands(const wf_jo_command_t *commands,unsigned count)
{
    unsigned reserved;unsigned short *keys;
    if(count>WF_VDP1_COMMANDS_MAX||(!commands&&count))return WF_BAD_STATE;
    if(jo_vdp1_frame_command_count()!=0)return WF_BAD_STATE;
    jo_vdp1_command *dest=__jo_vdp1_reserve_commands(count,&keys,&reserved);
    if(reserved!=count)return WF_BLOCKED;
    /* Borrowed/repeated commands already occupy this exact prefix. Do not
     * pass identical (or overlapping) storage to Jo's restrict memcpy. */
    if(count&&(const void *)dest!=(const void *)commands){
        uintptr_t d=(uintptr_t)dest,s=(uintptr_t)commands;
        size_t bytes=count*sizeof *commands;
        if((d>s?d-s:s-d)<bytes)return WF_BAD_STATE;
        jo_memcpy(dest,commands,(unsigned)bytes);
    }
    for(unsigned i=0;i<count;++i)keys[i]=200; /* Stable arcade submission order. */
    __jo_vdp1_commit_commands(count);
    last_commands=(wf_jo_command_t *)(void *)dest;
    return WF_OK;
}

wf_status_t wf_jo_video_submit(unsigned bank,const wf_jo_command_t *commands,
                               unsigned count,const void *tiles,size_t bytes,
                               const uint32_t *tile_versions)
{
    if(!video_running||bank>=WF_JO_TEXTURE_BANKS||!banks[bank].reserved||bytes>WF_JO_TEXTURE_BANK_BYTES||
       (bytes&127u)||(bytes&&!tiles)||pending_bank>=0)return WF_BAD_STATE;
    if(commands_borrowed&&(commands!=last_commands||count>borrowed_count||
       borrowed_tag!=__jo_vdp1_frame_tag()))return WF_BAD_STATE;
    if(enqueued_valid&&enqueued_tag==__jo_vdp1_frame_tag())return WF_BAD_STATE;
    int used=jo_vdp1_frame_command_count(),capacity=jo_vdp1_command_max();
    if(used!=0)return WF_BAD_STATE;
    if(used<0||used>capacity||count>(unsigned)(capacity-used))return WF_BLOCKED;
    unsigned uploaded=0,transfers=0,slots=(unsigned)bytes/128u;
    for(unsigned slot=0;slot<slots;){
        if(tile_versions&&(!tile_versions[slot]||
           tile_versions[slot]==banks[bank].tile_version[slot])){++slot;continue;}
        unsigned first=slot++;
        while(slot<slots&&(!tile_versions||(tile_versions[slot]&&
              tile_versions[slot]!=banks[bank].tile_version[slot])))++slot;
        unsigned length=(slot-first)*128u;
        jo_dma_copy((const uint8_t *)tiles+first*128u,
                    (void *)(JO_VDP1_VRAM+WF_JO_TEXTURE_FIRST+bank*WF_JO_TEXTURE_BANK_BYTES+first*128u),length);
        for(unsigned i=first;i<slot;++i)banks[bank].tile_version[i]=tile_versions?tile_versions[i]:0;
        uploaded+=length;++transfers;
    }
    wf_status_t status=append_commands(commands,count);
    if(status!=WF_OK)return status;
    last_count=count;last_bank=(int)bank;pending_bank=(int)bank;
    commands_borrowed=borrowed_count=0;last_commands_valid=1;
    last_tiles=tiles;last_tile_versions=tile_versions;last_tile_bytes=bytes;
    banks[bank].last_tag=banks[bank].write_tag=__jo_vdp1_frame_tag();
    banks[bank].used=1;banks[bank].reserved=0;
    wf_jo_video_stats.last_texture_bytes=uploaded;
    wf_jo_video_stats.texture_bytes+=uploaded;wf_jo_video_stats.texture_transfers+=transfers;
    return WF_OK;
}

static wf_status_t enqueue_bank(unsigned bank)
{
    unsigned int tag=__jo_vdp1_frame_tag();
    if(enqueued_valid&&enqueued_tag==tag)return WF_OK;
    unsigned sr=__jo_core_disable_interrupts();
    for(unsigned i=0;i<8;++i)if(!queue[i].ready){
        queue[i].tag=tag;queue[i].bank=bank;
        if(blank_pending&&!resume_valid){resume_tag=tag;resume_valid=1;}
        queue[i].ready=1;enqueued_tag=tag;enqueued_valid=1;
        ++wf_jo_video_stats.submitted_frames;
        __jo_core_restore_interrupts(sr);
        return WF_OK;
    }
    __jo_core_restore_interrupts(sr);
    return WF_BLOCKED;
}

static wf_status_t repeat_existing(void)
{
    if(last_bank<0||!last_commands_valid)return WF_BAD_STATE;
    if(enqueued_valid&&enqueued_tag==__jo_vdp1_frame_tag())return WF_OK;
    wf_status_t status=append_commands(last_commands,last_count);
    if(status!=WF_OK)return status;
    unsigned bank=(unsigned)last_bank;
    banks[bank].last_tag=__jo_vdp1_frame_tag();
    status=enqueue_bank(bank);
    if(status==WF_OK){++wf_jo_video_stats.repeated_frames;wf_jo_video_stats.last_texture_bytes=0;}
    return status;
}

static void snapshot(unsigned bank)
{
    video_bank_t *state=&banks[bank];
    if(state->palette_version!=palette_version){
        jo_memcpy(state->colors,colors,sizeof colors);state->palette_version=palette_version;
    }
    jo_memcpy(state->scroll,scroll_state,sizeof scroll_state);
    state->priority=priority_state;state->layers=layers_state;
    state->opaque=opaque_state;state->sprites=sprites_state;
    unsigned base=JO_VDP2_VRAM_A1+bank*WF_JO_MAP_GROUP_BYTES;
    for(unsigned n=0;n<3;++n)if(state->map_version[n]!=map_version[n]){
        const void *source=n==0?(const void *)fg_names:n==1?(const void *)bg_names:(const void *)hud_names;
        unsigned bytes=n==2?sizeof hud_names:sizeof fg_names;
        jo_dma_copy(source,(void *)(base+n*0x2000u),bytes);
        state->map_version[n]=map_version[n];wf_jo_video_stats.map_bytes+=bytes;
    }
    state->revision=revision;
}

wf_status_t wf_jo_video_commit(void)
{
    if(!video_running||!last_commands_valid)return WF_BAD_STATE;
    if(pending_bank<0){
        if(last_bank>=0&&banks[last_bank].revision==revision)return repeat_existing();
        if(enqueued_valid&&enqueued_tag==__jo_vdp1_frame_tag()){
            /* Jo has not ended this building tag yet. A later HUD commit in
             * the same callback can still update its unpublished snapshot. */
            if(banks[last_bank].write_tag!=__jo_vdp1_frame_tag())return WF_BLOCKED;
            snapshot((unsigned)last_bank);
            if(blank_pending&&!resume_valid){resume_tag=__jo_vdp1_frame_tag();resume_valid=1;}
            return WF_OK;
        }
        unsigned bank;
        wf_status_t status=wf_jo_video_acquire_bank(&bank);
        if(status!=WF_OK)return status;
        int delta=0;
        if(last_bank>=0){
            delta=((int)bank-last_bank)*(int)(WF_JO_TEXTURE_BANK_BYTES/8u);
            for(unsigned i=0;i<last_count;++i)last_commands[i].srca=(uint16_t)(last_commands[i].srca+delta);
        }
        status=wf_jo_video_submit(bank,last_commands,last_count,last_tiles,last_tile_bytes,last_tile_versions);
        if(status!=WF_OK){
            for(unsigned i=0;i<last_count;++i)last_commands[i].srca=(uint16_t)(last_commands[i].srca-delta);
            wf_jo_video_release_bank(bank);return status;
        }
    }
    unsigned bank=(unsigned)pending_bank;
    snapshot(bank);
    wf_status_t status=enqueue_bank(bank);
    if(status==WF_OK)pending_bank=-1;
    return status;
}

wf_status_t wf_jo_video_repeat(void)
{
    /* A wait may follow a HUD/fade commit without another sprite render. */
    return wf_jo_video_commit();
}

int wf_jo_video_frame_queued(void)
{
    return pending_bank>=0 || (enqueued_valid&&enqueued_tag==__jo_vdp1_frame_tag());
}
int wf_jo_video_pending_changes(void)
{
    return video_running&&(last_bank<0||banks[last_bank].revision!=revision);
}

void wf_jo_video_blank(void)
{
    unsigned sr=__jo_core_disable_interrupts();
    blank_pending=1;resume_valid=0;jo_wf_vdp2_blank();
    __jo_core_restore_interrupts(sr);
}
void wf_jo_video_gate(void)
{
    unsigned sr=__jo_core_disable_interrupts();
    blank_pending=1;resume_valid=0;jo_wf_vdp2_gate();
    __jo_core_restore_interrupts(sr);
}
void wf_jo_video_diagnostics(void)
{
    video_running=0;jo_wf_vdp2_diagnostics();
}
volatile uint16_t *wf_jo_video_map(unsigned plane)
{
    return plane==0?fg_names:plane==1?bg_names:plane==2?hud_names:NULL;
}
void wf_jo_video_map_written(unsigned plane)
{
    if(plane<3){++map_version[plane];++revision;}
}
volatile uint8_t *wf_jo_video_characters(unsigned plane)
{
    return plane==0?(volatile uint8_t *)JO_VDP2_VRAM_A0:
           plane==1?(volatile uint8_t *)JO_VDP2_VRAM_B0:
           plane==2?(volatile uint8_t *)JO_VDP2_VRAM_B1:NULL;
}
void wf_jo_video_scroll(unsigned plane,unsigned x,unsigned y)
{
    if(plane<2){
        x&=511u;y&=511u;
        if(scroll_state[plane*2]!=x||scroll_state[plane*2+1]!=y){
            scroll_state[plane*2]=(uint16_t)x;scroll_state[plane*2+1]=(uint16_t)y;++revision;
        }
    }
}
void wf_jo_video_ring_priority(uint8_t priority,unsigned show)
{
    if(priority_state!=priority)++revision;
    priority_state=priority;
    if(show)wf_jo_video_layers(7u,priority==0x78u?2u:1u,priority==0x7cu?2u:3u);
}
void wf_jo_video_layers(unsigned layers,unsigned opaque,unsigned sprites)
{
    layers&=7u;opaque&=layers;sprites&=7u;
    if(layers_state!=layers||opaque_state!=opaque||sprites_state!=sprites){
        layers_state=(uint8_t)layers;opaque_state=(uint8_t)opaque;
        sprites_state=(uint8_t)sprites;++revision;
    }
}
void wf_jo_video_cram_write(unsigned entry,uint32_t rgb888)
{
    if(entry<1024&&colors[entry]!=rgb888){colors[entry]=rgb888;++palette_version;++revision;}
}
#endif
