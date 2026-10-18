#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wf_r17_tag_6e34.h"
static FILE *trace;
static unsigned char mem[0x200000];
static unsigned rd(unsigned a,unsigned n){unsigned v=0;while(n--)v=(v<<8)|mem[a++];return v;}
static void wr(unsigned a,unsigned v,unsigned n){while(n){--n;mem[a+n]=(unsigned char)v;v>>=8;}}
static wf_status_t input(wf_runtime_t*r,uint32_t a,uint16_t*v,void*u){(void)r;(void)u;*v=(uint16_t)rd(a,2);return WF_OK;}
static wf_status_t commit(wf_runtime_t*r,const wf_r17_fg0_write_t*f,size_t nf,const wf_r17_fg0_write_t*v,size_t nv,void*u){size_t i;(void)r;(void)u;for(i=0;i<nf;i++){wr(f[i].addr,f[i].value,f[i].width);fprintf(trace,"%x %x %u\n",f[i].addr,f[i].value,f[i].width);}for(i=0;i<nv;i++){wr(v[i].addr,v[i].value,v[i].width);fprintf(trace,"%x %x %u\n",v[i].addr,v[i].value,v[i].width);}return WF_OK;}
int main(int argc,char**argv){static wf_runtime_t r;wf_r17_participant_io_t io;FILE*f;unsigned i;int st;if(argc!=4)return 2;f=fopen(argv[1],"rb");if(!f||fread(mem,1,sizeof mem,f)!=sizeof mem)return 3;fclose(f);
#define G(field,a,n) r.g.field=rd(0x1c0000+(a),n)
 G(credit_cycle_units_004b,0x4b,1);G(credit_remainder_004e,0x4e,1);G(credit_count_004f,0x4f,1);G(config_0066,0x66,2);G(credited_route,0x7c,2);G(mode_0160,0x160,1);G(mode_0161,0x161,1);G(progression_0162,0x162,2);G(global_1690,0x1690,2);G(global_1698,0x1698,2);G(global_1684,0x1684,4);G(global_1688,0x1688,4);for(i=0;i<4;i++){r.g.global_0156[i]=rd(0x1c0156+2*i,2);r.g.cabinet_tag_15e0[i]=mem[0x1c15e0+i];}memcpy(r.actors,mem+0x1c05b0,sizeof r.actors);for(i=0;i<8;i++)r.m68k_regs.d[i]=rd(0x1f0000+4*i,4);r.m68k_regs.valid=1;memset(&io,0,sizeof io);io.input_word=input;io.commit_batch=commit;
 trace=fopen(argv[3],"w");if(!trace)return 5;
 st=wf_r17_owner_6e34(&r,&io);if(st){fprintf(stderr,"status=%d dependency=%x\n",st,io.blocked_dependency_rom);return 4;}
#define O(field,a,n) wr(0x1c0000+(a),r.g.field,n)
 O(credit_remainder_004e,0x4e,1);O(credit_count_004f,0x4f,1);O(mode_0160,0x160,1);O(global_1690,0x1690,2);O(global_1698,0x1698,2);O(global_1684,0x1684,4);O(global_1688,0x1688,4);for(i=0;i<4;i++){wr(0x1c0156+2*i,r.g.global_0156[i],2);mem[0x1c15e0+i]=r.g.cabinet_tag_15e0[i];}memcpy(mem+0x1c05b0,r.actors,sizeof r.actors);for(i=0;i<8;i++)wr(0x1f0000+4*i,r.m68k_regs.d[i],4);f=fopen(argv[2],"wb");fwrite(mem,1,sizeof mem,f);fclose(f);fclose(trace);return 0;}
