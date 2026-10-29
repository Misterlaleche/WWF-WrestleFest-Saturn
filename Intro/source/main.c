/* Independent WWF WrestleFest WP0. Original-ROM cells, sprites and timing.
 * No gameplay engine, previous repository or prior frontend is modified. */
#include <yaul.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "assets_generated.h"
#include "wp0_audio_player.h"
#include "wp0_framebuffer.h"
#include "wp0_timer.h"
#include "wp0_error_screen.h"
#include "wp0_sequence_cache.h"

#define CMD_CAPACITY 256U
#define VIDEO_ADDRESS ((uint8_t *)0x25C10000UL)
#define SECTOR_ROUND(n) (((n)+2047U)&~2047U)
#define STAGING_BYTES 131072U
#define AUDIO_HIGH_BYTES (704U*1024U)
#define AUDIO_VDP2_BASE 0x25E43000UL
#define AUDIO_VDP2_BYTES (0x1D000U)
/* NTSC320: (315MHz/11)/1820 line clock,263 lines. MAME arcade:
 * 7MHz/(448*272). This rational ratio preserves every arcade source frame. */
#define ARCADE_ADD (11U*1820U*263U)
#define ARCADE_THRESHOLD (448U*272U*45U)

typedef struct {
    uint32_t magic,error,video_clock,output_frames,source_frame,arcade_steps;
    uint32_t missed_vblanks,max_work_ticks,last_work_ticks,done,sprite_count;
    uint32_t sequence_offset,audio_command,boot_stage,commit_count;
    uint32_t audio_state,audio_error,audio_blocks,audio_consumed,audio_bytes;
    /* Original20 fields stay at their R01 offsets. FRT ticks are CPU/32.
     * CPU work excludes the subsequent GPU draw; DRAW is IRQ-observed. */
    uint32_t last_draw_ticks,max_draw_ticks,draw_irq_count,cef_fallback_count;
    uint32_t audio_region,audio_advance,audio_gap_us,audio_max_gap_us;
    uint32_t audio_queued,video_standard,framebuffer_error,framebuffer_flags;
} wp0_debug_t;
volatile wp0_debug_t wp0_debug = {.magic=0x57503044U};

static const uintptr_t map_addresses[3] = {0x25E60000UL,0x25E62000UL,0x25E64000UL};
static const unsigned map_lengths[3] = {1024,1024,4096};
static uint32_t maps[6144] __aligned(32);
static uint32_t *const map_words[3] = {maps,maps+1024,maps+2048};
static uint32_t palette[1024] __aligned(32);
static uint16_t dirty_first[3],dirty_last[3],pal_first,pal_last;
static uint16_t sprite_data[WP0_MAX_SPRITES*4U+4U] __aligned(32);
static unsigned sprite_count,sequence_offset,arcade_phase;
static vdp1_vram_partitions_t partitions;
static vdp1_cmdt_list_t *commands;
static cdfs_filelist_t files;
static cdfs_filelist_entry_t entries[32];
static smpc_peripheral_digital_t pad;
static bool restart_down;
static uint8_t *stage;
static wp0_sequence_cache_t sequence_cache;
static uint8_t audio_high[AUDIO_HIGH_BYTES] __aligned(32);
static wp0_audio_player_t audio_player;
static uint32_t audio_bytes;
static volatile bool audio_start_pending;
typedef struct { uintptr_t base; unsigned length; } audio_segment_t;
static const audio_segment_t audio_segments[]={
    {0x00200000UL,1048576U},
    {(uintptr_t)audio_high,AUDIO_HIGH_BYTES},
    {0x25A10002UL,0x6FFFEU}, /*two-byte loop interpolation guard at0x10000*/
    {AUDIO_VDP2_BASE,AUDIO_VDP2_BYTES},
    {0x25E08000UL,0x18000U}
};
#define AUDIO_SEGMENTS (sizeof(audio_segments)/sizeof(audio_segments[0]))
static const vdp2_scrn_t screens[3]={VDP2_SCRN_NBG0,VDP2_SCRN_NBG1,VDP2_SCRN_NBG2};

