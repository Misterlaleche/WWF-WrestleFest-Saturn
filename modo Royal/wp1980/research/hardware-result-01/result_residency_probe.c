/* Regression probe only. Link the selected production cache/result sources.
 * File denial models unavailable post-boot CD I/O; no cache/result owner is
 * replaced. Physical VBlank and display-memory destinations are host fixtures. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wf_r18_present.h"
#include "wf_r18_result_screen.h"
#include "wf_r18_runtime_handoff.h"
#include "wf_r18_hud.h"
#include "wp1980_royal_init.h"
#if WP1980_CART && WF_R18_ROM_EXTRA_CAPACITY > 32u
#include "wp1980_royal_rom_pages_cart.h"
#else
#include "wp1980_royal_rom_pages.h"
#endif

static uint8_t rom[0x80000],init[WP1980_ROYINIT_BYTES];
static uint8_t cache[WF_SATURN_LWRAM_ROM_BYTES],tiles[128];
static wf_runtime_t runtime,initial_runtime;
static wf_r18_video_t video,initial_video;
static uint16_t hud_map[4096];
static unsigned denied,track_demand,file_calls,failed_reads,commits,wait_calls,wait_periods;
static unsigned level_calls,level_values[4],demand[512],churn_pages,churn_lines;
static uint32_t first_failed=UINT32_MAX;
static char events[16];
static unsigned event_count;
static const unsigned result_pages[]={10,11,14,17,18,148,151};

static int load(const char *path,void *out,size_t bytes)
{
    FILE *f=fopen(path,"rb");
    if(!f)return 0;
    int ok=fread(out,1,bytes,f)==bytes&&fgetc(f)==EOF;
    fclose(f);return ok;
}
static uint16_t be16(const uint8_t *p){return (uint16_t)((p[0]<<8)|p[1]);}
static uint32_t be32(const uint8_t *p){return ((uint32_t)be16(p)<<16)|be16(p+2);}
static int file_read(const char *name,uint32_t at,void *out,size_t bytes)
{
    ++file_calls;
    if(strcmp(name,"ROM68K.BIN")||at>sizeof rom||bytes>sizeof rom-at)return -1;
    if(denied){
        if(!failed_reads)first_failed=at;
        ++failed_reads;return -1;
    }
    memcpy(out,rom+at,bytes);return 0;
}
void wp1980_host_rom_demand(uint32_t at,size_t bytes)
{
    if(!track_demand||!bytes)return;
    for(unsigned page=at/1024u;page<=(at+(uint32_t)bytes-1u)/1024u;++page)
        if(page<512u)demand[page]=1u;
}
static void event(char c){if(event_count+1u<sizeof events)events[event_count++]=c;}
static wf_status_t level(wf_runtime_t *r,unsigned high,void *user)
{
    if(r!=&runtime||user!=&video||level_calls>=4u)return WF_BAD_STATE;
    level_values[level_calls++]=high;event(high?'H':'L');return WF_OK;
}
static wf_status_t commit(void *user)
{
    if(user!=&video)return WF_BAD_STATE;
    ++commits;event('C');
    wf_status_t status=wf_r18_hud_commit(&video);
    if(status!=WF_OK)return status;
    for(unsigned role=WF_R18_PALETTE_TEXT;role<=WF_R18_PALETTE_BG;++role){
        status=wf_r18_palettes_commit(&video,role);
        if(status!=WF_OK)return status;
    }
    /* The production host branch has no memory-mapped VDP2. Exercise its
     * actual HUD map writer with host-owned storage at the same boundary. */
    return wf_r18_hud_map_update(&video,hud_map,NULL);
}
static wf_status_t wait_frames(wf_runtime_t *r,uint16_t periods,void *user)
{
    if(r!=&runtime||user!=&video)return WF_BAD_STATE;
    ++wait_calls;wait_periods+=periods;event('W');return WF_OK;
}
static int fixture(const char *actors_path)
{
    wf_r17_m68k_shadow_t regs={.valid=1};
    for(unsigned i=0;i<8u;++i)regs.d[i]=be32(init+WP1980_ROYINIT_REGS+i*4u);
    if(wf_r18_runtime_import(&runtime,init+WP1980_ROYINIT_RAM,0x4000u,&regs)!=WF_OK)return 0;
    if(!load(actors_path,runtime.actors,sizeof runtime.actors))return 0;
    runtime.g.frame_counter=8474u;runtime.g.rng_state=0xb8a86c74u;
    runtime.g.global_16c4=3u;
    runtime.frame_ready=runtime.vblank_ack=1u;runtime.frame_stage=2u;
    memcpy(video.fg0,init+WP1980_ROYINIT_TEXT,8192u);
    memcpy(video.fg0+0x2000u,init+WP1980_ROYINIT_SPRITES,8192u);
    memcpy(video.background,init+WP1980_ROYINIT_FGMAP,4096u);
    memcpy(video.background+0x2000u,init+WP1980_ROYINIT_BGMAP,4096u);
    memcpy(video.palettes,init+WP1980_ROYINIT_PALETTE,sizeof video.palettes);
    memcpy(video.bg_palette,init+WP1980_ROYINIT_PALETTE+0x6000u,sizeof video.bg_palette);
    memcpy(video.fg_palette,init+WP1980_ROYINIT_PALETTE+0x8000u,sizeof video.fg_palette);
    memcpy(video.palette_used,init+WP1980_ROYINIT_RAM+0x1600u,sizeof video.palette_used);
    memcpy(video.palette_map,init+WP1980_ROYINIT_RAM+0x1610u,sizeof video.palette_map);
    video.read_rom=wf_r18_rom_read;
    wf_r18_hud_reset(&video);
    initial_runtime=runtime;initial_video=video;
    return runtime.g.global_15f4==0u;
}
static int fast_page(unsigned page)
{
    for(size_t i=0;i<sizeof wp1980_rom_fast_pages/sizeof *wp1980_rom_fast_pages;++i)
        if(wp1980_rom_fast_pages[i]==page)return 1;
    return 0;
}
static int preload(void)
{
    wf_r18_present_init(file_read,cache,sizeof cache,tiles,sizeof tiles);
    return wf_r18_rom_pages_preload(wp1980_rom_main_pages,sizeof wp1980_rom_main_pages/sizeof *wp1980_rom_main_pages)==WF_OK&&
        wf_r18_rom_extra_pages_preload(wp1980_rom_fast_pages,sizeof wp1980_rom_fast_pages/sizeof *wp1980_rom_fast_pages)==WF_OK;
}
static int churn(void)
{
    uint8_t bytes[1024];
    /* Read every non-pinned page through the production loader. More than
     * all209 LWRAM slots are replaced; no result dependency is refreshed. */
    for(unsigned page=0;page<512u;++page)if(!fast_page(page)){
        if(wf_r18_rom_read(NULL,page*1024u,bytes,sizeof bytes)!=WF_OK||
           memcmp(bytes,rom+page*1024u,sizeof bytes))return 0;
        ++churn_pages;
    }
    /* Real small reads fill every direct-mapped tiny-line cache index with
     * unrelated pages0/1. Full-page reads alone would not invalidate it. */
    for(unsigned line=0;line<64u;++line){
        uint32_t at=line*32u;
        if(wf_r18_rom_read(NULL,at,bytes,4u)!=WF_OK||memcmp(bytes,rom+at,4u))return 0;
        ++churn_lines;
    }
    return churn_pages>sizeof cache/1024u&&churn_lines==64u;
}
static int original_result_values(void)
{
    /* Expectations are literal original-world0 table locations, plus the
     * register effects independently disassembled at1AFC/2A06/21E6. */
    wf_runtime_t expected=initial_runtime;
    expected.g.global_15fc=1u;
    expected.m68k_regs.d[0]=(expected.m68k_regs.d[0]&0xffff0000u)|0x60u;
    expected.m68k_regs.d[1]=(expected.m68k_regs.d[1]&0xffff0000u)|0x3cu;
    expected.m68k_regs.d[2]=(expected.m68k_regs.d[2]&0xffff0000u)|0xffffu;
    if(memcmp(&runtime,&expected,sizeof runtime))return 0;
    for(unsigned bank=0;bank<16u;++bank){
        if(memcmp(video.fg_palette+bank*128u,rom+0x3922u+bank*32u,32u)||
           memcmp(video.bg_palette+bank*128u,rom+0x4722u+bank*32u,32u)||
           memcmp(video.palettes+bank*128u,rom+be32(rom+0x2bc2u+bank*4u),32u))return 0;
    }
    /* Independent fixed text fixture: original selector50 is type2, bank0,
     * base0E30, "GAME OVER". Four tiles per letter; the space clears four. */
    static const uint16_t glyphs[9]={0x170,0x158,0x188,0x168,0,0x190,0x1ac,0x168,0x19c};
    uint8_t expected_fg[sizeof video.fg0];memcpy(expected_fg,initial_video.fg0,sizeof expected_fg);
    for(unsigned i=0;i<9u;++i){
        static const unsigned offsets[4]={0,256,4,260};
        for(unsigned j=0;j<4u;++j){
            unsigned at=0xe30u+i*8u+offsets[j];
            if(!glyphs[i])memset(expected_fg+at,0,4u);
            else {uint16_t value=(uint16_t)(glyphs[i]+j);expected_fg[at+1u]=(uint8_t)value;expected_fg[at+3u]=(uint8_t)(value>>8);}
        }
    }
    return !memcmp(video.fg0,expected_fg,sizeof expected_fg)&&
        !memcmp(video.background,initial_video.background,sizeof video.background)&&
        !memcmp(video.palettes+0x2000u,initial_video.palettes+0x2000u,0x2000u);
}
static int snapshot(const char *path,const char *reference)
{
    wf_r18_video_t portable=video;
    portable.read_rom=NULL;portable.read_ram=NULL;portable.read_user=NULL;portable.ring_cell=NULL;
    FILE *f=fopen(path,"wb");if(!f)return 0;
    int ok=fwrite(&runtime,1,sizeof runtime,f)==sizeof runtime&&
        fwrite(&portable,1,sizeof portable,f)==sizeof portable&&
        fwrite(hud_map,1,sizeof hud_map,f)==sizeof hud_map;
    fclose(f);if(!ok||!reference)return ok;
    size_t bytes=sizeof runtime+sizeof portable+sizeof hud_map;
    uint8_t *expected=malloc(bytes),*actual=malloc(bytes);
    if(!expected||!actual){free(expected);free(actual);return 0;}
    ok=load(reference,expected,bytes)&&load(path,actual,bytes)&&!memcmp(expected,actual,bytes);
    free(expected);free(actual);return ok;
}
static int board_projection(const char *snapshot_path)
{
    char path[4096];
    if(snprintf(path,sizeof path,"%s.board.bin",snapshot_path)>=(int)sizeof path)return 0;
    FILE *f=fopen(path,"wb");if(!f)return 0;
    int ok=fwrite(video.fg0,1,sizeof video.fg0,f)==sizeof video.fg0&&
        fwrite(video.background,1,sizeof video.background,f)==sizeof video.background&&
        fwrite(video.palettes,1,sizeof video.palettes,f)==sizeof video.palettes&&
        fwrite(video.bg_palette,1,sizeof video.bg_palette,f)==sizeof video.bg_palette&&
        fwrite(video.fg_palette,1,sizeof video.fg_palette,f)==sizeof video.fg_palette;
    for(unsigned i=0;i<8u;++i){
        uint32_t v=runtime.m68k_regs.d[i];
        uint8_t b[4]={(uint8_t)(v>>24),(uint8_t)(v>>16),(uint8_t)(v>>8),(uint8_t)v};
        if(fwrite(b,1,sizeof b,f)!=sizeof b)ok=0;
    }
    if(fwrite(runtime.actors,1,sizeof runtime.actors,f)!=sizeof runtime.actors)ok=0;
    uint32_t scalar[2]={runtime.g.rng_state,runtime.g.frame_counter};
    for(unsigned i=0;i<2u;++i){
        uint32_t v=scalar[i];
        uint8_t b[4]={(uint8_t)(v>>24),(uint8_t)(v>>16),(uint8_t)(v>>8),(uint8_t)v};
        if(fwrite(b,1,sizeof b,f)!=sizeof b)ok=0;
    }
    uint8_t selector[2]={(uint8_t)(runtime.g.global_15fc>>8),(uint8_t)runtime.g.global_15fc};
    if(fwrite(selector,1,sizeof selector,f)!=sizeof selector)ok=0;
    fclose(f);return ok;
}
int main(int argc,char **argv)
{
    if(argc<6||argc>7){fprintf(stderr,"usage: probe ROM INIT ACTORS baseline|deny|churn SNAPSHOT [REFERENCE]\n");return 2;}
    if(!load(argv[1],rom,sizeof rom)||!load(argv[2],init,sizeof init)||!fixture(argv[3])||!preload()){
        fputs("SETUP_ERROR: real ROM/fixture/preload failed\n",stderr);return 2;
    }
    unsigned preload_reads=file_calls;
    int is_churn=!strcmp(argv[4],"churn"),baseline=!strcmp(argv[4],"baseline");
    if(!baseline&&strcmp(argv[4],"deny")&& !is_churn)return 2;
    if(is_churn&&!churn()){fputs("SETUP_ERROR: churn failed\n",stderr);return 2;}
    unsigned churn_reads=file_calls-preload_reads;
    file_calls=0;denied=!baseline;track_demand=1;
    wf_status_t status=wf_r18_result_royal_game_over(&runtime,&video,level,wait_frames,commit,&video);
    track_demand=0;
    unsigned presentation_reads=file_calls;
    int actors_equal=!memcmp(runtime.actors,initial_runtime.actors,sizeof runtime.actors);
    int expected_values=status==WF_OK&&original_result_values();
    int callback_order=!strcmp(events,"HCLCW")&&commits==2u&&wait_calls==1u&&wait_periods==97u&&
        level_calls==2u&&level_values[0]==1u&&level_values[1]==0u;
    int page_coverage=1,page_bytes=1;
    unsigned count=0;
    for(unsigned p=0;p<512u;++p){
        int expected=0;for(unsigned i=0;i<sizeof result_pages/sizeof *result_pages;++i)if(result_pages[i]==p)expected=1;
        if(demand[p])++count;
        if(demand[p]!=(unsigned)expected)page_coverage=0;
    }
    if(status==WF_OK){
        uint8_t bytes[1024];
        for(unsigned i=0;i<sizeof result_pages/sizeof *result_pages;++i){
            uint32_t at=result_pages[i]*1024u;
            if(wf_r18_rom_read(NULL,at,bytes,sizeof bytes)!=WF_OK||memcmp(bytes,rom+at,sizeof bytes))page_bytes=0;
        }
    }else page_bytes=0;
    int equivalent=snapshot(argv[5],argc==7?argv[6]:NULL)&&board_projection(argv[5]);
    printf("{\"variant\":\"%s\",\"mode\":\"%s\",\"status\":%u,\"capacity\":%u,\"preload_reads\":%u,\"churn_reads\":%u,\"churn_pages\":%u,\"churn_lines\":%u,\"post_preload_result_reads\":%u,\"failed_reads\":%u,\"first_failed_offset\":%u,\"video_failed_address\":%u,\"commits\":%u,\"wait_calls\":%u,\"wait_periods\":%u,\"events\":\"%s\",\"actors_equal\":%s,\"original_values_equal\":%s,\"snapshot_equal\":%s,\"result_page_count\":%u,\"result_page_coverage\":%s,\"result_page_bytes_equal\":%s,\"result_pages\":[",
        WP1980_CART?"cart":"base",argv[4],status,(unsigned)WF_R18_ROM_EXTRA_CAPACITY,preload_reads,churn_reads,churn_pages,churn_lines,presentation_reads,failed_reads,first_failed,video.failed_address,commits,wait_calls,wait_periods,events,
        actors_equal?"true":"false",expected_values?"true":"false",equivalent?"true":"false",count,page_coverage?"true":"false",page_bytes?"true":"false");
    unsigned separator=0;for(unsigned p=0;p<512u;++p)if(demand[p])printf("%s%u",separator++?",":"",p);puts("]}");
    int ok=status==WF_OK&&callback_order&&actors_equal&&expected_values&&page_coverage&&page_bytes&&equivalent&&
        (baseline||(!presentation_reads&&!failed_reads));
    if(!ok)fputs("FAIL: result must complete with unchanged original state and no denied post-preload read\n",stderr);
    return ok?0:1;
}
