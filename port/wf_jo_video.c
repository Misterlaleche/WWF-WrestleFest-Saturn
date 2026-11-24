#include "wf_jo_video.h"
#include "wf_jo_command_words.h"
#include "wf_saturn_memory.h"
#include <string.h>

static wf_jo_texture_cache_t texture_cache;
static unsigned texture_cache_initialized;
static void texture_reset(void);
#ifndef WF_PLATFORM_SATURN
static uint8_t host_textures[WF_JO_TEXTURE_BYTES];
static uint32_t host_frame_tag=1;
#endif
_Static_assert(sizeof texture_cache+WF_JO_TEXTURE_STAGE_BYTES<=50856u,
               "shared texture metadata and transfer stage budget");

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
    if(!((uintptr_t)pixels&3u)){
        typedef uint32_t crop_word_t __attribute__((may_alias));
        crop_word_t *words=(crop_word_t *)(void *)pixels;
        unsigned top=0,bottom=16;uint32_t left=0,right=0;
        while(top<16&&!(words[top*2u]|words[top*2u+1u]))++top;
        while(bottom>top&&!(words[(bottom-1u)*2u]|words[(bottom-1u)*2u+1u]))--bottom;
        /* Keep metadata writes in the byte routine's order: callers can
         * legally place this byte-aligned metadata inside the tile buffer. */
        crop->top=(uint8_t)top;crop->height=(uint8_t)(bottom-top);
        crop->left=0;crop->width=16;
        if(top==bottom)return;
        for(unsigned y=top;y<bottom;++y){left|=words[y*2u];right|=words[y*2u+1u];}
        if(!left||!right){
            unsigned column=left?0u:1u;
            crop->width=8;crop->left=(uint8_t)(column*8u);
            for(unsigned y=top;y<bottom;++y)words[y-top]=words[y*2u+column];
        }
        return;
    }
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
       texture_offset+128u>WF_JO_TEXTURE_FIRST+WF_JO_TEXTURE_BYTES)
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
extern void jo_core_set_frame_overlap(bool allow);
extern void __jo_core_synch(void);
#else
#include <jo/jo.h>
#include <jo/wf_video.h>
#include <internal/core.h>
#include <internal/vdp1_command_pipeline.h>
#endif
#if defined(JO_COMPILE_WITH_DUAL_CPU_SUPPORT) && !defined(WF_JO_DUAL_COMMAND_BUFFERS)
#error "Dual Jo video requires the audited two-buffer command/DMA contract"
#endif

_Static_assert(WF_JO_TEXTURE_BANKS==JO_WF_VIDEO_BANKS,"VDP1 and VDP2 frame bank counts agree");
_Static_assert(sizeof(wf_jo_command_t)==sizeof(jo_vdp1_command),"VDP1 command ABI");
_Static_assert(WF_JO_TEXTURE_FIRST>=JO_VDP1_TEXTURE_DEF_BASE_ADDRESS,"Jo command tables protected");
_Static_assert(WF_JO_TEXTURE_FIRST+WF_JO_TEXTURE_BYTES<=0x7e4f8u,"Jo Gouraud tables protected");
_Static_assert(WF_JO_TEXTURE_BANK_BYTES>=WF_VDP1_STREAM_TILES_MAX*128u,"Complete frame fits texture bank");
_Static_assert(WF_JO_MAP_GROUP_BYTES*WF_JO_TEXTURE_BANKS<WF_SATURN_VDP2_BACK_COLOR,"Back color protected");

