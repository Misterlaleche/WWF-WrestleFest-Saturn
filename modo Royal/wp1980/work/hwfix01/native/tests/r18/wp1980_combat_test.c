/* Integration continuation of the original Royal setup. The host backend
 * executes the same native actors, AI, collisions, HUD and sprite builder. */
#define main wp1980_launch_only_main
#include "wp1980_launch_test.c"
#undef main
#include "wf_r18_referee.h"
#include "wf_r18_match_result.h"
#include "wf_r18_present.h"
#include "wf_r18_rom_hot_pages.h"
#include "wf_r18_ring.h"
#include "wf_r18_hud.h"
#include "wf_r18_display.h"
#include "wf_r18_world.h"
#include "wf_r18_transition.h"
#include "wp1980_royal_init.h"
#include "wp1980_royal_rom_pages.h"

static uint8_t rom_cache[WF_SATURN_LWRAM_ROM_BYTES];
static uint8_t tile_cache[WF_SATURN_LWRAM_TILE_BYTES];
static uint8_t scene_blob[WF_SATURN_LWRAM_SCENE_BYTES];
static wf_r18_world_ctx_t combat_world;
static wf_r18_display_t display;
static wf_r18_transition_context_t transition;
static uint8_t priority;
static FILE *residency_trace;
static unsigned cold_sprite_reads,cold_rom_reads;
enum {ORACLE_RECORD_BYTES=106520};
static uint8_t oracle_record[ORACLE_RECORD_BYTES];
static FILE *oracle_file;
static unsigned oracle_frames,oracle_mismatch_frames,oracle_actor_byte_mismatches;
static unsigned oracle_rng_mismatches;
static unsigned oracle_referee_mismatch_frames,oracle_referee_byte_mismatches;
static unsigned oracle_sprite_mismatch_frames,oracle_sprite_word_mismatches;
static unsigned oracle_palette_mismatch_frames,oracle_palette_color_mismatches,oracle_scroll_mismatches;
static unsigned oracle_reported;
static FILE *native_context_trace;
static FILE *tile_demand_trace;
static const char *tile_trace_variant;
static uint8_t tiles_used[65536];
static unsigned tile_trace_frames,tile_trace_unique,tile_trace_peak;
static FILE *rom_demand_trace;
static unsigned rom_demand_enabled,rom_page_count;
static uint8_t rom_pages_used[512],frame_rom_pages[512];

static void finish_oracle_trace(void)
{
    if(oracle_reported||!oracle_frames)return;
    if(oracle_file){fclose(oracle_file);oracle_file=NULL;}
    if(native_context_trace){fclose(native_context_trace);native_context_trace=NULL;}
    printf("ORACLE_RESULT frames=%u mismatch_frames=%u actor_byte_mismatches=%u rng_mismatches=%u sprite_mismatch_frames=%u sprite_word_mismatches=%u palette_mismatch_frames=%u palette_color_mismatches=%u scroll_mismatches=%u referee_mismatch_frames=%u referee_byte_mismatches=%u\n",
        oracle_frames,oracle_mismatch_frames,oracle_actor_byte_mismatches,oracle_rng_mismatches,
        oracle_sprite_mismatch_frames,oracle_sprite_word_mismatches,oracle_palette_mismatch_frames,
        oracle_palette_color_mismatches,oracle_scroll_mismatches,
        oracle_referee_mismatch_frames,oracle_referee_byte_mismatches);
    oracle_reported=1u;
}

void wp1980_host_rom_demand(uint32_t address,size_t bytes)
{
    if(!rom_demand_enabled||!bytes)return;
    assert(address<0x80000u&&bytes<=0x80000u-address);
    for(unsigned page=address/1024u;page<=(address+(unsigned)bytes-1u)/1024u;page++){
        if(!rom_pages_used[page]){rom_pages_used[page]=1u;++rom_page_count;}
        frame_rom_pages[page]=1u;
    }
}

