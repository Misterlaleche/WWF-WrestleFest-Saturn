/* Actual Jo command pipeline + core synch, with deterministic delayed devices.
 * The two CPU contexts are cooperatively scheduled; no host thread races and
 * no host elapsed time is interpreted as Saturn performance. */
#define _GNU_SOURCE
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <ucontext.h>

#define JO_VDP1_MAX_COMMANDS 520
#define JO_TV_WIDTH 320
#define JO_TV_HEIGHT 240
#define JO_TV_WIDTH_2 160
#define JO_MULT_BY_65536(x) ((x)*65536)
#define JO_FIXED_TO_INT(x) ((x)>>16)
#define JO_NULL NULL
#define JO_FRAMERATE 1
#define JO_FRAMERATE_DYNAMIC 1
#define JO_VDP1_SORT_IN_C 1
#include "jo/types.h"
#include "jo/sega_saturn.h"
#include "jo/vdp1_command_pipeline.h"
#include "jo/colors.h"
#include "internal/vdp1_command_pipeline.h"
#include "wf_jo_video.h"
#include "wf_saturn_memory.h"
#include "wf_saturn_yaul_backend.h"

static void host_poll(void);
static void *host_shared(void *p);
static void __jo_core_add_slave_job(void (*function)(void *),void *argument);
static bool __jo_core_is_slave_running(void){return true;}
void __jo_core_pause(void){host_poll();}
static void __jo_input_frame_start(void);
unsigned __jo_core_disable_interrupts(void);
void __jo_core_restore_interrupts(unsigned);
void jo_dma_wait(void){}
void jo_dma_copy(const void *,void *,unsigned);
int jo_core_get_refresh_rate(void){return 60;}
#undef JO_CACHE_THROUGH
#define JO_CACHE_THROUGH(p) host_shared((void *)(p))
#undef JO_WORK_RAM_ENGINE_WORK_SECTION
#undef JO_WORK_RAM_ENGINE_WORK_ALIGNED_SECTION
#define JO_WORK_RAM_ENGINE_WORK_SECTION
#define JO_WORK_RAM_ENGINE_WORK_ALIGNED_SECTION
#define JO_ZERO(x) ((x)=0)
#define JO_DIV_BY_256(x) ((x)>>8)
#define jo_memset memset
#define jo_memcpy memcpy
#define jo_core_error(...) assert(!"Unexpected SDK error")
#define jo_profile_begin(...) ((void)0)
#define jo_profile_end(...) ((void)0)
#define jo_profile_count(...) ((void)0)
static struct {
    int width,height,width_2,height_2,background_width,background_height;
    bool hires,interlace,pal,wide;
} __jo_screen={320,240,160,120,320,240,false,false,false,false};

#include "sdk_pipeline.inc"

volatile unsigned __jo_core_vblank_count;
static unsigned __jo_core_last_synch_vblank,__jo_core_fps_frames,__jo_core_fps_vblank;
int __jo_core_fps;
unsigned __jo_core_late_frames;
#include "sdk_core.inc"
#if !TEST_CANDIDATE
static void jo_core_set_frame_overlap(bool allow){(void)allow;}
#endif

