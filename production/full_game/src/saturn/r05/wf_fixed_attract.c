/* Derived from supplied R05 attract_main.c, silent in-process integration.
 * Visual packet renderer/cadence retained. See PROVENANCE.md. */
#include <yaul.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "wp0_framebuffer.h"
#include "wp0_timer.h"
#include "wf_fixed_attract.h"
#include "wf_fixed_attract_field.h"
#include "../wf_workspace.h"
#include "../wf_platform.h"
#include "../wf_startup_notice.h"
#include "../wf_front_resource_pack.h"
#include "wf_r18_cdfs.h"
#include "wp2015_cd_rate.h"
#include "wp0_cd_stream.h"
#include "wp0_attract_packet.h"

#define CMD_CAPACITY 1024U
#define SPRITE_BYTES (3584U*128U)
#define PACKET_CAPACITY 131072U
#define RING_CAPACITY 524288U
#define PREFILL_BYTES 393216U
#define LOOP_CACHE_BYTES 262144U

volatile wf_fixed_attract_debug_t wf_attract_debug={.magic=0x57464135U};
#define wp0_debug wf_attract_debug
static const uintptr_t map_addresses[3]={0x25E60000UL,0x25E64000UL,0x25E68000UL};
static const uintptr_t line_addresses[2]={0x25E6C000UL,0x25E6C800UL};
static const uintptr_t texture_addresses[3]={0x25E00000UL,0x25E20000UL,0x25E40000UL};
#define maps (wf_workspace.attract.maps)
#define palette (wf_workspace.attract.palette)
#define line_scroll (wf_workspace.attract.lines)
#define sprites (wf_workspace.attract.sprites)
#define packet (wf_workspace.attract.packet)
#define loop_cache (wf_workspace.attract.loop_cache)
static uint16_t map_first[3],map_last[3],pal_first,pal_last;
static bool lines_dirty[2];
static unsigned deferred_full_maps;
static unsigned sprite_count,total_fields,file_bytes,next_output;
static uint32_t loop_offset,loop_file_bytes,loop_fad,loop_cache_bytes,logical_position;
static bool cache_mode,playing_loop;
static uint32_t prefetch_position,prefetch_output,prefetch_size,prefetch_copied;
static volatile uint32_t current_source,current_output,committed_source,committed_output;
static volatile bool display_start_pending;
static wp0_cd_stream_t cd_stream;
static wp0_attract_prepared_t first_loop_packet;
static vdp1_vram_partitions_t partitions;
static vdp1_cmdt_list_t command_list;
static vdp1_cmdt_list_t *commands=&command_list;
static cdfs_filelist_t files;
static cdfs_filelist_entry_t entries[64];

static uint32_t longword(const void *p){
    uint32_t value;__builtin_memcpy(&value,__builtin_assume_aligned(p,4),sizeof value);return value;
}
/* A loop seek proceeds asynchronously while the already verified prefix is
 * played from HWRAM. The CD still reads that prefix, so discard its duplicate
 * bytes only after they have become available. Never wait for the drive here. */
