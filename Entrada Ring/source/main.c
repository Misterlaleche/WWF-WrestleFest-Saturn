/* Independent WP01. Original corridor art and native SH-2 scene logic. */
#include <yaul.h>
#include <stdint.h>
#include <string.h>
#include "wp01_scene.h"
#include "wp01_audio.h"
#include "wp01_pairs.h"
#include "entrance_assets.h"
#include "environment_assets.h"
#include "wf_entry_labels_rom.h"

#define CMD_CAPACITY 128U
#define SPRITE_BYTES 90496U
#define SPRITE_PALETTE_BYTES 704U
/*7MHz/(448*272) arcade vs315MHz/(11*1820*263) Saturn NTSC320.*/
#define ARCADE_ADD (11U*1820U*263U)
#define ARCADE_THRESHOLD (448U*272U*45U)
#define SETUP_FRAMES 6U
#define TOTAL_FRAMES (SETUP_FRAMES+WP01_SCENE_FRAMES)

extern uint8_t asset_bg0[],asset_bg1[],asset_font[],asset_map1[],asset_text[];
extern uint8_t asset_crowd0[],asset_crowd1[],asset_crowd2[];
extern uint8_t asset_envpal[],asset_sprites[],asset_spritepal[];

/*Exported telemetry, sampled externally; no diagnostics on the arcade image.*/
typedef struct {
    uint32_t magic,error,video_clock,output_frames,source_frame,arcade_steps;
    uint32_t missed_vblanks,max_work_ticks,last_work_ticks,loops,sprite_count;
    uint32_t left_id,right_id,left_pose,right_pose,left_y,right_y;
    uint32_t light_y,light_visible,wipe_tile,audio_written,boot_stage;
    uint32_t current_crowd,seed,commit_count,display_source_frame;
} wp01_debug_t;
volatile wp01_debug_t wp01_debug={.magic=0x57503144U};
static wp01_scene_t scene,previous;
static unsigned source_frame,arcade_phase,selected[2],last_crowd=~0U;
static int last_wipe=-999;
static uint32_t random_state;
static uint32_t text_map[4096] __aligned(32);
static uint32_t normal_text[4096] __aligned(32);
static uint8_t raw_text[8192] __aligned(32);
static vdp1_cmdt_list_t *commands;
static vdp1_vram_partitions_t partitions;
static smpc_peripheral_digital_t pad;
static bool button_down,audio_start_pending;
static uint32_t audio_program_bytes;

static void vblank_out(void *u __unused){++wp01_debug.video_clock;smpc_peripheral_intback_issue();}
static void vblank_in(void *u __unused){
    ++wp01_debug.commit_count;
    if(audio_start_pending){audio_start_pending=false;wp01_audio_start();}
}
static void transfer(void *dst,const void *src,unsigned bytes){scu_dma_transfer(0,dst,src,bytes);scu_dma_transfer_wait(0);}
static uint16_t read16(const uint8_t *p){return (uint16_t)((p[0]<<8)|p[1]);}

