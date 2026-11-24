/* Regression for TAG ring exit at progression1: $F560->$F98C->$26E66.
 * Actual action and transition owners execute; only files/audio are host I/O.
 * Removing scene6 must stop here with the photographed diagnostic instead
 * of falsely succeeding or reusing another arena's geometry/palette.
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wf_gameplay_bindings.h"
#include "wf_r17_actions_r11.h"
#include "wf_r17_dispatch_1c03e.h"
#include "wf_r17_dependencies.h"
#include "wf_r18_runtime_handoff.h"
#include "wf_r18_transition.h"
#include "wf_r18_ring.h"

static wf_runtime_t runtime;
static wf_gameplay_binding_context_t gameplay;
static wf_r18_video_t video;
static wf_r18_world_ctx_t world;
static wf_r18_display_t display;
static wf_r18_transition_context_t transition;
static wf_r17_actions_r11_bridge_t actions;
static wf_r17_dispatch_1c03e_ctx_t dispatch;
static wf_r17_render_ctx_t render;
static uint8_t rom[0x80000],scene[131072],raw[0x4000],priority;
static const char *assets,*world6,*outdir;
static unsigned audio_count,file_reads,file_sizes;
static uint16_t audio_word;

static void path_for(char *path,size_t capacity,const char *name)
{
    int n;
    if(!strcmp(name,"R18MAP6.BIN")&&world6)n=snprintf(path,capacity,"%s",world6);
    else n=snprintf(path,capacity,"%s/%s",assets,name);
    assert(n>0&&(size_t)n<capacity);
}
static int file_size(const char *name,uint32_t *bytes)
{
    char path[4096];path_for(path,sizeof path,name);++file_sizes;
    FILE *f=fopen(path,"rb");if(!f)return -1;
    assert(!fseek(f,0,SEEK_END));long n=ftell(f);assert(n>=0);fclose(f);
    *bytes=(uint32_t)n;return 0;
}
static int file_read(const char *name,uint32_t at,void *data,size_t bytes)
{
    char path[4096];path_for(path,sizeof path,name);++file_reads;
    FILE *f=fopen(path,"rb");if(!f)return -1;
    int failed=fseek(f,(long)at,SEEK_SET)||fread(data,1,bytes,f)!=bytes;
    fclose(f);return failed?-1:0;
}
static wf_status_t read_rom(void *user,uint32_t at,void *data,size_t bytes)
{
    (void)user;if(at>sizeof rom||bytes>sizeof rom-at)return WF_BLOCKED;
    memcpy(data,rom+at,bytes);return WF_OK;
}
static wf_status_t audio(wf_runtime_t *r,uint16_t word,void *user)
{(void)r;(void)user;++audio_count;audio_word=word;return WF_OK;}
static void save(const char *name,const void *data,size_t bytes)
{
    char path[4096];int n=snprintf(path,sizeof path,"%s/%s",outdir,name);
    assert(n>0&&(size_t)n<sizeof path);
    FILE *f=fopen(path,"wb");assert(f);assert(fwrite(data,1,bytes,f)==bytes);assert(!fclose(f));
}
static void snapshot(const char *name)
{
    memset(raw,0,sizeof raw);
    assert(wf_r18_runtime_export(&runtime,raw,sizeof raw)==WF_OK);
    /* The transition's original address cache belongs to world, outside the
     * typed runtime exporter. Serialize its 32 source/destination pairs too.
     */
    for(unsigned layer=0;layer<2;layer++)for(unsigned cell=0;cell<16;cell++){
        unsigned at=0x16d6u+layer*128u+cell*8u;
        uint32_t words[]={world.cache[layer][cell].source,world.cache[layer][cell].destination};
        for(unsigned word=0;word<2;word++)for(unsigned byte=0;byte<4;byte++)
            raw[at+word*4u+byte]=(uint8_t)(words[word]>>(24u-byte*8u));
    }
    save(name,raw,sizeof raw);
}
static void snapshot_outputs(const char *prefix)
{
    char name[80];
#define OUTPUT(NAME,DATA,SIZE) do{snprintf(name,sizeof name,"%s%s",prefix,NAME);save(name,DATA,SIZE);}while(0)
    OUTPUT("background.bin",video.background,sizeof video.background);
    OUTPUT("fg_palette.bin",video.fg_palette,sizeof video.fg_palette);
    OUTPUT("bg_palette.bin",video.bg_palette,sizeof video.bg_palette);
    OUTPUT("palettes.bin",video.palettes,sizeof video.palettes);
    uint8_t hardware[9]={(uint8_t)(display.scroll[0]>>8),(uint8_t)display.scroll[0],
        (uint8_t)(display.scroll[1]>>8),(uint8_t)display.scroll[1],
        (uint8_t)(display.scroll[2]>>8),(uint8_t)display.scroll[2],
        (uint8_t)(display.scroll[3]>>8),(uint8_t)display.scroll[3],priority};
    OUTPUT("hardware.bin",hardware,sizeof hardware);
    uint8_t registers[32];
    for(unsigned reg=0;reg<8;reg++)for(unsigned byte=0;byte<4;byte++)
        registers[reg*4u+byte]=(uint8_t)(runtime.m68k_regs.d[reg]>>(24u-byte*8u));
    OUTPUT("registers.bin",registers,sizeof registers);
#undef OUTPUT
}

