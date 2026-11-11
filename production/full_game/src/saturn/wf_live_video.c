#include <yaul.h>
#include <string.h>
#include "wf_live_video.h"
#include "wf_video_plan.h"
#include "wf_cd.h"
#ifdef WF_FRONT_RESOURCE_PACK
#include "wf_front_resource_pack.h"
#define video_io wf_front_asset_io
#else
#define video_io wf_cd_asset_io
#endif
#include "wf_assets.h"
#include "assets.h"
#include "wf_workspace.h"
#include "wf_startup_notice.h"
#include "wf_platform.h"
#include "wf_post_atlas.h"
#ifdef WF_FINALE_SPRITE_COUNT
#include "wf_finale_atlas.h"
#endif
#define COMMANDS 1028
#ifdef WF_POST_SPRITE_BYTES
#define TEXTURE_BYTES (WF_POST_ATLAS_MAX_TILES*128u)
_Static_assert(WF_POST_SPRITE_BYTES==WF_POST_ATLAS_STATIC_MAX*128u,"Canonical POST atlas size");
_Static_assert(WF_FRONT_SPRITE_BYTES<=TEXTURE_BYTES,"Frontend atlas fits the shared texture lease");
static wf_post_atlas_t post_atlas;
#ifdef WF_POST_DEDUP
_Static_assert(WF_POST_DEDUP_PHYSICAL_SLOTS==WF_POST_ATLAS_STATIC_MAX,"POST physical base unchanged");
_Static_assert(WF_POST_DEDUP_LOGICAL_COUNT==sizeof wf_post_sprite_ids/sizeof *wf_post_sprite_ids,"POST logical count");
_Static_assert(sizeof wf_post_sprite_slots==sizeof wf_post_sprite_ids,"POST parallel mapping");
#define POST_MAPPING wf_post_sprite_slots
#define POST_MAPPING_COUNT WF_POST_DEDUP_LOGICAL_COUNT
#else
#define POST_MAPPING NULL
#define POST_MAPPING_COUNT 0u
#endif
#else
#define TEXTURE_BYTES WF_FRONT_SPRITE_BYTES
#endif
#define plan (wf_workspace.front.plan)
#define previous_bg (wf_workspace.front.previous_bg)
#define previous_text (wf_workspace.front.previous_text)
#define previous_palette (wf_workspace.front.previous_palette)
#define pieces (wf_workspace.front.pieces)
#define tile_stage (wf_workspace.front.tile_stage)
#define stage (wf_workspace.front.stage)
#define commands (wf_workspace.front.commands)
static bool valid_maps,map_ready[2],map_dirty[2];
static vdp1_vram_partitions_t partitions;
static bool submitted,hidden=true;
volatile uint32_t wf_render_work,wf_render_wait;
volatile uint32_t wf_video_error,wf_video_commits,wf_video_sprites,wf_video_tile,wf_video_atlas;
static const struct {const char*name;const uint16_t*ids;unsigned count,bytes,file_bytes;uint32_t crc;} atlases[]={
 {"FRONT.TEX",wf_front_sprite_ids,sizeof wf_front_sprite_ids/2,WF_FRONT_SPRITE_BYTES,WF_FRONT_SPRITE_BYTES,WF_FRONT_CRC},
 {"ENTRY.TEX",wf_entry_sprite_ids,sizeof wf_entry_sprite_ids/2,WF_ENTRY_SPRITE_BYTES,WF_ENTRY_SPRITE_BYTES,WF_ENTRY_CRC},
 {"RING.TEX",wf_ring_sprite_ids,sizeof wf_ring_sprite_ids/2,WF_RING_SPRITE_BYTES,WF_RING_SPRITE_BYTES,WF_RING_CRC}
#ifdef WF_POST_SPRITE_BYTES
 ,{"POST.TEX",wf_post_sprite_ids,sizeof wf_post_sprite_ids/2,WF_POST_SPRITE_BYTES,WF_POST_FILE_BYTES,WF_POST_TEXTURE_CRC}
#endif
 };
