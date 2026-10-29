#include <assert.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wf_r18_present.h"

wf_status_t ref_wf_vdp1_plan_retain_tile_slots(wf_vdp1_frame_plan_t *,const uint16_t *,const uint8_t *,unsigned);
wf_status_t opt_wf_vdp1_plan_retain_tile_slots(wf_vdp1_frame_plan_t *,const uint16_t *,const uint8_t *,unsigned);
wf_status_t ref_evaluate(const wf_r18_sprite_record_t *,size_t,const uint16_t *,const uint8_t *,wf_vdp1_frame_plan_t *,wf_r18_video_t *);
wf_status_t opt_evaluate(const wf_r18_sprite_record_t *,size_t,const uint16_t *,const uint8_t *,wf_vdp1_frame_plan_t *,wf_r18_video_t *);
void ref_poison(void);
void opt_poison(void);

static uint32_t rng=0x1979c0deu;
static uint64_t digest=UINT64_C(14695981039346656037);
static unsigned retain_cases,present_cases,compared_commands,compared_tiles;
static uint32_t random32(void){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return rng;}
static void hash_bytes(const void *p,size_t n){const uint8_t *b=p;while(n--){digest^=*b++;digest*=UINT64_C(1099511628211);}}
static void compare(const char *kind,unsigned frame,wf_status_t a,wf_status_t b,const wf_vdp1_frame_plan_t *pa,const wf_vdp1_frame_plan_t *pb){
 if(a!=b||memcmp(pa,pb,sizeof *pa)){
  fprintf(stderr,"%s mismatch case=%u status=%d/%d\n",kind,frame,a,b);
  for(size_t i=0;i<sizeof *pa;i++)if(((const uint8_t*)pa)[i]!=((const uint8_t*)pb)[i]){fprintf(stderr,"first byte offset=%zu: %02x/%02x\n",i,((const uint8_t*)pa)[i],((const uint8_t*)pb)[i]);break;}
  abort();
 }
 hash_bytes(&a,sizeof a);hash_bytes(pa,sizeof *pa);
}
static void check_identity(const wf_vdp1_frame_plan_t *before,const wf_vdp1_frame_plan_t *after){
 assert(before->tile_count==after->tile_count&&before->cmd_count==after->cmd_count);
 for(unsigned i=0;i<after->tile_count;i++){
  const wf_vdp1_tile_upload_t *u=&after->tile_upload[i];
  assert(u->stream_slot==i&&u->src_byte_offset==(uint32_t)u->source_tile*128u&&u->dst_byte_offset==i*128u);
 }
 for(unsigned i=0;i<after->cmd_count;i++){
  assert(after->cmd[i].stream_tile<after->tile_count);
  assert(before->tile_upload[before->cmd[i].stream_tile].source_tile==after->tile_upload[after->cmd[i].stream_tile].source_tile);
  wf_vdp1_sprite_cmd_t c=after->cmd[i];c.stream_tile=before->cmd[i].stream_tile;
  assert(!memcmp(&c,&before->cmd[i],sizeof c));
 }
 compared_commands+=after->cmd_count;compared_tiles+=after->tile_count;
}
static void retain_test(void){
 static wf_vdp1_frame_plan_t original,a,b;
 uint16_t residents[WF_VDP1_STREAM_TILES_MAX]={0};
 uint8_t bits[WF_VDP1_STREAM_TILES_MAX/8u]={0};
 uint16_t collision_ids[256];unsigned collisions=0;
 for(unsigned id=0;id<65536u&&collisions<256u;id++){
  unsigned h=((uint32_t)id*0x9e3779b1u)>>23;
  if(h==511u||h==0u||h==1u)collision_ids[collisions++]=(uint16_t)id;
 }
 assert(collisions==256u);
 for(unsigned frame=0;frame<8192u;frame++){
  memset(&original,(int)(random32()&255u),sizeof original);
  unsigned n=(frame%19u==0)?256u:(frame%19u==1)?0u:random32()%257u;
  original.tile_count=n;original.cmd_count=n?random32()%513u:0u;
  unsigned base=random32()&65535u,stride=(random32()&65535u)|1u;
  for(unsigned i=0;i<n;i++)original.tile_upload[i].source_tile=(uint16_t)(base+i*stride);
  if(frame%17u==0)for(unsigned i=0;i<n;i++)original.tile_upload[i].source_tile=collision_ids[i];
  if(frame%7u==0)for(unsigned i=0;i<n;i++)original.tile_upload[i].source_tile=(uint16_t)(i*251u+frame/7u);
  for(unsigned i=0;i<original.cmd_count;i++)original.cmd[i].stream_tile=(uint16_t)(random32()%n);
  unsigned resident_count=(frame%5u)?256u:random32()%257u;
  for(unsigned i=0;i<sizeof bits;i++)bits[i]=(uint8_t)random32();
  if(frame%11u==0)memset(bits,255,sizeof bits);
  if(frame%13u==0)memset(bits,0,sizeof bits);
  if(n&&frame%3u==0)for(unsigned i=0;i<256u;i++)residents[i]=original.tile_upload[random32()%n].source_tile;
  if(n>1u&&frame%23u==0)original.tile_upload[n-1u].source_tile=original.tile_upload[0].source_tile;
  if(original.cmd_count&&frame%29u==0)original.cmd[original.cmd_count-1u].stream_tile=(uint16_t)n;
  a=b=original;
  wf_status_t sa=ref_wf_vdp1_plan_retain_tile_slots(&a,residents,bits,resident_count);
  wf_status_t sb=opt_wf_vdp1_plan_retain_tile_slots(&b,residents,bits,resident_count);
  compare("retain",frame,sa,sb,&a,&b);retain_cases++;
  if(sa==WF_OK){check_identity(&original,&a);for(unsigned i=0;i<a.tile_count;i++)residents[i]=a.tile_upload[i].source_tile;}
 }
 const unsigned invalid[3][3]={{257,0,256},{0,513,256},{0,0,257}};
 for(unsigned i=0;i<3;i++){
  memset(&a,0,sizeof a);a.tile_count=invalid[i][0];a.cmd_count=invalid[i][1];b=a;
  wf_status_t sa=ref_wf_vdp1_plan_retain_tile_slots(&a,residents,bits,invalid[i][2]);
  wf_status_t sb=opt_wf_vdp1_plan_retain_tile_slots(&b,residents,bits,invalid[i][2]);
  compare("bounds",i,sa,sb,&a,&b);assert(sa==WF_BAD_STATE);retain_cases++;
 }
 assert(ref_wf_vdp1_plan_retain_tile_slots(NULL,residents,bits,0)==WF_BAD_STATE);
 assert(opt_wf_vdp1_plan_retain_tile_slots(NULL,residents,bits,0)==WF_BAD_STATE);
 memset(&a,0,sizeof a);b=a;
 assert(ref_wf_vdp1_plan_retain_tile_slots(&a,NULL,bits,0)==WF_BAD_STATE);
 assert(opt_wf_vdp1_plan_retain_tile_slots(&b,NULL,bits,0)==WF_BAD_STATE);
 assert(ref_wf_vdp1_plan_retain_tile_slots(&a,residents,NULL,0)==WF_BAD_STATE);
 assert(opt_wf_vdp1_plan_retain_tile_slots(&b,residents,NULL,0)==WF_BAD_STATE);
 retain_cases+=3;
}

