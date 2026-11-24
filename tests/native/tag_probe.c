/* Supplied WP1979 fixture; only file transport/cache diagnostics adapted for Jo. */
#include "assets_fixture.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "wf_gameplay_bindings.h"
#include "wf_r18_bootstrap.h"
#include "wf_r18_referee.h"
#include "wf_r18_match_result.h"
#include "wf_r18_result_screen.h"
#include "wf_r18_video.h"
#include "wf_r18_world.h"
#include "wf_r18_present.h"
#include "wp1979_rom_hot_pages_both_tags.h"
#include "wf_r18_ring.h"
#include "wf_r18_display.h"
#include "wf_r18_hud.h"
#include "wf_r18_transition.h"
static wf_r18_display_t display;
static wf_r18_transition_context_t transition;
static unsigned char rom[0x100000],ring_blob[WF_SATURN_LWRAM_SCENE_BYTES];static unsigned char rom_cache[WF_SATURN_LWRAM_ROM_BYTES],tile_cache[WF_SATURN_LWRAM_TILE_BYTES];static size_t romsize;
static unsigned play_started,play_sprite_reads,play_rom_reads;
static FILE *sprite_trace,*rom_trace;static uint8_t seen_rom_pages[512];
static uint32_t tile_use_count[65536];static const char *tile_use_path;
static void tile_use_dump(void){
 if(!tile_use_path)return;
 FILE *out=fopen(tile_use_path,"w");if(!out)return;
 fputs("tile,frames\n",out);
 for(unsigned i=0;i<65536u;i++)if(tile_use_count[i])fprintf(out,"%u,%u\n",i,tile_use_count[i]);
 fclose(out);
}
static void tile_use_record(void){
 if(!tile_use_path){tile_use_path=getenv("WF_PROBE_TILE_USE_TRACE");if(tile_use_path)atexit(tile_use_dump);}
 if(!tile_use_path)return;
 uint16_t tiles[WF_VDP1_STREAM_TILES_MAX];
 size_t count=wf_r18_present_frame_tiles(tiles,WF_VDP1_STREAM_TILES_MAX);
 for(size_t i=0;i<count;i++)tile_use_count[tiles[i]]++;
}
/* HOST-only device fixture: decimal frame,active-low-mask rows, no header.
 * Values are immutable after parsing; an event holds until the next row. */
typedef struct pad_change {uint32_t frame;uint8_t mask;} pad_change_t;
static pad_change_t *pad_changes;static size_t pad_count,pad_next;
static uint8_t pad_held=0xffu;
static int load_pad_schedule(const char *path){
 FILE *file=fopen(path,"r");char line[96];size_t line_number=0,capacity=0;
 if(!file){fputs("Cannot read pad schedule\n",stderr);return -1;}
 while(fgets(line,sizeof line,file)){
  char *end,*mask_text;unsigned long frame,mask;line_number++;
  if(line[0]<'0'||line[0]>'9')goto invalid;
  errno=0;frame=strtoul(line,&end,10);
  if(errno||frame>UINT32_MAX||*end!=',')goto invalid;
  mask_text=end+1;if(*mask_text<'0'||*mask_text>'9')goto invalid;
  errno=0;mask=strtoul(mask_text,&end,10);
  if(errno||mask>255u)goto invalid;
  if(*end=='\r')end++;
  if(*end=='\n')end++;
  if(*end||(pad_count&&frame<=pad_changes[pad_count-1u].frame))goto invalid;
  if(pad_count==capacity){
   if(capacity>SIZE_MAX/2u/sizeof *pad_changes)goto invalid;
   size_t next=capacity?capacity*2u:256u;
   pad_change_t *grown=realloc(pad_changes,next*sizeof *pad_changes);
   if(!grown)goto invalid;
   pad_changes=grown;capacity=next;
  }
  pad_changes[pad_count++]=(pad_change_t){(uint32_t)frame,(uint8_t)mask};
 }
 if(ferror(file)||!pad_count)goto invalid;
 fclose(file);return 0;
invalid:
 fprintf(stderr,"Invalid pad schedule at line %zu\n",line_number);
 fclose(file);free(pad_changes);pad_changes=NULL;pad_count=0;return -1;
}
static uint8_t scheduled_pad(uint32_t frame){
 while(pad_next<pad_count&&pad_changes[pad_next].frame<=frame)
  pad_held=pad_changes[pad_next++].mask;
 return pad_held;
}
/* Read-only controller derived from upstream full_flow_human_tag_probe.
 * It emits only the same active-low physical pad bits a player can press. */