typedef struct video_bank {
    uint32_t last_tag,write_tag,revision,palette_version,map_version[3];
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
static int pending_bank=-1,last_bank=-1;
static volatile uint32_t shown_tag,shown_field;
static volatile unsigned shown_valid,video_running,blank_pending,resume_valid;
static uint32_t resume_tag;
static int vblank_callback;
static uint32_t enqueued_tag;
static unsigned enqueued_valid;
/* A failed borrow invalidates ordinary replay. For closing an overlap epoch
 * only, retain the preceding committed prefix in the OTHER SDK CPU buffer.
 * This is a read-only pointer, not a third command buffer or restored replay. */
static const wf_jo_command_t *committed_commands;
static unsigned committed_count,committed_bank,committed_valid;
static unsigned overlap_active,overlap_eligible,overlap_draining;
static unsigned drain_started,drain_successor;
static uint32_t overlap_eligible_tag,drain_tag;

static void video_vblank(void)
{
    unsigned int tag;
    if(!__jo_vdp1_displayed_frame_changed(&tag)){
        if(video_running&&shown_valid)for(unsigned i=0;i<8;++i)if(queue[i].ready){
            ++wf_jo_video_stats.pipeline_stalled_fields;break;
        }
        return;
    }
    shown_tag=tag;shown_field=jo_core_get_vblank_count();shown_valid=1;
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
    last_count=0;last_commands=NULL;texture_reset();
    commands_borrowed=borrowed_count=0;last_commands_valid=1;
    pending_bank=last_bank=-1;shown_valid=0;
    layers_state=opaque_state=sprites_state=0;priority_state=0x7b;enqueued_valid=0;
    blank_pending=1;resume_valid=0;video_running=1;
    overlap_active=overlap_eligible=overlap_draining=drain_started=drain_successor=0;
    committed_valid=0;
    jo_core_set_frame_overlap(false);
    wf_saturn_cram_reset();
    jo_wf_vdp2_init();
    if(!vblank_callback)vblank_callback=jo_core_add_vblank_callback(video_vblank);
    return vblank_callback?WF_OK:WF_BLOCKED;
}

wf_status_t wf_jo_video_acquire_bank(unsigned *bank)
{
    jo_core_set_frame_overlap(false);overlap_eligible=0;
    if(!bank||!video_running||pending_bank>=0||overlap_draining||drain_started)return WF_BAD_STATE;
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
    jo_core_set_frame_overlap(false);overlap_eligible=0;
    if(!video_running||count>WF_VDP1_COMMANDS_MAX||pending_bank>=0||commands_borrowed||
       overlap_draining||drain_started||
       (enqueued_valid&&enqueued_tag==__jo_vdp1_frame_tag()))return NULL;
    /* Jo has waited for this current CPU buffer's old sort/DMA. The other
     * buffer may still be read by slave/DMA; its normal-sprite words remain
     * immutable. Nothing else may append while this adapter owns display. */
    if(jo_vdp1_frame_command_count()!=0)return NULL;
    unsigned reserved;unsigned short *keys;
    jo_vdp1_command *dest=__jo_vdp1_reserve_commands(count,&keys,&reserved);
    if(reserved!=count)return NULL;
    if(overlap_active&&committed_valid&&(const void *)dest==(const void *)committed_commands)
        return NULL; /* Never overwrite the sole drain source after an empty SDK sync. */
    last_commands=(wf_jo_command_t *)(void *)dest;
    borrowed_count=count;borrowed_tag=__jo_vdp1_frame_tag();
    commands_borrowed=1;last_commands_valid=0;
    return last_commands;
}
void wf_jo_video_commands_abort(void)
{
    jo_core_set_frame_overlap(false);overlap_eligible=0;
    commands_borrowed=borrowed_count=0;wf_jo_video_texture_abort();
}

static wf_status_t append_commands(const wf_jo_command_t *commands,unsigned count)
{
    unsigned reserved;unsigned short *keys;
    if(count>WF_VDP1_COMMANDS_MAX||(!commands&&count))return WF_BAD_STATE;
    if(jo_vdp1_frame_command_count()!=0)return WF_BAD_STATE;
    jo_vdp1_command *dest=__jo_vdp1_reserve_commands(count,&keys,&reserved);
    if(reserved!=count)return WF_BLOCKED;
    /* A fresh borrow already occupies this prefix. A repeat can refer to the
     * other CPU buffer, which the slave/DMA still READS: copy its immutable
     * normal-sprite words into the current buffer before advancing the saved
     * pointer. Jo's sort/DMA path does not mutate these words. Do not pass
     * identical (or overlapping) storage to Jo's restrict memcpy. */
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
                               unsigned count)
{
    if(!video_running||overlap_draining||drain_started||bank>=WF_JO_TEXTURE_BANKS||!banks[bank].reserved||
       pending_bank>=0)return WF_BAD_STATE;
    if(commands_borrowed&&(commands!=last_commands||count>borrowed_count||
       borrowed_tag!=__jo_vdp1_frame_tag()))return WF_BAD_STATE;
    if(enqueued_valid&&enqueued_tag==__jo_vdp1_frame_tag())return WF_BAD_STATE;
    int used=jo_vdp1_frame_command_count(),capacity=jo_vdp1_command_max();
    if(used!=0)return WF_BAD_STATE;
    if(used<0||used>capacity||count>(unsigned)(capacity-used))return WF_BLOCKED;
    if(texture_cache.building?!wf_jo_cache_can_publish(&texture_cache):
       !texture_cache.repeat_valid)return WF_BAD_STATE;
    wf_status_t status=append_commands(commands,count);
    if(status!=WF_OK)return status;
    unsigned tag=__jo_vdp1_frame_tag();
    overlap_eligible=commands_borrowed;overlap_eligible_tag=tag;
    /* The pre-append guard validated this exact builder. append_commands has
     * no callback/yield and changes only SDK command storage. VBlank updates
     * shown tags/maps, and DMA only VRAM; neither writes building/frame/ready.
     * Keep rejection before append, then publish without scanning twice. */
    if(texture_cache.building)wf_jo_cache_publish_validated(&texture_cache,tag);
    else{wf_jo_cache_repeat(&texture_cache,tag);wf_jo_video_stats.last_texture_bytes=0;}
    last_count=count;last_bank=(int)bank;pending_bank=(int)bank;
    commands_borrowed=borrowed_count=0;last_commands_valid=1;
    banks[bank].last_tag=banks[bank].write_tag=tag;
    banks[bank].used=1;banks[bank].reserved=0;
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
        committed_commands=last_commands;committed_count=last_count;
        committed_bank=bank;committed_valid=1;
        ++wf_jo_video_stats.submitted_frames;
        __jo_core_restore_interrupts(sr);
        return WF_OK;
    }
    __jo_core_restore_interrupts(sr);
    return WF_BLOCKED;
}