static bool cache_catch_up(void){
    if(!cache_mode||cd_stream.consumed>=logical_position)return true;
    uint32_t skip=logical_position-cd_stream.consumed;
    uint32_t available=wp0_cd_stream_available(&cd_stream);
    if(skip>available)skip=available;
    if(skip&&!wp0_cd_stream_consume(&cd_stream,skip)){wp0_debug.error=620;return false;}
    return true;
}
static bool stream_copy(uint32_t relative,void *dest,uint32_t bytes){
    if(cache_mode&&logical_position<=loop_cache_bytes&&
       relative<=loop_cache_bytes-logical_position&&
       bytes<=loop_cache_bytes-logical_position-relative){
        memcpy(dest,loop_cache+logical_position+relative,bytes);return true;
    }
    if(!cache_catch_up()||cd_stream.consumed!=logical_position)return false;
    cache_mode=false;
    return wp0_cd_stream_copy(&cd_stream,relative,dest,bytes);
}
static bool stream_consume(uint32_t bytes){
    if(cache_mode&&logical_position<=loop_cache_bytes&&bytes<=loop_cache_bytes-logical_position){
        logical_position+=bytes;return cache_catch_up();
    }
    if(!cache_catch_up()||cd_stream.consumed!=logical_position||
       !wp0_cd_stream_consume(&cd_stream,bytes))return false;
    cache_mode=false;logical_position+=bytes;return true;
}
static void prefetch_reset(void){prefetch_size=0;prefetch_copied=0;}
static bool prefetch_service(void){
    if(wp0_debug.boot_stage!=4||next_output>=total_fields)return false;
    if(prefetch_copied&&(prefetch_position!=logical_position||prefetch_output!=next_output))prefetch_reset();
    if(!prefetch_copied){
        if(!stream_copy(0,packet,64))return false;
        uint32_t size=longword(packet+4);
        if(size<64||size>PACKET_CAPACITY||(size&3U)){wp0_debug.error=602;return false;}
        if(longword(packet+8)!=next_output){wp0_debug.error=604;return false;}
        prefetch_position=logical_position;prefetch_output=next_output;
        prefetch_size=size;prefetch_copied=64;
        return true;
    }
    if(prefetch_copied==prefetch_size)return false;
    uint32_t bytes=prefetch_size-prefetch_copied;
    if(bytes>4096U)bytes=4096U;
    if(!stream_copy(prefetch_copied,packet+prefetch_copied,bytes))return false;
    prefetch_copied+=bytes;return true;
}

static void vblank_out(void *unused __unused){
    vdp2_tvmd_vcount_wait(0);
    wf_platform_vblank_out();
    wp0_debug.video_clock=wf_fields;
    if(display_start_pending){
        wp0_debug.display_source=committed_source;
        wp0_debug.display_output=committed_output;
        wp0_debug.active=1;
        display_start_pending=false;
    }

}
static void vblank_in(void *unused __unused){
    ++wp0_debug.commit_count;
    wp0_framebuffer_vblank_in();
    if(wp0_framebuffer_status.error&&!wp0_debug.error)
        wp0_debug.error=wp0_framebuffer_status.error;
    if(wp0_debug.boot_stage==4&&!wp0_debug.error){
        committed_source=current_source;committed_output=current_output;
        display_start_pending=true;
    }
}

