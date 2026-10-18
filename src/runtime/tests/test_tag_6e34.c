#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "wf_r17_tag_6e34.h"
static int probe_reentry;static wf_r17_participant_io_t reentry_io;
static uint16_t hw[4];static size_t fg_count,video_count;static wf_r17_fg0_write_t first_fg;
static wf_status_t input(wf_runtime_t*r,uint32_t a,uint16_t*v,void*u){(void)u;if(probe_reentry){probe_reentry=0;assert(wf_r17_owner_6e34(r,&reentry_io)==WF_BAD_STATE);}assert(a>=0x140020&&a<=0x140026);*v=hw[(a-0x140020)/2];return WF_OK;}
static wf_status_t commit(wf_runtime_t*r,const wf_r17_fg0_write_t*f,size_t nf,const wf_r17_fg0_write_t*v,size_t nv,void*u){(void)r;(void)v;(void)u;if(nf)first_fg=f[0];fg_count+=nf;video_count+=nv;return WF_OK;}
static void setup(wf_runtime_t*r,wf_r17_participant_io_t*io){unsigned i;memset(r,0,sizeof *r);memset(io,0,sizeof *io);io->input_word=input;io->commit_batch=commit;r->g.credited_route=1;r->g.config_0066=4;r->g.credit_count_004f=1;for(i=0;i<4;i++)hw[i]=0xffff;hw[0]=0xff7f;fg_count=video_count=0;}
int main(void){wf_runtime_t r,before;wf_r17_participant_io_t io;setup(&r,&io);r.actors[0].b[0]=0x80;assert(wf_r17_owner_6e34(&r,&io)==WF_OK);assert(r.g.credit_count_004f==0);assert(r.actors[0].b[0xa1]==1);assert(r.actors[0].b[0x33]==2);assert(wf_be32(&r.actors[0],0x8a)==0x140020);assert(wf_be16(&r.actors[0],0x66)==110);assert(wf_be16(&r.actors[0],0x6e)==110);assert(r.m68k_regs.d[0]==2);assert(fg_count==0);
 setup(&r,&io);r.actors[0].b[0]=128;r.g.credit_count_004f=0;assert(wf_r17_owner_6e34(&r,&io)==WF_OK);assert(!r.actors[0].b[0xa1]);assert(r.m68k_regs.d[4]==65535);
 setup(&r,&io);r.actors[0].b[0]=128;r.actors[0].b[0x33]=1;wf_set_be32(&r.actors[0],0x86,wf_actor_rom_base(1));r.actors[1].b[0x56]=0x40;assert(wf_r17_owner_6e34(&r,&io)==WF_OK);assert(r.actors[0].b[0xa1]==1);assert(r.actors[1].b[0xa6]==1);assert(r.actors[1].b[0x56]==0);assert(wf_be16(&r.actors[1],0x6e)==110);
 setup(&r,&io);r.actors[4].b[0]=r.actors[5].b[0]=128;r.actors[4].b[0x33]=1;r.actors[4].b[0x57]=2;r.actors[5].b[0x57]=3;wf_set_be32(&r.actors[4],0x26,wf_actor_rom_base(6));wf_set_be32(&r.actors[6],0x9c,wf_actor_rom_base(4));r.g.global_1684=wf_actor_rom_base(5);assert(wf_r17_owner_6e34(&r,&io)==WF_OK);assert(r.actors[4].b[0]==0&&r.actors[5].b[0]==0);assert(r.actors[0].b[0x57]==2&&r.actors[1].b[0x57]==3);assert(wf_be32(&r.actors[6],0x9c)==wf_actor_rom_base(0));assert(r.g.global_1684==wf_actor_rom_base(1));assert(r.actors[1].b[0x56]==64);assert(fg_count>0&&video_count==64);
 setup(&r,&io);r.actors[4].b[0]=r.actors[5].b[0]=128;r.actors[4].b[0x33]=1;io.commit_batch=0;before=r;assert(wf_r17_owner_6e34(&r,&io)==WF_BLOCKED);assert(memcmp(&before,&r,sizeof r)==0);
 setup(&r,&io);r.g.config_0066|=0x800;r.actors[2].b[0]=128;assert(wf_r17_owner_6e34(&r,&io)==WF_OK);assert(r.g.credit_count_004f==0);assert(r.g.global_1690==0x2040);assert(first_fg.addr==0xc1770&&first_fg.value==0x93001e);assert(fg_count==60);hw[0]=0xffff;assert(wf_r17_owner_6e34(&r,&io)==WF_OK);assert(r.g.credit_count_004f==0&&r.g.global_1698==1);hw[0]=0xffef;assert(wf_r17_owner_6e34(&r,&io)==WF_OK);assert(r.g.global_1690==0&&r.g.global_1698==0);assert(r.g.mode_0160==64);
 setup(&r,&io);r.actors[0].b[0]=128;memset(&reentry_io,0,sizeof reentry_io);probe_reentry=1;assert(wf_r17_owner_6e34(&r,&io)==WF_OK);assert(r.g.credit_count_004f==0&&wf_be16(&r.actors[0],0x66)==110);
 puts("TAG6E34 native regression: PASS");return 0;}