static uint16_t word(const void *p) { return *(const uint16_t *)p; }
static uint32_t longword(const void *p) { return *(const uint32_t *)p; }
static void vblank_out(void *unused __unused) {
    /* Yaul only does this wait in its VDP1 sync path, which R02 does not use.
     * Do not upload/draw into a framebuffer until its hardware swap ends. */
    vdp2_tvmd_vcount_wait(0);
    ++wp0_debug.video_clock;
    smpc_peripheral_intback_issue();
}
static void vblank_in(void *unused __unused) {
    ++wp0_debug.commit_count;
    wp0_framebuffer_vblank_in();
    if (wp0_framebuffer_status.error && !wp0_debug.error)
        wp0_debug.error=wp0_framebuffer_status.error;
    if (audio_start_pending && !wp0_debug.error) {
        audio_start_pending=false;
        if (!wp0_audio_start(&audio_player)) wp0_debug.error=305;
    }
}

static unsigned next_field(unsigned previous)
{
    uint32_t mask=cpu_intc_mask_get();
    cpu_intc_mask_set(0);
    while (wp0_debug.video_clock==previous) { }
    unsigned now=wp0_debug.video_clock;
    cpu_intc_mask_set(mask);
    return now;
}

static unsigned prime_framebuffers(void)
{
    volatile vdp1_ioregs_t *const regs=(volatile vdp1_ioregs_t *)VDP1_IOREG_BASE;
    volatile uint16_t *const pixels=(volatile uint16_t *)0x25C80000UL;
    wp0_framebuffer_stop();
    vdp2_tvmd_display_clear();
    vdp2_sync(); vdp2_sync_wait();
    /* Sega VDP1 2.1: this 256KiB window always maps the drawing buffer;
     * 16bpp CPU access must be word-sized. Manual change holds that bank
     * indefinitely, even if this one-time fill takes several fields.
     * This explicitly initializes BOTH buffers without assuming that
     * display-period erase operates when VDP2 DISP=0. */
    unsigned clock;
    for (unsigned bank=0;bank<2;++bank) {
        clock=next_field(wp0_debug.video_clock);
        regs->fbcr=3; /* One manual change at the following field boundary. */
        clock=next_field(clock);
        /* FB access is prohibited around the first HBlanks after a swap. */
        vdp2_tvmd_vcount_wait(2);
        for (unsigned i=0;i<0x20000U;++i) pixels[i]=0;
    }
    /* Sega VDP1 4.2: manual erase in the field immediately preceding
     * return to one-cycle mode. Both banks are already explicitly zero. */
    clock=next_field(wp0_debug.video_clock);
    regs->fbcr=2;
    clock=next_field(clock);
    regs->fbcr=0;
    clock=next_field(clock);
    return clock;
}

static void debug_observations(void)
{
    wp0_debug.audio_state=audio_player.state;
    wp0_debug.audio_error=audio_player.error;
    wp0_debug.audio_blocks=audio_player.decoded_blocks;
    wp0_debug.audio_consumed=audio_player.consumed_lower_bound;
    wp0_debug.audio_region=audio_player.last_region;
    wp0_debug.audio_advance=audio_player.last_advance;
    wp0_debug.audio_gap_us=audio_player.last_service_us;
    wp0_debug.audio_max_gap_us=audio_player.max_service_us;
    wp0_debug.audio_queued=audio_player.queued_samples;
    wp0_debug.last_draw_ticks=wp0_framebuffer_status.last_draw_ticks;
    wp0_debug.max_draw_ticks=wp0_framebuffer_status.max_draw_ticks;
    wp0_debug.draw_irq_count=wp0_framebuffer_status.draw_irq_count;
    wp0_debug.cef_fallback_count=wp0_framebuffer_status.cef_fallback_count;
    wp0_debug.framebuffer_error=wp0_framebuffer_status.error;
    /* FLAGS: EDSR bits0..1; submitted2; completed3; PAL4; CEF fallback5;
     * last CA region advance bits8..11 (15 means it was not read). */
    wp0_debug.framebuffer_flags=(wp0_framebuffer_status.deadline_edsr&3U)|
        (wp0_framebuffer_status.submitted?4U:0U)|
        (wp0_framebuffer_status.draw_done?8U:0U)|
        (wp0_debug.video_standard?16U:0U)|
        (wp0_framebuffer_status.cef_fallback_count?32U:0U)|
        ((audio_player.last_advance&15U)<<8);
}