static bool io_service(void){
    uint16_t start=wp0_timer_count_get();
    bool ok=wp0_cd_stream_service(&cd_stream,wp0_debug.video_clock);
    if(ok)ok=cache_catch_up();
    uint16_t ticks=(uint16_t)(wp0_timer_count_get()-start);
    if(ticks>wp0_debug.max_io_ticks)wp0_debug.max_io_ticks=ticks;
    ++wp0_debug.io_calls;
    if(!ok)wp0_debug.error=500U+cd_stream.error;
    return ok;
}
static unsigned next_field(unsigned previous,bool service){
    uint32_t mask=cpu_intc_mask_get();cpu_intc_mask_set(0);
    while(wp0_debug.video_clock==previous){
        /* VCOUNT is latched, not live: refresh before testing the window.
         * Reserve71 lines for a bounded8KiB FIFO read and VBlank DMA. */
        if(service&&!wp0_debug.error){
            vdp2_tvmd_extern_latch();
            unsigned line=vdp2_tvmd_vcount_get();
            if(wp0_debug.video_clock!=previous)break;
            /* After submit every packet consumer has completed. Copy at
             * most4KiB of the next packet during otherwise idle CPU time,
             * leaving the stream position untouched until normal dispatch.
             * One bounded operation per iteration; relatch before another. */
            if(line<224U&&prefetch_service())continue;
            if(line<192U)io_service();
        }
    }
    unsigned now=wp0_debug.video_clock;cpu_intc_mask_set(mask);return now;
}
static void transfer(void *dst,const void *src,unsigned bytes){
    if(!bytes)return;
    scu_dma_transfer(0,dst,src,bytes);scu_dma_transfer_wait(0);
}
static unsigned prime_framebuffers(void){
    volatile vdp1_ioregs_t *const regs=(volatile vdp1_ioregs_t *)VDP1_IOREG_BASE;
    volatile uint16_t *const pixels=(volatile uint16_t *)0x25C80000UL;
    wp0_framebuffer_stop();vdp2_tvmd_display_clear();vdp2_sync();vdp2_sync_wait();
    unsigned clock;
    for(unsigned bank=0;bank<2;++bank){
        clock=next_field(wp0_debug.video_clock,false);regs->fbcr=3;
        clock=next_field(clock,false);vdp2_tvmd_vcount_wait(2);
        for(unsigned i=0;i<0x20000U;++i)pixels[i]=0;
    }
    clock=next_field(wp0_debug.video_clock,false);regs->fbcr=2;
    clock=next_field(clock,false);regs->fbcr=0;
    return next_field(clock,false);
}
static void reset_dirty(void){
    for(unsigned i=0;i<3;++i){map_first[i]=0xffff;map_last[i]=0;}
    pal_first=0xffff;pal_last=0;lines_dirty[0]=lines_dirty[1]=false;
    wp0_debug.texture_bytes=0;
}
static bool upload_texture(void *u __unused,uint16_t kind,uint16_t first,uint16_t count,const uint8_t *raw){
    unsigned stride=kind==2?32U:128U;
    uintptr_t base=kind==3?(uintptr_t)partitions.texture_base:texture_addresses[kind];
    transfer((void *)(base+first*stride),raw,count*stride);
    wp0_debug.texture_bytes+=count*stride;
    return true;
}
static bool write_map(void *u __unused,uint16_t kind,uint16_t first,uint16_t count,const uint8_t *raw){
    memcpy(maps[kind]+first,raw,count*4U);
    if(first<map_first[kind])map_first[kind]=first;
    if(first+count>map_last[kind])map_last[kind]=first+count;
    return true;
}
static bool write_palette(void *u __unused,uint16_t index,uint16_t value){
    unsigned r=(value&15U)*17U,g=((value>>4)&15U)*17U,b=((value>>8)&15U)*17U;
    palette[index]=(b<<16)|(g<<8)|r;
    if(index<pal_first)pal_first=index;
    if(index+1>pal_last)pal_last=index+1;
    return true;
}
static bool write_sprites(void *u __unused,uint16_t count,const uint8_t *raw){
    if(count==0xffff)return true;
    sprite_count=count;if(count)memcpy(sprites,raw,count*8U);return true;
}
static bool write_lines(void *u __unused,uint16_t kind,uint16_t first,uint16_t count,int16_t x,int16_t y){
    for(unsigned i=0;i<count;++i){
        line_scroll[kind][first+i][0]=(uint32_t)x<<16;
        line_scroll[kind][first+i][1]=(uint32_t)(y+i)<<16;
    }
    lines_dirty[kind]=true;return true;
}
static bool reject_pcm(void *u __unused,uint32_t offset,uint16_t count,const uint8_t *raw __unused){
    return !offset&&!count;
}
static const wp0_attract_packet_ops_t packet_ops={NULL,upload_texture,write_map,write_palette,write_sprites,write_lines,reject_pcm};

static bool begin_section(bool loop){
    prefetch_reset();
    if(!stream_copy(0,packet,2048)){wp0_debug.error=610;return false;}
    uint32_t flags=longword(packet+56),expected=loop?loop_file_bytes:file_bytes;
    if(memcmp(packet,"WFATTR05",8)||longword(packet+8)!=1||
       longword(packet+24)!=0||longword(packet+28)>PACKET_CAPACITY||
       longword(packet+32)!=expected||longword(packet+36)!=2048||
       longword(packet+40)!=320||longword(packet+44)!=240||
       longword(packet+48)!=0||longword(packet+52)!=0||
       flags!=(loop?0U:2U)||
       longword(packet+60)!=(loop?0U:loop_offset)){
        wp0_debug.error=611;return false;
    }
    total_fields=longword(packet+12);
    if(!total_fields){wp0_debug.error=612;return false;}
    if(!stream_consume(2048)){wp0_debug.error=613;return false;}
    playing_loop=loop;next_output=0;
    return true;
}

static bool advance_section(void){
    if(!playing_loop){
        if(logical_position>loop_offset||!stream_consume(loop_offset-logical_position)){
            wp0_debug.error=621;return false;
        }
    }else{
        if(!cache_catch_up()||!wp0_cd_stream_restart_async(&cd_stream,loop_fad,loop_file_bytes)){
            wp0_debug.error=500+cd_stream.error;return false;
        }
        logical_position=0;cache_mode=true;++wp0_debug.loops;
    }
    return begin_section(true);
}