static bool atlas_loaded;
#if defined(WF_FIXED_ATTRACT) && WF_FIXED_ATTRACT
static bool program_valid;
bool wf_live_video_program_load(void);
#endif
static const vdp2_scrn_t screens[3]={VDP2_SCRN_NBG0,VDP2_SCRN_NBG1,VDP2_SCRN_NBG2};
static const uintptr_t maps[3]={0x25e60000,0x25e62000,0x25e64000};
static unsigned rw(const uint8_t*p){return (p[0]<<8)|p[1];}
/* The SDK memcmp reads one byte per iteration. These fixed-size shadows are
 * aligned; an alias-safe word comparison keeps unchanged frames inexpensive. */
typedef uint32_t alias_u32 __attribute__((may_alias));
static bool equal(const void*a,const void*b,unsigned n){
 if(((uintptr_t)a|(uintptr_t)b|n)&3u)return !memcmp(a,b,n);
 const alias_u32*x=a,*y=b;
 for(unsigned i=0;i<n/4;i++)if(x[i]!=y[i])return false;
 return true;
}
static void transfer(void*dst,const void*src,unsigned n){scu_dma_transfer(0,dst,src,n);scu_dma_transfer_wait(0);}
static bool file(const char*name,void*dst,unsigned n,uint32_t crc){
 uint32_t size;if(video_io.size(NULL,name,&size)||size!=n||video_io.read(NULL,name,0,dst,n))return false;
 return wf_assets_crc32(dst,n)==crc;
}
static bool upload(void*u __unused,unsigned layer,unsigned slot,unsigned code){
 uintptr_t dst=VDP2_VRAM_ADDR(layer,slot*128u),src=0x00280000u+code*128u;
 memcpy(tile_stage,(const void*)src,128);transfer((void*)dst,tile_stage,128);
 for(unsigned i=0;i<64;i++)if(((volatile uint16_t*)dst)[i]!=((const uint16_t*)src)[i]){wf_video_error=9;wf_video_tile=code;return false;}
 return true;
}
void wf_live_video_reveal(void){hidden=false;}
void wf_live_video_hide(void){hidden=true;vdp2_tvmd_display_clear();vdp2_sync();vdp2_sync_wait();}
void wf_live_video_finish(void){wf_live_video_hide();if(submitted){vdp1_sync_wait();submitted=false;}scu_dma_transfer_wait(0);}
static bool live_video_init(bool startup){
 wf_live_video_hide();wf_video_plan_init(&plan);
 vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE,VDP2_TVMD_HORZ_NORMAL_A,VDP2_TVMD_VERT_240);vdp2_cram_mode_set(2);
 const vdp1_env_t env={.bpp=VDP1_ENV_BPP_16,.rotation=VDP1_ENV_ROTATION_0,.color_mode=VDP1_ENV_COLOR_MODE_PALETTE,.sprite_type=0,.erase_color=RGB1555_INITIALIZER(0,0,0,0),.erase_points={INT16_VEC2_INITIALIZER(0,0),INT16_VEC2_INITIALIZER(319,239)}};
 vdp1_env_set(&env);vdp1_vram_partitions_set(COMMANDS,TEXTURE_BYTES,0,0);vdp1_vram_partitions_get(&partitions);
 vdp1_sync_interval_set(-1);vdp1_sync_mode_set(VDP1_SYNC_MODE_CHANGE_ONLY);vdp2_sprite_priority_set(0,3);
 vdp2_scrn_back_color_set(VDP2_VRAM_ADDR(3,0x1fffe),RGB1555(1,0,0,0));
 for(unsigned i=0;i<3;i++){
  const vdp2_scrn_cell_format_t fmt={.scroll_screen=screens[i],.ccc=VDP2_SCRN_CCC_PALETTE_16,.char_size=i==2?VDP2_SCRN_CHAR_SIZE_1X1:VDP2_SCRN_CHAR_SIZE_2X2,.pnd_size=2,.aux_mode=VDP2_SCRN_AUX_MODE_0,.plane_size=VDP2_SCRN_PLANE_SIZE_1X1,.cpd_base=VDP2_VRAM_ADDR(i,0),.palette_base=VDP2_CRAM_MODE_2_OFFSET(i==2?0:i+1,0,0)};
  const vdp2_scrn_normal_map_t map={.base_addr={maps[i],maps[i],maps[i],maps[i]}};
  vdp2_scrn_cell_format_set(&fmt,&map);vdp2_scrn_scroll_x_set(screens[i],0);vdp2_scrn_scroll_y_set(screens[i],FIX16(8));vdp2_scrn_priority_set(screens[i],i==2?5:i+1);
 }
 vdp2_vram_cycp_t cycles;cycles.pt[0].raw=0x4FEEEEEE;cycles.pt[1].raw=0x5FEEEEEE;cycles.pt[2].raw=0x6FFFEEEE;cycles.pt[3].raw=0x012FEEEE;vdp2_vram_cycp_set(&cycles);
 if(startup&&wf_cart_valid)wf_startup_notice_begin(wf_cart_id,wf_cart_bytes);
 #if defined(WF_FIXED_ATTRACT) && WF_FIXED_ATTRACT
 if(!wf_live_video_program_load()||!file("BACK.TEX",(void*)0x00280000,0x80000,WF_BACK_CRC))