/* VRAM and SCSP are16bit buses. Stage each compressed block in HWRAM, using
 * legal word accesses even when Rice block boundaries have an odd byte. */
static void bus_read(void *dst,uintptr_t address,unsigned n) {
    uint8_t *out=dst;
    if (address&1U) { *out++=(uint8_t)(*(volatile uint16_t *)(address-1U)); ++address; --n; }
    while (n>=2) {
        uint16_t value=*(volatile uint16_t *)address;
        *out++=value>>8; *out++=value; address+=2; n-=2;
    }
    if (n) *out=(uint8_t)(*(volatile uint16_t *)address>>8);
}
static bool audio_read(uint32_t offset,void *dst,uint32_t bytes,void *unused __unused) {
    if (offset>audio_bytes || bytes>audio_bytes-offset) return false;
    uint8_t *out=dst;
    for (unsigned i=0;i<AUDIO_SEGMENTS && bytes;++i) {
        if (offset>=audio_segments[i].length) { offset-=audio_segments[i].length; continue; }
        unsigned n=audio_segments[i].length-offset; if (n>bytes) n=bytes;
        bus_read(out,audio_segments[i].base+offset,n);
        out+=n; bytes-=n; offset=0;
    }
    return bytes==0;
}
static bool audio_store(unsigned offset,const uint8_t *src,unsigned bytes) {
    for (unsigned i=0;i<AUDIO_SEGMENTS && bytes;++i) {
        if (offset>=audio_segments[i].length) { offset-=audio_segments[i].length; continue; }
        unsigned n=audio_segments[i].length-offset; if (n>bytes) n=bytes;
        uintptr_t address=audio_segments[i].base+offset;
        if ((address|n|(uintptr_t)src)&1U) return false;
        for (unsigned p=0;p<n;p+=2) *(volatile uint16_t *)(address+p)=word(src+p);
        src+=n; bytes-=n; offset=0;
    }
    return bytes==0;
}
static bool audio_prepare(void) {
    const wp0_audio_ops_t ops=wp0_scsp_ops();
    return wp0_audio_prepare(&audio_player,audio_read,NULL,audio_bytes,&ops);
}
static bool audio_load(void) {
    unsigned capacity=1048576U+AUDIO_HIGH_BYTES+0x6FFFEU+AUDIO_VDP2_BYTES+0x18000U;
    for (unsigned i=0;i<files.entries_count;++i) if (!strcmp(files.entries[i].name,"TITLE02.RIC")) {
        audio_bytes=files.entries[i].size;
        if (audio_bytes>capacity) { wp0_debug.error=301; return false; }
        for (unsigned offset=0;offset<audio_bytes;) {
            unsigned n=audio_bytes-offset; if (n>STAGING_BYTES) n=STAGING_BYTES;
            if (cd_block_sectors_read(files.entries[i].starting_fad+offset/2048,stage,n)) {
                wp0_debug.error=302; return false;
            }
            if (n&1U) stage[n]=0;
            if (!audio_store(offset,stage,n+(n&1U))) { wp0_debug.error=302; return false; }
            offset+=n;
        }
        wp0_debug.audio_bytes=audio_bytes;
        if (!audio_prepare()) { wp0_debug.error=303; return false; }
        return true;
    }
    wp0_debug.error=304; return false;
}

static void transfer(void *dst,const void *src,unsigned size) {
    if (!size) return;
    scu_dma_transfer(0,dst,src,size);
    scu_dma_transfer_wait(0);
}

static unsigned read_file(const char *name,void *dst,unsigned capacity) {
    for (unsigned i=0;i<files.entries_count;++i) {
        if (strcmp(name,files.entries[i].name)) continue;
        unsigned size=files.entries[i].size;
        if (SECTOR_ROUND(size)>capacity) { wp0_debug.error=101; return 0; }
        if (cd_block_sectors_read(files.entries[i].starting_fad,dst,size)) {
            wp0_debug.error=102; return 0;
        }
        return size;
    }
    wp0_debug.error=103;
    return 0;
}

static bool load_texture(const char *name,uintptr_t dst,unsigned expected) {
    unsigned size=read_file(name,stage,STAGING_BYTES);
    if (size!=expected || wp0_debug.error) { wp0_debug.error=104; return false; }
    transfer((void *)dst,stage,size);
    return true;
}

