#include "wp01_oki.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static uint16_t regs[0x200];
static unsigned writes, invalid_write;
static void wr(void *u,uint16_t reg,uint16_t value){
    (void)u;
    if(reg<24u*32u||reg>=28u*32u||(reg&1u)){invalid_write++;return;}
    regs[reg/2]=value;writes++;
}
static const wp01_oki_asset_t assets[]={
 {0x24000u,1802u,0u,1u,3u},
 {0x24E20u,32018u,0u,18u,2u},
 {0x35000u,100u,0u,19u,WP01_OKI_RAW_VOLUME}
};
static void start(wp01_oki_t *s,unsigned id,unsigned voice,unsigned vol,uint32_t t){
 wp01_oki_write(s,(uint8_t)(0x80u|id),t);
 wp01_oki_write(s,(uint8_t)((1u<<voice)<<4|vol),t);
}
int main(void){
 wp01_oki_t s;
 wp01_oki_init(&s,assets,3,wr,0);
 assert(writes>0);assert(!invalid_write);
 assert(wp01_oki_status(&s)==0xf0u);
 start(&s,1,0,3,100);
 assert(wp01_oki_status(&s)==0xf1u);
 assert(s.voices[0].duration==9934u);
 assert(regs[(24*32+0)/2]==(0x1800u|2u));
 assert(regs[(24*32+2)/2]==0x4000u);
 assert(regs[(24*32+6)/2]==1802u);
 assert(regs[(24*32+0xC)/2]==0x300u);
 assert(regs[(24*32+0x10)/2]==WP01_OKI_PITCH_8000);
 assert(regs[(24*32+0x16)/2]==0xE000u);
 unsigned old_writes=writes;
 start(&s,18,0,2,101);
 assert(s.stats.ignored_busy==1u&&writes==old_writes);
 assert(s.voices[0].sample_id==1u&&s.voices[0].started_at==100u);
 start(&s,18,2,2,101);
 assert(wp01_oki_status(&s)==0xf5u);
 wp01_oki_advance(&s,10033);
 assert(wp01_oki_status(&s)==0xf5u);
 wp01_oki_advance(&s,10034);
 assert(wp01_oki_status(&s)==0xf4u&&s.stats.natural_ends==1u);
 wp01_oki_write(&s,0x20u,10034); /* Stop original voice 2 only. */
 assert(wp01_oki_status(&s)==0xf0u);
 assert(regs[(26*32+0x16)/2]==0u);
 assert(!(regs[(26*32+0)/2]&0x800u));
 /* Same voice accepts a new start at its exact modelled expiry. */
 start(&s,1,0,3,11000);
 start(&s,1,0,3,20934);
 assert(s.voices[0].started_at==20934u&&s.stats.starts==4u);
 /* Pending sample consumes any second byte, even one with bit 7 set. */
 wp01_oki_write(&s,0x81u,21000);
 wp01_oki_write(&s,0x83u,21000);
 assert(wp01_oki_status(&s)==0xf9u&&s.voices[3].sample_id==1u);
 assert(s.pending_sample==-1);
 /* Volume >= 9 is silent but the original busy flag still becomes set. */
 start(&s,1,1,15,21001);
 assert(wp01_oki_status(&s)==0xfbu);
 assert(regs[(25*32+0x16)/2]==0u);
 wp01_oki_write(&s,0x78u,21002);
 assert(wp01_oki_status(&s)==0xf0u);
 assert(!wp01_oki_set_bank(&s,1));
 assert(s.stats.unsupported_bank==1u&&s.bank==0u);
 start(&s,127,0,0,22000);
 assert(wp01_oki_status(&s)==0xf0u&&s.stats.missing_asset==1u);
 start(&s,1,0,2,22001); /* No matching pre-scaled or raw asset. */
 assert(wp01_oki_status(&s)==0xf0u&&s.stats.missing_asset==2u);
 start(&s,19,0,3,22002); /* Raw fallback uses documented TL quantization. */
 assert(regs[(24*32+0xC)/2]==(0x200u|25u));
 assert(regs[(24*32+8)/2]==0x3fu);
 wp01_oki_reset(&s);
 assert(wp01_oki_status(&s)==0xf0u&&s.pending_sample==-1);
 assert(s.stats.starts==0u&&s.stats.missing_asset==0u);
 /* Invalid sample region would overlap/end outside 512 KiB sound RAM. */
 const wp01_oki_asset_t bad={0x7FFF0u,100u,0u,1u,3u};
 wp01_oki_init(&s,&bad,1,wr,0);
 start(&s,1,0,3,0);
 assert(wp01_oki_status(&s)==0xf0u&&s.stats.invalid_asset==1u);
 assert(!invalid_write);
 static const uint8_t expected[]={0,9,16,25,32,39,48,55,64,255,255,255,255,255,255,255};
 for(unsigned i=0;i<16;i++)assert(wp01_oki_volume_tl(i)==expected[i]);
 puts("PASS: original OKI pending/start/busy/stop/expiry semantics and SCSP slot ownership");
 return 0;
}
