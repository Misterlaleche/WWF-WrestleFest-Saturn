/* Regression for the $99D0 Demolition logo's byte writes to FG0.
 *
 * Unlike $9ACA's ordinary logos, $99D0 writes tile-low at an even CPU
 * address. The board's 8-bit FG0 latches either byte lane into the same
 * cell (mame_ddragon3_v.cpp, wwfwfest_fg0_videoram_w, in the supplied
 * wp1979/audit_rom reference). The host RAM image consumed by the Jo HUD
 * stores that effective value at the odd byte of each 68000 word.
 *
 * Exercise the real TAG join, strip, palette and HUD map consumers. The
 * expected tile IDs come directly from the original $9D36 38x8 table;
 * the RGB golden was independently decoded from original ROM01's
 * 31e12-0.ic33 and palette $9F96, without the production renderer.
 */
#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wf_r18_hud.h"
#include "wf_r18_match_support.h"

static uint8_t rom[0x80000],font[131072],raw[0x4000];
static wf_runtime_t runtime;
static wf_r18_video_t video;
static wf_r17_m68k_shadow_t regs;
static wf_r18_prematch_t machine;
static uint16_t names[4096];
static unsigned strip_calls;
static const char *dump_directory;

static uint16_t read16(const uint8_t *p)
{return (uint16_t)(((unsigned)p[0]<<8)|p[1]);}
static void load(const char *path,uint8_t *out,size_t bytes)
{
    FILE *file=fopen(path,"rb");assert(file);
    assert(fread(out,1,bytes,file)==bytes);assert(fgetc(file)==EOF);assert(!fclose(file));
}
static wf_status_t read_rom(void *user,uint32_t at,void *out,size_t bytes)
{
    (void)user;
    if(at>sizeof rom||bytes>sizeof rom-at)return WF_BLOCKED;
    memcpy(out,rom+at,bytes);return WF_OK;
}
static wf_status_t support_read(void *user,uint32_t at,uint8_t *out,size_t bytes)
{return read_rom(user,at,out,bytes);}
static wf_status_t palette_write(void *user,uint32_t at,uint16_t word)
{
    wf_r17_fg0_write_t write={at,word,2};
    return wf_r18_video_batch(&runtime,&write,1,user);
}
static wf_status_t original_strip(void *user,uint32_t owner,wf_r18_prematch_t *m)
{
    assert(owner==0x9acau);++strip_calls;
    return wf_r18_strip_9aca(&runtime,(uint16_t)m->regs->d[2],
        (uint16_t)m->regs->d[7],m->a[2],user);
}
static void setup(unsigned identity,unsigned side,unsigned slot)
{
    memset(raw,0,sizeof raw);memset(&video,0,sizeof video);
    memset(&regs,0,sizeof regs);memset(&machine,0,sizeof machine);
    memset(names,0xa5,sizeof names);strip_calls=0;
    regs.valid=1;video.read_rom=read_rom;
    machine.ram=raw;machine.regs=&regs;machine.effect=original_strip;machine.user=&video;
    uint8_t *actor=raw+0x5b0u+slot*0x10cu;
    actor[0]=0x80u;actor[0x33]=side?0x80u:0u;actor[0x57]=(uint8_t)identity;
    memset(video.fg0+0x2000,0xa5,0x2000);wf_r18_hud_reset(&video);
}
static void join(void)
{
    wf_r18_match_support_t support={.machine=&machine,.fg_0c0000=video.fg0,
        .fg_bytes=sizeof video.fg0,.read_rom=support_read,
        .write_palette=palette_write,.user=&video};
    assert(wf_r18_match_support_call(&support,0x98bau)==WF_OK);
    size_t writes=0;
    assert(wf_r18_hud_map_update(&video,names,&writes)==WF_OK&&writes==4096u);
    for(unsigned i=0x2000u;i<sizeof video.fg0;i++)assert(video.fg0[i]==0xa5u);
}
static uint64_t logo_rgb(unsigned row)
{
    uint64_t hash=UINT64_C(14695981039346656037);
    for(unsigned y=0;y<64u;y++)for(unsigned x=0;x<304u;x++){
        uint16_t name=names[(row+y/8u)*64u+1u+x/8u];
        unsigned tile=name&0xfffu,bank=name>>12;
        uint8_t packed=font[tile*32u+(y&7u)*4u+(x&7u)/2u];
        unsigned pen=x&1u?packed&15u:packed>>4;
        uint16_t color=read16(video.palettes+bank*128u+pen*2u);
        for(unsigned channel=0;channel<3u;channel++){
            hash^=((color>>(channel*4u))&15u)*17u;
            hash*=UINT64_C(1099511628211);
        }
    }
    return hash;
}
static void dump_hud(unsigned identity,unsigned side,unsigned slot)
{
    if(!dump_directory)return;
    char path[1024];
    int length=snprintf(path,sizeof path,"%s/demolition-%u-%u-%u.ppm",
        dump_directory,identity,side,slot);
    assert(length>0&&(size_t)length<sizeof path);
    FILE *file=fopen(path,"wb");assert(file);fputs("P6\n320 240\n255\n",file);
    for(unsigned y=8u;y<248u;y++)for(unsigned x=0;x<320u;x++){
        uint16_t name=names[(y/8u)*64u+x/8u];
        unsigned tile=name&0xfffu,bank=name>>12;
        uint8_t packed=font[tile*32u+(y&7u)*4u+(x&7u)/2u];
        unsigned pen=x&1u?packed&15u:packed>>4;
        uint16_t color=read16(video.palettes+bank*128u+pen*2u);
        for(unsigned channel=0;channel<3u;channel++)
            assert(fputc((int)(((color>>(channel*4u))&15u)*17u),file)!=EOF);
    }
    assert(!fclose(file));
}
static int demolition(unsigned identity,unsigned side,unsigned slot)
{
    setup(identity,side,slot);join();assert(strip_calls==1u); /* final VS only */
    unsigned row=side?21u:5u,bank=(side?14u:6u)+slot;
    for(unsigned y=0;y<8u;y++)for(unsigned x=0;x<38u;x++){
        unsigned cell=(row+y)*64u+1u+x;
        uint16_t want=(uint16_t)((bank<<12)|0x900u|rom[0x9d36u+y*38u+x]);
        if(names[cell]!=want||names[cell+2048u]!=want){
            fprintf(stderr,"Demolition id=%u side=%u slot=%u cell=(%u,%u): "
                "expected tile/palette %04x, got %04x/%04x\n",identity,side,slot,
                x,y,want,names[cell],names[cell+2048u]);return 0;
        }
    }
    for(unsigned color=0;color<16u;color++)
        assert(read16(video.palettes+bank*128u+color*2u)==read16(rom+0x9f96u+color*2u));
    uint64_t hash=logo_rgb(row);
    if(hash!=UINT64_C(0x25455a40232d1c87)){
        fprintf(stderr,"Demolition original 304x64 RGB hash: expected 25455a40232d1c87, got %016" PRIx64 "\n",hash);
        return 0;
    }
    dump_hud(identity,side,slot);
    return 1;
}
int main(int argc,char **argv)
{
    assert(argc==3||argc==4);load(argv[1],rom,sizeof rom);load(argv[2],font,sizeof font);
    if(argc==4)dump_directory=argv[3];
    for(unsigned identity=10u;identity<=11u;identity++)
        for(unsigned side=0;side<2u;side++)for(unsigned slot=0;slot<2u;slot++)
            if(!demolition(identity,side,slot))return 1;
    /* Ordinary logo strips keep their original path and the final VS. */
    for(unsigned identity=0;identity<10u;identity++){
        setup(identity,0,0);join();assert(strip_calls==2u);
    }
    puts("Demolition logo: 8 TAG placements, 304 exact tiles, original palette/RGB, 10 ordinary logo routes: PASS");
    return 0;
}