static void reset_dirty(void) {
    for (unsigned i=0;i<3;++i) { dirty_first[i]=0xffff; dirty_last[i]=0; }
    pal_first=0xffff; pal_last=0;
}

static bool apply_frame(void) {
    if (sequence_offset==WP0_VIDEO_BYTES) { wp0_debug.done=1; return false; }
    if (sequence_offset+32U>WP0_VIDEO_BYTES) { wp0_debug.error=201; return false; }
    const uint8_t *base=wp0_sequence_cache_resolve(&sequence_cache,sequence_offset);
    if(!base) { wp0_debug.error=209; return false; }
    const uint8_t *p=base+32;
    unsigned size=longword(base),runs=word(base+4),colors=word(base+6),count=word(base+8);
    if (size<32 || size%4 || size>WP0_VIDEO_BYTES-sequence_offset) { wp0_debug.error=202; return false; }
    const uint8_t *end=base+size;
    for (unsigned i=0;i<runs;++i) {
        if (p+8>end) { wp0_debug.error=203; return false; }
        unsigned layer=word(p),first=word(p+2),length=word(p+4); p+=8;
        if (layer>=3 || first+length>map_lengths[layer] || p+length*4>end) {
            wp0_debug.error=204; return false;
        }
        memcpy(map_words[layer]+first,p,length*4); p+=length*4;
        if (first<dirty_first[layer]) dirty_first[layer]=first;
        if (first+length>dirty_last[layer]) dirty_last[layer]=first+length;
    }
    for (unsigned i=0;i<colors;++i) {
        if (p+4>end) { wp0_debug.error=205; return false; }
        unsigned index=word(p),v=word(p+2); p+=4;
        if (index>=1024) { wp0_debug.error=206; return false; }
        unsigned r=(v&15)*17,g=((v>>4)&15)*17,b=((v>>8)&15)*17;
        palette[index]=(b<<16)|(g<<8)|r;
        if (index<pal_first) pal_first=index;
        if (index+1>pal_last) pal_last=index+1;
    }
    if (count!=0xffffU) {
        if (count>WP0_MAX_SPRITES || p+count*8>end) { wp0_debug.error=207; return false; }
        memcpy(sprite_data,p,count*8); p+=count*8; sprite_count=count;
    }
    if (p!=end) { wp0_debug.error=208; return false; }
    unsigned priority=word(base+10);
    unsigned fgx=word(base+12),fgy=word(base+14),bgx=word(base+16),bgy=word(base+18);
    if (priority!=0x78) {
        unsigned t=fgx; fgx=bgx; bgx=t; t=fgy; fgy=bgy; bgy=t;
    }
    vdp2_scrn_scroll_x_set(VDP2_SCRN_NBG0,FIX16(bgx&511));
    vdp2_scrn_scroll_y_set(VDP2_SCRN_NBG0,FIX16((bgy+8)&511));
    vdp2_scrn_scroll_x_set(VDP2_SCRN_NBG1,FIX16(fgx&511));
    vdp2_scrn_scroll_y_set(VDP2_SCRN_NBG1,FIX16((fgy+8)&511));
    unsigned display=VDP2_SCRN_DISP_NBG0|VDP2_SCRN_DISPTP_NBG1|VDP2_SCRN_DISPTP_NBG2;
    if (priority==0x78) {
        vdp2_scrn_priority_set(VDP2_SCRN_NBG0,1);
        vdp2_scrn_priority_set(VDP2_SCRN_NBG1,2);
    } else {
        display=VDP2_SCRN_DISPTP_NBG0|VDP2_SCRN_DISP_NBG1|VDP2_SCRN_DISPTP_NBG2;
        vdp2_scrn_priority_set(VDP2_SCRN_NBG0,priority==0x7c?4:2);
        vdp2_scrn_priority_set(VDP2_SCRN_NBG1,1);
    }
    vdp2_scrn_display_set((vdp2_scrn_disp_t)display);
    sequence_offset+=size;
    wp0_debug.sequence_offset=sequence_offset;
    wp0_debug.source_frame=longword(base+20);
    wp0_debug.audio_command=word(base+24);
    if (wp0_debug.audio_command==2) audio_start_pending=true;
    ++wp0_debug.arcade_steps;
    return true;
}

