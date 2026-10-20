#include "wf_r18_bootstrap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static wf_status_t pal(wf_runtime_t*r,uint16_t ch,void*u){(void)r;(void)ch;(void)u;return WF_OK;}
static wf_status_t simple(wf_runtime_t*r,void*u){(void)r;(void)u;return WF_OK;}
static wf_status_t owner(wf_runtime_t*r,uint32_t pc,void*u){(void)r;(void)pc;(void)u;return WF_OK;}
static uint32_t writes[16384][3];static uint32_t nw;
static wf_status_t batch(wf_runtime_t*r,const wf_r17_fg0_write_t*w,size_t n,void*u){(void)r;(void)u;for(size_t i=0;i<n;i++){writes[nw][0]=w[i].addr;writes[nw][1]=w[i].value;writes[nw++][2]=w[i].width;}return WF_OK;}
int main(int argc,char**argv){if(argc<3)return 1;wf_runtime_t r;wf_r18_bootstrap_context_t c;memset(&r,0,sizeof r);memset(&c,0,sizeof c);r.g.rng_state_valid=r.m68k_regs.valid=1;r.g.rng_state=(uint32_t)strtoul(argv[1],0,0);r.g.progression_0162=(uint16_t)atoi(argv[2]);r.g.config_0066=0x300;r.g.credited_route=1;r.g.credit_count_004f=argc>4?(uint8_t)atoi(argv[4]):0;r.g.credit_remainder_004e=argc>5?(uint8_t)atoi(argv[5]):0;for(unsigned i=0;i<8;i++)r.m68k_regs.d[i]=0x11110000+i*0x1111;for(unsigned i=0;i<12;i++)c.history_0598[i]=65535;c.history_0598[0]=0;c.history_0598[1]=1;for(unsigned i=0;i<2;i++){wf_set_be16(&r.actors[i],0,0x8000);wf_set_be16(&r.actors[i],0x56,i);wf_set_be16(&r.actors[i],0xa0,1);}
c.palette_2aea=pal;
int direct=argc>3&&!strcmp(argv[3],"direct");
if(direct){wf_r18_launch_config_t cfg={0,1,0x300,r.g.rng_state,r.m68k_regs};c.text_owner=owner;c.video_batch=batch;c.fg0_batch=batch;c.palette_release_2b58=pal;c.palette_reset=simple;c.map_26e66=simple;if(wf_r18_direct_boot(&r,&c,&cfg)!=WF_OK){fprintf(stderr,"direct blocked%x\n",c.blocked_pc);return 4;}}
else if(wf_r18_tag_setup_1086c(&r,&c)!=WF_OK)return 2;
if(argc>3&&!direct){c.text_owner=owner;c.video_batch=batch;c.fg0_batch=batch;if(wf_r18_live_finalize_8ae6(&r,&c)!=WF_OK){fprintf(stderr,"blocked %x\n",c.blocked_pc);return 3;}}
fwrite(r.actors,1,sizeof r.actors,stdout);fwrite(r.referee.b,1,0x64,stdout);fwrite(r.m68k_regs.d,4,8,stdout);fwrite(&r.g.rng_state,4,1,stdout);fwrite(c.history_0598,2,12,stdout);fwrite(c.roster_0380,2,12,stdout);fwrite(&r.g.global_0063,1,1,stdout);fwrite(&r.g.global_0072,2,1,stdout);fwrite(&r.g.global_169e,2,1,stdout);if(direct){fwrite(r.fixed_object0.b,1,0x10c,stdout);fwrite(r.fixed_object1.b,1,0x10c,stdout);fwrite(r.fixed_four_1134.b,1,0x30,stdout);fwrite(r.referee_aux.b,1,0x30,stdout);fwrite(r.fixed_four_1194.b,1,0x30,stdout);fwrite(r.fixed_four_11c4.b,1,0x30,stdout);
uint16_t g[]={r.g.global_16d2,r.g.global_16d4,r.g.global_17d6,r.g.global_17d8,r.g.global_17da,r.g.global_17dc,r.g.global_17de,r.g.global_17e0,r.g.global_17e2,r.g.global_17e4,r.g.global_17e6,r.g.global_17ea,r.g.global_17ee,r.g.global_17f2_word,r.g.global_17f6,r.g.global_17fa,r.g.global_17fe,r.g.global_1804,r.g.global_1806,r.g.global_15fc};fwrite(g,2,20,stdout);}
fwrite(&nw,4,1,stdout);fwrite(writes,12,nw,stdout);return 0;}