enum {MAX_PUBLICATIONS=16384,MAX_JOBS=16,STACK_BYTES=262144};
typedef struct {
    unsigned tag,image,bank,count,required,shown,shown_field,presented;
    wf_jo_command_t *commands;
    const jo_vdp1_frame *cpu_buffer;
} publication_t;
typedef struct {
    void (*function)(void *);void *argument;
    unsigned publication;
} job_t;
static publication_t publications[MAX_PUBLICATIONS];
static unsigned published,displayed,last_shown_index=UINT32_MAX,irq_shown_index;
static job_t jobs[MAX_JOBS],active_job;
static unsigned job_head,job_tail,worker_active,master_done;
static ucontext_t scheduler_context,master_context,slave_context;
static unsigned current_cpu,inside_irq,interrupt_mask,simulation_started;
static uint32_t random_state;
static unsigned steps,next_vblank,draw_until,dma_until,dma_active,dma_publication;
static unsigned dma_entry,dma_offset,dma_count;
static unsigned *dma_list;
static uint8_t dma_expected[0x8000];
static unsigned table_publication[2]={UINT32_MAX,UINT32_MAX};
static unsigned requested_frames,overlap_mode,drain_mode,negative_mode,drop_injected;
static unsigned edge_mode,zero_drains,dirty_drains,abort_drains,shown_drains;
static unsigned service_calls,service_last_field,service_error,reentrant_checks;
static unsigned service_target,service_wait_shown;
static unsigned current_image,current_required,input_starts,same_field_returns,overlap_returns;
static unsigned dma_checks,present_checks,repeat_count,abort_count,gate_count;
static unsigned max_live_maps,max_outstanding,jobs_behind_pending;
static void (*video_callback)(void);
static wf_saturn_yaul_backend_t backend;
static uint8_t texture_stage[WF_JO_TEXTURE_STAGE_BYTES] __attribute__((aligned(32)));
static uint8_t pixels[128] __attribute__((aligned(32)));

static unsigned rnd(void){random_state^=random_state<<13;random_state^=random_state>>17;random_state^=random_state<<5;return random_state;}
static publication_t *find_tag(unsigned tag)
{for(unsigned i=published;i--;)if(publications[i].tag==tag)return &publications[i];return NULL;}
static int protected_map(unsigned bank)
{for(unsigned i=0;i<published;i++)if(i>=last_shown_index||!publications[i].shown){
    if(publications[i].bank==bank)return 1;}return 0;}
static int protected_texture(unsigned offset,unsigned bytes)
{
    for(unsigned i=0;i<published;i++)if(i>=last_shown_index||!publications[i].shown){
        publication_t *p=&publications[i];
        for(unsigned j=0;j<p->count;j++){
            unsigned tile=((unsigned)p->commands[j].srca*8u-WF_JO_TEXTURE_FIRST)&~127u;
            if(offset<tile+128u&&tile<offset+bytes)return 1;
        }
    }
    return 0;
}
unsigned jo_core_get_vblank_count(void){return __jo_core_vblank_count;}
int jo_core_add_vblank_callback(void (*fn)(void)){video_callback=fn;return 1;}
unsigned __jo_core_disable_interrupts(void){unsigned old=interrupt_mask;interrupt_mask=1;return old;}
void __jo_core_restore_interrupts(unsigned old){interrupt_mask=old;}
static void __jo_input_frame_start(void){++input_starts;}
void jo_wf_vdp2_init(void){}
void jo_wf_vdp2_blank(void){}
void jo_wf_vdp2_gate(void)
{
    for(unsigned i=0;i<published;i++)if(publications[i].required){
        assert(publications[i].shown);
        assert(publications[i].presented);
        assert(__jo_core_vblank_count-publications[i].shown_field>=1u);
    }
    ++gate_count;
}
void jo_wf_vdp2_diagnostics(void){}
void jo_wf_vdp2_present(unsigned bank,const unsigned short scroll[4],unsigned char priority,
                         unsigned layers,unsigned opaque,unsigned sprites)
{
    assert(inside_irq&&irq_shown_index<published);
    publication_t *p=&publications[irq_shown_index];
    assert(p->bank==bank);
    assert(*(uint16_t *)(uintptr_t)(JO_VDP2_VRAM_A1+bank*WF_JO_MAP_GROUP_BYTES)==(uint16_t)p->image);
    assert(*(uint32_t *)(uintptr_t)JO_VDP2_CRAM==(p->image*37u&0xffffffu));
    assert(scroll[0]==(p->image&511u)&&scroll[1]==((p->image*3u)&511u));
    assert(priority==0x7bu&&layers==7u&&opaque==1u&&sprites==2u);
    assert(!p->presented);p->presented=1;
    ++present_checks;
}
void jo_dma_copy(const void *source,void *dest,unsigned bytes)
{
    uintptr_t address=(uintptr_t)dest;
    if(address>=JO_VDP1_VRAM+WF_JO_TEXTURE_FIRST&&address+bytes<=JO_VDP1_VRAM+0x7e4f8u){
        assert(!protected_texture((unsigned)address-JO_VDP1_VRAM-WF_JO_TEXTURE_FIRST,bytes));
    }else if(address>=JO_VDP2_VRAM_A1&&address+bytes<=JO_VDP2_VRAM_A1+WF_JO_MAP_GROUP_BYTES*4u){
        unsigned bank=((unsigned)address-JO_VDP2_VRAM_A1)/WF_JO_MAP_GROUP_BYTES;
        assert(!protected_map(bank));
    }else assert(address==JO_VDP2_CRAM&&inside_irq&&bytes==4096u);
    memcpy(dest,source,bytes);
}

