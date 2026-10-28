#include "wf_r17_tag_6e34.h"
#include "wf_r17_participant_helpers.h"
#include "wf_payment.h"
#include <string.h>
#include "wf_r17_tag_tables.inc"
#define R(o,w) wf_participant_read(m,m->a[0]+(o),(w))
#define W(o,v,w) wf_participant_write(m,m->a[0]+(o),(v),(w))
static void dw(wf_participant_machine_t*m,unsigned r,uint16_t v){m->d[r]=(m->d[r]&0xffff0000u)|v;}
static void db(wf_participant_machine_t*m,unsigned r,uint8_t v){m->d[r]=(m->d[r]&0xffffff00u)|v;}
static int pay(wf_participant_machine_t*m){m->d[0]=5;dw(m,0,2);return wf_payment_consume_055a(m->rt,5);}
static void message(wf_participant_machine_t*m,unsigned k,int erase){
 const uint16_t *p=k==0?tag_message0:k==1?tag_message1:tag_message2;
 const size_t lengths[]={sizeof tag_message0/sizeof tag_message0[0],sizeof tag_message1/sizeof tag_message1[0],sizeof tag_message2/sizeof tag_message2[0]};
 size_t n=lengths[k],i;
 for(i=0;i<n;i+=3){unsigned j;for(j=0;j<=p[i+2];j++)wf_participant_write(m,0xc1770u+p[i]+4*j,erase?0:((uint32_t)(p[i+1]+j)<<16)|0x1eu,4);}
}
static void palettes(wf_participant_machine_t*m){unsigned i,j;if(!m->rt->g.credited_route||(m->rt->g.mode_0161&1))return;
 for(i=0;i<4;i++){wf_actor_raw_t*a=&m->rt->actors[i];unsigned c=wf_be16(a,0x56)&15u;if(!(a->b[0]&128))continue;if(c>=12){m->status=WF_BAD_STATE;return;}for(j=16;j-->0;)wf_participant_write(m,0x180100u+c*128+2*j,tag_palettes[c+((a->b[0x33]&1)?0:12)][j],2);}
}
wf_status_t wf_r17_tag_palettes_c1ea(wf_runtime_t *rt,wf_r17_fg0_batch_fn batch,void *user){
 wf_r17_fg0_write_t writes[64];size_t n=0;
 if(!rt->g.credited_route||(rt->g.mode_0161&1u))return WF_OK;
 for(unsigned i=0;i<4u;i++){
  wf_actor_raw_t*a=&rt->actors[i];if(!(wf_u8(a,0u)&128u))continue;
  unsigned c=wf_be16(a,0x56u)&15u;if(c>=12u)return WF_BAD_STATE;
  for(unsigned j=16;j-->0;)writes[n++]=(wf_r17_fg0_write_t){0x180100u+c*128u+j*2u,tag_palettes[c+((wf_u8(a,0x33u)&1u)?0u:12u)][j],2u};
 }
 if(!n)return WF_OK;
 return batch?batch(rt,writes,n,user):WF_BLOCKED;
}
static void refill(wf_participant_machine_t*m){
 W(0xa1,R(0xa1,1)|1,1);
 if(R(0x33,1)&1){W(0x33,R(0x33,1)|2,1);m->a[2]=R(0x86,4);wf_participant_write(m,m->a[2]+0x8a,m->a[1],4);wf_participant_write(m,m->a[2]+0x33,wf_participant_read(m,m->a[2]+0x33,1)|2,1);wf_participant_write(m,m->a[2]+0x56,wf_participant_read(m,m->a[2]+0x56,1)&~64u,1);m->a[0]=m->a[2];}
 else{if(R(0x33,1)&2){m->a[2]=R(0x86,4);wf_participant_write(m,m->a[2]+0x33,wf_participant_read(m,m->a[2]+0x33,1)|2,1);}W(0x8a,m->a[1],4);W(0x33,R(0x33,1)|2,1);W(0x56,R(0x56,1)&~64u,1);}
 W(0xa6,1,1);wf_participant_1079e(m);W(0x6e,R(0x66,2),2);W(0x35,R(0x35,1)|16,1);
}
static int repair(wf_participant_machine_t*m,uint32_t link,uint32_t old,uint32_t dest){
 if(!link)return 0;
 m->a[5]=link;
 if(wf_participant_read(m,link+0x26,4)==old){wf_participant_write(m,link+0x26,dest,4);return 1;}
 if(wf_participant_read(m,link+0x9c,4)==old){wf_participant_write(m,link+0x9c,dest,4);return 2;}return 0;
}
static void relocate(wf_participant_machine_t*m){unsigned i;uint32_t saved[3];
 m->a[3]=0x1c09e0;m->a[4]=0x1c0aec;
 if(!(wf_participant_read(m,m->a[3]+0x33,1)&1)){uint32_t t=m->a[3];m->a[3]=m->a[4];m->a[4]=t;}
 m->a[2]=m->d[4]==2?0x1c05b0:m->d[4]>=3?0x1c06bc:m->d[4]==1?0x1c08d4:0x1c07c8;
 m->a[5]=m->a[0];m->a[6]=m->a[2];m->d[5]=m->a[3];m->d[6]=m->a[4];m->d[1]=0x85;
 for(i=0;i<0x86;i++){wf_participant_write(m,m->a[5],wf_participant_read(m,m->a[3],2),2);m->a[5]+=2;wf_participant_write(m,m->a[6],wf_participant_read(m,m->a[4],2),2);m->a[6]+=2;wf_participant_write(m,m->a[3],0,2);m->a[3]+=2;wf_participant_write(m,m->a[4],0,2);m->a[4]+=2;}dw(m,1,0xffff);m->a[3]=m->d[5];m->a[4]=m->d[6];
 W(0x56,0,1);W(0x33,R(0x33,1)|2,1);W(0x8a,m->a[1],4);W(0xaa,0x4000,2);W(0xa6,1,1);W(0xa1,R(0xa1,1)|1,1);W(0x86,m->a[2],4);
 repair(m,R(0x26,4),m->a[3],m->a[0]);repair(m,R(0x9c,4),m->a[3],m->a[0]);W(0x35,R(0x35,1)|16,1);wf_participant_1079e(m);wf_participant_782c(m);palettes(m);W(0x6e,R(0x66,2),2);
 memcpy(saved,m->a,sizeof saved);wf_participant_7720(m);wf_participant_75ee(m);memcpy(m->a,saved,sizeof saved);
 wf_participant_write(m,m->a[2]+0x56,64,1);wf_participant_write(m,m->a[2]+0xaa,0x4000,2);wf_participant_write(m,m->a[2]+0x8a,m->a[1],4);wf_participant_write(m,m->a[2]+0x86,m->a[0],4);
 if(repair(m,wf_participant_read(m,m->a[2]+0x26,4),m->a[4],m->a[2])!=2)repair(m,wf_participant_read(m,m->a[2]+0x9c,4),m->a[4],m->a[2]);
 if(m->rt->g.global_1684==m->a[4])m->rt->g.global_1684=m->a[2];else m->rt->g.global_1688=m->a[2];
 m->a[0]=m->a[2];W(0x35,R(0x35,1)|16,1);wf_participant_1079e(m);wf_participant_782c(m);palettes(m);W(0x6e,R(0x66,2),2);wf_participant_7720(m);wf_participant_75ee(m);
}
static int reserve_busy(wf_participant_machine_t*m){m->a[3]=0x1c09e0;return wf_participant_read(m,m->a[3]+0xfe,2)||wf_participant_read(m,m->a[3]+0xa0,1);}
static void selected_refill(wf_participant_machine_t*m){m->rt->g.mode_0160=(m->rt->g.mode_0160|128)&~64;m->rt->g.global_1690=0;m->a[0]=(m->rt->actors[0].b[0]&128)?0x1c06bc:0x1c08d4;refill(m);}
static void selection_input(wf_participant_machine_t*m){uint8_t*e=m->rt->g.cabinet_tag_15e0;uint16_t v;m->a[3]=((uint32_t)m->rt->g.global_0156[0]<<16)|m->rt->g.global_0156[1];v=(uint16_t)wf_participant_read(m,m->a[3],2);dw(m,3,(~v)&255);e[3]=(uint8_t)m->d[3]&15;db(m,4,e[2]);if((uint8_t)m->d[4]==e[3])e[3]=0;else e[2]=e[3];db(m,3,((uint8_t)m->d[3]>>4)&3);e[1]=((uint8_t)~e[0])&(uint8_t)m->d[3];e[0]=(uint8_t)m->d[3];}
static void choose(wf_participant_machine_t*m){static const uint8_t left[16]={0,0,0,0,0,0,0,0,1,1,1,0,0,0,0,1};static const uint8_t right[16]={0,0,0,0,1,1,1,0,0,0,0,1,2,3,4,5};int first;
 m->d[0]=m->rt->g.cabinet_tag_15e0[3];first=(m->rt->g.global_1690&64)?!left[m->d[0]&15]:!!right[m->d[0]&15];
 m->d[0]=first?2:1;message(m,m->d[0],1);m->d[0]=first?1:2;message(m,m->d[0],0);if(first)m->rt->g.global_1690=(m->rt->g.global_1690|64)&~128;else m->rt->g.global_1690=(m->rt->g.global_1690|128)&~64;
}
static void run(wf_participant_machine_t*m){wf_runtime_t*r=m->rt;unsigned i;if(!r->g.credited_route)return;dw(m,0,r->g.config_0066&0x800);
 if(!(m->d[0]&0x800)){m->d[4]=3;m->a[0]=0x1c05b0;m->a[1]=0x140020;for(i=0;i<4;i++){
 if(!(wf_participant_read(m,m->a[1],2)&128)){if(R(0,1)&128){if(!R(0xfe,2)&&!R(0xa0,1)&&!(R(0xa1,1)&1)&&pay(m)){refill(m);return;}}
 else if(!reserve_busy(m)&&wf_participant_read(m,m->a[3]+0x57,1)!=4&&pay(m)){relocate(m);return;}}
 m->a[0]+=0x10c;m->a[1]+=2;dw(m,4,(uint16_t)(m->d[4]-1));}return;}
 if(r->g.mode_0160&0xc0)return;
 if(!(r->g.global_1690&0x2000)){dw(m,0,0);for(i=0;i<9;i++){m->a[0]=wf_actor_rom_base(i);dw(m,0,i+1);if(R(0,1)&128)break;}if(i==9||R(0xfe,2))return;
 r->g.global_0156[0]=0x14;r->g.global_0156[1]=(r->actors[0].b[0]&128)?0x22:0x20;m->a[2]=0x140000u+r->g.global_0156[1];dw(m,0,wf_participant_read(m,m->a[2],2)&128);if((m->d[0]&65535)||!pay(m))return;
 if(((r->g.progression_0162&255)==4||(r->g.progression_0162&255)==9)&&!reserve_busy(m)){m->a[1]=m->a[2];selected_refill(m);return;}
 r->g.global_1690|=0x2000;m->d[0]=0;message(m,0,0);m->d[0]=1;message(m,1,0);r->g.global_1690|=64;return;}
 if(r->g.global_1690&0x4000)return;
 r->g.global_1698++;
 if(r->g.global_1698<0x200){selection_input(m);if(!r->g.cabinet_tag_15e0[1]){choose(m);return;}}
 r->g.global_1698=0;if(reserve_busy(m)){choose(m);return;}
 for(i=0;i<3;i++){m->d[0]=i;message(m,i,1);}m->a[1]=((uint32_t)r->g.global_0156[0]<<16)|r->g.global_0156[1];
 if(r->g.global_1690&64){m->d[4]=3;m->a[0]=0x1c05b0;if(r->actors[0].b[0]&128){m->a[0]=0x1c07c8;m->d[4]=1;}r->g.mode_0160=(r->g.mode_0160|64)&~128;r->g.global_1690=0;relocate(m);}else selected_refill(m);
}
/* The participant workspace already owns all write records. Rotate its two
 * recursively partitioned halves in place, preserving order within each
 * destination without a second 48KiB buffer in scarce SH-2 HWRAM. */