int main(int argc,char **argv)
{
    assert(argc==6);assets=argv[1];world6=argv[2];outdir=argv[3];
    unsigned progression=(unsigned)strtoul(argv[4],NULL,0),facing=(unsigned)strtoul(argv[5],NULL,0);
    assert(progression<10&&facing<2);
    assert(!file_read("ROM68K.BIN",0,rom,sizeof rom));file_reads=0;
    wf_runtime_init(&runtime,NULL,0);
    runtime.g.credited_route=0x8000;runtime.g.mode_0161=0;
    runtime.g.progression_0162=(uint16_t)progression;
    runtime.g.global_007e=progression==1||progression==3||progression==5||progression==8?5:0;
    runtime.g.global_007f=(uint8_t)runtime.g.global_007e;
    runtime.g.gameplay_vblank_enable=0x80;
    runtime.g.rng_state=0x1234abcd;runtime.g.rng_state_valid=1;
    runtime.g.frame_counter=2608;runtime.m68k_regs.valid=1;
    for(unsigned i=0;i<8;i++)runtime.m68k_regs.d[i]=0x12340000u+i;
    wf_runtime_bind_audio(&runtime,audio,NULL);
    /* Four normal TAG records. Slot0 is the legal human landing outside,
     * slot4 is the opposing legal wrestler; slots1/5 are apron partners.
     * The original raw68 callback owns the transition flag/selector writes.
     */
    const unsigned slots[]={0,1,4,5},ids[]={0,1,9,7};
    const uint8_t flags[]={0x07,0,0x81,0x80},roles[]={0,0x40,0x80,0xc0};
    for(unsigned j=0;j<4;j++){
        unsigned i=slots[j];wf_actor_raw_t *a=&runtime.actors[i];
        wf_set_u8(a,0,0x80);wf_set_be16(a,2,(uint16_t)ids[j]);
        wf_set_be16(a,6,(uint16_t)(i<4?0x240:0x2c0));
        wf_set_be16(a,10,0x160);wf_set_be16(a,14,0x140);
        wf_set_be16(a,0x66,100);wf_set_be16(a,0x72,100);
        wf_set_u8(a,0x33,flags[j]);wf_set_u8(a,0x56,roles[j]);wf_set_u8(a,0x57,(uint8_t)ids[j]);
        wf_set_be32(a,0x86,wf_actor_rom_base(slots[j^1u]));
        wf_set_be32(a,0x7a,wf_actor_rom_base(i<4?4:0));
    }
    wf_actor_raw_t *a=&runtime.actors[0];
    wf_set_be16(a,WF_A_STATE,5);wf_set_be16(a,WF_A_LATCHED,0x8005);
    wf_set_be16(a,WF_A_SELECTOR,0x68);wf_set_be16(a,WF_A_CONTACT,0x10);
    wf_set_be16(a,WF_A_ANIM_INDEX,0);wf_set_be16(a,WF_A_ANIM_COUNTER,1);
    wf_set_u8(a,WF_A_FACING,facing?0x80:0);
    actions.fixed_object0=&runtime.fixed_object0;actions.fixed_object1=&runtime.fixed_object1;
    render.queue=&gameplay.render_queue;render.visibility_2188=wf_r17_visibility_2188_exact;
    actions.render=&render;actions.video_batch=wf_r18_video_batch;actions.video_user=&video;
    dispatch.actions=&actions;
    video.read_rom=read_rom;
    snapshot("before_action.bin");
    assert(wf_r17_dispatch_1c03e_bounded(&runtime,a,&dispatch)==WF_OK);
    assert(runtime.g.global_1678==0x80&&runtime.g.global_1679==(facing?0x80:0));
    assert(wf_be16(a,WF_A_SELECTOR)==0x6a&&audio_count==0);
    snapshot("before_f98c.bin");
    gameplay.r17_owner_user=&video;gameplay.audio_command=audio;
    gameplay.transition_reset=wf_r18_transition_reset;gameplay.transition_barrier=wf_r18_transition_barrier;
    transition=(wf_r18_transition_context_t){.video=&video,.world=&world,.display=&display,
        .camera=&gameplay.camera,.render_queue=&gameplay.render_queue,.scene_blob=scene,
        .scene_capacity=sizeof scene,.file_size=file_size,.file_read=file_read,.priority_140010=&priority};
    wf_r18_transition_bind(&transition);
    wf_status_t status=wf_r17_f98c(&runtime,&gameplay);
    printf("TAG_RINGOUT progression=%u facing=%u status=%u owner=%08x dependency=%08x tick=%u world=%u selector=%04x file_sizes=%u file_reads=%u audio=%u/%04x\n",
        progression,facing,status,transition.blocked_pc?0xf560u:gameplay.last_blocked_phase,
        transition.blocked_pc?transition.blocked_pc:gameplay.last_blocked_dependency,
        runtime.g.frame_counter,runtime.g.global_007e,wf_be16(a,WF_A_SELECTOR),
        file_sizes,file_reads,audio_count,audio_word);
    snapshot("after_f98c.bin");
    snapshot_outputs("");
    assert(runtime.g.rng_state==0x1234abcd&&runtime.g.frame_counter==2608);
    if(status!=WF_OK){fprintf(stderr,"FAIL TAG ring exit: required original arena%u is unavailable at F560/%05x\n",runtime.g.global_007e,transition.blocked_pc);return 1;}
    assert(runtime.g.global_1678==0&&runtime.g.global_1679==0);
    assert(runtime.g.gameplay_vblank_enable==0x80&&audio_count==1&&audio_word==0x80);
    assert(runtime.g.global_15f4==runtime.g.global_007e&&runtime.g.global_15f8==runtime.g.global_007e);
    assert(file_sizes==1&&file_reads==1&&priority==0x7c);
    assert(wf_be16(&runtime.referee,WF_A_STATE)==0x8003);
    assert(gameplay.camera.scene_index==runtime.g.global_007e&&video.ring_cell);
    puts("PASS original TAG exit transition completed with unchanged tick/RNG");

    /* Continue the same arena, RAM, actors, maps and palettes. Admit the
     * controlled wrestler at raw69's normal climb entry after floor movement;
     * do not inject the C0 transition flag or replace the animation callback.
     * Every raw69 animation callback then runs until the original two-legal-
     * wrestlers predicate requests reentry through F98C.
     */
    unsigned exterior=runtime.g.global_007e;
    wf_set_be16(a,WF_A_STATE,5);wf_set_be16(a,WF_A_LATCHED,5);
    wf_set_be16(a,WF_A_SELECTOR,0x69);wf_set_be16(a,WF_A_ANIM_INDEX,0);
    wf_set_be16(a,WF_A_ANIM_COUNTER,0);wf_set_u8(a,WF_A_FLAGS32,(uint8_t)(wf_u8(a,WF_A_FLAGS32)&~0x20u));
    wf_set_be16(a,WF_A_X,facing?0x42c:0x210);wf_set_be16(a,WF_A_Y,0x120);
    wf_set_be16(a,WF_A_Z,0x100);wf_set_be16(a,WF_A_CONTACT,0x0400);
    snapshot("before_return_action.bin");
    unsigned callbacks=0;
    while(!(runtime.g.global_1678&0x80u)){
        assert(callbacks++<128u);
        assert(wf_r17_dispatch_1c03e_bounded(&runtime,a,&dispatch)==WF_OK);
    }
    assert(runtime.g.global_1678==0xc0&&runtime.g.global_007e==exterior&&audio_count==1);
    snapshot("before_return_f98c.bin");
    status=wf_r17_f98c(&runtime,&gameplay);
    printf("TAG_REENTRY progression=%u facing=%u status=%u worlds=%u->%u raw69_callbacks=%u file_reads=%u audio=%u/%04x\n",
        progression,facing,status,exterior,runtime.g.global_007e,callbacks,file_reads,audio_count,audio_word);
    snapshot("after_return_f98c.bin");snapshot_outputs("return_");
    assert(status==WF_OK&&!transition.blocked_pc);
    static const uint8_t interior[10]={0,5,0,5,0,5,0,0,5,0};
    assert(runtime.g.global_007e==interior[progression]&&!(runtime.g.mode_0161&2u));
    assert(!runtime.g.global_1678&&!runtime.g.global_1679&&runtime.g.gameplay_vblank_enable==0x80);
    assert(runtime.g.rng_state==0x1234abcd&&runtime.g.frame_counter==2608);
    assert(audio_count==2&&audio_word==0x20&&file_reads==2&&file_sizes==2);
    assert(gameplay.camera.scene_index==interior[progression]&&video.ring_cell);
    puts("PASS original TAG exit/reentry chain completed with preserved state and unchanged tick/RNG");
    return 0;
}