static void trace_frame_tiles(void)
{
    uint16_t tiles[WF_VDP1_STREAM_TILES_MAX];
    size_t count=wf_r18_present_frame_tiles(tiles,sizeof tiles/sizeof tiles[0]);
    assert(count<=WF_VDP1_STREAM_TILES_MAX);
    if(count>tile_trace_peak)tile_trace_peak=(unsigned)count;
    fprintf(tile_demand_trace,"%u,%u,%zu,",runtime.g.frame_counter,wf_r18_present_commands(),count);
    for(size_t i=0;i<count;i++){
        if(!tiles_used[tiles[i]]){tiles_used[tiles[i]]=1u;++tile_trace_unique;}
        fprintf(tile_demand_trace,"%s%04x",i?" ":"",tiles[i]);
    }
    fputc('\n',tile_demand_trace);++tile_trace_frames;
    fprintf(rom_demand_trace,"%u,",runtime.g.frame_counter);
    unsigned seen=0u;
    for(unsigned page=0;page<512u;page++)if(frame_rom_pages[page])
        fprintf(rom_demand_trace,"%s%03x",seen++?" ":"",page);
    fputc('\n',rom_demand_trace);memset(frame_rom_pages,0,sizeof frame_rom_pages);
}
static void finish_tile_trace(void)
{
    if(!tile_demand_trace)return;
    fclose(tile_demand_trace);tile_demand_trace=NULL;
    fclose(rom_demand_trace);rom_demand_trace=NULL;
    char path[160];
    snprintf(path,sizeof path,"audit/wp1980-host/royal-tiles-%s.txt",tile_trace_variant);
    FILE *file=fopen(path,"w");assert(file);
    for(unsigned i=0;i<65536u;i++)if(tiles_used[i])fprintf(file,"0x%04x\n",i);
    fclose(file);
    snprintf(path,sizeof path,"audit/wp1980-host/royal-rom-pages-%s.txt",tile_trace_variant);
    file=fopen(path,"w");assert(file);
    for(unsigned page=0;page<512u;page++)if(rom_pages_used[page])fprintf(file,"0x%03x\n",page);
    fclose(file);
    snprintf(path,sizeof path,"audit/wp1980-host/tile-demand-%s.json",tile_trace_variant);
    file=fopen(path,"w");assert(file);
    fprintf(file,"{\"variant\":\"%s\",\"frames\":%u,\"unique_tiles\":%u,\"peak_frame_unique_tiles\":%u,\"rom_pages\":%u}\n",
        tile_trace_variant,tile_trace_frames,tile_trace_unique,tile_trace_peak,rom_page_count);
    fclose(file);
    printf("TILE_DEMAND variant=%s frames=%u unique=%u peak_frame_unique=%u rom_pages=%u\n",
        tile_trace_variant,tile_trace_frames,tile_trace_unique,tile_trace_peak,rom_page_count);
}