static void apply_label(const wf_entry_text_patch_t *p,unsigned n){
    for(unsigned i=0;i<n;i++){
        raw_text[p[i].offset+1]=p[i].packed&255;
        raw_text[p[i].offset+3]=p[i].packed>>8;
    }
}
static void prepare_names(void){
    memcpy(raw_text,asset_text,sizeof(raw_text));
    for(unsigned i=0;i<sizeof(wf_entry_name_clear_offsets)/sizeof(wf_entry_name_clear_offsets[0]);i++)
        memset(raw_text+wf_entry_name_clear_offsets[i],0,4);
    const wf_entry_names_t *left=&wf_entry_names[scene.actor[0].internal_id];
    const wf_entry_names_t *right=&wf_entry_names[scene.actor[1].internal_id];
    apply_label(left->first,left->first_count);apply_label(right->second,right->second_count);
    for(unsigned i=0;i<2048;i++){
        unsigned a=read16(raw_text+4*i),b=read16(raw_text+4*i+2);
        unsigned code=(a&255)|((b&15)<<8),pal=(b>>4)&15;
        normal_text[i]=normal_text[i+2048]=(pal<<16)|(0x40000U/32U+code);
    }
}
static uint32_t entropy(void){
    return wp01_rng_21b4(&random_state,wp01_debug.video_clock+wp01_debug.loops*0x109U,source_frame);
}
static void choose_pair(void){
    wp01_pair_from_ticket(entropy()%29,selected);
}
static void restart(bool new_pair){
    wp01_audio_stop();
    if(new_pair)choose_pair();
    wp01_scene_init(&scene,selected[0],selected[1],entropy());
    memset(&previous,0,sizeof(previous));
    source_frame=0;arcade_phase=ARCADE_THRESHOLD-1U;
    last_wipe=-999;last_crowd=~0U;prepare_names();
    wp01_audio_prepare();
    audio_start_pending=true;
    ++wp01_debug.loops;
}
static void advance(void){
    if(++source_frame>=TOTAL_FRAMES){restart(true);return;}
    if(source_frame>SETUP_FRAMES){previous=scene;wp01_scene_step(&scene);}
    ++wp01_debug.arcade_steps;
}
static void backgrounds(void){
    static const unsigned order[8]={0,1,0,1,0,1,2,1};
    static const void *const crowd[3]={asset_crowd0,asset_crowd1,asset_crowd2};
    unsigned ticks=scene.frame>24?scene.frame-24:0;if(ticks>319)ticks=319;
    unsigned c=order[(ticks/9)&7U];
    if(c!=last_crowd){vdp_dma_enqueue((void *)0x25E60000UL,crowd[c],4096);last_crowd=c;}
    int wipe=source_frame<SETUP_FRAMES?0x4c5:scene.wipe_tile;
    int text_key=wipe<0?(source_frame==30?-2:-1):wipe;
    if(text_key!=last_wipe){
        if(wipe<0){
            memcpy(text_map,normal_text,sizeof(text_map));
            /*Original credit draw follows the full reveal by one tick.*/
            if(source_frame==30){
                for(unsigned col=27;col<=35;col++)if(col!=34){
                    unsigned i=30*64+col;
                    text_map[i]=text_map[i+2048]=0x40000U/32U;
                }
            }
        }
        else for(unsigned i=0;i<4096;i++)text_map[i]=0x40000U/32U+(unsigned)wipe;
        vdp_dma_enqueue((void *)0x25E64000UL,text_map,sizeof(text_map));
        last_wipe=text_key;
    }
    wp01_debug.current_crowd=c;
    wp01_debug.wipe_tile=(uint32_t)wipe;
}
static void put_pose(unsigned *count,const wp01_pose_t *pose,int x,int y){
    const vdp1_cmdt_draw_mode_t mode={.color_mode=0,.pre_clipping_disable=true,.end_code_disable=true};
    for(unsigned i=0;i<pose->count;i++){
        const wp01_piece_t *p=&wp01_entrance_cells[pose->first+i];
        int px=x+p->x,py=y+p->y;
        if(px<=-16||px>=320||py<=-16||py>=240)continue;
        if(*count>=CMD_CAPACITY-1){wp01_debug.error=201;return;}
        vdp1_cmdt_t *cmd=&commands->cmdts[(*count)++];
        const vdp1_cmdt_color_bank_t bank={.type_0.dc=768+p->palette};
        vdp1_cmdt_normal_sprite_set(cmd);vdp1_cmdt_draw_mode_set(cmd,mode);
        vdp1_cmdt_color_mode0_set(cmd,bank);
        vdp1_cmdt_char_base_set(cmd,(vdp1_vram_t)partitions.texture_base+p->tile*128U);
        vdp1_cmdt_char_size_set(cmd,16,16);
        vdp1_cmdt_char_flip_set(cmd,(vdp1_cmdt_char_flip_t)p->flip);
        cmd->cmd_xa=px;cmd->cmd_ya=py;
    }
}
static void sprites(void){
    memset(commands->cmdts,0,CMD_CAPACITY*sizeof(vdp1_cmdt_t));
    vdp1_cmdt_t *list=commands->cmdts;
    vdp1_cmdt_system_clip_coord_set(list);vdp1_cmdt_vtx_system_clip_coord_set(list,INT16_VEC2(319,239));
    vdp1_cmdt_local_coord_set(list+1);vdp1_cmdt_vtx_local_coord_set(list+1,INT16_VEC2(0,0));
    const vdp1_cmdt_draw_mode_t clear={.pre_clipping_disable=true,.trans_pixel_disable=true,.end_code_disable=true};
    const int16_vec2_t rect[4]={INT16_VEC2_INITIALIZER(0,0),INT16_VEC2_INITIALIZER(319,0),INT16_VEC2_INITIALIZER(319,239),INT16_VEC2_INITIALIZER(0,239)};
    vdp1_cmdt_polygon_set(list+2);vdp1_cmdt_draw_mode_set(list+2,clear);list[2].cmd_colr=0;vdp1_cmdt_vtx_set(list+2,rect);
    unsigned n=3;
    if(source_frame>=SETUP_FRAMES){
        if(previous.light.visible)put_pose(&n,&wp01_floor_light_frame,previous.light.x,previous.light.y);
        for(unsigned a=0;a<2;a++){
            const wp01_scene_actor_t *actor=&previous.actor[a];
            if(actor->visible)put_pose(&n,&wp01_entrance_frames[actor->type-0x40][actor->selector],actor->anchor_x,actor->anchor_y);
        }
    }
    vdp1_cmdt_end_set(list+n);commands->count=n+1;
    wp01_debug.sprite_count=n-3;
    wp01_debug.left_id=scene.actor[0].internal_id;wp01_debug.right_id=scene.actor[1].internal_id;
    wp01_debug.left_pose=previous.actor[0].selector;wp01_debug.right_pose=previous.actor[1].selector;
    wp01_debug.left_y=previous.actor[0].anchor_y;wp01_debug.right_y=previous.actor[1].anchor_y;
    wp01_debug.light_y=previous.light.y;wp01_debug.light_visible=previous.light.visible;
}