#else
 if(!file("PROGRAM.BIN",(void*)0x00200000,0x80000,WF_PROGRAM_CRC)||!file("BACK.TEX",(void*)0x00280000,0x80000,WF_BACK_CRC))
#endif
 {wf_video_error=1;return false;}
 uint32_t font_crc=0xffffffffu,size;
 if(video_io.size(NULL,"FONT.TEX",&size)||size!=0x20000){wf_video_error=1;return false;}
 for(unsigned at=0;at<size;){unsigned n=size-at;if(n>sizeof stage)n=sizeof stage;
  if(video_io.read(NULL,"FONT.TEX",at,stage,n)){wf_video_error=1;return false;}
  font_crc=wf_assets_crc32_update(font_crc,stage,n);
  transfer((void*)VDP2_VRAM_ADDR(2,at),stage,n);at+=n;
 }
 if((font_crc^0xffffffffu)!=WF_FONT_CRC){wf_video_error=1;return false;}
 if(startup&&!wf_live_video_atlas(0))return false;
 if(startup)wf_startup_notice_end();return true;
}
bool wf_live_video_init(void){return live_video_init(true);}
bool wf_live_video_init_return(void){return live_video_init(false);}
bool wf_live_video_atlas(unsigned which){
 if(which>=sizeof atlases/sizeof atlases[0])return false;
 if(atlas_loaded&&wf_video_atlas==which)return true;
 if(!wf_startup_notice_active)wf_live_video_hide();
 if(submitted){vdp1_sync_wait();submitted=false;}
 const char*name=atlases[which].name;uint32_t size;
 if(video_io.size(NULL,name,&size)||size!=atlases[which].file_bytes||atlases[which].bytes>TEXTURE_BYTES){wf_video_error=2;return false;}
#ifdef WF_POST_DEDUP
 if(which==WF_LIVE_VIDEO_POST_ATLAS&&
    (!wf_post_atlas_mapping_valid(wf_post_sprite_ids,WF_POST_DEDUP_LOGICAL_COUNT,POST_MAPPING,POST_MAPPING_COUNT)||
     wf_post_atlas_table_crc(wf_post_sprite_ids,WF_POST_DEDUP_LOGICAL_COUNT)!=WF_POST_IDS_CRC||
     wf_post_atlas_table_crc(POST_MAPPING,POST_MAPPING_COUNT)!=WF_POST_MAPPING_CRC)){
  wf_video_error=2;return false;
 }
#endif
 size=atlases[which].bytes; /* A POST checksum tail is never a VDP1 texture. */
#ifdef WF_POST_SPRITE_BYTES
 memset(&post_atlas,0,sizeof post_atlas);
#endif
 uint32_t crc=0xffffffffu;
 for(unsigned at=0;at<size;){unsigned n=size-at;if(n>sizeof stage)n=sizeof stage;
  if(video_io.read(NULL,name,at,stage,n)){wf_video_error=3;return false;}
  crc=wf_assets_crc32_update(crc,stage,n);
  transfer((void*)((uintptr_t)partitions.texture_base+at),stage,n);at+=n;
 }
 if((crc^0xffffffffu)!=atlases[which].crc){wf_video_error=4;return false;}
 wf_video_atlas=which;atlas_loaded=true;return true;
}
static int sprite_slot(unsigned code){
#ifdef WF_POST_SPRITE_BYTES
 if(wf_video_atlas==WF_LIVE_VIDEO_POST_ATLAS){
  if(post_atlas.fixed)return wf_post_atlas_slot_mapped(&post_atlas,code,POST_MAPPING,POST_MAPPING_COUNT);
  return wf_post_atlas_fixed_slot(wf_post_sprite_ids,sizeof wf_post_sprite_ids/2,
   code,POST_MAPPING,POST_MAPPING_COUNT);
 }
#endif
 const uint16_t*ids=atlases[wf_video_atlas].ids;unsigned count=atlases[wf_video_atlas].count,lo=0,hi=count;
 while(lo<hi){unsigned mid=(lo+hi)/2;if(ids[mid]<code)lo=mid+1;else hi=mid;}
 return lo<count&&ids[lo]==code?(int)lo:-1;
}
#ifdef WF_POST_SPRITE_BYTES
static bool return_upload(void*u __unused,unsigned slot,const uint8_t*data){
 if(slot>=WF_POST_ATLAS_MAX_TILES)return false;
 transfer((void*)((uintptr_t)partitions.texture_base+slot*128u),data,128u);return true;
}
#endif
bool wf_live_video_return_prepare(wf_session_t*s,const wf_board_t*b){
#ifdef WF_POST_SPRITE_BYTES
 if(!s||!b||wf_video_error){if(!wf_video_error)wf_video_error=10;return false;}
 wf_live_video_hide();
 if(!wf_live_video_atlas(WF_LIVE_VIDEO_POST_ATLAS))return false;
 const uint8_t*records=s->video.fg0+0x2000;unsigned count=rw(s->ram+0x19b2),n;
 if(!wf_video_plan_sprites(records,count,pieces,1024,&n)||
    !wf_post_atlas_plan_mapped(&post_atlas,wf_post_sprite_ids,sizeof wf_post_sprite_ids/2,
     POST_MAPPING,POST_MAPPING_COUNT,pieces,n)){
  wf_video_error=10;return false;
 }
 wf_asset_error_t error=wf_post_atlas_load_mapped(&post_atlas,&video_io,"POST.TEX",
  WF_POST_SPRITE_BYTES,WF_POST_TILE_CRC,stage,sizeof stage,return_upload,NULL,POST_MAPPING,POST_MAPPING_COUNT);
 if(error!=WF_ASSET_OK){wf_video_error=10u+(unsigned)error;return false;}
 /* These acknowledgements describe old hardware residency, not arcade
  * colors. The returned raw palettes must all reach the fresh CRAM. */
 memset(s->video.palette_committed,0,sizeof s->video.palette_committed);
 if(!wf_live_video_commit(s,b,records,count))return false;
 if(submitted){vdp1_sync_wait();submitted=false;}
 vdp2_sync_wait();scu_dma_transfer_wait(0);return true;
#else
 (void)s;(void)b;wf_video_error=10;return false;
#endif
}
bool wf_live_video_finale_prepare(wf_session_t*s){
#ifdef WF_FINALE_SPRITE_COUNT
 if(!s||wf_video_error){if(!wf_video_error)wf_video_error=10;return false;}
 /* Drain the previous picture before replacing its supplement or borrowing
  * the shared CD stage for the resource-only decoder. Original1FDE already
  * discarded its latched sprites; arcade RAM/registers remain untouched. */
 wf_live_video_finish();
 if(!wf_live_video_atlas(WF_LIVE_VIDEO_POST_ATLAS))return false;
 wf_r18_frontend_platform_t*p=&s->frontend.platform;
 _Static_assert(sizeof stage>=512u*sizeof(wf_r18_sprite_record_t),"Finale scratch fits shared lease");
 if(!wf_finale_atlas_plan_mapped(&post_atlas,wf_post_sprite_ids,sizeof wf_post_sprite_ids/2,
    POST_MAPPING,POST_MAPPING_COUNT,
    wf_finale_sprite_ids,WF_FINALE_SPRITE_COUNT,s->ram,sizeof s->ram,
    p->read_rom,p->user,(wf_r18_sprite_record_t*)stage,512u)){
  wf_video_error=10;return false;
 }
 wf_asset_error_t error=wf_post_atlas_load_mapped(&post_atlas,&video_io,"POST.TEX",
  WF_POST_SPRITE_BYTES,WF_POST_TILE_CRC,stage,sizeof stage,return_upload,NULL,POST_MAPPING,POST_MAPPING_COUNT);
 if(error!=WF_ASSET_OK){wf_video_error=10u+(unsigned)error;return false;}
 return true;
#else
 (void)s;wf_video_error=10;return false;
#endif
}
bool wf_live_video_prepare(wf_session_t*s){
 /* The previous commit borrows plan's maps, text and palette until VBlank.
  * Transport can call prepare outside commit, so fence their reuse here. */
 vdp2_sync_wait();
 for(unsigned i=0;i<2;i++){
  const uint8_t*raw=s->video.background+(i?0:0x2000);unsigned bytes=i?4096:2048;
  if(!map_ready[i]||!equal(previous_bg[i],raw,bytes)){
   if(!wf_video_plan_map(&plan,i,raw,upload,NULL)){if(!wf_video_error)wf_video_error=6;return false;}
   memcpy(previous_bg[i],raw,bytes);map_dirty[i]=true;map_ready[i]=true;
  }
 }return true;
}
bool wf_live_video_commit(wf_session_t*s,const wf_board_t*b,const uint8_t*records,unsigned count){
 unsigned begin=cpu_frt_count_get();
 if(submitted){vdp1_sync_wait();submitted=false;}
 wf_render_wait=(uint16_t)(cpu_frt_count_get()-begin);begin=cpu_frt_count_get();
 if(b->scroll[11]&1){wf_video_error=5;return false;}
 if(!wf_live_video_prepare(s))return false;
 bool dirty_text=!valid_maps||!equal(previous_text,s->video.fg0,8192);
 if(dirty_text){wf_video_plan_text(&plan,s->video.fg0);memcpy(previous_text,s->video.fg0,8192);}
 const wf_r18_palette_role_t roles[4]={WF_R18_PALETTE_TEXT,WF_R18_PALETTE_BG,WF_R18_PALETTE_FG,WF_R18_PALETTE_SPRITES};
 const uint8_t*raw_palettes[4]={s->video.palettes,s->video.bg_palette,s->video.fg_palette,s->video.palettes+0x2000};
 uint16_t pending[4];
 for(unsigned i=0;i<4;i++){pending[i]=wf_r18_video_palette_pending(&s->video,roles[i]);if(pending[i])wf_video_plan_palette(plan.palette+i*256,raw_palettes[i]);}
 unsigned n;if(!wf_video_plan_sprites(records,count,pieces,1024,&n)){wf_video_error=7;return false;}
 memset(commands,0,(n+4)*sizeof *commands);
 vdp1_cmdt_system_clip_coord_set(commands);vdp1_cmdt_vtx_system_clip_coord_set(commands,INT16_VEC2(319,239));vdp1_cmdt_local_coord_set(commands+1);vdp1_cmdt_vtx_local_coord_set(commands+1,INT16_VEC2(0,0));
 /* Variable manual swaps do not erase the drawing bank. VBlank erase alone
  * cannot cover240 lines. Use the complete clear polygon from Entrada R02. */
 const vdp1_cmdt_draw_mode_t clear={.pre_clipping_disable=true,.trans_pixel_disable=true,.end_code_disable=true};
 const int16_vec2_t corners[4]={INT16_VEC2_INITIALIZER(0,0),INT16_VEC2_INITIALIZER(319,0),INT16_VEC2_INITIALIZER(319,239),INT16_VEC2_INITIALIZER(0,239)};
 vdp1_cmdt_polygon_set(commands+2);vdp1_cmdt_draw_mode_set(commands+2,clear);vdp1_cmdt_vtx_set(commands+2,corners);commands[2].cmd_colr=0;
 const vdp1_cmdt_draw_mode_t mode={.color_mode=0,.pre_clipping_disable=true,.end_code_disable=true};
 for(unsigned i=0;i<n;i++){
  wf_video_piece_t*p=pieces+i;int slot=sprite_slot(p->code);if(slot<0){wf_video_error=8;wf_video_tile=p->code;return false;}
  vdp1_cmdt_t*c=commands+i+3;const vdp1_cmdt_color_bank_t bank={.type_0.dc=768+p->bank*16};
  vdp1_cmdt_normal_sprite_set(c);vdp1_cmdt_draw_mode_set(c,mode);vdp1_cmdt_color_mode0_set(c,bank);
  vdp1_cmdt_char_base_set(c,(vdp1_vram_t)partitions.texture_base+(unsigned)slot*128);vdp1_cmdt_char_size_set(c,16,16);vdp1_cmdt_char_flip_set(c,(vdp1_cmdt_char_flip_t)(p->flip<<4));c->cmd_xa=p->x;c->cmd_ya=p->y;
 }
 vdp1_cmdt_end_set(commands+n+3);
 unsigned priority=b->control[17],fgx=rw(b->scroll),fgy=rw(b->scroll+2),bgx=rw(b->scroll+4),bgy=rw(b->scroll+6);
 if(priority!=0x78){unsigned t=fgx;fgx=bgx;bgx=t;t=fgy;fgy=bgy;bgy=t;}
 vdp2_scrn_scroll_x_set(screens[0],FIX16(bgx&511));vdp2_scrn_scroll_y_set(screens[0],FIX16((bgy+8)&511));vdp2_scrn_scroll_x_set(screens[1],FIX16(fgx&511));vdp2_scrn_scroll_y_set(screens[1],FIX16((fgy+8)&511));
 unsigned display=VDP2_SCRN_DISPTP_NBG2;
 if(priority==0x78){display|=VDP2_SCRN_DISP_NBG0|VDP2_SCRN_DISPTP_NBG1;vdp2_scrn_priority_set(screens[0],1);vdp2_scrn_priority_set(screens[1],2);}
 else if(priority==0x7b||priority==0x7c){display|=VDP2_SCRN_DISPTP_NBG0|VDP2_SCRN_DISP_NBG1;vdp2_scrn_priority_set(screens[0],priority==0x7c?4:2);vdp2_scrn_priority_set(screens[1],1);}
 /* Publish complete maps in VBlank, as in Entrada R02. Immediate DMA here
  * can mix two crowd poses (or two text rows) within one scanned field. */
 for(unsigned i=0;i<2;i++)if(map_dirty[i]){vdp_dma_enqueue((void*)maps[i],plan.maps[i],sizeof plan.maps[i]);map_dirty[i]=false;}
 if(dirty_text)vdp_dma_enqueue((void*)maps[2],plan.text,sizeof plan.text);
 unsigned first=1024,last=0;
 for(unsigned i=0;i<1024;i++)if(!valid_maps||previous_palette[i]!=plan.palette[i]){if(first==1024)first=i;last=i+1;previous_palette[i]=plan.palette[i];}
 if(first<last)vdp_dma_enqueue((void*)(0x25f00000+first*4),plan.palette+first,(last-first)*4);
 for(unsigned i=0;i<4;i++)wf_r18_video_palette_ack(&s->video,roles[i],pending[i]);
 valid_maps=true;
 vdp2_scrn_display_set((vdp2_scrn_disp_t)display);
 vdp1_sync_cmdt_put(commands,n+4,0);vdp1_sync_put_wait();vdp1_sync_render();vdp1_sync();vdp2_sync();if(!hidden)vdp2_tvmd_display_set();submitted=true;
 wf_render_work=(uint16_t)(cpu_frt_count_get()-begin);
 wf_video_sprites=n;wf_video_commits++;return true;
}

#if defined(WF_FIXED_ATTRACT) && WF_FIXED_ATTRACT
bool wf_live_video_program_load(void){
 if(program_valid)return true;
 program_valid=file("PROGRAM.BIN",(void*)0x00200000,0x80000,WF_PROGRAM_CRC);
 return program_valid;
}
bool wf_live_video_restore_attract(wf_session_t*s){
 atlas_loaded=false;valid_maps=false;submitted=false;
 memset(map_ready,0,sizeof map_ready);memset(map_dirty,0,sizeof map_dirty);
 memset(s->video.palette_committed,0,sizeof s->video.palette_committed);
 /* SCRCTL: R05 set NBG0/NBG1 line-scroll; ordinary cell formats do not reset it. */
 vdp2_regs_get()->scrctl=0;
 if(!live_video_init(false))return false;
 return wf_live_video_atlas(0);
}
#endif