static int file_size(const char *name,uint32_t *size)
{
    char path[256];snprintf(path,sizeof path,"audit/runtime18-cd/%s",name);
    FILE *f=fopen(path,"rb");if(!f)return -1;
    fseek(f,0,SEEK_END);*size=(uint32_t)ftell(f);fclose(f);return 0;
}
static int file_read(const char *name,uint32_t offset,void *out,size_t bytes)
{
    if(!strcmp(name,"HOTSPR.BIN"))name="ROYSPR.BIN";
    if(residency_trace){
        fprintf(residency_trace,"%u,%s,%u,%zu\n",runtime.g.frame_counter,name,offset,bytes);
        if(!strcmp(name,"SPRITES.PAK"))++cold_sprite_reads;
        if(!strcmp(name,"ROM68K.BIN"))++cold_rom_reads;
    }
    char path[256];snprintf(path,sizeof path,"audit/runtime18-cd/%s",name);
    FILE *f=fopen(path,"rb");if(!f)return -1;
    int status=fseek(f,(long)offset,SEEK_SET)||fread(out,1,bytes,f)!=bytes;
    fclose(f);return status?-1:0;
}
static wf_status_t ram_word(wf_runtime_t *r,uint32_t at,uint16_t *out,void *u)
{
    (void)u;
    switch(at){
    case 0x1c0062u:case 0x1c0063u:*out=r->g.global_0063;return WF_OK;
    case 0x1c004eu:*out=(uint16_t)((r->g.credit_remainder_004e<<8)|r->g.credit_count_004f);return WF_OK;
    case 0x1c004au:*out=r->g.credit_cycle_units_004b;return WF_OK;
    default:video.failed_address=at;return WF_BLOCKED;
    }
}
static wf_status_t input_word(wf_runtime_t *r,uint32_t at,uint16_t *out,void *u)
{
    (void)r;(void)u;at&=0xffffffu;
    if(at<0x140020u||at>0x140026u||(at&1u))return WF_BLOCKED;
    *out=(uint16_t)(0xff00u|gameplay.input_hw_low[(at-0x140020u)/2u]);return WF_OK;
}
static wf_status_t audio(wf_runtime_t *r,uint16_t command,void *u)
{(void)r;(void)command;(void)u;return WF_OK;}
static wf_status_t wait_periods(wf_runtime_t *r,uint16_t periods,void *u)
{(void)r;(void)u;clock_count+=periods+1u;return WF_OK;}
static wf_status_t world_288f0(wf_runtime_t *r,uint8_t record[4],uint8_t bank,void *u)
{(void)u;return wf_r18_world_288f0(r,record,bank,&combat_world);}
static wf_status_t world_26b68(wf_runtime_t *r,uint8_t side,uint16_t direction,void *u)
{(void)u;return wf_r18_world_26b68(r,side,direction,&combat_world);}
static uint32_t rl(const uint8_t *p){return ((uint32_t)rw(p)<<16)|rw(p+2);}

