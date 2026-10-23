#include <stdio.h>
#include <string.h>
#include <yaul.h>
#include "wf_r18_hud.h"
#include "wf_saturn_memory.h"

/* Instrument bank invocations while retaining the production effective-color
 * cache, conversion and volatile CRAM stores from memory.c. */
#define wf_saturn_cram_write_bank real_cram_write_bank
#include "../../src/wf_saturn_memory.c"
#undef wf_saturn_cram_write_bank

#define CHECK(c) do {if(!(c)){fprintf(stderr,"palette-work:%u: %s\n",__LINE__,#c);return 1;}}while(0)
uint8_t mock_vdp2_vram[0x80000];
uint32_t mock_cram[1024];
unsigned mock_screen_priorities[3],mock_display;
static wf_r18_video_t video;
static unsigned calls;
static uint16_t banks;
static uint16_t fail_first=0xffffu;
static wf_r18_video_t tracking;
static wf_r18_video_t windows;
static wf_runtime_t runtime;
static unsigned font_reads;
wf_status_t wf_saturn_cram_write_bank(uint16_t first,const uint8_t *colors){
    ++calls;
    if(first>=WF_SATURN_CRAM_TEXT_FIRST&&first<WF_SATURN_CRAM_BG_FIRST)
        banks|=(uint16_t)(1u<<((first-WF_SATURN_CRAM_TEXT_FIRST)/16u));
    if(first==fail_first)return WF_BLOCKED;
    return real_cram_write_bank(first,colors);
}
static int read_font(const char *name,uint32_t at,void *out,size_t bytes){
    if(strcmp(name,"FONT8.BIN")||at!=font_reads*2048u||bytes!=2048u)return 1;
    font_reads++;memset(out,0,bytes);return 0;
}
static wf_status_t read_palette(void *user,uint32_t address,void *out,size_t bytes){
    (void)user;
    if(address!=0x2f22u+3u*32u||bytes!=32u)return WF_BLOCKED;
    memset(out,0,bytes);return WF_OK;
}
static int check_tracking(void){
    static const uint32_t bases[4]={0x180000u,0x182000u,0x188000u,0x186000u};
    CHECK(WF_R18_PALETTE_TEXT==0&&WF_R18_PALETTE_SPRITES==1&&WF_R18_PALETTE_FG==2&&WF_R18_PALETTE_BG==3);
    CHECK(offsetof(wf_r18_video_t,palette_dirty)==offsetof(wf_r18_video_t,failed_address)+4u);
    CHECK(offsetof(wf_r18_video_t,palette_committed)==offsetof(wf_r18_video_t,palette_dirty)+8u);
    CHECK(offsetof(wf_r18_video_t,ring_cell)==(sizeof(void*)==8u?53824u:53808u));
    CHECK(sizeof tracking==(sizeof(void*)==8u?53832u:53812u)); /* palette masks + one live-ring callback pointer */
    for(unsigned role=0;role<4;role++){
        CHECK(wf_r18_video_palette_pending(&tracking,(wf_r18_palette_role_t)role)==0xffffu);
        wf_r18_video_palette_ack(&tracking,(wf_r18_palette_role_t)role,0x5555u);
        CHECK(wf_r18_video_palette_pending(&tracking,(wf_r18_palette_role_t)role)==0xaaaau);
        wf_r18_video_palette_ack(&tracking,(wf_r18_palette_role_t)role,0xaaaau);
    }
    for(unsigned role=0;role<4;role++)for(unsigned bank=0;bank<16;bank++){
        wf_r17_fg0_write_t write={bases[role]+bank*128u+31u,bank+1u,1};
        CHECK(wf_r18_video_batch(NULL,&write,1,&tracking)==WF_OK);
        for(unsigned other=0;other<4;other++)CHECK(wf_r18_video_palette_pending(&tracking,(wf_r18_palette_role_t)other)==(other==role?(1u<<bank):0u));
        wf_r18_video_palette_ack(&tracking,(wf_r18_palette_role_t)role,(uint16_t)(1u<<bank));
        CHECK(wf_r18_video_batch(NULL,&write,1,&tracking)==WF_OK);
        CHECK(!wf_r18_video_palette_pending(&tracking,(wf_r18_palette_role_t)role));
        wf_r17_fg0_write_t padding[2]={{bases[role]+bank*128u+32u,0xa5u,1},{bases[role]+bank*128u+127u,0x5au,1}};
        CHECK(wf_r18_video_batch(NULL,padding,2,&tracking)==WF_OK);
        CHECK(!wf_r18_video_palette_pending(&tracking,(wf_r18_palette_role_t)role));
    }
    {
        wf_r17_fg0_write_t unused[4]={{0x180800u,1,1},{0x181000u,2,1},{0x182800u,3,1},{0x183fffu,4,1}};
        CHECK(wf_r18_video_batch(NULL,unused,4,&tracking)==WF_OK);
        for(unsigned role=0;role<4;role++)CHECK(!wf_r18_video_palette_pending(&tracking,(wf_r18_palette_role_t)role));
    }
    {
        wf_r17_fg0_write_t cross={0x18807fu,0x12345678u,4};
        CHECK(wf_r18_video_batch(NULL,&cross,1,&tracking)==WF_OK);
        CHECK(wf_r18_video_palette_pending(&tracking,WF_R18_PALETTE_FG)==2u);
        wf_r18_video_palette_ack(&tracking,WF_R18_PALETTE_FG,2u);
        cross=(wf_r17_fg0_write_t){0x18601eu,0xabcdef12u,4};
        CHECK(wf_r18_video_batch(NULL,&cross,1,&tracking)==WF_OK);
        CHECK(wf_r18_video_palette_pending(&tracking,WF_R18_PALETTE_BG)==1u);
        wf_r18_video_palette_ack(&tracking,WF_R18_PALETTE_BG,1u);
    }
    /* Both staged buses validate before changing bytes or palette masks. */
    {
        wf_r17_fg0_write_t first={0x180003u,0xff,1},bad[2]={{0x182003u,0x3f,1},{0xdeadbeefu,1,1}};
        uint8_t old_text=tracking.palettes[3],old_sprite=tracking.palettes[0x2003];
        CHECK(wf_r18_video_commit(NULL,&first,1,bad,2,&tracking)==WF_BLOCKED);
        CHECK(tracking.palettes[3]==old_text&&tracking.palettes[0x2003]==old_sprite);
        for(unsigned role=0;role<4;role++)CHECK(!wf_r18_video_palette_pending(&tracking,(wf_r18_palette_role_t)role));
    }
    /* $2AEA writes directly and must dirty its selected sprite bank even
     * when reallocated bytes are already equal to the effective palette. */
    memset(tracking.palettes+0x2000u,0,32);tracking.read_rom=read_palette;
    CHECK(wf_r18_palette_2aea(NULL,3,&tracking)==WF_OK);
    CHECK(wf_r18_video_palette_pending(&tracking,WF_R18_PALETTE_SPRITES)==1u);
    wf_r18_video_palette_ack(&tracking,WF_R18_PALETTE_SPRITES,1u);
    CHECK(wf_r18_palette_2b58(NULL,3,&tracking)==WF_OK);
    CHECK(!wf_r18_video_palette_pending(&tracking,WF_R18_PALETTE_SPRITES));
    CHECK(wf_r18_palette_2aea(NULL,3,&tracking)==WF_OK);
    CHECK(wf_r18_video_palette_pending(&tracking,WF_R18_PALETTE_SPRITES)==1u);
    wf_r18_video_palette_ack(&tracking,WF_R18_PALETTE_SPRITES,1u);
    CHECK(wf_r18_palette_2aea(NULL,4,&tracking)==WF_BLOCKED);
    CHECK(!wf_r18_video_palette_pending(&tracking,WF_R18_PALETTE_SPRITES));
    return 0;
}

