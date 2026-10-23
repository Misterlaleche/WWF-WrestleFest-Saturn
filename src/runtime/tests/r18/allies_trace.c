#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "allies_trace.h"
/* Each sampled fixture is a literal observation, not a synthetic actor.
 * File layout: six host-endian uint32 metadata fields, then pre/post each
 * eight host-endian D registers and 0x2000 bytes of big-endian arcade RAM. */
static FILE *trace;
static const char *out;
static uint8_t before[0x2000];
static uint32_t regs[8],meta[6],keys[4096];
static unsigned key_count,sequence;
static int sampled;
static void w(uint8_t *m,unsigned at,uint32_t v,unsigned n){for(unsigned i=0;i<n;i++)m[at+i]=(uint8_t)(v>>(8u*(n-1u-i)));}
static void snapshot(wf_runtime_t *r,uint8_t m[0x2000]){
 memset(m,0,0x2000);memcpy(m+0x5b0,r->actors,sizeof r->actors);memcpy(m+0x1258,r->transient_1258,sizeof r->transient_1258);
 memcpy(m+0x15e4,r->g.direction_scratch_15e4,8);
#define G(at,field,n) w(m,at,r->g.field,n)
 G(0x5c,rng_state,4);G(0x7c,credited_route,2);G(0x7e,global_007e,2);G(0x80,frame_counter,4);
 G(0x160,mode_0160,1);G(0x161,mode_0161,1);G(0x162,progression_0162,2);G(0x166,global_0166,2);
 G(0x1150,global_1150,2);G(0x1180,global_1180,2);G(0x11b0,global_11b0,2);G(0x11e0,global_11e0,2);
 G(0x15d2,global_15d2,1);G(0x15d3,global_15d3,1);G(0x1670,global_1670,4);G(0x1674,global_1674,4);
 G(0x1678,global_1678,1);G(0x1679,global_1679,1);G(0x167a,global_167a,1);G(0x167b,global_167b,1);
 G(0x1682,global_1682,2);G(0x1684,global_1684,4);G(0x1688,global_1688,4);G(0x168c,global_168c,4);
 G(0x1690,global_1690,2);G(0x1692,global_1692,2);G(0x1694,global_1694,2);G(0x1697,global_1697,1);G(0x1698,global_1698,2);G(0x169a,global_169a,2);
 G(0x169e,global_169e,2);G(0x16a0,global_16a0,2);G(0x16a2,global_16a2,2);G(0x16a4,global_16a4,2);G(0x16c4,global_16c4,2);
 G(0x17e6,global_17e6,2);G(0x17ea,global_17ea,2);G(0x17ee,global_17ee,2);G(0x17f2,global_17f2_word,2);
 G(0x1800,global_1800,2);G(0x1802,global_1802,2);
#undef G
}
static uint32_t hash_key(const wf_actor_raw_t*a,unsigned slot,unsigned kind){
 static const unsigned offsets[]={0x20,0x21,0x60,0x61,0xae,0xaf,0x33,0x34,0xb5,0xb7};
 uint32_t h=2166136261u^(slot*17u+kind);
 for(unsigned i=0;i<sizeof offsets/sizeof offsets[0];i++)h=(h^a->b[offsets[i]])*16777619u;
 return h;
}
void allies_trace_begin(wf_runtime_t*r,unsigned slot,unsigned kind,uint32_t pc){
 if(slot!=1u&&slot!=5u)return;
 if(!out){out=getenv("WF_ALLIES_OUT");if(!out){fputs("WF_ALLIES_OUT missing\n",stderr);exit(2);}char name[4096];snprintf(name,sizeof name,"%s/trace.csv",out);trace=fopen(name,"w");if(!trace)exit(2);fputs("frame,slot,kind,owner,state,ae,selector,pose,x,y,target,team,link,flags33,flags34,b5,b7,stamina,after_state,after_ae,after_selector,after_pose,after_target,status,snapshot\n",trace);}
 uint32_t h=hash_key(&r->actors[slot],slot,kind);unsigned i=0;for(;i<key_count&&keys[i]!=h;i++);
 sampled=i==key_count||r->g.frame_counter%60u==0u;
 if(i==key_count&&key_count<sizeof keys/sizeof keys[0])keys[key_count++]=h;
 snapshot(r,before);memcpy(regs,r->m68k_regs.d,sizeof regs);
 meta[0]=r->g.frame_counter;meta[1]=slot;meta[2]=kind;meta[3]=pc;meta[4]=sequence;meta[5]=0;
}
void allies_trace_end(wf_runtime_t*r,unsigned slot,unsigned kind,uint32_t pc,wf_status_t status){
 if(slot!=1u&&slot!=5u)return;
 wf_actor_raw_t b;memcpy(&b,before+0x5b0+slot*WF_ACTOR_BYTES,sizeof b);wf_actor_raw_t*a=&r->actors[slot];
 fprintf(trace,"%u,%u,%s,%05x,%04x,%04x,%04x,%04x,%u,%u,%08x,%08x,%08x,%02x,%02x,%02x,%02x,%u,%04x,%04x,%04x,%04x,%08x,%u,",meta[0],slot,kind?"dispatch":"cpu",pc,wf_be16(&b,0x20),wf_be16(&b,0xae),wf_be16(&b,0x60),wf_be16(&b,4),wf_be16(&b,6),wf_be16(&b,10),wf_be32(&b,0x7a),wf_be32(&b,0x86),wf_be32(&b,0x26),wf_u8(&b,0x33),wf_u8(&b,0x34),wf_u8(&b,0xb5),wf_u8(&b,0xb7),wf_be16(&b,0x66),wf_be16(a,0x20),wf_be16(a,0xae),wf_be16(a,0x60),wf_be16(a,4),wf_be32(a,0x7a),status);
 if(sampled){
  char name[4096];snprintf(name,sizeof name,"%s/snapshot-%05u.bin",out,sequence);FILE*f=fopen(name,"wb");if(!f)exit(2);uint8_t after[0x2000];snapshot(r,after);meta[5]=status;
  fwrite(meta,sizeof meta,1,f);fwrite(regs,sizeof regs,1,f);fwrite(before,sizeof before,1,f);fwrite(r->m68k_regs.d,sizeof regs,1,f);fwrite(after,sizeof after,1,f);fclose(f);fprintf(trace,"snapshot-%05u.bin",sequence++);
 }
 fputc('\n',trace);
}