static void priorities(unsigned priority){
    unsigned display=VDP2_SCRN_DISP_NBG0|VDP2_SCRN_DISPTP_NBG1|VDP2_SCRN_DISPTP_NBG2;
    if(priority==0x78){
        vdp2_scrn_priority_set(VDP2_SCRN_NBG0,1);vdp2_scrn_priority_set(VDP2_SCRN_NBG1,2);
    }else{
        display=VDP2_SCRN_DISPTP_NBG0|VDP2_SCRN_DISP_NBG1|VDP2_SCRN_DISPTP_NBG2;
        vdp2_scrn_priority_set(VDP2_SCRN_NBG0,priority==0x7c?4:2);vdp2_scrn_priority_set(VDP2_SCRN_NBG1,1);
    }
    if(priority!=0x78&&priority!=0x7b&&priority!=0x7c)display=0;
    vdp2_scrn_display_set((vdp2_scrn_disp_t)display);
}
static bool apply_next(void){
    uint16_t started=wp0_timer_count_get();
    uint32_t header[16] __aligned(4);
    if(next_output==total_fields&&!advance_section())return false;
    if(!stream_copy(0,header,64)){wp0_debug.error=601;return false;}
    unsigned size=header[1];
    if(size<64||size>PACKET_CAPACITY||(size&3)){wp0_debug.error=602;return false;}
    /* The immutable first loop packet was prefetched before display. Its
     * large initial map must not be copied from slow LWRAM every return.
     * Its immutable prepared view was fully validated with CRC at startup. */
    const uint8_t *input=packet;
    if(playing_loop&&next_output==0){
        if(size>loop_cache_bytes-2048U){wp0_debug.error=619;return false;}
        input=loop_cache+2048;
    }else{
        uint32_t copied=0;
        if(prefetch_position==logical_position&&prefetch_output==next_output&&
           prefetch_size==size&&prefetch_copied>=64U&&prefetch_copied<=size){
            if(memcmp(header,packet,64)){wp0_debug.error=623;return false;}
            copied=prefetch_copied;
        }
        if(copied<size&&!stream_copy(copied,packet+copied,size-copied)){wp0_debug.error=603;return false;}
    }
    wp0_attract_meta_t meta;
    if(header[2]!=next_output){wp0_debug.error=604;return false;}
    bool dispatched;
    if(playing_loop&&next_output==0){
        if(size!=first_loop_packet.bytes||memcmp(header,first_loop_packet.data,64)){
            wp0_debug.error=619;return false;
        }
        dispatched=wp0_attract_packet_dispatch_prepared(&first_loop_packet,&packet_ops,&meta);
        deferred_full_maps=7U;
    }else dispatched=wp0_attract_packet_dispatch(input,size,false,&packet_ops,&meta);
    if(!dispatched){
        if(!wp0_debug.error)wp0_debug.error=605;return false;
    }
    if(!stream_consume(size)){wp0_debug.error=606;return false;}
    prefetch_reset();
    priorities(meta.priority);current_source=meta.source_frame;current_output=meta.output_frame;
    ++next_output;
    wp0_debug.source_frame=meta.source_frame;wp0_debug.output_frames=next_output;
    wp0_debug.arcade_steps=meta.source_frame;wp0_debug.sequence_offset=logical_position;
    wp0_debug.packet_bytes=size;if(size>wp0_debug.max_packet_bytes)wp0_debug.max_packet_bytes=size;
    if(wp0_debug.texture_bytes>wp0_debug.max_texture_bytes)wp0_debug.max_texture_bytes=wp0_debug.texture_bytes;
    wp0_debug.packet_ticks=(uint16_t)(wp0_timer_count_get()-started);
    return true;
}
static bool enqueue_changes(void){
    unsigned full_plane=3U;
    if(deferred_full_maps){
        /* Original loop fields409..411 have a completely black palette.
         * Commit one full map per blank while those original black fields
         * are displayed. Keep the latest shadow contents for pending maps;
         * never extend the black interval or defer a coloured field. */
        for(unsigned i=0;i<1024;++i)if(palette[i]){wp0_debug.error=622;return false;}
        for(unsigned i=0;i<3;++i)if(deferred_full_maps&(1U<<i)){full_plane=i;break;}
    }
    for(unsigned i=0;i<3;++i){
        if((deferred_full_maps&(1U<<i))&&i!=full_plane)continue;
        if(i==full_plane){
            vdp_dma_enqueue((void *)map_addresses[i],maps[i],sizeof(maps[i]));
        }else if(map_first[i]!=0xffff){
            unsigned first=map_first[i],count=map_last[i]-first;
            vdp_dma_enqueue((void *)(map_addresses[i]+first*4),maps[i]+first,count*4);
        }
    }
    if(full_plane<3U)deferred_full_maps&=~(1U<<full_plane);
    if(pal_first!=0xffff)vdp_dma_enqueue((void *)(0x25F00000UL+pal_first*4),palette+pal_first,(pal_last-pal_first)*4);
    for(unsigned i=0;i<2;++i)if(lines_dirty[i])vdp_dma_enqueue((void *)line_addresses[i],line_scroll[i],sizeof(line_scroll[i]));
    return true;
}
static void draw_commands(void){
    vdp1_cmdt_t *list=commands->cmdts;memset(list,0,(sprite_count+3)*sizeof(*list));
    vdp1_cmdt_system_clip_coord_set(&list[0]);vdp1_cmdt_vtx_system_clip_coord_set(&list[0],INT16_VEC2(319,239));
    vdp1_cmdt_local_coord_set(&list[1]);vdp1_cmdt_vtx_local_coord_set(&list[1],INT16_VEC2(0,0));
    const vdp1_cmdt_draw_mode_t mode={.color_mode=0,.pre_clipping_disable=true,.user_clipping_mode=2,.end_code_disable=true};
    for(unsigned i=0;i<sprite_count;++i){
        const uint16_t *r=sprites+i*4;vdp1_cmdt_t *cmd=&list[i+2];
        if(r[0]==0xffff){
            vdp1_cmdt_user_clip_coord_set(cmd);
            vdp1_cmdt_vtx_user_clip_coord_set(cmd,INT16_VEC2(0,r[2]),INT16_VEC2(319,r[3]));continue;
        }
        const vdp1_cmdt_color_bank_t bank={.type_0.dc=768+(r[3]&15)*16};
        vdp1_cmdt_normal_sprite_set(cmd);vdp1_cmdt_draw_mode_set(cmd,mode);vdp1_cmdt_color_mode0_set(cmd,bank);
        vdp1_cmdt_char_base_set(cmd,(vdp1_vram_t)partitions.texture_base+r[0]*128U);vdp1_cmdt_char_size_set(cmd,16,16);
        vdp1_cmdt_char_flip_set(cmd,(vdp1_cmdt_char_flip_t)(r[3]&0x30));cmd->cmd_xa=(int16_t)r[1];cmd->cmd_ya=(int16_t)r[2];
    }
    vdp1_cmdt_end_set(&list[sprite_count+2]);commands->count=sprite_count+3;wp0_debug.sprite_count=sprite_count;
}
static void observe(void){
    wp0_debug.last_draw_ticks=wp0_framebuffer_status.last_draw_ticks;wp0_debug.max_draw_ticks=wp0_framebuffer_status.max_draw_ticks;
    wp0_debug.draw_irq_count=wp0_framebuffer_status.draw_irq_count;wp0_debug.cef_fallback_count=wp0_framebuffer_status.cef_fallback_count;
    wp0_debug.framebuffer_error=wp0_framebuffer_status.error;
    wp0_debug.framebuffer_flags=(wp0_framebuffer_status.deadline_edsr&3U)|(wp0_framebuffer_status.submitted?4U:0U)|(wp0_framebuffer_status.draw_done?8U:0U);
    wp0_debug.stream_available=wp0_cd_stream_available(&cd_stream);
    if(wp0_debug.stream_available<wp0_debug.stream_min_available)wp0_debug.stream_min_available=wp0_debug.stream_available;
    wp0_debug.stream_produced=cd_stream.produced;wp0_debug.stream_consumed=cd_stream.consumed;
    wp0_debug.stream_state=cd_stream.state;wp0_debug.stream_error=cd_stream.error;wp0_debug.stream_commands=cd_stream.commands_issued;
}

