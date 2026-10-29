/* WP02 is an isolated native Saturn prematch test. No other project is linked. */
#include <yaul.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "wp02_scene.h"
#include "wp02_assets.h"
#include "wp02_audio.h"
#include "wp02_bgm.h"
#include "wp02_diag.h"

#define WIDTH 320
#define HEIGHT 240
/* Exhaustive roster/pose audit bounds this scene at 220 commands. */
#define COMMANDS 256u
#define STAGING 32768u
#define BG_ADDR VDP2_VRAM_ADDR(0,0)
static vdp1_vram_partitions_t video_mem;
static vdp1_cmdt_list_t *commands;
static cdfs_filelist_t files;
static cdfs_filelist_entry_t entries[16];
static uint8_t staging[STAGING] __aligned(32);
static uint8_t driver[10240] __aligned(32);
static smpc_peripheral_digital_t pad;
static volatile uint32_t vblank_count;
static wp02_scene scene;
static uint8_t identities[4];
static uint32_t entropy=0x57503201u;
static unsigned max_commands, dropped_vblanks, render_count;
static bool sound_ready;
typedef struct { uint32_t magic[4], ticks,renders,missed,max_commands,elapsed_us,done,ids,state,vblanks,end_us,bgm_refills,bgm_failures,bgm_max_service_us; } wp02_metrics_t;
volatile wp02_metrics_t wp02_metrics={.magic={0x57503032u,0x4d455452u,0x49435332u,0xfeedc0deu}};
typedef struct {
    uint32_t magic[4], code, reason, raw_monitor, fill_bytes, refills, gap_us;
    uint32_t elapsed_us, renders, missed, max_service_us, target, source_samples, source_offset;
} wp02_fault_record_t;
volatile wp02_fault_record_t wp02_fault={.magic={0x57503032u,0x4641554cu,0x54523032u,0xfeedc0deu}};