static wf_status_t repeat_frozen(const wf_jo_command_t *commands,unsigned count,unsigned bank)
{
    if(enqueued_valid&&enqueued_tag==__jo_vdp1_frame_tag())return WF_OK;
    if(texture_cache.building||!texture_cache.repeat_valid)return WF_BAD_STATE;
    wf_status_t status=append_commands(commands,count);
    if(status!=WF_OK)return status;
    wf_jo_cache_repeat(&texture_cache,__jo_vdp1_frame_tag());
    last_count=count;last_bank=(int)bank;
    banks[bank].last_tag=__jo_vdp1_frame_tag();
    status=enqueue_bank(bank);
    if(status==WF_OK){++wf_jo_video_stats.repeated_frames;wf_jo_video_stats.last_texture_bytes=0;}
    return status;
}

static wf_status_t repeat_existing(void)
{
    if(last_bank<0||!last_commands_valid)return WF_BAD_STATE;
    return repeat_frozen(last_commands,last_count,(unsigned)last_bank);
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
    jo_core_set_frame_overlap(false);
    if(!video_running||!last_commands_valid||overlap_draining||drain_started)return WF_BAD_STATE;
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
        /* VDP2 snapshots rotate independently. Shared texture addresses stay
         * unchanged, and submission extends every referenced texture's fence. */
        status=wf_jo_video_submit(bank,last_commands,last_count);
        if(status!=WF_OK){wf_jo_video_release_bank(bank);return status;}
    }
    unsigned bank=(unsigned)pending_bank;
    snapshot(bank);
    wf_status_t status=enqueue_bank(bank);
    if(status==WF_OK)pending_bank=-1;
    return status;
}