void user_init(void){
    cpu_frt_init(CPU_FRT_CLOCK_DIV_32);
    if(cd_block_init())wp01_debug.error=101;
    smpc_peripheral_init();
    vdp_sync_vblank_out_set(vblank_out,NULL);vdp_sync_vblank_in_set(vblank_in,NULL);
    vdp2_tvmd_display_clear();
    vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE,VDP2_TVMD_HORZ_NORMAL_A,VDP2_TVMD_VERT_240);
    vdp2_cram_mode_set(2);
    const vdp1_env_t env={.bpp=VDP1_ENV_BPP_16,.rotation=VDP1_ENV_ROTATION_0,.color_mode=VDP1_ENV_COLOR_MODE_PALETTE,.sprite_type=0,
        .erase_color=RGB1555_INITIALIZER(0,0,0,0),.erase_points={INT16_VEC2_INITIALIZER(0,0),INT16_VEC2_INITIALIZER(320,240)}};
    vdp1_env_set(&env);vdp1_vram_partitions_set(CMD_CAPACITY,SPRITE_BYTES,0,0);vdp1_vram_partitions_get(&partitions);
    vdp1_sync_interval_set(-1);vdp1_sync_mode_set(VDP1_SYNC_MODE_CHANGE_ONLY);vdp2_sprite_priority_set(0,3);
    vdp2_scrn_back_color_set(VDP2_VRAM_ADDR(3,0x1fffe),RGB1555(1,0,0,0));
    const vdp2_scrn_t screens[3]={VDP2_SCRN_NBG0,VDP2_SCRN_NBG1,VDP2_SCRN_NBG2};
    const uintptr_t maps[3]={0x25E60000UL,0x25E62000UL,0x25E64000UL};
    for(unsigned i=0;i<3;i++){
        const vdp2_scrn_cell_format_t fmt={.scroll_screen=screens[i],.ccc=VDP2_SCRN_CCC_PALETTE_16,
            .char_size=i==2?VDP2_SCRN_CHAR_SIZE_1X1:VDP2_SCRN_CHAR_SIZE_2X2,.pnd_size=2,.aux_mode=VDP2_SCRN_AUX_MODE_0,
            .plane_size=VDP2_SCRN_PLANE_SIZE_1X1,.cpd_base=VDP2_VRAM_ADDR(i,0),.palette_base=VDP2_CRAM_MODE_2_OFFSET(i==2?0:i+1,0,0)};
        const vdp2_scrn_normal_map_t map={.base_addr={maps[i],maps[i],maps[i],maps[i]}};
        vdp2_scrn_cell_format_set(&fmt,&map);
        vdp2_scrn_scroll_x_set(screens[i],i==2?0:FIX16(320));vdp2_scrn_scroll_y_set(screens[i],FIX16(8));
        vdp2_scrn_priority_set(screens[i],i==2?5:(i==0?2:1));
    }
    vdp2_vram_cycp_t cycles;cycles.pt[0].raw=0x4FEEEEEE;cycles.pt[1].raw=0x5FEEEEEE;
    cycles.pt[2].raw=0x6FFFEEEE;cycles.pt[3].raw=0x012FEEEE;vdp2_vram_cycp_set(&cycles);
    vdp2_scrn_display_set(0);cpu_frt_init(CPU_FRT_CLOCK_DIV_32);wp01_audio_init();
    vdp2_sync();vdp2_sync_wait();
}