int main(void){
    memset(mock_vdp2_vram,0xa5,sizeof mock_vdp2_vram);wf_saturn_cram_reset();
    CHECK(wf_r18_hud_commit(&video)==WF_OK);
    CHECK(calls==16&&banks==0xffffu&&wf_saturn_cram_writes==256u);
    calls=0;banks=0;
    CHECK(wf_r18_hud_commit(&video)==WF_OK);
    printf("Unchanged TEXT commit: %u bank calls\n",calls);fflush(stdout);
    CHECK(calls==0&&banks==0&&wf_saturn_cram_writes==256u);
    CHECK(!check_tracking());
    {
        wf_r17_fg0_write_t write={0x180183u,15u,1}; /* TEXT bank3, pen1 low byte */
        CHECK(wf_r18_video_batch(NULL,&write,1,&video)==WF_OK);
        CHECK(wf_r18_hud_commit(&video)==WF_OK&&calls==1&&banks==8u);
        CHECK(mock_cram[WF_SATURN_CRAM_TEXT_FIRST+3u*16u+1u]==0xffu&&wf_saturn_cram_writes==257u);
        calls=0;banks=0;
        CHECK(wf_r18_video_batch(NULL,&write,1,&video)==WF_OK);
        CHECK(wf_r18_hud_commit(&video)==WF_OK&&!calls&&wf_saturn_cram_writes==257u);
        write=(wf_r17_fg0_write_t){0x180182u,0xf0u,1};
        CHECK(wf_r18_video_batch(NULL,&write,1,&video)==WF_OK);
        CHECK(wf_r18_hud_commit(&video)==WF_OK&&calls==1&&wf_saturn_cram_writes==257u);
        CHECK(mock_cram[WF_SATURN_CRAM_TEXT_FIRST+3u*16u+1u]==0xffu);
    }
    runtime.g.gameplay_vblank_enable=128;runtime.g.frame_handshake[1]=1;runtime.g.global_169e=0x2000;runtime.g.global_16a4=1;
    calls=0;banks=0;
    CHECK(wf_r18_hud_irq(&runtime,&video)==WF_OK);
    CHECK(wf_r18_hud_commit(&video)==WF_OK&&calls==1&&banks==(1u<<14));
    CHECK(mock_cram[WF_SATURN_CRAM_TEXT_FIRST+14u*16u+14u]==0xffu);
    CHECK(mock_cram[WF_SATURN_CRAM_TEXT_FIRST+14u*16u+15u]==0x33u);
    runtime.g.global_16a4=0;calls=0;banks=0;
    CHECK(wf_r18_hud_irq(&runtime,&video)==WF_OK);
    CHECK(wf_r18_hud_commit(&video)==WF_OK&&calls==1&&banks==(1u<<14));
    CHECK(mock_cram[WF_SATURN_CRAM_TEXT_FIRST+14u*16u+14u]==0xffffu);
    CHECK(mock_cram[WF_SATURN_CRAM_TEXT_FIRST+14u*16u+15u]==0xffu);
    calls=0;CHECK(wf_r18_hud_irq(&runtime,&video)==WF_OK);
    CHECK(wf_r18_hud_commit(&video)==WF_OK&&!calls);
    runtime.g.gameplay_vblank_enable=0;runtime.g.global_16a4=1;
    CHECK(wf_r18_hud_irq(&runtime,&video)==WF_OK);
    CHECK(wf_r18_hud_commit(&video)==WF_OK&&!calls);
    /* A physical failure acknowledges only the banks preceding that failure. */
    wf_r18_hud_reset(&video);calls=0;banks=0;fail_first=WF_SATURN_CRAM_TEXT_FIRST+32u;
    CHECK(wf_r18_hud_commit(&video)==WF_BLOCKED&&calls==3&&banks==7u);
    CHECK(wf_r18_video_palette_pending(&video,WF_R18_PALETTE_TEXT)==0xfffcu);
    fail_first=0xffffu;calls=0;banks=0;
    CHECK(wf_r18_hud_commit(&video)==WF_OK&&calls==14&&banks==0xfffcu);
    CHECK(!wf_r18_video_palette_pending(&video,WF_R18_PALETTE_TEXT));
    wf_saturn_cram_reset();wf_r18_hud_reset(&video);calls=0;banks=0;
    CHECK(wf_r18_hud_commit(&video)==WF_OK&&calls==16&&wf_saturn_cram_writes==256u);
    for(unsigned role=0;role<4;role++)wf_r18_video_palette_ack(&video,(wf_r18_palette_role_t)role,0xffffu);
    CHECK(wf_r18_hud_bind(&video,read_font)==WF_OK&&font_reads==64);
    for(unsigned role=0;role<4;role++)CHECK(wf_r18_video_palette_pending(&video,(wf_r18_palette_role_t)role)==0xffffu);
    calls=0;banks=0;
    CHECK(wf_r18_hud_commit(&video)==WF_OK&&calls==16&&wf_saturn_cram_writes==256u);
    CHECK(wf_r18_hud_commit(&video)==WF_OK&&calls==16&&wf_saturn_cram_writes==256u);
    CHECK(mock_cram[WF_SATURN_CRAM_TEXT_FIRST+14u*16u+14u]==0xffffu);
    {
        static const uint16_t first[4]={0x200u,0x100u,0x000u,0x300u};
        static const uint32_t base[4]={0x180000u,0x182000u,0x188000u,0x186000u};
        uint32_t before[1024];
        CHECK(wf_r18_palettes_commit(NULL,0)==WF_BAD_STATE);
        calls=0;
        CHECK(wf_r18_palettes_commit(&windows,4)==WF_BAD_STATE);
        CHECK(wf_r18_palettes_commit(&windows,~0u)==WF_BAD_STATE&&!calls);
        for(unsigned role=0;role<4;role++){
            memcpy(before,mock_cram,sizeof before);calls=0;
            CHECK(wf_r18_palettes_commit(&windows,role)==WF_OK&&calls==16);
            for(unsigned entry=0;entry<1024;entry++){
                if(entry>=first[role]&&entry<first[role]+256u)CHECK(mock_cram[entry]==0);
                else CHECK(mock_cram[entry]==before[entry]);
            }
            calls=0;CHECK(wf_r18_palettes_commit(&windows,role)==WF_OK&&!calls);
            wf_r17_fg0_write_t write={base[role]+15u*128u+31u,0x1fu+role*16u,1};
            CHECK(wf_r18_video_batch(NULL,&write,1,&windows)==WF_OK);
            CHECK(wf_r18_palettes_commit(&windows,role)==WF_OK&&calls==1);
            CHECK(mock_cram[first[role]+255u]==(0xffu|((role+1u)*0x11u<<8)));
            calls=0;CHECK(wf_r18_palettes_commit(&windows,role)==WF_OK&&!calls);
        }
    }
    puts("PASS palette work:16 added bytes, first zero/all windows, dirty-only stride, atomic buses, allocator, reset/rebind, flash RGB and real CRAM stores");
    return 0;
}