static void enqueue_changes(void) {
    for (unsigned i=0;i<3;++i) if (dirty_first[i]!=0xffff) {
        unsigned first=dirty_first[i],count=dirty_last[i]-first;
        vdp_dma_enqueue((void *)(map_addresses[i]+first*4),map_words[i]+first,count*4);
    }
    if (pal_first!=0xffff)
        vdp_dma_enqueue((void *)(0x25F00000UL+pal_first*4),palette+pal_first,(pal_last-pal_first)*4);
}

static void draw_commands(void) {
    vdp1_cmdt_t *list=commands->cmdts;
    memset(list,0,(sprite_count+3)*sizeof(*list));
    vdp1_cmdt_system_clip_coord_set(&list[0]);
    vdp1_cmdt_vtx_system_clip_coord_set(&list[0],INT16_VEC2(319,239));
    vdp1_cmdt_local_coord_set(&list[1]);
    vdp1_cmdt_vtx_local_coord_set(&list[1],INT16_VEC2(0,0));
    /* The normal one-cycle erase clears the displayed buffer during scanout.
     * No 320x240 clear polygon is serialized with these original sprites. */
    const vdp1_cmdt_draw_mode_t mode={.color_mode=0,.pre_clipping_disable=true,.end_code_disable=true};
    for (unsigned i=0;i<sprite_count;++i) {
        const uint16_t *r=sprite_data+i*4;
        vdp1_cmdt_t *cmd=&list[i+2];
        const vdp1_cmdt_color_bank_t bank={.type_0.dc=768+(r[3]&15)*16};
        vdp1_cmdt_normal_sprite_set(cmd); vdp1_cmdt_draw_mode_set(cmd,mode);
        vdp1_cmdt_color_mode0_set(cmd,bank);
        vdp1_cmdt_char_base_set(cmd,(vdp1_vram_t)partitions.texture_base+r[0]*128U);
        vdp1_cmdt_char_size_set(cmd,16,16);
        vdp1_cmdt_char_flip_set(cmd,(vdp1_cmdt_char_flip_t)(r[3]&0x30));
        cmd->cmd_xa=(int16_t)r[1]; cmd->cmd_ya=(int16_t)r[2];
    }
    vdp1_cmdt_end_set(&list[sprite_count+2]);
    commands->count=sprite_count+3;
    wp0_debug.sprite_count=sprite_count;
}

/* Initial maps and all1024 colors are a bulk load, not one active field.
 * This same path owns cold start and replay: commit the complete state while
 * hidden and with the deadline controller OFF, then start the timed scene. */
static bool first_frame_prepare_hidden(void) {
    wp0_framebuffer_stop();
    vdp2_tvmd_display_clear();
    vdp2_sync(); vdp2_sync_wait();
    sequence_offset=0; arcade_phase=0; sprite_count=0;
    wp0_debug.done=0; wp0_debug.arcade_steps=0;
    reset_dirty();
    if (!apply_frame()) return false;
    enqueue_changes(); draw_commands();
    vdp2_sync(); vdp2_sync_wait();
    /* The common active tail must not queue the bulk upload a second time. */
    reset_dirty();
    return !wp0_debug.error;
}