static void *host_shared(void *p){host_poll();return p;}
static void host_poll(void)
{
    if(!simulation_started||inside_irq||current_cpu==0)return;
    ucontext_t *from=current_cpu==1?&master_context:&slave_context;
    assert(!swapcontext(from,&scheduler_context));
}
static void register_publication(const jo_vdp1_frame *frame)
{
    assert(published<MAX_PUBLICATIONS&&frame->count<=512u&&!find_tag(frame->tag));
    publication_t *p=&publications[published];
    p->tag=frame->tag;p->image=current_image;p->required=current_required;p->count=frame->count;
    p->cpu_buffer=frame;
    p->commands=malloc((p->count?p->count:1u)*sizeof *p->commands);assert(p->commands);
    memcpy(p->commands,frame->commands,p->count*sizeof *p->commands);
    unsigned found=0;
    for(unsigned bank=0;bank<4u;bank++){
        unsigned value=*(uint16_t *)(uintptr_t)(JO_VDP2_VRAM_A1+bank*WF_JO_MAP_GROUP_BYTES);
        if(value==(uint16_t)p->image){p->bank=bank;++found;}
    }
    assert(found==1u);
    unsigned live=0;bool banks[4]={false};
    for(unsigned i=0;i<=published;i++)if(i>=last_shown_index||!publications[i].shown)banks[publications[i].bank]=true;
    for(unsigned i=0;i<4u;i++)live+=banks[i];
    if(live>max_live_maps)max_live_maps=live;
    assert(live<=4u);
    ++published;
}
static void __jo_core_add_slave_job(void (*function)(void *),void *argument)
{
    assert(current_cpu==1&&job_head-job_tail<MAX_JOBS);
    register_publication(argument);
    jobs[job_head%MAX_JOBS]=(job_t){function,argument,published-1u};++job_head;
    if(__jo_vdp1_pending_table)++jobs_behind_pending;
    unsigned outstanding=__jo_vdp1_frames_sent-__jo_vdp1_frames_done;
    if(outstanding>max_outstanding)max_outstanding=outstanding;
    assert(outstanding<=2u); /* Existing end_frame waits to <=1 before return. */
}
static void slave_run(void)
{
    for(;;){
        if(job_tail<job_head){
            active_job=jobs[job_tail%MAX_JOBS];worker_active=1;
            active_job.function(active_job.argument);
            worker_active=0;++job_tail;
        }
        host_poll();
    }
}
static void check_cpu_sources(void)
{
    for(unsigned i=job_tail;i<job_head;i++){
        job_t *job=&jobs[i%MAX_JOBS];const jo_vdp1_frame *frame=job->argument;
        if(job->publication<__jo_vdp1_frames_done&&(!dma_active||dma_publication!=job->publication))continue;
        publication_t *p=&publications[job->publication];
        assert(frame->tag==p->tag&&frame->count==p->count);
        assert(!memcmp(frame->commands,p->commands,p->count*sizeof *p->commands));
        for(unsigned j=0;j<p->count;j++)assert(frame->keys[j]==200u);
    }
}
static void start_dma(void)
{
    assert(worker_active&&!dma_active);
    dma_publication=active_job.publication;
    dma_list=(unsigned *)(uintptr_t)JO_SCU_DMA_WRITE(JO_VDP1_SCU_DMA_LEVEL);
    dma_count=0;unsigned bytes=0;
    do{
        unsigned *entry=dma_list+dma_count*3u;
        assert(entry[0]&&entry[0]<=0x8000u&&bytes+entry[0]<=sizeof dma_expected);
        memcpy(dma_expected+bytes,(void *)(uintptr_t)(entry[2]&0x7fffffffu),entry[0]);
        bytes+=entry[0];++dma_count;assert(dma_count<1026u);
    }while(!(dma_list[(dma_count-1u)*3u+2u]&0x80000000u));
    dma_entry=dma_offset=0;dma_active=1;dma_until=steps+2u+rnd()%13u;
    JO_SCU_DMA_STATUS|=JO_SCU_DMA_STATUS_BUSY(0);
}
static void progress_dma(void)
{
    if(!dma_active&&JO_SCU_DMA_ENABLE(0)==JO_SCU_DMA_ENABLE_START)start_dma();
    if(!dma_active||steps<dma_until)return;
    unsigned expected_at=0;
    for(unsigned i=0;i<dma_entry;i++)expected_at+=dma_list[i*3u];
    unsigned *entry=dma_list+dma_entry*3u;
    unsigned bytes=entry[0]-dma_offset;if(bytes>64u)bytes=64u;
    const void *source=(void *)(uintptr_t)((entry[2]&0x7fffffffu)+dma_offset);
    assert(!memcmp(source,dma_expected+expected_at+dma_offset,bytes));++dma_checks;
    memcpy((void *)(uintptr_t)(entry[1]+dma_offset),source,bytes);
    dma_offset+=bytes;
    if(dma_offset==entry[0]){dma_offset=0;++dma_entry;}
    if(dma_entry<dma_count){dma_until=steps+1u+rnd()%3u;return;}
    publication_t *p=&publications[dma_publication];
    unsigned table=dma_list[1]-JO_VDP1_VRAM;
    jo_vdp1_command *cmd=(jo_vdp1_command *)(uintptr_t)(JO_VDP1_VRAM+table);
    unsigned actual=0;
    for(unsigned i=0;!(cmd[i].ctrl&JO_VDP1_CMD_END);i++){
        assert(i<1024u);
        if((cmd[i].ctrl&15u)==JO_VDP1_CMD_NORMAL_SPRITE){
            assert(actual<p->count&&!memcmp(&cmd[i],&p->commands[actual],sizeof cmd[i]));++actual;
        }
    }
    assert(actual==p->count);
    table_publication[table==JO_VDP1_TABLE_A_OFFSET?0:1]=dma_publication;
    dma_active=0;JO_SCU_DMA_ENABLE(0)=0;JO_SCU_DMA_STATUS&=~JO_SCU_DMA_STATUS_BUSY(0);
}
static void vblank(void)
{
    inside_irq=1;++__jo_core_vblank_count;
    unsigned old_taken=__jo_vdp1_taken_tag,old_table=__jo_vdp1_pending_table;
    if(negative_mode&&!drop_injected&&published>5u&&old_table&&!dma_active){
        __jo_vdp1_pending_table=0;drop_injected=1;
    }
    __jo_vdp1_vblank();
    unsigned shown;
    if(__jo_vdp1_displayed_frame_changed(&shown)){
        publication_t *p=find_tag(shown);assert(p);
        unsigned index=(unsigned)(p-publications);
        assert(index==displayed); /* No dropped, duplicated or reordered Jo tag. */
        assert(!p->shown);p->shown=1;p->shown_field=__jo_core_vblank_count;
        irq_shown_index=index;last_shown_index=index;++displayed;
    }
    if(video_callback)video_callback();
    if(__jo_vdp1_taken_tag!=old_taken){
        assert(old_table&&!dma_active);
        unsigned table_index=old_table==JO_VDP1_TABLE_A_OFFSET?0:1;
        assert(table_publication[table_index]<published);
        assert(publications[table_publication[table_index]].tag==__jo_vdp1_taken_tag);
        draw_until=steps+8u+rnd()%240u;
        JO_VDP1_EDSR&=(unsigned short)~JO_VDP1_EDSR_CURRENT_END;
    }
    inside_irq=0;
}
static wf_status_t read_tile(uint16_t source,const uint8_t **out)
{
    unsigned value=(source*73u^(source>>8))&255u;
    memset(pixels,value?value:0x11u,sizeof pixels);*out=pixels;return WF_OK;
}
#ifndef TEST_PREPARATION
#define TEST_PREPARATION 0
#endif
#ifndef TEST_PRIVATE_CONSUME
#define TEST_PRIVATE_CONSUME 0
#endif
#if TEST_PREPARATION
static unsigned preparation_restores,previous_texture,previous_count,have_preparation;
static unsigned private_consumes;
extern int __real_wf_jo_video_texture_restore_preparation(const void *,uint32_t,const uint8_t *);
int __wrap_wf_jo_video_texture_restore_preparation(const void *owner,uint32_t generation,const uint8_t *active)
{
    int restored=__real_wf_jo_video_texture_restore_preparation(owner,generation,active);
    preparation_restores+=!!restored;return restored;
}
#endif
static void fresh(unsigned image,unsigned count,unsigned abort_first)
{
    const wf_saturn_render_ops_t *ops=wf_saturn_yaul_render_ops();
    uint16_t sources[256];uint8_t touched[256];wf_vdp1_sprite_cmd_t sprites[256];
    unsigned texture_image=image;
#if TEST_PREPARATION
    texture_image=(image-1u)/4u;
    count=texture_image%19u==0u?0u:texture_image%31u==0u?256u:1u+(texture_image*13u)%24u;
#endif
    for(unsigned i=0;i<count;i++){sources[i]=(uint16_t)(texture_image*(TEST_PREPARATION?257u:17u)+i);touched[i]=1;
        sprites[i]=(wf_vdp1_sprite_cmd_t){.stream_tile=(uint16_t)i,
            .x=(int16_t)((i*3u+(TEST_PREPARATION?image:0u))%320u),.y=(int16_t)(8u+i%240u)};}
    if(abort_first){
        assert(ops->begin_frame(&backend,count,0,count)==WF_OK);
        ops->abort_frame(&backend);++abort_count;
    }
    assert(ops->begin_frame(&backend,count,0,count)==WF_OK);
#if TEST_PREPARATION
    unsigned restores_before=preparation_restores;
    assert(wf_jo_backend_prepare_rom_tiles(&backend,sources,touched,1,count,
        have_preparation&&previous_texture==texture_image&&previous_count==count,1)==WF_OK);
    previous_texture=texture_image;previous_count=count;have_preparation=0;
#else
    assert(wf_jo_backend_protect_rom_tiles(&backend,sources,touched,1,count)==WF_OK);
#endif
    unsigned consumed=0;
#if TEST_PRIVATE_CONSUME
    consumed=(unsigned)wf_jo_backend_consume_prepared_rom_tiles(&backend);
    assert(consumed==(preparation_restores!=restores_before));
    assert(!wf_jo_backend_consume_prepared_rom_tiles(&backend));private_consumes+=consumed;
#endif
    if(!consumed)assert(wf_jo_backend_consume_rom_tiles(&backend,sources,touched,1,count,read_tile)==WF_OK);
    assert(wf_jo_backend_emit_sprites(&backend,sprites,count)==WF_OK);
    assert(ops->end_frame(&backend)==WF_OK);
#if TEST_PREPARATION
    have_preparation=1;
#endif
    current_image=image;current_required=1;
    wf_jo_video_map(0)[0]=(uint16_t)image;wf_jo_video_map_written(0);
    wf_jo_video_cram_write(0,image*37u&0xffffffu);
    wf_jo_video_scroll(0,image&511u,(image*3u)&511u);
    wf_jo_video_ring_priority(0x7b,1);wf_jo_video_layers(7,1,2);
    assert(wf_jo_video_commit()==WF_OK);
}
static void repeat(void)
{current_required=0;assert(wf_jo_video_repeat()==WF_OK);++repeat_count;}
static void baseline_drain(unsigned target)
{
    jo_core_set_frame_overlap(false);
    publication_t *p=find_tag(target);assert(p);
    if(!p->shown){repeat();__jo_core_synch();}
    while(!p->shown||__jo_core_vblank_count==p->shown_field)host_poll();
}
static void application_drain(unsigned target)
{
    if(!drain_mode&&!overlap_mode){baseline_drain(target);return;}
    current_required=0;
#if TEST_CANDIDATE
    assert(wf_jo_video_drain_overlap(NULL,NULL)==WF_OK);
#else
    (void)target; /* Missing feature: the base cannot close an overlap epoch. */
#endif
}
static void outer_sync(void)
{
#if TEST_CANDIDATE
    jo_core_synch_callbacks();
#else
    __jo_core_synch();
#endif
}
#if TEST_CANDIDATE
static wf_status_t drain_service(void *unused)
{
    (void)unused;
    assert(__jo_core_vblank_count!=service_last_field);
    service_last_field=__jo_core_vblank_count;++service_calls;
    unsigned bank=0,gates=gate_count;
    assert(wf_jo_video_drain_overlap(NULL,NULL)==WF_BAD_STATE);
    assert(wf_jo_video_allow_overlap()==WF_BAD_STATE);
    assert(!wf_jo_video_commands_begin(1));
    assert(wf_jo_video_acquire_bank(&bank)==WF_BAD_STATE);
    assert(wf_jo_video_texture_begin(1)==WF_BAD_STATE);
    assert(wf_jo_video_commit()==WF_BAD_STATE);
    wf_jo_video_gate();assert(gate_count==gates);
    ++reentrant_checks;
    if(service_error&&(!service_wait_shown||find_tag(service_target)->shown)){
        service_error=0;return WF_BLOCKED;
    }
    return WF_OK;
}
static void edge_drain(unsigned target,unsigned kind)
{
    current_required=0;
    if(kind==1){
        const wf_saturn_render_ops_t *ops=wf_saturn_yaul_render_ops();
        uint16_t sources[17];uint8_t touched[17];wf_vdp1_sprite_cmd_t sprites[17];
        for(unsigned i=0;i<17;i++){
            sources[i]=(uint16_t)(60000u+i);touched[i]=1;
            sprites[i]=(wf_vdp1_sprite_cmd_t){.stream_tile=(uint16_t)i,.x=(int16_t)i,.y=8};
        }
        /* Force a true staging flush, then overwrite the borrowed command
         * prefix before abort. The old immutable prefix/pixels must survive. */
        assert(ops->begin_frame(&backend,17,0,17)==WF_OK);
        assert(wf_jo_backend_protect_rom_tiles(&backend,sources,touched,1,17)==WF_OK);
        unsigned uploads=wf_jo_video_stats.texture_transfers;
        assert(wf_jo_backend_consume_rom_tiles(&backend,sources,touched,1,17,read_tile)==WF_OK);
        if(abort_drains==0u)assert(wf_jo_video_stats.texture_transfers>uploads);
        assert(wf_jo_backend_emit_sprites(&backend,sprites,17)==WF_OK);
        assert(wf_jo_video_drain_overlap(NULL,NULL)==WF_BAD_STATE);
        ops->abort_frame(&backend);++abort_count;
        assert(wf_jo_video_repeat()==WF_BAD_STATE);
        ++abort_drains;
    }
    /* The private successor must use the frozen snapshot even with edits
     * waiting in canonical frontend state. This edit must survive the drain. */
    wf_jo_video_map(0)[0]=0xf00du;wf_jo_video_map_written(0);
    wf_jo_video_cram_write(0,0xfedcbau);wf_jo_video_scroll(0,510,511);
    assert(wf_jo_video_pending_changes());++dirty_drains;
    service_last_field=__jo_core_vblank_count;
    service_error=1;service_target=target;service_wait_shown=kind==2;
    wf_status_t result=wf_jo_video_drain_overlap(drain_service,NULL);
    if(result==WF_BLOCKED){
        if(kind==2){assert(find_tag(target)->shown);++shown_drains;}
        unsigned before=published;
        assert(wf_jo_video_drain_overlap(drain_service,NULL)==WF_OK);
        assert(published==before); /* Retry must not add another successor. */
    }else assert(result==WF_OK);
    assert(wf_jo_video_stats.overlap_last_target==target);
    assert(wf_jo_video_pending_changes());
    if(kind==1)assert(wf_jo_video_repeat()==WF_BAD_STATE); /* Replay stays invalid. */
    if(target==0u)++zero_drains;
    unsigned fields=__jo_core_vblank_count,before=published,calls=service_calls;
    assert(wf_jo_video_drain_overlap(drain_service,NULL)==WF_OK);
    assert(__jo_core_vblank_count==fields&&published==before&&service_calls==calls);
    wf_jo_video_gate();
}
#endif
static void master_run(void)
{
    assert(wf_jo_video_init()==WF_OK);
    wf_saturn_yaul_backend_init(&backend,texture_stage,sizeof texture_stage);
    for(unsigned i=0;i<requested_frames;i++){
        for(unsigned work=rnd()%70u;work--;)host_poll();
        fresh(i+1u,i%19u==0?0u:i%31u==0?256u:1u+rnd()%24u,i%37u==9u);
        unsigned before=__jo_core_vblank_count;
        unsigned caught_up=before==__jo_core_last_synch_vblank;
        if(overlap_mode||drain_mode){
#if TEST_CANDIDATE
            assert(wf_jo_video_allow_overlap()==WF_OK);
#else
            jo_core_set_frame_overlap(true);
#endif
        }
        outer_sync();
        same_field_returns+=before==__jo_core_vblank_count;
        overlap_returns+=caught_up&&before==__jo_core_vblank_count;
        assert(__jo_vdp1_frames_sent-__jo_vdp1_frames_done<=1u);
        assert(!dma_active||publications[dma_publication].cpu_buffer!=__jo_vdp1_frame);
#if TEST_CANDIDATE
        if(edge_mode&&(publications[published-1u].tag==0u||i%157u==51u||i%157u==102u)){
            edge_drain(publications[published-1u].tag,i%157u==51u?1u:i%157u==102u?2u:0u);
            continue;
        }
#endif
        if(i%41u==7u){
            repeat();
#if TEST_CANDIDATE
            assert(wf_jo_video_allow_overlap()==WF_BAD_STATE);
#endif
            jo_core_set_frame_overlap(true); /* An explicit wait must cancel even an armed hint. */
            before=__jo_core_vblank_count;caught_up=before==__jo_core_last_synch_vblank;
            __jo_core_synch();assert(!caught_up||__jo_core_vblank_count!=before);
        }
        if(i%173u==172u){application_drain(publications[published-1u].tag);wf_jo_video_gate();}
    }
    application_drain(publications[published-1u].tag);
    wf_jo_video_gate();
    for(unsigned i=0;i<published;i++)if(publications[i].required)assert(publications[i].shown);
    if(overlap_mode)assert(overlap_returns>0u&&"The opt-in still forces a new field");
    if(edge_mode)assert(zero_drains&&dirty_drains&&abort_drains&&shown_drains&&service_calls&&reentrant_checks);
    master_done=1;host_poll();assert(!"scheduler resumed completed master");
}
static void setup_context(ucontext_t *context,void (*function)(void))
{
    void *memory=mmap(NULL,STACK_BYTES+8192u,PROT_NONE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);assert(memory!=MAP_FAILED);
    void *stack=(char *)memory+4096u;assert(!mprotect(stack,STACK_BYTES,PROT_READ|PROT_WRITE));
    assert(!getcontext(context));context->uc_stack.ss_sp=stack;context->uc_stack.ss_size=STACK_BYTES;
    context->uc_link=&scheduler_context;makecontext(context,function,0);
}
int main(int argc,char **argv)
{
    assert(argc==4);overlap_mode=!strcmp(argv[1],"overlap");drain_mode=!strcmp(argv[1],"drain");
    edge_mode=!strcmp(argv[1],"edges");overlap_mode|=edge_mode;
    negative_mode=!strcmp(argv[1],"lost-frame");
    requested_frames=(unsigned)strtoul(argv[2],NULL,0);random_state=(uint32_t)strtoul(argv[3],NULL,0);
    assert(random_state&&requested_frames>0u&&requested_frames<12000u);
    void *mmio=mmap((void *)0x25c00000u,0x400000u,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE,-1,0);
    assert(mmio==(void *)0x25c00000u);
    __jo_vdp1_init();__jo_vdp1_erase_measure=false;
    __jo_vdp1_building_tag=UINT32_MAX-11u; /* Cross tag wrap in every run. */
    JO_VDP1_EDSR=JO_VDP1_EDSR_CURRENT_END;
    next_vblank=200u;setup_context(&master_context,master_run);setup_context(&slave_context,slave_run);
    simulation_started=1;
    while(!master_done){
        assert(++steps<requested_frames*20000u+10000u);
        check_cpu_sources();progress_dma();
        if(draw_until&&steps>=draw_until)JO_VDP1_EDSR|=JO_VDP1_EDSR_CURRENT_END;
        if(steps>=next_vblank&&!interrupt_mask){vblank();next_vblank=steps+100u+rnd()%80u;}
        current_cpu=job_tail<job_head&&(rnd()&3u)?2u:1u;
        assert(!swapcontext(&scheduler_context,current_cpu==1?&master_context:&slave_context));current_cpu=0;
    }
    assert(present_checks<=displayed&&displayed>=requested_frames&&max_live_maps<=4u);
    printf("PASS actual_sdk frames=%u publications=%u displayed=%u fields=%u same_field=%u overlap_returns=%u maps=%u outstanding_enqueue=%u dma_checks=%u repeats=%u aborts=%u gates=%u input_starts=%u jobs_behind_pending=%u\n",
        requested_frames,published,displayed,__jo_core_vblank_count,same_field_returns,overlap_returns,max_live_maps,max_outstanding,
        dma_checks,repeat_count,abort_count,gate_count,input_starts,jobs_behind_pending);
    if(edge_mode)printf("PASS drain_edges zero=%u dirty=%u aborted=%u already_shown=%u services=%u reentrancy_checks=%u\n",
        zero_drains,dirty_drains,abort_drains,shown_drains,service_calls,reentrant_checks);
#if TEST_PREPARATION
    if(requested_frames>16u)assert(preparation_restores>requested_frames/4u);
    printf("PASS preparation ownership: %u restorations, fresh moving commands, delayed DMA, eviction, tag wrap, repeats and aborts\n",preparation_restores);
#if TEST_PRIVATE_CONSUME
    assert(private_consumes==preparation_restores);
    printf("PASS private consume ownership: %u one-use all-hit consumptions\n",private_consumes);
#endif
#endif
    return 0;
}