wf_status_t wf_jo_video_repeat(void)
{
    overlap_eligible=0;
    /* A wait may follow a HUD/fade commit without another sprite render. */
    return wf_jo_video_commit();
}

wf_status_t wf_jo_video_allow_overlap(void)
{
    jo_core_set_frame_overlap(false);
    unsigned tag=__jo_vdp1_frame_tag();
    if(!video_running||!overlap_eligible||overlap_eligible_tag!=tag||
       commands_borrowed||texture_cache.building||pending_bank>=0||
       !last_commands_valid||last_bank<0||!enqueued_valid||enqueued_tag!=tag||
       banks[last_bank].write_tag!=tag||overlap_draining||drain_started)return WF_BAD_STATE;
    overlap_eligible=0;overlap_active=1;
    ++wf_jo_video_stats.overlap_requests;
    jo_core_set_frame_overlap(true);
    return WF_OK;
}

static int drain_visible(void)
{
    if(!shown_valid)return 0;
    int32_t after=(int32_t)(shown_tag-drain_tag);
    /* VBlank-IN publishes the counter before the framebuffer change. Another
     * physical field boundary guarantees the target had a complete scanout. */
    return after>0||(after==0&&jo_core_get_vblank_count()!=shown_field);
}

wf_status_t wf_jo_video_drain_overlap_fields(wf_status_t (*service)(void *),void *user,
                                            uint32_t *elapsed_fields)
{
    if(elapsed_fields)*elapsed_fields=0;
    jo_core_set_frame_overlap(false);overlap_eligible=0;
    if(overlap_draining)return WF_BAD_STATE;
    if(!overlap_active)return WF_OK;
    if(commands_borrowed||texture_cache.building||pending_bank>=0||
       !committed_valid||!enqueued_valid)return WF_BAD_STATE;
    overlap_draining=1;
    unsigned start=jo_core_get_vblank_count(),serviced=start;
    wf_status_t status=WF_OK;
    if(!drain_started){
        drain_tag=enqueued_tag;drain_started=1;drain_successor=0;
        wf_jo_video_stats.overlap_last_target=drain_tag;
    }
    if(__jo_vdp1_frame_tag()==drain_tag)__jo_core_synch();
    if(!drain_visible()&&(!shown_valid||(int32_t)(shown_tag-drain_tag)<0)&&!drain_successor){
        /* Use the last frozen snapshot, even if live maps/palette changed.
         * The normal commit/repeat API could publish those transition edits. */
        status=repeat_frozen(committed_commands,committed_count,committed_bank);
        if(status!=WF_OK)goto complete;
        drain_successor=1;
        __jo_core_synch();
    }
    for(;;){
        unsigned now=jo_core_get_vblank_count();
        if(now!=serviced){
            serviced=now;
            if(service&&(status=service(user))!=WF_OK)goto complete;
        }
        if(drain_visible())break;
        if(now-start>=16u){status=WF_BLOCKED;goto complete;}
        __jo_core_pause();
    }
    ++wf_jo_video_stats.overlap_drains;
    overlap_active=drain_started=drain_successor=0;
complete:
    {
        uint32_t now=jo_core_get_vblank_count(),elapsed=now-start;
        if(elapsed_fields)*elapsed_fields=elapsed;
        wf_jo_video_stats.overlap_drain_fields+=elapsed;
        if(status==WF_OK){
            wf_jo_video_stats.overlap_last_shown_field=shown_field;
            wf_jo_video_stats.overlap_last_drain_field=now;
        }
    }
    overlap_draining=0;
    return status;
}