int main(void){
    wp01_debug.boot_stage=1;commands=vdp1_cmdt_list_alloc(CMD_CAPACITY);
    if(wp01_debug.error)goto failure;
    if(!commands){wp01_debug.error=100;goto failure;}
    transfer((void *)0x25E00000UL,asset_bg0,WP01_BG0_BYTES);
    transfer((void *)0x25E20000UL,asset_bg1,WP01_BG1_BYTES);
    transfer((void *)0x25E40000UL,asset_font,WP01_FONT_BYTES);
    transfer((void *)0x25E62000UL,asset_map1,4096);
    transfer((void *)0x25F00000UL,asset_envpal,3072);
    transfer((void *)0x25F00C00UL,asset_spritepal,SPRITE_PALETTE_BYTES);
    transfer((void *)partitions.texture_base,asset_sprites,SPRITE_BYTES);
    wp01_debug.boot_stage=2;
    cdfs_filelist_t files;cdfs_filelist_entry_t entries[16];
    cdfs_config_default_set();cdfs_filelist_init(&files,entries,16);cdfs_filelist_root_read(&files);
    bool bank_loaded=false;
    for(unsigned i=0;i<files.entries_count;i++)if(!strcmp(files.entries[i].name,"SOUNDRAM.BIN")){
        unsigned size=files.entries[i].size;
        if(size>0x80000U||(size&1U)||cd_block_sectors_read(files.entries[i].starting_fad,(void *)LWRAM(0),size)
            ||!wp01_audio_bank_load((void *)LWRAM(0),size)){wp01_debug.error=102;goto failure;}
        bank_loaded=true;
    }
    if(!bank_loaded){wp01_debug.error=103;goto failure;}
    for(unsigned i=0;i<files.entries_count;i++)if(!strcmp(files.entries[i].name,"AUDIOREG.BIN")){
        unsigned size=files.entries[i].size;
        if(size>1048576U||cd_block_sectors_read(files.entries[i].starting_fad,(void *)LWRAM(0),size)
            ||!wp01_audio_program_load((void *)LWRAM(0),size)){wp01_debug.error=104;goto failure;}
        audio_program_bytes=size;
    }
    if(!audio_program_bytes){wp01_debug.error=105;goto failure;}
    smpc_peripheral_process();const smpc_time_t *rtc=smpc_rtc_time_get();
    random_state=0x57503101U^rtc->seconds^((uint32_t)rtc->minutes<<8)^((uint32_t)rtc->hours<<16)^((uint32_t)rtc->day<<24)^cpu_frt_count_get();
    restart(true);wp01_debug.boot_stage=3;
    backgrounds();sprites();vdp1_sync_cmdt_list_put(commands,0);vdp1_sync_put_wait();vdp1_sync_render();vdp1_sync();
    vdp2_scrn_display_set(VDP2_SCRN_DISPTP_NBG0|VDP2_SCRN_DISP_NBG1|VDP2_SCRN_DISPTP_NBG2);vdp2_sync();
    unsigned last_clock=0;
    for(;;){
        vdp1_sync_wait();vdp2_sync_wait();
        unsigned now=wp01_debug.video_clock;
        if(last_clock&&now-last_clock>1)wp01_debug.missed_vblanks+=now-last_clock-1;
        last_clock=now;vdp2_tvmd_display_set();uint16_t start=cpu_frt_count_get();
        smpc_peripheral_process();smpc_peripheral_digital_port(1,&pad);
        bool fresh=pad.pressed.button.start,again=pad.pressed.button.a;
        if((fresh||again)&&!button_down)restart(fresh);
        else{
            arcade_phase+=ARCADE_ADD;
            if(arcade_phase>=ARCADE_THRESHOLD){arcade_phase-=ARCADE_THRESHOLD;advance();}
        }
        button_down=fresh||again;
        wp01_audio_service();backgrounds();sprites();
        if(wp01_debug.error)goto failure;
        vdp1_sync_cmdt_list_put(commands,0);vdp1_sync_put_wait();vdp1_sync_render();vdp1_sync();vdp2_sync();
        wp01_debug.source_frame=source_frame;wp01_debug.display_source_frame=1271+source_frame;
        wp01_debug.audio_written=wp01_audio_written();wp01_debug.seed=random_state;
        wp01_debug.last_work_ticks=(uint16_t)(cpu_frt_count_get()-start);
        if(wp01_debug.last_work_ticks>wp01_debug.max_work_ticks)wp01_debug.max_work_ticks=wp01_debug.last_work_ticks;
        ++wp01_debug.output_frames;
    }
failure:
    wp01_audio_stop();vdp2_scrn_display_set(0);vdp2_scrn_back_color_set(VDP2_VRAM_ADDR(3,0x1fffe),RGB1555(1,20,0,0));
    vdp2_tvmd_display_set();for(;;){vdp2_sync();vdp2_sync_wait();}
}