void user_init(void) {
    cd_block_init(); smpc_peripheral_init();
    vdp_sync_vblank_out_set(vblank_out,NULL);
    vdp_sync_vblank_in_set(vblank_in,NULL);
    vdp2_tvmd_display_clear();
    vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE,VDP2_TVMD_HORZ_NORMAL_A,VDP2_TVMD_VERT_240);
    vdp2_cram_mode_set(2);
    const vdp1_env_t env={.bpp=VDP1_ENV_BPP_16,.rotation=VDP1_ENV_ROTATION_0,
        .color_mode=VDP1_ENV_COLOR_MODE_PALETTE,.sprite_type=0,
        .erase_color=RGB1555_INITIALIZER(0,0,0,0),
        .erase_points={INT16_VEC2_INITIALIZER(0,0),INT16_VEC2_INITIALIZER(319,239)}};
    vdp1_env_set(&env);
    vdp1_vram_partitions_set(CMD_CAPACITY,WP0_SPRITE_BYTES,0,0);
    vdp1_vram_partitions_get(&partitions);
    vdp2_sprite_priority_set(0,3);
    vdp2_scrn_back_color_set(VDP2_VRAM_ADDR(3,0x1fffe),RGB1555(1,0,0,0));
    for (unsigned i=0;i<3;++i) {
        const vdp2_scrn_cell_format_t fmt={.scroll_screen=screens[i],
            .ccc=VDP2_SCRN_CCC_PALETTE_16,.char_size=i==2?VDP2_SCRN_CHAR_SIZE_1X1:VDP2_SCRN_CHAR_SIZE_2X2,
            .pnd_size=2,.aux_mode=VDP2_SCRN_AUX_MODE_0,.plane_size=VDP2_SCRN_PLANE_SIZE_1X1,
            .cpd_base=VDP2_VRAM_ADDR(i,0),.palette_base=VDP2_CRAM_MODE_2_OFFSET(i==2?0:i+1,0,0)};
        const vdp2_scrn_normal_map_t map={.base_addr={map_addresses[i],map_addresses[i],map_addresses[i],map_addresses[i]}};
        vdp2_scrn_cell_format_set(&fmt,&map);
        vdp2_scrn_scroll_x_set(screens[i],0); vdp2_scrn_scroll_y_set(screens[i],FIX16(8));
        vdp2_scrn_priority_set(screens[i],i==2?5:i+1);
    }
    /* Sega VDP2 §3.3: both split halves share CPU windows; each window
     * starts after NO_ACCESS in both halves. The spare VRAM stores audio. */
    vdp2_vram_cycp_t cycles;
    cycles.pt[0].raw=0x4FEEEEEE;
    cycles.pt[1].raw=0x5FEEEEEE;
    cycles.pt[2].raw=0x6FFFEEEE;
    cycles.pt[3].raw=0x012FEEEE;
    vdp2_vram_cycp_set(&cycles);
    vdp2_scrn_display_set(0);
    wp0_scsp_initialize();
    cpu_frt_init(CPU_FRT_CLOCK_DIV_32);
    wp0_framebuffer_initialize();
    wp0_debug.video_standard=vdp2_tvmd_tv_standard_get();
    vdp2_sync(); vdp2_sync_wait();
}