static uint8_t wp1979_policy(const wf_runtime_t*r){
 const wf_actor_raw_t*a=&r->actors[(wf_u8(&r->actors[1],0x33)&1u)?1:0],*b=&r->actors[4],*c=&r->actors[5],*t=b;
 int br=(wf_u8(b,0)&128u)&&((wf_u8(b,0x33)&5u)==1u);
 int cr=(wf_u8(c,0)&128u)&&((wf_u8(c,0x33)&5u)==1u);
 if(cr&&!br)t=c;
 unsigned state=wf_u8(a,0x21),sel=wf_u8(a,0x61),ts=wf_u8(t,0x21),sub=wf_u8(t,0x65);
 int ground=ts==4u&&(sub==6u||sub==7u||sub==8u||sub==9u);
 int dx=(int16_t)(wf_be16(t,6)-wf_be16(a,6)),dy=(int16_t)(wf_be16(t,10)-wf_be16(a,10));
 uint8_t bits=0xffu;unsigned frame=r->g.frame_counter;
 if(state==0u||state==1u||state==2u){
  if(dx>24)bits&=(uint8_t)~1u;else if(dx< -24)bits&=(uint8_t)~2u;
  if(dy>4)bits&=(uint8_t)~4u;else if(dy< -4)bits&=(uint8_t)~8u;
 }
 if(frame%8u<4u){
  uint8_t press=0;
  if(state==4u||state==6u||(state==5u&&(sel==0x4au||sel==0x7bu||sel==0x52u||sel==0x53u)))press=0x30u;
  else if(state==5u&&(sel==0x15u||sel==0x16u))press=((frame/8u)&1u)?0x10u:0x20u;
  else if(dx> -104&&dx<104&&dy> -32&&dy<24)press=ground?0x20u:((frame/8u)&1u)?0x10u:0x20u;
  bits&=(uint8_t)~press;
 }
 return bits;
}


/* Read-only controller for a real corner tag; never changes actor/RNG state. */
static uint8_t wp1979_tag_policy(const wf_runtime_t*r){
 if(getenv("WP1979_TAG_AFTER")&&r->g.frame_counter<(unsigned)atoi(getenv("WP1979_TAG_AFTER")))return wp1979_policy(r);
 const wf_actor_raw_t*a=&r->actors[0],*p=&r->actors[1];
 if(wf_u8(p,0x33)&1u)return wp1979_policy(r);
 unsigned state=wf_u8(a,0x21),frame=r->g.frame_counter;
 int x=wf_be16(a,6),y=wf_be16(a,10);
 uint8_t mask=255u;
 if(state==0u||state==1u){
  int tx=getenv("WP1979_TAG_X")?atoi(getenv("WP1979_TAG_X")):500;
  if(x>tx+2)mask&=(uint8_t)~2u;else if(x<tx-2)mask&=(uint8_t)~1u;
  int ty=getenv("WP1979_TAG_Y")?atoi(getenv("WP1979_TAG_Y")):394;
  if(y<ty-2)mask&=(uint8_t)~4u;else if(y>ty+2)mask&=(uint8_t)~8u;
  if(x<0x220&&y>=0x186&&(wf_be16(p,0xae)&0x80ffu)==0x8002u&&frame%8u<4u)
   mask&=(uint8_t)~0x10u;
 }else if(state==4u||state==6u||state==12u||state==255u){
  if(frame%8u<4u)mask&=(uint8_t)~0x30u;
 }
 if(getenv("WP1979_TAG_DEFEND")&&frame%8u<4u){
  const wf_actor_raw_t*t=&r->actors[(wf_u8(&r->actors[5],0x33)&1u)?5:4];
  int dx=(int16_t)(wf_be16(t,6)-wf_be16(a,6)),dy=(int16_t)(wf_be16(t,10)-wf_be16(a,10));
  if(dx>-104&&dx<104&&dy>-32&&dy<24)mask&=(uint8_t)~0x10u;
 }
 return mask;
}