void wf_fixed_attract_video_begin(bool notice){
    memset((void *)&wf_attract_debug,0,sizeof wf_attract_debug);wf_attract_debug.magic=0x57464135U;
    wp0_debug.video_clock=wf_fields;cache_mode=false;playing_loop=false;logical_position=0;
    deferred_full_maps=0;sprite_count=0;next_output=0;display_start_pending=false;
    current_source=current_output=committed_source=committed_output=0;prefetch_reset();
    vdp_sync_vblank_out_set(vblank_out,NULL);vdp_sync_vblank_in_set(vblank_in,NULL);
    vdp2_tvmd_display_clear();vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE,VDP2_TVMD_HORZ_NORMAL_A,VDP2_TVMD_VERT_240);vdp2_cram_mode_set(2);
    const vdp1_env_t env={.bpp=VDP1_ENV_BPP_16,.rotation=VDP1_ENV_ROTATION_0,.color_mode=VDP1_ENV_COLOR_MODE_PALETTE,.sprite_type=0,.erase_color=RGB1555_INITIALIZER(0,0,0,0),.erase_points={INT16_VEC2_INITIALIZER(0,0),INT16_VEC2_INITIALIZER(319,239)}};
    vdp1_env_set(&env);vdp1_vram_partitions_set(CMD_CAPACITY,SPRITE_BYTES,0,0);vdp1_vram_partitions_get(&partitions);
    vdp2_sprite_priority_set(0,3);vdp2_scrn_back_color_set(VDP2_VRAM_ADDR(3,0x1fffe),RGB1555(1,0,0,0));
    const vdp2_scrn_t screens[3]={VDP2_SCRN_NBG0,VDP2_SCRN_NBG1,VDP2_SCRN_NBG2};
    for(unsigned i=0;i<3;++i){
        const vdp2_scrn_cell_format_t fmt={.scroll_screen=screens[i],.ccc=VDP2_SCRN_CCC_PALETTE_16,.char_size=VDP2_SCRN_CHAR_SIZE_1X1,.pnd_size=2,.aux_mode=VDP2_SCRN_AUX_MODE_0,.plane_size=VDP2_SCRN_PLANE_SIZE_1X1,.cpd_base=VDP2_VRAM_ADDR(i,0),.palette_base=VDP2_CRAM_MODE_2_OFFSET(i==2?0:i+1,0,0)};
        const vdp2_scrn_normal_map_t map={.base_addr={map_addresses[i],map_addresses[i],map_addresses[i],map_addresses[i]}};
        vdp2_scrn_cell_format_set(&fmt,&map);vdp2_scrn_scroll_x_set(screens[i],0);vdp2_scrn_scroll_y_set(screens[i],0);vdp2_scrn_priority_set(screens[i],i==2?5:i+1);
        if(i<2){const vdp2_scrn_ls_format_t ls={.scroll_screen=screens[i],.table_base=line_addresses[i],.interval=1,.type=VDP2_SCRN_LS_TYPE_HORZ|VDP2_SCRN_LS_TYPE_VERT};vdp2_scrn_ls_set(&ls);}
    }
    vdp2_vram_cycp_t cycles;cycles.pt[0].raw=0x4FEEEEEE;cycles.pt[1].raw=0x5FEEEEEE;cycles.pt[2].raw=0x6FFFEEEE;cycles.pt[3].raw=0x012FEEEE;vdp2_vram_cycp_set(&cycles);
    vdp2_scrn_display_set(0);wp0_framebuffer_initialize();
    wp0_debug.video_standard=vdp2_tvmd_tv_standard_get();vdp2_sync();vdp2_sync_wait();
    if(notice&&wf_cart_valid)wf_startup_notice_begin(wf_cart_id,wf_cart_bytes);
    wp0_debug.notice=wf_startup_notice_active;
}