int main(int argc,char **argv)
{
    unsigned limit=argc>2?(unsigned)strtoul(argv[2],NULL,10):3000u;
    int demo=argc>3?atoi(argv[3]):1;
    int direct=argc>4&&strstr(argv[4],".BIN")!=NULL;
    if(direct){
        size_t count=wf_gameplay_bindings_build(&gameplay,bindings,48);
        assert(count);wf_runtime_init(&runtime,bindings,count);
        FILE *file=fopen(argv[4],"rb");assert(file);
        size_t bytes=fread(scene_blob,1,sizeof scene_blob,file);fclose(file);
        wp1980_royal_init_info_t original;
        assert(wp1980_royal_init_apply(&runtime,&video,&combat_world,scene_blob,bytes,&original)==WF_OK);
        memcpy(raw,scene_blob+WP1980_ROYINIT_RAM,sizeof raw);priority=original.priority;
        printf("Direct original Royal imported: pc=%x source_rng=%08x post_F8C_rng=%08x mode=%u paid=%x\n",
            original.source_pc,original.source_rng,runtime.g.rng_state,runtime.g.mode_0161,runtime.g.credited_route);
        for(unsigned i=0;i<WF_ACTOR_SLOTS;i++)if(wf_u8(&runtime.actors[i],WF_A_ACTIVE)&128u)
            printf("slot=%u id=%u x=%u y=%u\n",i,wf_be16(&runtime.actors[i],2),wf_be16(&runtime.actors[i],6),wf_be16(&runtime.actors[i],10));
    }else if(wp1980_launch_only_main(2,argv))return 2;
    wp1980_frontend_input_disable();wp1980_demo_active=(uint32_t)demo;
    if(argc>4&&!direct){
        char path[512],json[1024];FILE *file;
        snprintf(path,sizeof path,"%s/entry_ram.bin",argv[4]);file=fopen(path,"rb");assert(file);
        assert(fread(raw,1,sizeof raw,file)==sizeof raw);fclose(file);
        snprintf(path,sizeof path,"%s/entry_registers.json",argv[4]);file=fopen(path,"r");assert(file);
        assert(fgets(json,sizeof json,file));fclose(file);
        wf_r17_m68k_shadow_t regs={.valid=1};
        for(unsigned i=0;i<8u;i++){
            char name[16];snprintf(name,sizeof name,"\"D%u\":",i);char *p=strstr(json,name);assert(p);
            regs.d[i]=(uint32_t)strtoul(p+strlen(name),NULL,10);
        }
        assert(wf_r18_runtime_handoff_f8c(&runtime,raw,sizeof raw,&regs)==WF_OK);
        snprintf(path,sizeof path,"%s/frames.bin",argv[4]);oracle_file=fopen(path,"rb");assert(oracle_file);
        assert(fread(oracle_record,1,sizeof oracle_record,oracle_file)==sizeof oracle_record);
        memcpy(video.palette_used,raw+0x1600u,sizeof video.palette_used);
        memcpy(video.palette_map,raw+0x1610u,sizeof video.palette_map);
        const uint8_t *palette=oracle_record+16u+16384u+8192u+4096u+4096u+8192u;
        memcpy(video.palettes,palette,sizeof video.palettes);
        memcpy(video.bg_palette,palette+0x6000u,sizeof video.bg_palette);
        memcpy(video.fg_palette,palette+0x8000u,sizeof video.fg_palette);
        memcpy(video.fg0,oracle_record+16u+16384u+8192u+4096u+4096u,8192u);
        printf("Oracle entry imported: original F94 snapshot, original D0-D7;mode=%u paid=%x\n",runtime.g.mode_0161,runtime.g.credited_route);
    }
    if(direct&&argc>5){
        char path[512];snprintf(path,sizeof path,"%s/frames.bin",argv[5]);
        oracle_file=fopen(path,"rb");assert(oracle_file);
        assert(fread(oracle_record,1,sizeof oracle_record,oracle_file)==sizeof oracle_record);
    }
    rom_demand_enabled=1u;
    uint32_t bytes;assert(!file_size("R18RING.BIN",&bytes));assert(bytes<=sizeof scene_blob);
    assert(!file_read("R18RING.BIN",0,scene_blob,bytes));assert(!wf_r18_ring_bind(scene_blob,bytes));
    assert(wf_r18_hud_bind(&video,file_read)==WF_OK);
    wf_r18_present_init(file_read,rom_cache,sizeof rom_cache,tile_cache,sizeof tile_cache);
    if(direct){
        assert(wf_r18_rom_pages_preload(wp1980_rom_main_pages,sizeof wp1980_rom_main_pages/sizeof *wp1980_rom_main_pages)==WF_OK);
        assert(wf_r18_rom_extra_pages_preload(wp1980_rom_fast_pages,sizeof wp1980_rom_fast_pages/sizeof *wp1980_rom_fast_pages)==WF_OK);
    }else{
        assert(wf_r18_rom_pages_preload(wf_r18_rom_hot_pages,sizeof wf_r18_rom_hot_pages/sizeof *wf_r18_rom_hot_pages)==WF_OK);
        assert(wf_r18_rom_extra_pages_preload(wf_r18_rom_royal_extra_pages,sizeof wf_r18_rom_royal_extra_pages/sizeof *wf_r18_rom_royal_extra_pages)==WF_OK);
    }
    assert(wf_r18_hot_sprites_load()==WF_OK);
    video.read_rom=wf_r18_rom_read;video.read_ram=ram_word;video.read_user=NULL;
    wf_r18_video_ring_bind(&video);combat_world.read_rom=wf_r18_rom_read;
    for(unsigned side=0;side<2u;side++)for(unsigned i=0;i<16u;i++){
        const uint8_t *p=raw+0x16d6u+side*128u+i*8u;
        combat_world.cache[side][i]=(wf_r18_world_transport_t){rl(p),rl(p+4)};
    }
    gameplay.cabinet_words_valid=gameplay.camera_valid=1;
    gameplay.camera.scene_index=runtime.g.global_007e;
    gameplay.camera.scroll0_x=runtime.g.global_17e6;gameplay.camera.scroll1_x=runtime.g.global_17ea;
    gameplay.camera.scroll0_y=runtime.g.global_17ee;gameplay.camera.scroll1_y=runtime.g.global_17f2_word;
    gameplay.input_valid_mask=0x1ffu;memset(gameplay.input_hw_low,0xff,sizeof gameplay.input_hw_low);
    gameplay.audio_command=audio;
    gameplay.referee_direction_20c8=wf_r18_referee_direction_20c8;
    gameplay.referee_display_2067c=wf_r18_referee_display_2067c;
    gameplay.referee_number_2067c=wf_r18_referee_number_2067c;
    gameplay.referee_clear_206fe=wf_r18_referee_clear_206fe;
    gameplay.referee_tag_phase6_90d6=wf_r18_tag_bonus_90d6;
    gameplay.referee_result_20156=wf_r18_result_sound_20156;
    gameplay.participant_input_word=input_word;gameplay.participant_commit_batch=wf_r18_video_commit;
    gameplay.post7_wait_21e6=wait_periods;gameplay.post7_fg0_batch=wf_r18_video_batch;
    gameplay.post7_video_batch=wf_r18_video_batch;gameplay.post7_resource_2aea=wf_r18_palette_2aea;
    gameplay.post7_resource_2b58=wf_r18_palette_2b58;gameplay.post7_raw_2503c=wf_r18_text_2503c;
    gameplay.post7_raw_9aca=wf_r18_strip_9aca;gameplay.post7_fg0_peek_long=wf_r18_video_peek;
    gameplay.post7_render_2836_batch=wf_r18_present_batch;
    gameplay.branchb_288f0=world_288f0;gameplay.branchb_26b68=world_26b68;gameplay.r17_owner_user=&video;
    assert(wf_r18_display_init(&display,&runtime,wf_r18_rom_read,NULL)==WF_OK);
    if(!direct)priority=(uint8_t)rw(control+0x10u);
    transition=(wf_r18_transition_context_t){.video=&video,.world=&combat_world,.display=&display,
        .render_queue=&gameplay.render_queue,.camera=&gameplay.camera,
        .scene_blob=scene_blob,.scene_capacity=sizeof scene_blob,.file_size=file_size,.file_read=file_read,
        .priority_140010=&priority};
    wf_r18_transition_bind(&transition);gameplay.transition_reset=wf_r18_transition_reset;
    gameplay.transition_barrier=wf_r18_transition_barrier;
    tile_trace_variant=direct?(oracle_file?"direct-oracle":demo?"direct-demo":"direct-noinput"):
        argc>4?"oracle":demo?"demo":"noinput";
    char tile_path[160];snprintf(tile_path,sizeof tile_path,"audit/wp1980-host/residency-%s.csv",tile_trace_variant);
    residency_trace=fopen(tile_path,"w");
    assert(residency_trace);fputs("frame,file,offset,bytes\n",residency_trace);
    snprintf(tile_path,sizeof tile_path,"audit/wp1980-host/frame-tiles-%s.csv",tile_trace_variant);
    tile_demand_trace=fopen(tile_path,"w");assert(tile_demand_trace);
    fputs("frame,commands,unique_tiles,tile_ids\n",tile_demand_trace);
    snprintf(tile_path,sizeof tile_path,"audit/wp1980-host/frame-rom-pages-%s.csv",tile_trace_variant);
    rom_demand_trace=fopen(tile_path,"w");assert(rom_demand_trace);fputs("frame,rom_pages\n",rom_demand_trace);
    if(oracle_file){native_context_trace=fopen(direct?"audit/wp1980-host/native-context-direct-oracle.csv":
        "audit/wp1980-host/native-context.csv","w");assert(native_context_trace);
        fputs("frame,rng,D0,D1,D2,D3,D4,D5,D6,D7\n",native_context_trace);}
    unsigned max_commands=0u;
    while(runtime.g.frame_counter<limit){
        gameplay.input_hw_low[0]=wp1980_input(&runtime,0xffu);
        gameplay.cabinet_word_140020=(uint16_t)(0xff00u|gameplay.input_hw_low[0]);
        gameplay.cabinet_word_140022=0xffffu;
        wf_status_t status=wf_runtime_frame(&runtime);
        if(status==WF_BLOCKED&&runtime.frame_ready){
            trace_frame_tiles();
            if(oracle_file){
                assert(runtime.g.frame_counter==rl(oracle_record+4u));
                fprintf(native_context_trace,"%u,%u",runtime.g.frame_counter,runtime.g.rng_state);
                for(unsigned d=0;d<8u;d++)fprintf(native_context_trace,",%u",runtime.m68k_regs.d[d]);
                fputc('\n',native_context_trace);
                uint32_t expected_rng=rl(oracle_record+16u+0x5cu);
                if(runtime.g.rng_state!=expected_rng){
                    if(!oracle_rng_mismatches)printf("ORACLE_RNG_DIFF frame=%u native=%08x original=%08x\n",runtime.g.frame_counter,runtime.g.rng_state,expected_rng);
                    ++oracle_rng_mismatches;
                }
                const uint8_t *original=oracle_record+16u+0x5b0u;
                const uint8_t *native=(const uint8_t *)runtime.actors;
                unsigned differences=0;
                for(unsigned i=0;i<sizeof runtime.actors;i++)if(native[i]!=original[i]){
                    if(oracle_mismatch_frames<2u&&differences<16u)
                        printf("ORACLE_DIFF frame=%u slot=%u offset=%02x native=%02x original=%02x\n",
                            runtime.g.frame_counter,i/WF_ACTOR_BYTES,i%WF_ACTOR_BYTES,native[i],original[i]);
                    ++differences;
                }
                ++oracle_frames;oracle_actor_byte_mismatches+=differences;
                if(differences)++oracle_mismatch_frames;
                unsigned referee_differences=0;
                const uint8_t *referee=(const uint8_t *)&runtime.referee;
                for(unsigned i=0;i<0x64u;i++)if(referee[i]!=oracle_record[16u+0x11f4u+i]){
                    if(!oracle_referee_mismatch_frames&&referee_differences<16u)
                        printf("ORACLE_REFEREE_DIFF frame=%u offset=%02x native=%02x original=%02x\n",
                            runtime.g.frame_counter,i,referee[i],oracle_record[16u+0x11f4u+i]);
                    ++referee_differences;
                }
                if(referee_differences){
                    if(!oracle_referee_mismatch_frames){
                        uint8_t raw[0x4000]={0};
                        assert(wf_r18_runtime_export(&runtime,raw,sizeof raw)==WF_OK);
                        FILE *snapshot=fopen("audit/wp1980-host/referee-first-diff-native.bin","wb");
                        assert(snapshot&&fwrite(raw,1,sizeof raw,snapshot)==sizeof raw);fclose(snapshot);
                        snapshot=fopen("audit/wp1980-host/referee-first-diff-original.bin","wb");
                        assert(snapshot&&fwrite(oracle_record+16u,1,sizeof raw,snapshot)==sizeof raw);fclose(snapshot);
                    }
                    ++oracle_referee_mismatch_frames;oracle_referee_byte_mismatches+=referee_differences;
                }
                /* Compare every hardware-significant sprite word. Original
                 * 68000 writes leave unused upper bits and trailing words
                 * stale; these bits do not participate in sprite decoding. */
                size_t capacity,native_count=wf_r18_present_records_count();
                const wf_r18_sprite_record_t *native_records=wf_r18_present_records_buffer(&capacity);
                unsigned original_count=rw(oracle_record+16u+0x19b2u);
                const uint8_t *original_records=oracle_record+16u+16384u;
                static const uint16_t sprite_masks[6]={0x00ffu,0x00ffu,0x00ffu,0x00ffu,0x000fu,0x00ffu};
                unsigned sprite_differences=native_count!=original_count;
                assert(native_count<=capacity&&original_count<=512u);
                for(unsigned i=0;i<native_count&&i<original_count;i++)for(unsigned word=0;word<6u;word++){
                    unsigned native_word=rw(native_records[i].bytes+word*2u)&sprite_masks[word];
                    unsigned original_word=rw(original_records+i*16u+word*2u)&sprite_masks[word];
                    if(native_word!=original_word){
                        if(!oracle_sprite_mismatch_frames&&sprite_differences<8u)
                            printf("ORACLE_SPRITE_DIFF frame=%u record=%u word=%u native=%x original=%x\n",
                                runtime.g.frame_counter,i,word,native_word,original_word);
                        ++sprite_differences;
                    }
                }
                if(sprite_differences){
                    if(!oracle_sprite_mismatch_frames)printf("ORACLE_SPRITE_COUNTS frame=%u native=%zu original=%u\n",runtime.g.frame_counter,native_count,original_count);
                    ++oracle_sprite_mismatch_frames;oracle_sprite_word_mismatches+=sprite_differences;
                }
                const uint8_t *original_palette=oracle_record+16u+16384u+8192u+4096u+4096u+8192u;
                const uint8_t *native_palettes[4]={video.palettes,video.palettes+0x2000u,video.fg_palette,video.bg_palette};
                static const unsigned palette_offsets[4]={0u,0x2000u,0x8000u,0x6000u};
                unsigned palette_differences=0u;
                for(unsigned role=0;role<4u;role++)for(unsigned bank=0;bank<16u;bank++)for(unsigned color=0;color<16u;color++){
                    unsigned offset=bank*128u+color*2u;
                    unsigned native_color=rw(native_palettes[role]+offset)&0xfffu;
                    unsigned original_color=rw(original_palette+palette_offsets[role]+offset)&0xfffu;
                    if(native_color!=original_color){
                        if(!oracle_palette_mismatch_frames&&palette_differences<8u)
                            printf("ORACLE_PALETTE_DIFF frame=%u role=%u bank=%u color=%u native=%03x original=%03x\n",
                                runtime.g.frame_counter,role,bank,color,native_color,original_color);
                        ++palette_differences;
                    }
                }
                if(palette_differences){++oracle_palette_mismatch_frames;oracle_palette_color_mismatches+=palette_differences;}
                for(unsigned port=0;port<4u;port++)if(display.scroll[port]!=rw(oracle_record+ORACLE_RECORD_BYTES-8u+port*2u)){
                    if(!oracle_scroll_mismatches)printf("ORACLE_SCROLL_DIFF frame=%u port=%u native=%04x original=%04x\n",
                        runtime.g.frame_counter,port,display.scroll[port],rw(oracle_record+ORACLE_RECORD_BYTES-8u+port*2u));
                    ++oracle_scroll_mismatches;
                }
                if(fread(oracle_record,1,sizeof oracle_record,oracle_file)!=sizeof oracle_record){
                    finish_oracle_trace();
                }
            }
            assert(wf_r18_display_irq(&runtime,&display)==WF_OK);
            assert(wf_r18_world_commit(&runtime,&combat_world)==WF_OK);
            assert(wf_r18_hud_irq(&runtime,&video)==WF_OK);
            assert(wf_r18_hud_commit(&video)==WF_OK);
            assert(!wf_r18_ring_update((int16_t)display.scroll[0],(int16_t)(2048u-display.scroll[1]),
                (int16_t)display.scroll[2],(int16_t)(2048u-display.scroll[3]),priority));
            unsigned commands=wf_r18_present_commands();if(commands>max_commands)max_commands=commands;
            wf_runtime_vblank_ack(&runtime);status=wf_runtime_frame(&runtime);
        }
        if(status==WF_BLOCKED&&wf_r18_royal_game_over(&runtime)){
            finish_oracle_trace();
            finish_tile_trace();
            fclose(residency_trace);residency_trace=NULL;
            printf("PASS: original Royal gameover frame=%u max_commands=%u demo=%d cold_sprite_reads=%u cold_rom_reads=%u\n",runtime.g.frame_counter,max_commands,demo,cold_sprite_reads,cold_rom_reads);return 0;
        }
        if(status!=WF_OK){
            finish_oracle_trace();
            finish_tile_trace();
            uint8_t failure_ram[0x4000];memcpy(failure_ram,raw,sizeof failure_ram);
            assert(wf_r18_runtime_export(&runtime,failure_ram,sizeof failure_ram)==WF_OK);
            FILE *failure=fopen("audit/wp1980-host/failure-ram.bin","wb");assert(failure);
            assert(fwrite(failure_ram,1,sizeof failure_ram,failure)==sizeof failure_ram);fclose(failure);
            failure=fopen("audit/wp1980-host/failure-context.txt","w");assert(failure);
            fprintf(failure,"frame=%u stage=%u phase=%x dependency=%x\n",runtime.g.frame_counter,
                runtime.frame_stage,gameplay.last_blocked_phase,gameplay.last_blocked_dependency);
            for(unsigned i=0;i<8u;i++)fprintf(failure,"D%u=%08x\n",i,runtime.m68k_regs.d[i]);
            for(unsigned i=0;i<WF_ACTOR_SLOTS;i++)fprintf(failure,
                "slot%u target=%08x target2=%08x link=%08x action=%04x state=%04x fresh=%04x\n",i,
                wf_be32(&runtime.actors[i],WF_A_TARGET),wf_be32(&runtime.actors[i],WF_A_TARGET2),
                wf_be32(&runtime.actors[i],WF_A_LINK),wf_be16(&runtime.actors[i],WF_A_SELECTOR),
                wf_be16(&runtime.actors[i],WF_A_STATE),wf_be16(&runtime.actors[i],WF_A_INPUT_FRESH));
            fclose(failure);
            fclose(residency_trace);residency_trace=NULL;
            fprintf(stderr,"BLOCKED frame=%u phase=%x dep=%x slot=%u video=%x transition=%x stage=%u ready=%u ack=%u\n",
                runtime.g.frame_counter,gameplay.last_blocked_phase,gameplay.last_blocked_dependency,
                gameplay.last_blocked_slot,video.failed_address,transition.blocked_pc,
                runtime.frame_stage,runtime.frame_ready,runtime.vblank_ack);
            for(unsigned i=0;i<WF_ACTOR_SLOTS;i++)if(wf_u8(&runtime.actors[i],WF_A_ACTIVE)&0x80u)
                fprintf(stderr," actor%u id=%u state=%x action=%x hp=%u xy=%u,%u cpu_dep=%x action_dep=%x\n",i,
                    wf_be16(&runtime.actors[i],2),wf_be16(&runtime.actors[i],0x20),wf_be16(&runtime.actors[i],0x60),
                    wf_be16(&runtime.actors[i],0x66),wf_be16(&runtime.actors[i],6),wf_be16(&runtime.actors[i],10),
                    gameplay.cpu_trace[i].blocked_dependency_rom,gameplay.f4c2_actions.trace[i].blocked_dependency_rom);
            return 2;
        }
    }
    fclose(residency_trace);residency_trace=NULL;
    finish_oracle_trace();
    finish_tile_trace();
    printf("PASS: nativeRoyal frames=%u max_commands=%u demo=%d cold_sprite_reads=%u cold_rom_reads=%u\n",runtime.g.frame_counter,max_commands,demo,cold_sprite_reads,cold_rom_reads);
    return 0;
}