int main(void) {
    wp0_debug.boot_stage=1;
    commands=vdp1_cmdt_list_alloc(CMD_CAPACITY);
    stage=malloc(STAGING_BYTES);
    if (!commands || !stage) { wp0_debug.error=100; goto failure; }
    cdfs_config_default_set(); cdfs_filelist_init(&files,entries,32); cdfs_filelist_root_read(&files);
    if (!load_texture("BG1.TEX",VDP2_VRAM_ADDR(0,0),WP0_BG1_BYTES) ||
        !load_texture("BG0.TEX",VDP2_VRAM_ADDR(1,0),WP0_BG0_BYTES) ||
        !load_texture("FONT.TEX",VDP2_VRAM_ADDR(2,0),WP0_FONT_BYTES) ||
        !load_texture("SPR.TEX",(uintptr_t)partitions.texture_base,WP0_SPRITE_BYTES)) goto failure;
    wp0_debug.boot_stage=2;
    if ((uintptr_t)partitions.texture_base+WP0_SPRITE_BYTES>(uintptr_t)VIDEO_ADDRESS ||
        WP0_VIDEO_BYTES>0x70000U) { wp0_debug.error=105; goto failure; }
    if (read_file("SEQ.DAT",(void *)LWRAM(0),1048576U)!=WP0_VIDEO_BYTES) goto failure;
    for (unsigned offset=0;offset<WP0_VIDEO_BYTES;) {
        unsigned n=WP0_VIDEO_BYTES-offset; if (n>STAGING_BYTES) n=STAGING_BYTES;
        memcpy(stage,(const void *)LWRAM(offset),n);
        transfer(VIDEO_ADDRESS+offset,stage,n); offset+=n;
    }
    wp0_debug.boot_stage=3;
    if (WP0_FONT_BYTES>0x3000U || WP0_BG1_BYTES>0x8000U || !audio_load()) goto failure;
    /* Keep the existing128KiB load buffer. Six active bulk packets total
     *106072bytes; preload them now to avoid B-bus reads during playback.
     *The initial full packet is still prepared hidden, outside the guard. */
    if(!wp0_sequence_cache_prepare(&sequence_cache,VIDEO_ADDRESS,WP0_VIDEO_BYTES,
        stage,STAGING_BYTES)) { wp0_debug.error=209; goto failure; }
    wp0_debug.boot_stage=3;
    if (!first_frame_prepare_hidden()) goto failure;
    unsigned submitted_clock=prime_framebuffers();
    if (!wp0_framebuffer_begin()) { wp0_debug.error=wp0_framebuffer_status.error; goto failure; }
    /* Publish the VDP2 commit request before PTMR=1. Otherwise a VBlank-IN
     * between the two could accept sprites/audio but omit scroll registers. */
    vdp2_sync();
    if (wp0_debug.error || !wp0_framebuffer_submit(commands->cmdts,commands->count*sizeof(vdp1_cmdt_t))) goto failure;
    unsigned last_clock=submitted_clock;
    for (;;) {
        unsigned now=next_field(submitted_clock);
        vdp2_sync_wait();
        if (wp0_framebuffer_status.error) { wp0_debug.error=wp0_framebuffer_status.error; goto failure; }
        unsigned elapsed_frames=now-last_clock;
        if (elapsed_frames>1) {
            wp0_debug.missed_vblanks+=elapsed_frames-1;
            wp0_debug.error=WP0_FB_PREPARE_LATE; goto failure;
        }
        last_clock=now;
        if (!wp0_framebuffer_begin()) { wp0_debug.error=wp0_framebuffer_status.error; goto failure; }
        vdp2_tvmd_display_set();
        uint16_t started=wp0_timer_count_get();
        reset_dirty();
        smpc_peripheral_process(); smpc_peripheral_digital_port(1,&pad);
        bool restart=(pad.pressed.button.a && pad.pressed.button.b && pad.pressed.button.c && pad.pressed.button.start) ||
                     (wp0_debug.done && pad.pressed.button.start);
        if (restart && !restart_down) {
            wp0_framebuffer_stop();
            wp0_audio_stop(&audio_player); audio_start_pending=false;
            vdp2_tvmd_display_clear(); vdp2_sync(); vdp2_sync_wait();
            if (!audio_prepare()) { wp0_debug.error=306; goto failure; }
            if (!first_frame_prepare_hidden()) goto failure;
            now=prime_framebuffers(); last_clock=now; elapsed_frames=1;
            started=wp0_timer_count_get();
            if (!wp0_framebuffer_begin()) { wp0_debug.error=wp0_framebuffer_status.error; goto failure; }
        } else if (!wp0_debug.done) {
            arcade_phase+=ARCADE_ADD;
            if (arcade_phase>=ARCADE_THRESHOLD) { arcade_phase-=ARCADE_THRESHOLD; apply_frame(); }
        }
        restart_down=restart;
        if (wp0_debug.done) wp0_audio_stop(&audio_player);
        if (!wp0_audio_service(&audio_player,elapsed_frames*16716U)) wp0_debug.error=307;
        debug_observations();
        if (wp0_debug.error) goto failure;
        enqueue_changes(); draw_commands();
        vdp2_sync();
        if (!wp0_framebuffer_submit(commands->cmdts,commands->count*sizeof(vdp1_cmdt_t))) {
            wp0_debug.error=wp0_framebuffer_status.error; goto failure;
        }
        submitted_clock=now;
        wp0_debug.last_work_ticks=(uint16_t)(wp0_timer_count_get()-started);
        if (wp0_debug.last_work_ticks>wp0_debug.max_work_ticks) wp0_debug.max_work_ticks=wp0_debug.last_work_ticks;
        ++wp0_debug.output_frames;
    }
failure:
    audio_start_pending=false;
    wp0_audio_stop(&audio_player);
    wp0_framebuffer_stop();
    if (!wp0_debug.error) wp0_debug.error=wp0_framebuffer_status.error;
    debug_observations();
    const uint32_t diagnostic[12]={
        wp0_debug.error,wp0_debug.audio_error,wp0_debug.source_frame,
        wp0_debug.missed_vblanks,wp0_debug.max_work_ticks,wp0_debug.max_draw_ticks,
        wp0_debug.audio_region,wp0_debug.audio_gap_us,wp0_debug.audio_queued,
        wp0_debug.audio_consumed,wp0_debug.audio_blocks,wp0_debug.framebuffer_flags
    };
    wp0_error_screen(diagnostic);
    for (;;) { vdp2_sync(); vdp2_sync_wait(); }
    return 0;
}