static void intback(void *unused)
{
    (void)unused;
    vblank_count++;
    smpc_peripheral_intback_issue();
}
static void sound_vblank(void *unused)
{
    (void)unused;
    if(sound_ready) wp02_audio_vblank(NULL);
}
static const cdfs_filelist_entry_t *file_named(const char *name)
{
    for(unsigned i=0;i<files.entries_count;i++)
        if(!strcmp(files.entries[i].name,name)) return &files.entries[i];
    return NULL;
}
static bool preload_video(const char *name,uintptr_t destination,unsigned expected)
{
    const cdfs_filelist_entry_t *f=file_named(name);
    if(!f||f->size!=expected) return false;
    for(unsigned off=0;off<expected;){
        unsigned n=expected-off;
        if(n>STAGING)n=STAGING;
        if(cd_block_sectors_read(f->starting_fad+off/2048u,staging,(n+2047u)&~2047u))return false;
        scu_dma_transfer(0,(void *)(destination+off),staging,n);
        scu_dma_transfer_wait(0);
        off+=n;
    }
    return true;
}
static void fatal(unsigned code)
{
    /* Freeze the reason before audio_stop resets the driver's local record.
       These values remain available both in a save state and on a real CRT. */
    const wp02_bgm_failure_snapshot_t f=*wp02_bgm_failure_snapshot();
    wp02_fault.code=code;wp02_fault.reason=f.reason;
    wp02_fault.raw_monitor=f.raw_monitor;wp02_fault.fill_bytes=f.fill_bytes;
    wp02_fault.refills=f.refills;wp02_fault.gap_us=f.service_gap_us;
    wp02_fault.elapsed_us=wp02_metrics.elapsed_us;
    wp02_fault.renders=render_count;wp02_fault.missed=dropped_vblanks;
    wp02_fault.max_service_us=wp02_audio_bgm_max_service_us();
    wp02_fault.target=(uint32_t)(int32_t)f.target;
    wp02_fault.source_samples=f.source_samples;wp02_fault.source_offset=f.source_offset;
    const uint32_t words[4]={
        ((code&255u)<<24)|((uint32_t)f.reason<<16)|f.raw_monitor,
        ((uint32_t)f.fill_bytes<<16)|(f.refills&65535u),
        ((render_count&65535u)<<16)|(dropped_vblanks&65535u),
        wp02_fault.max_service_us
    };
    sound_ready=false;wp02_audio_stop();
    dbgio_printf("WP02 R02 ERROR %u reason=%u gap=%lu fill=%u refills=%lu\n",code,f.reason,(unsigned long)f.service_gap_us,f.fill_bytes,(unsigned long)f.refills);
    wp02_diag_show(words);
    for(;;){vdp2_tvmd_display_set();vdp2_sync();vdp2_sync_wait();}
}
#ifndef WP02_REFERENCE
static uint32_t draw_random(void)
{
    static const uint32_t mix[7]={1,0,0,6,0,0x001c1610u,0};
    entropy=wp02_rng_step(entropy,mix,0);
    return entropy;
}
#endif
static void choose_roster(void)
{
#ifdef WP02_REFERENCE
    const uint8_t ref[4]={0,6,9,1};
    memcpy(identities,ref,4);entropy=0xceb7c58eu;
#else
    /* Demo selection only. Grouped Demolition always retains its two members;
       all character animation/name/sound behavior remains the ROM's. */
    static const uint8_t selectable[10]={0,7,2,8,6,9,1,3,10,11};
    uint8_t used[12]={0};
    const smpc_time_t *t=smpc_rtc_time_get();
    const uint8_t *bytes=(const uint8_t *)t;
    for(unsigned i=0;i<sizeof(*t);i++)entropy=(entropy*33u)^bytes[i];
    entropy^=vblank_count*0x10201u;
    for(unsigned team=0;team<2;team++){
        uint8_t pool[10];unsigned count=0;
        for(unsigned j=0;j<10;j++)if(!used[selectable[j]])pool[count++]=selectable[j];
        unsigned a=pool[draw_random()%count];
        if(a==10u||a==11u){
            identities[team*2]=10;identities[team*2+1]=11;used[10]=used[11]=1;
        }else{
            identities[team*2]=(uint8_t)a;used[a]=1;count=0;
            for(unsigned j=0;j<8;j++)if(!used[selectable[j]])pool[count++]=selectable[j];
            unsigned b=pool[draw_random()%count];identities[team*2+1]=(uint8_t)b;used[b]=1;
        }
    }
#endif
}
static void add_cell(const wp02_piece_t *p,int ax,int ay,bool mirror,unsigned *count)
{
    int x=ax+(mirror?-p->x:p->x),y=ay+p->y;
    if(x>=WIDTH||x+(int)p->width<=0||y>=HEIGHT||y+(int)p->height<=0)return;
    if(*count>=COMMANDS-1)fatal(11);
    vdp1_cmdt_t *cmd=&commands->cmdts[(*count)++];
    const vdp1_cmdt_draw_mode_t mode={.color_mode=0,.trans_pixel_disable=(p->flags&1u)!=0,.pre_clipping_disable=true,.end_code_disable=true};
    const vdp1_cmdt_color_bank_t bank={.type_0.dc=p->palette};
    vdp1_cmdt_normal_sprite_set(cmd);vdp1_cmdt_draw_mode_set(cmd,mode);
    vdp1_cmdt_color_mode0_set(cmd,bank);
    vdp1_cmdt_char_size_set(cmd,p->width,p->height);
    vdp1_cmdt_char_base_set(cmd,(vdp1_vram_t)video_mem.texture_base+(unsigned)p->tile*128u);
    vdp1_cmdt_char_flip_set(cmd,(vdp1_cmdt_char_flip_t)((p->flip^(mirror?1u:0u))<<4));
    cmd->cmd_xa=(int16_t)x;cmd->cmd_ya=(int16_t)y;
}
static void draw_frame(const wp02_frame_t *frame,int ax,int ay,bool mirror,unsigned *count)
{
    if(!frame)fatal(12);
    for(unsigned i=0;i<frame->count;i++)add_cell(&wp02_pieces[frame->first+i],ax,ay,mirror,count);
}
static void draw_actor(const wp02_actor *actor,unsigned *count)
{
    if(!(actor->active&0x8000u))return;
    int x=(int)(actor->x>>16)-480;
    int raw_y=(int)(actor->y>>16)+(int)(actor->z>>16)-560;
    draw_frame(wp02_pose_find(actor->type,actor->selector),x,232-raw_y,(actor->selector&0x8000u)!=0,count);
}
static void render(void)
{
    vdp1_cmdt_t *c=commands->cmdts;
    memset(c,0,COMMANDS*sizeof(*c));
    vdp1_cmdt_system_clip_coord_set(&c[0]);vdp1_cmdt_vtx_system_clip_coord_set(&c[0],INT16_VEC2(319,239));
    vdp1_cmdt_local_coord_set(&c[1]);vdp1_cmdt_vtx_local_coord_set(&c[1],INT16_VEC2(0,0));
    /* An explicit transparent polygon clears the complete 240-line framebuffer.
       NTSC vblank erase alone cannot clear this size reliably. */
    const vdp1_cmdt_draw_mode_t clear_mode={.pre_clipping_disable=true,.trans_pixel_disable=true,.end_code_disable=true};
    const int16_vec2_t corners[4]={INT16_VEC2_INITIALIZER(0,0),INT16_VEC2_INITIALIZER(319,0),INT16_VEC2_INITIALIZER(319,239),INT16_VEC2_INITIALIZER(0,239)};
    vdp1_cmdt_polygon_set(&c[2]);vdp1_cmdt_draw_mode_set(&c[2],clear_mode);c[2].cmd_colr=0;vdp1_cmdt_vtx_set(&c[2],corners);
    unsigned n=3;
    /* Actual $2836 depth order at the ROM's fixed prematch anchors. */
    draw_actor(&scene.referee,&n);
    draw_actor(&scene.presenter,&n);
    draw_actor(&scene.wrestlers[1],&n);
    draw_actor(&scene.wrestlers[3],&n);
    draw_actor(&scene.wrestlers[0],&n);
    draw_actor(&scene.wrestlers[2],&n);
    draw_frame(&wp02_frames[WP02_FOREGROUND_FRAME],0,0,false,&n);
    draw_frame(&wp02_frames[WP02_CREDIT_FRAME],0,0,false,&n);
    if(scene.names_visible){
        for(unsigned i=0;i<4;i++){
            if((i&1u)&&identities[i]==11u&&identities[i-1u]==10u)continue;
            draw_frame(wp02_name_find(identities[i],i),0,0,false,&n);
        }
        draw_frame(&wp02_frames[WP02_VS_FRAME],0,0,false,&n);
    }
    vdp1_cmdt_end_set(&c[n++]);commands->count=(uint16_t)n;
    if(n>max_commands)max_commands=n;
    vdp1_sync_cmdt_list_put(commands,0);vdp1_sync_put_wait();vdp1_sync_render();vdp1_sync();vdp2_sync();
}
void user_init(void)
{
    cd_block_init();smpc_peripheral_init();
    vdp_sync_vblank_out_set(intback,NULL);vdp_sync_vblank_in_set(sound_vblank,NULL);
    vdp2_tvmd_display_clear();
    vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE,VDP2_TVMD_HORZ_NORMAL_A,VDP2_TVMD_VERT_240);
    vdp2_cram_mode_set(2);
    const vdp1_env_t env={.bpp=VDP1_ENV_BPP_16,.rotation=VDP1_ENV_ROTATION_0,
        .color_mode=VDP1_ENV_COLOR_MODE_PALETTE,.sprite_type=0,
        .erase_color=RGB1555_INITIALIZER(0,0,0,0),
        .erase_points={INT16_VEC2_INITIALIZER(0,0),INT16_VEC2_INITIALIZER(320,240)}};
    vdp1_env_set(&env);
    vdp1_vram_partitions_set(COMMANDS,WP02_TEXTURE_BYTES,0,0);
    vdp1_vram_partitions_get(&video_mem);
    vdp1_sync_interval_set(-1);vdp1_sync_mode_set(VDP1_SYNC_MODE_CHANGE_ONLY);
    vdp2_sprite_priority_set(0,6);
    vdp2_scrn_back_color_set(VDP2_VRAM_ADDR(3,0x1fffe),RGB1555(1,0,0,0));
    const vdp2_scrn_bitmap_format_t format={.scroll_screen=VDP2_SCRN_NBG0,
        .ccc=VDP2_SCRN_CCC_PALETTE_256,.bitmap_size=VDP2_SCRN_BITMAP_SIZE_512X256,
        .palette_base=VDP2_CRAM_MODE_2_OFFSET(WP02_RING_PALETTE_BASE/256u,0,0),.bitmap_base=BG_ADDR};
    vdp2_scrn_bitmap_format_set(&format);
    vdp2_scrn_scroll_x_set(VDP2_SCRN_NBG0,0);vdp2_scrn_scroll_y_set(VDP2_SCRN_NBG0,0);
    vdp2_scrn_priority_set(VDP2_SCRN_NBG0,3);
    vdp2_vram_cycp_t cycles={0};
    for(unsigned b=0;b<4;b++)for(unsigned t=0;t<8;t++)cycles.pt[b].raw|=(uint32_t)VDP2_VRAM_CYCP_CPU_RW<<(t*4);
    cycles.pt[0].t0=VDP2_VRAM_CYCP_CHPNDR_NBG0;cycles.pt[0].t1=VDP2_VRAM_CYCP_CHPNDR_NBG0;
    cycles.pt[0].t2=VDP2_VRAM_CYCP_CHPNDR_NBG0;cycles.pt[0].t3=VDP2_VRAM_CYCP_CHPNDR_NBG0;
    vdp2_vram_cycp_set(&cycles);
    dbgio_init();dbgio_dev_default_init(DBGIO_DEV_MEDNAFEN_DEBUG);
    vdp2_sync();vdp2_sync_wait();
}
int main(void)
{
    _Static_assert(COMMANDS*32u+WP02_TEXTURE_BYTES+32u<=524288u,"VDP1 overflow");
    commands=vdp1_cmdt_list_alloc(COMMANDS);if(!commands)fatal(1);
    cdfs_config_default_set();cdfs_filelist_init(&files,entries,16);cdfs_filelist_root_read(&files);
    if(!preload_video("TEX.BIN",(uintptr_t)video_mem.texture_base,WP02_TEXTURE_BYTES))fatal(2);
    if(!preload_video("RING.BIN",BG_ADDR,WP02_RING_BYTES))fatal(3);
    if(!preload_video("CRAM.BIN",VDP2_CRAM_MODE_2_OFFSET(0,0,0),4096))fatal(4);
    const cdfs_filelist_entry_t *drv=file_named("SDRV.BIN"),*pcm=file_named("WP02PCM.PAK"),*bgm=file_named("WP02BGM.PAK");
    if(!drv||!pcm||!bgm||drv->size>sizeof(driver))fatal(5);
    if(cd_block_sectors_read(drv->starting_fad,driver,(drv->size+2047u)&~2047u))fatal(6);
    for(;;){
        vdp2_tvmd_display_clear();sound_ready=false;
        choose_roster();
        if(!wp02_init(&scene,identities,entropy))fatal(7);
        if(!wp02_audio_init(pcm->starting_fad,driver,drv->size,identities))fatal(8);
        if(!wp02_audio_bgm_load(bgm->starting_fad,bgm->size))fatal(13);
        sound_ready=true;
        max_commands=dropped_vblanks=render_count=0;
        wp02_metrics.end_us=0;
        dbgio_printf("WP02 START ids=%u,%u,%u,%u seed=%08lx soundend=%lu\n",identities[0],identities[1],identities[2],identities[3],(unsigned long)entropy,(unsigned long)wp02_audio_sound_ram_end());
        uint64_t start_us=wp02_audio_time_us();
        uint32_t old_vblank=vblank_count;
        bool logged=false,first=true;
        for(;;){
            vdp1_sync_wait();vdp2_sync_wait();
            uint32_t now_vblank=vblank_count;
            if(!first&&now_vblank-old_vblank>1u)dropped_vblanks+=now_vblank-old_vblank-1u;
            old_vblank=now_vblank;
            /* Sample scene time at the display boundary. Variable music
               decoding time must not shift the arcade update deadlines. */
            uint64_t elapsed=wp02_audio_time_us()-start_us;
            /* Preserve early rejection of a latched asynchronous audio error;
               the expensive stream service runs after VDP1 submission. */
            if(wp02_audio_state()!=WP02_AUDIO_READY){
                wp02_metrics.bgm_failures=wp02_audio_bgm_failures();
                wp02_metrics.bgm_max_service_us=wp02_audio_bgm_max_service_us();
                fatal(14);
            }
            smpc_peripheral_process();smpc_peripheral_digital_port(1,&pad);
            if(pad.held.button.start && !first)break;
            uint32_t wanted=1u+(uint32_t)((elapsed*7000000ull)/(448ull*272ull*1000000ull));
            while(!scene.done&&scene.ticks<wanted){
                wp02_tick(&scene);
                for(unsigned i=0;i<scene.sound_count;i++)if(!wp02_audio_emit((uint8_t)scene.effective_commands[i]))fatal(9);
                if(scene.error)fatal(10);
                if(scene.done)wp02_audio_scene_done();
            }
            render();render_count++;
            wp02_metrics.ticks=scene.ticks;wp02_metrics.renders=render_count;
            wp02_metrics.missed=dropped_vblanks;wp02_metrics.max_commands=max_commands;
            wp02_metrics.elapsed_us=(uint32_t)elapsed;wp02_metrics.done=scene.done;
            wp02_metrics.ids=((uint32_t)identities[0]<<24)|((uint32_t)identities[1]<<16)|((uint32_t)identities[2]<<8)|identities[3];
            wp02_metrics.state=scene.presenter.state;wp02_metrics.vblanks=vblank_count;
            /* render() starts VDP1 drawing and queues the sync; it does not
               wait for drawing or framebuffer change. Decode during that work. */
            if(!wp02_audio_service()){
                wp02_metrics.bgm_failures=wp02_audio_bgm_failures();
                wp02_metrics.bgm_max_service_us=wp02_audio_bgm_max_service_us();
                fatal(14);
            }
            wp02_metrics.bgm_refills=wp02_audio_bgm_refills();
            wp02_metrics.bgm_failures=wp02_audio_bgm_failures();
            wp02_metrics.bgm_max_service_us=wp02_audio_bgm_max_service_us();
            if(first){vdp1_sync_wait();vdp2_sync_wait();vdp2_scrn_display_set(VDP2_SCRN_DISP_NBG0);vdp2_tvmd_display_set();first=false;old_vblank=vblank_count;start_us=wp02_audio_time_us();}
            if(scene.done&&!logged){
                wp02_metrics.end_us=(uint32_t)elapsed;
                dbgio_printf("WP02 END ticks=%lu renders=%u missed=%u maxcmd=%u usec=%lu\n",(unsigned long)scene.ticks,render_count,dropped_vblanks,max_commands,(unsigned long)elapsed);
                logged=true;
            }
        }
        sound_ready=false;wp02_audio_stop();entropy^=vblank_count;
    }
}