static void tag_reverse(wf_r17_fg0_write_t*w,size_t first,size_t last){
 while(first<last){wf_r17_fg0_write_t tmp=w[first];w[first++]=w[--last];w[last]=tmp;}
}
static size_t tag_partition(wf_r17_fg0_write_t*w,size_t first,size_t last){
 if(last-first<2u)return first<last&&w[first].addr<0x180000u?1u:0u;
 size_t middle=first+(last-first)/2u;
 size_t left=tag_partition(w,first,middle),right=tag_partition(w,middle,last);
 if(left<middle-first&&right){
  tag_reverse(w,first+left,middle);
  tag_reverse(w,middle,middle+right);
  tag_reverse(w,first+left,middle+right);
 }
 return left+right;
}
static void tag_stage_outputs(wf_participant_machine_t*m,size_t*nf,size_t*nv){
 *nf=tag_partition(m->writes,0u,m->write_count);
 *nv=m->write_count-*nf;
}
/* The shared lease retains machine/snapshot storage through output callbacks. */
wf_status_t wf_r17_owner_6e34(wf_runtime_t *rt,wf_r17_participant_io_t *io){
 wf_r17_fg0_write_t*fg,*vid;
 wf_participant_workspace_t*workspace;
 wf_status_t status;
 size_t nf=0,nv=0;
 if(!rt||!io)return WF_BAD_STATE;
 status=wf_participant_workspace_acquire(WF_PARTICIPANT_TAG,&workspace);if(status!=WF_OK)return status;
 wf_runtime_t*saved=&workspace->snapshot;wf_participant_machine_t*m=&workspace->machine;
 fg=m->writes;
 *saved=*rt;
 wf_participant_init(m,rt,io->input_word,io->user);
 run(m);
 status=m->status;
 if(status==WF_OK){
  tag_stage_outputs(m,&nf,&nv);vid=m->writes+nf;
  if(!io->commit_batch&&((nf&&nv)||(nf&&!io->fg0_batch)||(nv&&!io->video_batch))){
   status=WF_BLOCKED;
   m->failed_address=nf&&!io->fg0_batch?0x75ee:0xc1ea;
  }else if(io->commit_batch&&(nf||nv)){
   status=io->commit_batch(rt,fg,nf,vid,nv,io->user);
  }else if(nf){
   status=io->fg0_batch(rt,fg,nf,io->user);
  }else if(nv){
   status=io->video_batch(rt,vid,nv,io->user);
  }
 }
 if(status!=WF_OK){
  *rt=*saved;
  io->blocked_dependency_rom=m->failed_address?m->failed_address:0x6e34;
 }else{
  rt->m68k_regs=saved->m68k_regs;
    memcpy(rt->m68k_regs.d,m->d,sizeof m->d);
  io->blocked_dependency_rom=0;
 }
 (void)wf_participant_workspace_release(WF_PARTICIPANT_TAG,workspace);
 return status;
}