wf_status_t wf_jo_video_drain_overlap(wf_status_t (*service)(void *),void *user)
{return wf_jo_video_drain_overlap_fields(service,user,NULL);}

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
    if(wf_jo_video_drain_overlap(NULL,NULL)!=WF_OK)return;
    unsigned sr=__jo_core_disable_interrupts();
    blank_pending=1;resume_valid=0;jo_wf_vdp2_blank();
    __jo_core_restore_interrupts(sr);
}
void wf_jo_video_gate(void)
{
    if(wf_jo_video_drain_overlap(NULL,NULL)!=WF_OK)return;
    unsigned sr=__jo_core_disable_interrupts();
    blank_pending=1;resume_valid=0;jo_wf_vdp2_gate();
    __jo_core_restore_interrupts(sr);
}
void wf_jo_video_diagnostics(void)
{
    if(wf_jo_video_drain_overlap(NULL,NULL)!=WF_OK)return;
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

static void texture_reset(void)
{
    wf_jo_cache_init(&texture_cache);texture_cache_initialized=1;
#ifndef WF_PLATFORM_SATURN
    memset(host_textures,0,sizeof host_textures);host_frame_tag=1;
#endif
}
wf_status_t wf_jo_video_texture_invalidate(void)
{
#ifdef WF_PLATFORM_SATURN
    if(overlap_draining||drain_started)return WF_BAD_STATE;
#endif
    if(!texture_cache_initialized)texture_reset();
    return wf_jo_cache_invalidate_sources(&texture_cache)?WF_OK:WF_BAD_STATE;
}
wf_status_t wf_jo_video_texture_begin(unsigned count)
{
#ifdef WF_PLATFORM_SATURN
    if(overlap_draining||drain_started)return WF_BAD_STATE;
#endif
    if(!texture_cache_initialized)texture_reset();
    if(!wf_jo_cache_begin(&texture_cache,count))return WF_BAD_STATE;
    wf_jo_video_stats.last_texture_bytes=0;return WF_OK;
}
void wf_jo_video_texture_abort(void)
{wf_jo_cache_abort(&texture_cache);}
int wf_jo_video_texture_preparation_available(const void *owner,uint32_t generation,unsigned count)
{return wf_jo_cache_preparation_available(&texture_cache,owner,generation,count);}
int wf_jo_video_texture_restore_preparation(const void *owner,uint32_t generation,const uint8_t *active)
{return wf_jo_cache_restore_preparation(&texture_cache,owner,generation,active);}
int wf_jo_video_texture_consume_preparation(const void *owner,uint32_t generation,unsigned count)
{return wf_jo_cache_consume_preparation(&texture_cache,owner,generation,count);}
void wf_jo_video_texture_remember_preparation(const void *owner,uint32_t generation,const uint8_t *active)
{(void)wf_jo_cache_remember_preparation(&texture_cache,owner,generation,active);}
wf_status_t wf_jo_video_texture_protect(unsigned logical,uint16_t source,
                                       wf_jo_texture_ref_t *ref,unsigned *hit)
{
    if(!ref||!hit||!wf_jo_cache_protect(&texture_cache,logical,source))return WF_BAD_STATE;
    unsigned physical=texture_cache.frame[logical];*hit=physical!=WF_JO_GPU_NONE;
    ref->physical=(uint16_t)physical;
    ref->meta=*hit?(uint8_t)(texture_cache.entries[physical].crop_meta&127u):0;
    ref->height=*hit?texture_cache.entries[physical].crop_height:0;
    return WF_OK;
}
wf_status_t wf_jo_video_texture_resolve(unsigned logical,uint16_t source,unsigned immutable,
                                       wf_jo_texture_ref_t *ref,unsigned *hit)
{
    if(!ref||!hit)return WF_BAD_STATE;
    for(;;){
        unsigned physical;
#ifdef WF_PLATFORM_SATURN
        int result=wf_jo_cache_resolve(&texture_cache,logical,source,immutable,
                                       shown_valid,shown_tag,&physical);
#else
        int result=wf_jo_cache_resolve(&texture_cache,logical,source,immutable,
                                       host_frame_tag>1u,host_frame_tag-1u,&physical);
#endif
        if(result==WF_JO_CACHE_ERROR)return WF_BAD_STATE;
        if(result!=WF_JO_CACHE_BLOCKED){
            ref->physical=(uint16_t)physical;*hit=result==WF_JO_CACHE_HIT;
            ref->meta=*hit?(uint8_t)(texture_cache.entries[physical].crop_meta&127u):0;
            ref->height=*hit?texture_cache.entries[physical].crop_height:0;
            return WF_OK;
        }
#ifdef WF_PLATFORM_SATURN
        /* Keep this exact logic/render frame. A real display fence, never a
         * guessed age or dropped command, makes an occupied slot writable. */
        __jo_core_pause();
#else
        return WF_BLOCKED;
#endif
    }
}

wf_status_t wf_jo_video_texture_transfer(unsigned physical,const void *pixels,unsigned tiles)
{
    if(!texture_cache.building||!pixels||!tiles||tiles>WF_JO_TEXTURE_STAGE_BYTES/128u||
       physical>=WF_JO_GPU_TILES||tiles>WF_JO_GPU_TILES-physical)return WF_BAD_STATE;
    wf_jo_cache_forget_preparation(&texture_cache);
    unsigned bytes=tiles*128u;
#ifdef WF_PLATFORM_SATURN
    jo_dma_copy(pixels,(void *)(uintptr_t)(0x25c00000u+WF_JO_TEXTURE_FIRST+physical*128u),bytes);
#else
    memcpy(host_textures+physical*128u,pixels,bytes);
#endif
    wf_jo_video_stats.texture_bytes+=bytes;wf_jo_video_stats.last_texture_bytes+=bytes;
    ++wf_jo_video_stats.texture_transfers;return WF_OK;
}
wf_status_t wf_jo_video_texture_ready(unsigned logical,uint16_t source,unsigned immutable,
                                     uint8_t meta,uint8_t height)
{
    return wf_jo_cache_filled(&texture_cache,logical,source,immutable,meta,height)?WF_OK:WF_BAD_STATE;
}
const uint16_t *wf_jo_video_texture_frame_slots(void)
{return texture_cache.frame;}
#ifndef WF_PLATFORM_SATURN
/* Native logic/render oracles have no asynchronous SDK display pipeline. The
 * actual SDK ownership path is verified separately with the hardware model. */
wf_status_t wf_jo_video_allow_overlap(void){return WF_OK;}
wf_status_t wf_jo_video_drain_overlap_fields(wf_status_t (*service)(void *),void *user,
                                            uint32_t *elapsed_fields)
{(void)service;(void)user;if(elapsed_fields)*elapsed_fields=0;return WF_OK;}
wf_status_t wf_jo_video_drain_overlap(wf_status_t (*service)(void *),void *user)
{return wf_jo_video_drain_overlap_fields(service,user,NULL);}
wf_status_t wf_jo_video_submit(unsigned bank,const wf_jo_command_t *commands,unsigned count)
{
    if(bank>=WF_JO_TEXTURE_BANKS||(!commands&&count)||count>WF_VDP1_COMMANDS_MAX||
       !wf_jo_cache_publish(&texture_cache,host_frame_tag))return WF_BAD_STATE;
    ++host_frame_tag;return WF_OK;
}
const uint8_t *wf_jo_video_texture_pixels(unsigned offset,unsigned bytes)
{
    if(offset>sizeof host_textures||bytes>sizeof host_textures-offset)return NULL;
    return host_textures+offset;
}
#endif