bool wf_fixed_attract_run(bool skip,int (*on_field)(void *),void *user){
    bool stream_started=false;
    uint32_t exit_field=wf_fields;
    if(skip){wp0_debug.exit_reason=2;goto release;}
    wp0_debug.boot_stage=1;
    if(wp0_debug.video_standard!=VDP2_TVMD_TV_STANDARD_NTSC){wp0_debug.error=104;goto failure;}
    vdp1_cmdt_list_init(commands,(vdp1_cmdt_t *)wf_workspace.attract.commands);
    if(wp2015_cd_rate_before_frt_reset()){wp0_debug.error=630;goto failure;}
    wf_front_resource_pack_invalidate();
    cdfs_config_default_set();cdfs_filelist_init(&files,entries,64);cdfs_filelist_root_read(&files);
    fad_t fad=0;
    for(unsigned i=0;i<files.entries_count;++i)if(!strcmp(files.entries[i].name,"ATTRACT.DAT")){fad=files.entries[i].starting_fad;file_bytes=files.entries[i].size;break;}
    if(!fad){wp0_debug.error=103;goto failure;}
    const wp0_cd_stream_ops_t ops=wp0_cd_stream_saturn_ops();
    if(!wp0_cd_stream_init(&cd_stream,(void *)0x00280000UL,RING_CAPACITY,file_bytes,&ops)||!wp0_cd_stream_start_hidden(&cd_stream,fad)){wp0_debug.error=500+cd_stream.error;goto failure;}
    stream_started=true;
    wf_fixed_attract_field_t input_field={wf_fields,0};
    wp0_debug.boot_stage=2;
    while(wp0_cd_stream_available(&cd_stream)<(file_bytes<PREFILL_BYTES?file_bytes:PREFILL_BYTES)){
        int input=wf_fixed_attract_prefill_step(&input_field,wf_fields,on_field,user,io_service);
        if(input<0){if(!wp0_debug.error)wp0_debug.error=640;goto failure;}
        if(input>0){exit_field=input_field.last_field;wp0_debug.exit_reason=1;goto release;}
    }
    if(!wp0_cd_stream_copy(&cd_stream,0,packet,2048)){wp0_debug.error=610;goto failure;}
    loop_offset=longword(packet+60);
    if(!(longword(packet+56)&2U)||!loop_offset||(loop_offset&2047U)||loop_offset>=file_bytes){wp0_debug.error=615;goto failure;}
    loop_file_bytes=file_bytes-loop_offset;loop_fad=fad+loop_offset/2048U;
    loop_cache_bytes=loop_file_bytes<LOOP_CACHE_BYTES?loop_file_bytes:LOOP_CACHE_BYTES;
    if(loop_offset>RING_CAPACITY-loop_cache_bytes){wp0_debug.error=616;goto failure;}
    while(wp0_cd_stream_available(&cd_stream)<loop_offset+loop_cache_bytes){
        int input=wf_fixed_attract_prefill_step(&input_field,wf_fields,on_field,user,io_service);
        if(input<0){if(!wp0_debug.error)wp0_debug.error=640;goto failure;}
        if(input>0){exit_field=input_field.last_field;wp0_debug.exit_reason=1;goto release;}
    }
    if(!wp0_cd_stream_copy(&cd_stream,loop_offset,loop_cache,loop_cache_bytes)){wp0_debug.error=617;goto failure;}
    /* This buffer is written only by the preceding hidden-startup copy.
     * Full bounds + CRC validation publishes an immutable prepared view.
     * Every subsequent ordinary CD packet keeps per-field full validation. */
    if(loop_cache_bytes<2112U||longword(loop_cache+56)!=0U){wp0_debug.error=619;goto failure;}
    uint32_t first_size=longword(loop_cache+2052);
    if(first_size>loop_cache_bytes-2048U||
       !wp0_attract_packet_prepare(&first_loop_packet,loop_cache+2048,first_size,true)){
        wp0_debug.error=619;goto failure;
    }
    if(!begin_section(false))goto failure;
    wf_startup_notice_end();wp0_debug.notice=0;wp0_debug.notice_fields=wf_startup_notice_fields;
    wp0_debug.boot_stage=3;reset_dirty();if(!apply_next())goto failure;
    if(!enqueue_changes())goto failure;
    draw_commands();vdp2_sync();vdp2_sync_wait();reset_dirty();
    unsigned submitted_clock=prime_framebuffers();
    if(!wp0_framebuffer_begin())goto failure;
    wp0_debug.boot_stage=4;wp0_debug.stream_min_available=wp0_cd_stream_available(&cd_stream);
    vdp2_tvmd_display_set();vdp2_sync();
    if(!wp0_framebuffer_submit(commands->cmdts,commands->count*sizeof(vdp1_cmdt_t)))goto failure;
    unsigned last_clock=submitted_clock;
    for(;;){
        unsigned now=next_field(submitted_clock,input_field.result<=0);vdp2_sync_wait();
        if(wp0_debug.error||wp0_framebuffer_status.error)goto failure;
        unsigned elapsed=now-last_clock;last_clock=now;
        if(elapsed!=1){wp0_debug.missed_vblanks+=elapsed?elapsed-1:1;wp0_debug.error=401;goto failure;}
        /* Acceptance came after the preceding submit. Complete that same
         * frame before releasing VDP/IRQ; no new prefetch ran while waiting. */
        if(input_field.result>0)goto release;
        if(!wp0_framebuffer_begin())goto failure;
        uint16_t started=wp0_timer_count_get();reset_dirty();
        if(!wp0_debug.done){
            if(!apply_next())goto failure;
        }
        observe();if(!enqueue_changes())goto failure;
        draw_commands();vdp2_sync();
        if(!wp0_framebuffer_submit(commands->cmdts,commands->count*sizeof(vdp1_cmdt_t)))goto failure;
        submitted_clock=now;wp0_debug.last_work_ticks=(uint16_t)(wp0_timer_count_get()-started);
        if(wp0_debug.last_work_ticks>wp0_debug.max_work_ticks)wp0_debug.max_work_ticks=wp0_debug.last_work_ticks;
        /* Coin IRQs touch original RAM/board only. Overlap this CPU work with
         * the already-launched VDP1 draw, leaving its strict VBlank-IN deadline
         * unchanged. Work timing above retains its prepare/submit meaning. */
        vdp2_tvmd_extern_latch();wp0_debug.submit_vcount=vdp2_tvmd_vcount_get();
        uint16_t input_started=wp0_timer_count_get();
        int input=wf_fixed_attract_field_service(&input_field,now,on_field,user);
        wp0_debug.last_input_ticks=(uint16_t)(wp0_timer_count_get()-input_started);
        if(wp0_debug.last_input_ticks>wp0_debug.max_input_ticks)wp0_debug.max_input_ticks=wp0_debug.last_input_ticks;
        if(input<0){wp0_debug.error=640;goto failure;}
        if(input>0){exit_field=wf_fields;wp0_debug.exit_reason=1;}

    }
release:
    /* During playback, VBlank-IN/sync_wait completed draw/DMA. During prefill,
     * no picture has been submitted yet. Stop has the same CD boundary in both
     * cases; no producer or replay runs from here on. */
    wp0_debug.active=0;display_start_pending=false;wp0_debug.boot_stage=5;
    wf_startup_notice_end();wp0_debug.notice=0;wp0_debug.notice_fields=wf_startup_notice_fields;
    /* COMMITTED has already finished drawing. Release before another sync,
     * which would otherwise diagnose a missing next submission as late. */
    scu_dma_transfer_wait(0);wp0_framebuffer_release();
    vdp2_tvmd_display_clear();vdp2_sync();vdp2_sync_wait();
    if(stream_started){
        uint32_t start=exit_field;wp0_debug.stop_start=start;
        if(!wp0_cd_stream_stop_request(&cd_stream,start))goto failure;
        int stopped;
        while((stopped=wp0_cd_stream_stop_service(&cd_stream,wf_fields))==0){
            wp0_debug.stop_fields=wf_fields-start;
            wf_platform_hold_fields(1);
        }
        wp0_debug.stop_fields=wf_fields-start;observe();
        if(stopped<0){wp0_debug.error=500+cd_stream.error;goto failure;}
        /* Drain direct-stream command history before the guarded CDFS owner.
         * No timer reset occurs; binding establishes a fresh guarded epoch. */
        if(wp2015_cd_rate_bind(&wf_fields)){wp0_debug.error=631;goto failure;}
        if(wf_r18_cdfs_init()){wp0_debug.error=632;goto failure;}
    }
    wf_front_resource_pack_invalidate();
    wf_platform_video_callbacks_restore();wp0_debug.boot_stage=6;return true;
failure:
    display_start_pending=false;wp0_debug.active=0;
    wp0_framebuffer_stop();
    if(!wp0_debug.error)wp0_debug.error=wp0_framebuffer_status.error;
    observe();return false; /* fail closed: never return CD/workspace to CDFS */
}