/* HOST read-only evidence: legal participant, CPU reserve role and input ownership. */
static FILE *tag_roles;
static void tag_observe(const wf_runtime_t *r, unsigned pad) {
 if(!tag_roles)return;
 const unsigned slots[]={0,1,4,5};
 for(unsigned i=0;i<4;i++) {
  unsigned slot=slots[i];const wf_actor_raw_t*a=&r->actors[slot];
  fprintf(tag_roles,"%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n",
   r->g.frame_counter,slot,wf_u8(a,0x57),pad,
   wf_u8(a,0),wf_u8(a,0x32),wf_u8(a,0x33),wf_u8(a,0x34),wf_u8(a,0x56),
   !!(wf_u8(a,0x33)&1u),!!(wf_u8(a,0x33)&2u),!!(wf_u8(a,0x56)&64u),
   wf_be16(a,0x20),wf_be16(a,0x60),wf_be16(a,0xae),
   wf_be16(a,6),wf_be16(a,10),wf_be16(a,14),wf_be16(a,0x66),
   wf_be32(a,0x8a),wf_be32(a,0x86),wf_be32(a,0x26),
   wf_be16(a,0xe6),wf_be16(a,0x24),wf_be16(a,0x22),wf_be16(a,4),
   wf_be16(a,0xa2),wf_u8(a,0x37),wf_u8(a,0x2e));
 }
}