/* Exact production submit_records() is included by the generated wrappers.
 * These sinks validate its full counted contract without GPU timing. */
static wf_status_t sink_begin(void *ctx,unsigned nt,unsigned np,unsigned nc){
 wf_saturn_yaul_backend_t *b=ctx;assert(!b->active);
 b->active=1;b->expected_tiles=nt;b->expected_palettes=np;b->expected_cmds=nc;
 b->emitted_tiles=b->emitted_palettes=b->emitted_cmds=0;return WF_OK;
}
static wf_status_t sink_tile(void *ctx,uint16_t slot,const uint8_t *p,size_t n){(void)ctx;(void)slot;(void)p;(void)n;abort();}
wf_status_t wf_saturn_yaul_upload_rom_tile(wf_saturn_yaul_backend_t *b,uint16_t slot,uint16_t source,wf_status_t(*reader)(uint16_t,const uint8_t**)){
 (void)reader;assert(b->active&&slot==b->emitted_tiles&&slot<b->expected_tiles);
 b->rom_source[slot]=source;b->emitted_tiles++;return WF_OK;
}
static wf_status_t sink_palette(void *ctx,uint8_t bank,const uint8_t *p,size_t n){
 wf_saturn_yaul_backend_t *b=ctx;assert(b->active&&bank<16&&p&&n==32&&b->emitted_palettes<b->expected_palettes);b->emitted_palettes++;return WF_OK;
}
static wf_status_t sink_cmd(void *ctx,const wf_vdp1_sprite_cmd_t *c){
 wf_saturn_yaul_backend_t *b=ctx;assert(b->active&&c->stream_tile<b->expected_tiles&&c->cram_slot<16&&b->emitted_cmds<b->expected_cmds);b->emitted_cmds++;return WF_OK;
}
static wf_status_t sink_end(void *ctx){
 wf_saturn_yaul_backend_t *b=ctx;assert(b->active&&b->emitted_tiles==b->expected_tiles&&b->emitted_palettes==b->expected_palettes&&b->emitted_cmds==b->expected_cmds);b->active=0;return WF_OK;
}
static void sink_abort(void *ctx){((wf_saturn_yaul_backend_t*)ctx)->active=0;}
const wf_saturn_render_ops_t *wf_saturn_yaul_render_ops(void){
 static const wf_saturn_render_ops_t ops={sink_begin,sink_tile,sink_palette,sink_cmd,sink_end,sink_abort};return &ops;
}
static void record_set(wf_r18_sprite_record_t *r,unsigned tile,int x,int y,unsigned chain,unsigned flip,unsigned pal,unsigned active){
 memset(r,0,sizeof *r);unsigned xx=(unsigned)x&511u,yy=(unsigned)y&511u;
 unsigned attr=active|(chain<<5)|((flip&1u)<<4)|((flip&2u)<<2)|((xx>>6)&4u)|((yy>>7)&2u);
 r->bytes[1]=(uint8_t)yy;r->bytes[2]=(uint8_t)(attr>>8);r->bytes[3]=(uint8_t)attr;
 r->bytes[5]=(uint8_t)tile;r->bytes[7]=(uint8_t)(tile>>8);r->bytes[9]=(uint8_t)pal;r->bytes[11]=(uint8_t)xx;
}
static void present_test(void){
 static wf_r18_sprite_record_t records[512];
 static wf_vdp1_frame_plan_t a,b;
 static wf_r18_video_t va,vb;
 uint16_t residents[256]={0};uint8_t bits[32]={0};
 ref_poison();opt_poison();
 for(unsigned frame=0;frame<4096u;frame++){
  unsigned count=frame%512u;
  if(frame%8u==0)count=512;
  for(unsigned i=0;i<count;i++){
   unsigned mode=frame%8u,tile=(mode==0)?0x1234u:(mode==1)?i:(mode==2)?i%16u:random32()&65535u;
   unsigned chain=(mode<3)?0u:random32()%8u;
   int x=(mode<3)?(int)(i%20u)*16:(int)(random32()%512u)-16;
   int y=(mode<3)?(int)(i%15u)*16:(int)(random32()%512u)-16;
   record_set(&records[i],tile,x,y,chain,random32()%4u,random32()%256u,(mode<3)?1u:random32()%2u);
  }
  if(frame%31u==0)for(unsigned i=0;i<count;i++)record_set(&records[i],i%32u,(i%4u==0)?-16:(i%4u==1)?-15:(i%4u==2)?319:320,(i%4u==0)?-16:(i%4u==1)?-15:(i%4u==2)?239:240,random32()%8u,i%4u,i%16u,1);
  for(unsigned i=0;i<sizeof bits;i++)bits[i]=(uint8_t)random32();
  if(frame%5u==0)memset(bits,255,sizeof bits);
  if(frame%7u==0)for(unsigned i=1;i<256u;i++)residents[i]=residents[0];
  memset(&va,0xa5,sizeof va);va.palette_dirty[1]=(uint16_t)random32();va.palette_committed[1]=(uint16_t)random32();vb=va;
  wf_status_t sa=ref_evaluate(records,count,residents,bits,&a,&va);
  wf_status_t sb=opt_evaluate(records,count,residents,bits,&b,&vb);
  compare("present",frame,sa,sb,&a,&b);assert(!memcmp(&va,&vb,sizeof va));present_cases++;
  if(sa==WF_OK){
   compared_commands+=a.cmd_count;compared_tiles+=a.tile_count;
   for(unsigned i=0;i<a.tile_count;i++)residents[i]=a.tile_upload[i].source_tile;
   for(unsigned i=0;i<a.cmd_count;i++){
    const uint8_t *c=(const uint8_t*)&a.cmd[i];
    for(size_t j=offsetof(wf_vdp1_sprite_cmd_t,chain_index);j<sizeof a.cmd[i];j++)assert(!c[j]);
   }
  }
 }
}
#ifdef WF_R18_RETAIN_TEST_STATS
extern volatile uint32_t wf_r18_retain_fast_hits,wf_r18_retain_slow_calls;
static void stable_retention_test(void){
 static wf_vdp1_frame_plan_t original,a,b;
 uint16_t residents[256];uint8_t bits[32];
 for(unsigned i=0;i<256u;i++)residents[i]=(uint16_t)((255u-i)*251u);
 uint32_t first_fast=wf_r18_retain_fast_hits,first_slow=wf_r18_retain_slow_calls;
 for(unsigned frame=0;frame<8192u;frame++){
  memset(&original,(int)(random32()&255u),sizeof original);memset(bits,255,sizeof bits);
  unsigned group=frame/256u,phase=frame%256u,n=(group==0)?256u:(group==1)?0u:(group*37u)%257u;
  unsigned resident_count=256u;
  original.tile_count=n;original.cmd_count=n?512u:0u;
  for(unsigned i=0;i<n;i++)original.tile_upload[i].source_tile=(uint16_t)(i*251u+group);
  for(unsigned i=0;i<original.cmd_count;i++)original.cmd[i].stream_tile=(uint16_t)(random32()%n);
  if(n>1u&&phase==3u)original.tile_upload[n-1u].source_tile=original.tile_upload[0].source_tile;
  if(original.cmd_count&&phase==7u)original.cmd[original.cmd_count-1u].stream_tile=(uint16_t)n;
  if(n&&phase==11u)resident_count=n-1u;
  if(n&&phase==15u)bits[(n-1u)>>3]&=(uint8_t)~(1u<<((n-1u)&7u));
  if(n>1u&&phase==19u)residents[n-1u]=residents[0];
  if(n&&phase==23u)residents[n-1u]^=0x8000u;
  if(n>1u&&phase==27u){uint16_t t=original.tile_upload[0].source_tile;original.tile_upload[0].source_tile=original.tile_upload[n-1u].source_tile;original.tile_upload[n-1u].source_tile=t;}
  if(n&&phase==31u){original.cmd_count=1;original.cmd[0].stream_tile=(uint16_t)(n-1u);}
  a=b=original;
  wf_status_t sa=ref_wf_vdp1_plan_retain_tile_slots(&a,residents,bits,resident_count);
  wf_status_t sb=opt_wf_vdp1_plan_retain_tile_slots(&b,residents,bits,resident_count);
  compare("stable-retain",frame,sa,sb,&a,&b);retain_cases++;
  if(sa==WF_OK){check_identity(&original,&a);for(unsigned i=0;i<a.tile_count;i++)residents[i]=a.tile_upload[i].source_tile;}
 }
 printf("FAST synthetic_stable_hits=%" PRIu32 " rebuilds=%" PRIu32 " (includes deliberate failures/residency changes)\n",wf_r18_retain_fast_hits-first_fast,wf_r18_retain_slow_calls-first_slow);
 assert(wf_r18_retain_fast_hits-first_fast>7000u);
 assert(wf_r18_retain_slow_calls-first_slow>200u);
}
#endif
int main(void){retain_test();present_test();
#ifdef WF_R18_RETAIN_TEST_STATS
 stable_retention_test();
#endif
 printf("PASS retain_cases=%u present_cases=%u compared_commands=%u compared_tiles=%u full_plan_bytes=%zu digest=%016" PRIx64 "\n",retain_cases,present_cases,compared_commands,compared_tiles,sizeof(wf_vdp1_frame_plan_t),digest);return 0;}