static void trace_rom(uint32_t off,size_t n){if(rom_trace&&n)for(unsigned page=off/1024;page<=(off+n-1)/1024&&page<512;page++)if(!seen_rom_pages[page]){seen_rom_pages[page]=1;fprintf(rom_trace,"%u\n",page);}}
static wf_r18_world_ctx_t world;static wf_runtime_t rt;static wf_gameplay_binding_context_t gc;static wf_r18_video_t video;static wf_r18_bootstrap_context_t boot;static wf_phase_binding_t binds[48];
static wf_status_t readrom(void*u,uint32_t a,void*out,size_t n){(void)u;if(a+n>romsize)return WF_BLOCKED;trace_rom(a,n);return wf_r18_rom_read(u,a,out,n);}
static wf_status_t readram(wf_runtime_t*r,uint32_t a,uint16_t*out,void*u){(void)u;switch(a){case 0x1c0062:case 0x1c0063:*out=r->g.global_0063;return WF_OK;case 0x1c004e:*out=(r->g.credit_remainder_004e<<8)|r->g.credit_count_004f;return WF_OK;case 0x1c004a:*out=r->g.credit_cycle_units_004b;return WF_OK;default:fprintf(stderr,"RAM dependency%x\n",a);return WF_BLOCKED;}}
static wf_status_t text(wf_runtime_t*r,uint32_t pc,void*u){return pc==0x2503c?wf_r18_text_2503c(r,(uint16_t)r->m68k_regs.d[0],u):wf_r18_number_26122(r,(uint16_t)r->m68k_regs.d[0],u);}
static wf_status_t resetpal(wf_runtime_t*r,void*u){wf_r18_video_t*v=u;(void)r;memset(v->palette_map,0,sizeof v->palette_map);memset(v->palette_used,0,sizeof v->palette_used);return WF_OK;}
static wf_status_t map(wf_runtime_t*r,void*u){(void)r;(void)u;return WF_OK;} /* Host has no screen; camera/state asserted by ROM oracle. */
static wf_status_t input(wf_runtime_t*r,uint32_t address,uint16_t*out,void*u){(void)r;(void)u;if(address<0x140020||address>0x140026)return WF_BAD_STATE;*out=address==0x140020u?gc.input_hw_low[0]:0xffffu;return WF_OK;}
static wf_status_t world288(wf_runtime_t*r,uint8_t a[4],uint8_t b,void*u){(void)u;return wf_r18_world_288f0(r,a,b,&world);}
static wf_status_t world26(wf_runtime_t*r,uint8_t s,uint16_t d,void*u){(void)u;return wf_r18_world_26b68(r,s,d,&world);}
static wf_status_t audio(wf_runtime_t*r,uint16_t cmd,void*u){(void)u;printf("AUDIO frame=%u command=%04x\n",r->g.frame_counter,cmd);return WF_OK;} /* Explicit silent output device. */
static int file_read(const char*name,uint32_t off,void*dst,size_t n){return wf_jo_assets_file_read(name,off,dst,n);}
static int scene_file_size(const char*name,uint32_t*out){return wf_jo_assets_file_size(name,out);}
static int scene_file_read(const char*name,uint32_t off,void*dst,size_t n){return wf_jo_assets_file_read(name,off,dst,n);}
static uint32_t host_wait_vblanks;
static wf_status_t wait_21e6(wf_runtime_t*r,uint16_t raw,void*u){(void)r;(void)u;host_wait_vblanks+=(uint32_t)raw+1u;return WF_OK;} /* Host transport counts hardware edges; gameplay does not advance during $21E6. */
static wf_status_t result_wait(wf_runtime_t*r,uint16_t count,void*u){if(!count)return WF_BAD_STATE;return wait_21e6(r,(uint16_t)(count-1u),u);}
static wf_status_t result_commit(void*u){return wf_r18_hud_commit(u);}
static wf_status_t render(wf_runtime_t*r,wf_actor_raw_t *const*a,size_t n,void*u){(void)u;
if((uint16_t)gc.camera.scroll0_x!=r->g.global_17e6||(uint16_t)gc.camera.scroll1_x!=r->g.global_17ea||(uint16_t)gc.camera.scroll0_y!=r->g.global_17ee||(uint16_t)gc.camera.scroll1_y!=r->g.global_17f2_word){fprintf(stderr,"Camera/ring desynchronization at frame %u\n",r->g.frame_counter);return WF_BAD_STATE;}
if(r->g.frame_counter==30u||r->g.frame_counter==120u||r->g.frame_counter==600u||r->g.frame_counter%1000u==0u)printf("CAMERA frame=%u sprites=%u,%u/%u,%u ring=%u,%u/%u,%u\n",r->g.frame_counter,(uint16_t)gc.camera.scroll0_x,(uint16_t)gc.camera.scroll0_y,(uint16_t)gc.camera.scroll1_x,(uint16_t)gc.camera.scroll1_y,r->g.global_17e6,r->g.global_17ee,r->g.global_17ea,r->g.global_17f2_word);
wf_status_t s=wf_r18_present_batch(r,a,n,&video);if(s==WF_OK)tile_use_record();if(r->g.frame_counter%1000u==0u||r->g.frame_counter==30u||r->g.frame_counter==60u||r->g.frame_counter==120u||r->g.frame_counter==300u||r->g.frame_counter==600u||r->g.frame_counter==900u)printf("RENDER frame=%u actors=%zu cmds=%u status=%u p0=%u,%u p1=%u,%u p4=%u,%u p5=%u,%u hp=%u,%u,%u,%u state=%04x,%04x,%04x,%04x\n",r->g.frame_counter,n,wf_r18_present_commands(),s,wf_be16(&r->actors[0],WF_A_X),wf_be16(&r->actors[0],WF_A_Y),wf_be16(&r->actors[1],WF_A_X),wf_be16(&r->actors[1],WF_A_Y),wf_be16(&r->actors[4],WF_A_X),wf_be16(&r->actors[4],WF_A_Y),wf_be16(&r->actors[5],WF_A_X),wf_be16(&r->actors[5],WF_A_Y),wf_be16(&r->actors[0],WF_A_STAMINA),wf_be16(&r->actors[1],WF_A_STAMINA),wf_be16(&r->actors[4],WF_A_STAMINA),wf_be16(&r->actors[5],WF_A_STAMINA),wf_be16(&r->actors[0],WF_A_STATE),wf_be16(&r->actors[1],WF_A_STATE),wf_be16(&r->actors[4],WF_A_STATE),wf_be16(&r->actors[5],WF_A_STATE));return s;}
int main(int argc,char**argv){wf_host_assets_init();if(argc!=2)return 2;const char *pad_schedule_path=getenv("WF_PROBE_PAD_SCHEDULE");if(pad_schedule_path&&load_pad_schedule(pad_schedule_path))return 2;const char *rom_path=getenv("WF_PROBE_ROM_TRACE");if(rom_path){rom_trace=fopen(rom_path,"w");if(!rom_trace)return 2;setvbuf(rom_trace,NULL,_IOLBF,0);}const char *trace_path=getenv("WF_PROBE_SPRITE_TRACE");if(trace_path){sprite_trace=fopen(trace_path,"w");if(!sprite_trace)return 2;setvbuf(sprite_trace,NULL,_IOLBF,0);fputs("frame,tile,bytes\n",sprite_trace);}uint32_t ring_size;if(wf_jo_assets_file_size("R18RING.BIN",&ring_size)||ring_size>sizeof ring_blob||wf_jo_assets_file_read("R18RING.BIN",0,ring_blob,ring_size)||wf_r18_ring_bind(ring_blob,ring_size))return 2;wf_r18_present_init(file_read,rom_cache,sizeof rom_cache,tile_cache,sizeof tile_cache);if(wf_r18_hot_sprites_load()!=WF_OK)return 2;FILE*f=fopen(argv[1],"rb");if(!f)return 2;romsize=fread(rom,1,sizeof rom,f);fclose(f);if(wf_r18_rom_pages_preload(wf_r18_rom_hot_pages,sizeof wf_r18_rom_hot_pages/sizeof wf_r18_rom_hot_pages[0])!=WF_OK)return 2;if(wf_r18_rom_extra_pages_preload(wf_r18_rom_tag_extra_pages,sizeof wf_r18_rom_tag_extra_pages/sizeof wf_r18_rom_tag_extra_pages[0])!=WF_OK)return 2;size_t n=wf_gameplay_bindings_build(&gc,binds,48);wf_runtime_init(&rt,binds,n);video.read_rom=readrom;video.read_ram=readram;boot.palette_2aea=wf_r18_palette_2aea;boot.palette_release_2b58=wf_r18_palette_2b58;boot.palette_reset=resetpal;boot.map_26e66=map;boot.video_batch=wf_r18_video_batch;boot.fg0_batch=wf_r18_video_batch;boot.text_owner=text;boot.user=&video;
/* wp1979 audit: only boot seed selection differs from the upstream device probe.
 * Production wf_r18_direct_boot owns all actor and opponent initialization. */
wf_r18_launch_config_t config={.human=0,.partner=1,.cabinet_config=0x300,.rng_state=0};
wf_status_t st=WF_BAD_STATE;
unsigned selected_seed=0u,found_seed=0u;
const char *seed_text=getenv("WP1979_SEED");
const unsigned first_seed=seed_text?(unsigned)strtoul(seed_text,NULL,0):0u;
const unsigned last_seed=seed_text?first_seed:65535u;
for(unsigned seed=first_seed;seed<=last_seed;seed++){
 config.rng_state=seed;wf_r17_m68k_software_reset(&config.registers);
 st=wf_r18_direct_boot(&rt,&boot,&config);
 if(st!=WF_OK){fprintf(stderr,"BOOT_FAIL seed=%u status=%u pc=%x video=%x\n",seed,st,boot.blocked_pc,video.failed_address);return 1;}
 unsigned opponent0=wf_u8(&rt.actors[4],0x57u),opponent1=wf_u8(&rt.actors[5],0x57u);
 if((opponent0==9u&&opponent1==7u)||(opponent0==7u&&opponent1==9u)){
  selected_seed=seed;found_seed=1u;break;
 }
}
if(!found_seed){fputs("No requested pair selected by native bootstrap\n",stderr);return 1;}
printf("WP1979_BOOT seed=%u ids=%u,%u,%u,%u state=%u\n",selected_seed,
 wf_u8(&rt.actors[0],0x57u),wf_u8(&rt.actors[1],0x57u),
 wf_u8(&rt.actors[4],0x57u),wf_u8(&rt.actors[5],0x57u),rt.frame_stage);
if(getenv("WP1979_SEARCH_ONLY"))return 0;
printf("BOOT %u PC %x VIDEO %x\n",st,boot.blocked_pc,video.failed_address);

wf_r18_video_ring_bind(&video);
size_t fg_n=0,bg_n=0;for(size_t i=0;i<sizeof video.fg_palette;i++)fg_n+=video.fg_palette[i]!=0;for(size_t i=0;i<sizeof video.bg_palette;i++)bg_n+=video.bg_palette[i]!=0;printf("BOOT palettes fg=%zu bg=%zu\n",fg_n,bg_n);
for(unsigned layer=0;layer<2;layer++){unsigned nonzero=0,visible=0,unique[256]={0};int sx=layer?rt.g.global_17ea:rt.g.global_17e6,sy=layer?rt.g.global_17f2_word:rt.g.global_17ee;uint8_t *pal=layer?video.bg_palette:video.fg_palette;for(unsigned y=0;y<240;y++)for(unsigned x=0;x<320;x++){uint8_t v=0;wf_r18_ring_pixel(layer,sx+(int)x,2048-sy+(int)y,&v);nonzero+=v!=0;unique[v]++;unsigned off=(v>>4)*128+(v&15)*2;visible+=(pal[off]|pal[off+1])!=0;}unsigned colors=0;for(unsigned i=0;i<256;i++)colors+=unique[i]!=0;printf("ring layer=%u scroll=%d,%d nonzero=%u visible=%u colors=%u\n",layer,sx,sy,nonzero,visible,colors);}
gc.cabinet_words_valid=1;gc.cabinet_word_140020=gc.cabinet_word_140022=0xffff;gc.camera_valid=1;gc.camera.scene_index=0;gc.camera.scroll0_x=rt.g.global_17e6;gc.camera.scroll1_x=rt.g.global_17ea;gc.camera.scroll0_y=rt.g.global_17ee;gc.camera.scroll1_y=rt.g.global_17f2_word;gc.input_valid_mask=0x1ff;memset(gc.input_hw_low,0xff,sizeof gc.input_hw_low);gc.audio_command=audio;
gc.referee_direction_20c8=wf_r18_referee_direction_20c8;
gc.referee_display_2067c=wf_r18_referee_display_2067c;
gc.referee_number_2067c=wf_r18_referee_number_2067c;
gc.referee_clear_206fe=wf_r18_referee_clear_206fe;
gc.referee_tag_phase6_90d6=wf_r18_tag_bonus_90d6;
gc.referee_result_20156=wf_r18_result_sound_20156;
gc.post7_wait_21e6=wait_21e6;gc.participant_input_word=input;gc.participant_commit_batch=wf_r18_video_commit;gc.post7_fg0_batch=gc.post7_video_batch=wf_r18_video_batch;gc.post7_resource_2aea=wf_r18_palette_2aea;gc.post7_resource_2b58=wf_r18_palette_2b58;gc.post7_raw_2503c=wf_r18_text_2503c;gc.post7_raw_9aca=wf_r18_strip_9aca;gc.post7_fg0_peek_long=wf_r18_video_peek;gc.post7_render_2836_batch=render;world.read_rom=readrom;gc.branchb_288f0=world288;gc.branchb_26b68=world26;gc.r17_owner_user=&video;
const char *limit_text=getenv("WF_PROBE_FRAMES");
unsigned frame_limit=limit_text?(unsigned)strtoul(limit_text,NULL,10):1000u;
int attack=!pad_schedule_path&&getenv("WF_PROBE_ATTACK")!=NULL;
if(pad_schedule_path)printf("INPUT schedule=%s initial=FF seed selected by native bootstrap\n",pad_schedule_path);
else printf("INPUT replay=%s seed selected by native bootstrap\n",getenv("WP1979_POLICY")?"adaptive normal-pad controller":attack?"RIGHT60-179/A180-183":"none (all ports FF)");
uint16_t observed_count=0u;
if(wf_r18_display_init(&display,&rt,readrom,NULL)!=WF_OK)return 2;
transition=(wf_r18_transition_context_t){.video=&video,.world=&world,.display=&display,.render_queue=&gc.render_queue,.camera=&gc.camera,.scene_blob=ring_blob,.scene_capacity=sizeof ring_blob,.file_size=scene_file_size,.file_read=scene_file_read,.priority_140010=&boot.priority_140010};
wf_r18_transition_bind(&transition);gc.transition_reset=wf_r18_transition_reset;gc.transition_barrier=wf_r18_transition_barrier;

play_started=1;wf_host_assets_play();
unsigned four_present=0u;
FILE*padtrace=NULL;const char*padtrace_path=getenv("WP1979_PAD_TRACE");
if(padtrace_path){padtrace=fopen(padtrace_path,"w");if(!padtrace)return 2;setvbuf(padtrace,NULL,_IOLBF,0);}
const char *tag_path=getenv("WP1979_TAG_ROLES");
if(tag_path){tag_roles=fopen(tag_path,"w");if(!tag_roles)return 2;
fputs("frame,slot,id,pad,active,flags32,flags33,flags34,role56,legal,human,reserve,state,selector,ae,x,y,z,stamina,input8a,partner86,link26,tag_timer_e6,anim_index,anim_timer,pose,input_fresh,contact37,facing\n",tag_roles);setvbuf(tag_roles,NULL,_IOLBF,0);}
tag_observe(&rt,255u);
unsigned previous_pad=256u;
for(unsigned frame=0;frame<frame_limit;frame++){wf_host_assets_frame(rt.g.frame_counter);
gc.input_hw_low[0]=pad_schedule_path?scheduled_pad(frame):(attack&&frame>=60u&&frame<180u)?0xfeu:(attack&&frame>=180u&&frame<184u)?0xefu:0xffu;
if(getenv("WP1979_POLICY"))gc.input_hw_low[0]=wp1979_policy(&rt);
if(getenv("WP1979_TAG_POLICY"))gc.input_hw_low[0]=wp1979_tag_policy(&rt);
if(padtrace&&gc.input_hw_low[0]!=previous_pad){fprintf(padtrace,"%u,%u\n",frame,gc.input_hw_low[0]);previous_pad=gc.input_hw_low[0];}
four_present+=!!((wf_u8(&rt.actors[0],0)&wf_u8(&rt.actors[1],0)&wf_u8(&rt.actors[4],0)&wf_u8(&rt.actors[5],0))&128u);
st=wf_runtime_frame(&rt);if(st==WF_BLOCKED&&rt.frame_ready){st=wf_r18_display_irq(&rt,&display);if(st==WF_OK)st=wf_r18_world_commit(&rt,&world);if(st==WF_OK)st=wf_r18_hud_irq(&rt,&video);if(st==WF_OK){wf_runtime_vblank_ack(&rt);st=wf_runtime_frame(&rt);}}tag_observe(&rt,gc.input_hw_low[0]);if(st==WF_BLOCKED&&wf_r18_tag_match_complete(&rt)){
uint32_t before_wait=host_wait_vblanks;
if(wf_r18_result_in_ring(&rt,&video,result_wait,result_commit,&video)!=WF_OK||host_wait_vblanks-before_wait!=131u){fputs("In-ring result presentation failed\n",stderr);return 1;}
printf("RESULT SCREEN original11CE-122B wait=%u VBlanks\n",host_wait_vblanks-before_wait);
wf_host_assets_dump(rt.actors,sizeof rt.actors);printf("MATCH COMPLETE frame=%u ROM endpoint=0011B6 result=%04x,%04x,%04x,%04x referee=%04x/%04x\n",rt.g.frame_counter,wf_be16(&rt.actors[0],WF_A_RESULT),wf_be16(&rt.actors[1],WF_A_RESULT),wf_be16(&rt.actors[4],WF_A_RESULT),wf_be16(&rt.actors[5],WF_A_RESULT),wf_be16(&rt.referee,WF_A_STATE),wf_be16(&rt.referee,WF_A_ANIM_INDEX));
printf("FINISH count169A=%04x timer16A0=%04x\n",rt.g.global_169a,rt.g.global_16a0);
printf("WP1979_PRESENCE frames=%u all_four_active=%u ids=%u,%u,%u,%u\n",rt.g.frame_counter,four_present,wf_u8(&rt.actors[0],0x57),wf_u8(&rt.actors[1],0x57),wf_u8(&rt.actors[4],0x57),wf_u8(&rt.actors[5],0x57));
printf("HARDWARE WAIT vblanks=%u\n",host_wait_vblanks);printf("PLAY CD reads ROM=%u SPRITES=%u\n",play_rom_reads,play_sprite_reads);if(getenv("WF_PROBE_REQUIRE_RESIDENT")&&(play_rom_reads||play_sprite_reads)){fputs("Resident playback caused CD reads\n",stderr);return 1;}return 0;
}if(st==WF_OK&&rt.g.global_169a!=observed_count){observed_count=rt.g.global_169a;printf("REFEREE frame=%u count169A=%04x\n",rt.g.frame_counter,observed_count);}
if(st!=WF_OK){printf("BLOCK frame=%u status=%u phase=%x dependency=%x trace=%zu video=%x transition=%x\n",frame,st,gc.last_blocked_phase,gc.last_blocked_dependency,rt.trace_count,video.failed_address,transition.blocked_pc);for(size_t j=0;j<rt.trace_count;j++)printf(" %x",rt.trace[j]);puts("");printf("world mode=%u bank=%u animations=%u/%u\n",rt.g.global_007e,rt.g.global_16d2,world.animation_count[0],world.animation_count[1]);for(unsigned j=0;j<46;j++){uint8_t*p=rt.branchb_objs_188a[j];if(p[0]||p[1]||p[2]||p[3])printf("world%u=%02x %02x %02x %02x\n",j,p[0],p[1],p[2],p[3]);}for(unsigned i=0;i<6;i++)printf("cpu%u st=%u pc=%x dep=%x af=%u b2=%u b4=%u\n",i,gc.cpu_trace[i].state,gc.cpu_trace[i].handler_rom,gc.cpu_trace[i].blocked_dependency_rom,wf_u8(&rt.actors[i],0xaf),wf_be16(&rt.actors[i],0xb2),wf_u8(&rt.actors[i],0xb4));for(unsigned i=0;i<6;i++)printf("actor%u state%04x raw%04x flags%04x health%u subtype%04x\n",i,wf_be16(&rt.actors[i],0x20),wf_be16(&rt.actors[i],4),wf_be16(&rt.actors[i],0x32),wf_be16(&rt.actors[i],0x66),wf_be16(&rt.actors[i],0x64));for(unsigned i=0;i<6;i++)printf("actor%u target=%08x b5=%02x b6=%02x b7=%02x f32=%04x f33=%02x f34=%02x mode=%02x latch=%04x ae=%04x role=%04x\n",i,wf_be32(&rt.actors[i],0x7a),wf_u8(&rt.actors[i],0xb5),wf_u8(&rt.actors[i],0xb6),wf_u8(&rt.actors[i],0xb7),wf_be16(&rt.actors[i],0x32),wf_u8(&rt.actors[i],0x33),wf_u8(&rt.actors[i],0x34),rt.g.mode_0161,wf_be16(&rt.actors[i],0x1c),wf_be16(&rt.actors[i],0xae),wf_be16(&rt.actors[i],0x56));printf("route=%04x team5=%08x pos5=%u,%u pos4=%u,%u target4=%08x\n",rt.g.credited_route,wf_be32(&rt.actors[5],WF_A_TEAM_LINK),wf_be16(&rt.actors[5],WF_A_X),wf_be16(&rt.actors[5],WF_A_Y),wf_be16(&rt.actors[4],WF_A_X),wf_be16(&rt.actors[4],WF_A_Y),wf_be32(&rt.actors[4],WF_A_TARGET));for(unsigned k=0;k<6u;k++)printf("pos%u=%u,%u team=%08x link=%08x\n",k,wf_be16(&rt.actors[k],WF_A_X),wf_be16(&rt.actors[k],WF_A_Y),wf_be32(&rt.actors[k],WF_A_TEAM_LINK),wf_be32(&rt.actors[k],WF_A_LINK));for(unsigned k=0;k<6u;k++)printf("selector%u=%04x callback=%05x dependency=%05x\n",k,wf_be16(&rt.actors[k],WF_A_SELECTOR),gc.f4c2_actions.trace[k].callback_rom,gc.f4c2_actions.trace[k].blocked_dependency_rom);return 1;}}
printf("WP1979_PRESENCE frames=%u all_four_active=%u ids=%u,%u,%u,%u\n",frame_limit,four_present,wf_u8(&rt.actors[0],0x57),wf_u8(&rt.actors[1],0x57),wf_u8(&rt.actors[4],0x57),wf_u8(&rt.actors[5],0x57));
if(padtrace)fclose(padtrace);
if(attack&&frame_limit>=300u&&wf_be16(&rt.actors[4],WF_A_STAMINA)>=145u){fputs("No ROM-backed damage observed\n",stderr);return 1;}
if(getenv("WF_PROBE_REQUIRE_COMPLETE")){fputs("No match endpoint reached within frame budget\n",stderr);return 1;}
printf("PLAY CD reads ROM=%u SPRITES=%u\n",play_rom_reads,play_sprite_reads);if(getenv("WF_PROBE_REQUIRE_RESIDENT")&&(play_rom_reads||play_sprite_reads)){fputs("Resident playback caused CD reads\n",stderr);return 1;}
printf("PASS %u direct-combat frames; stamina=%u,%u,%u,%u\n",frame_limit,wf_be16(&rt.actors[0],WF_A_STAMINA),wf_be16(&rt.actors[1],WF_A_STAMINA),wf_be16(&rt.actors[4],WF_A_STAMINA),wf_be16(&rt.actors[5],WF_A_STAMINA));return 0;}

