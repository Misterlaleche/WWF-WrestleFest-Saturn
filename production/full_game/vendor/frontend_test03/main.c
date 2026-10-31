#include <yaul.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "nav_tables.h"
#include "selection_masks.h"

#define SCREEN_H 240U
#define MODE_BITMAP_BYTES (512U * SCREEN_H)
#define MODE_BITMAP_BASE VDP2_VRAM_ADDR(0, 0)
#define MODE_RLE_SIZE 43003U
#define MODE_TIMEOUT_QUANTA 0x0200U
#define MODE_PULSE_PHASES 10U
#define MODE_PULSE_DWELL 5U
#define SRC_BG_W 512U
#define SRC_WORLD_W 528U
#define VDP_BG_W 512U
#define VDP_FG_W 1024U
#define VDP_H 256U
#define SRC_BG_BYTES (SRC_BG_W * SCREEN_H)
#define SRC_WORLD_BYTES (SRC_WORLD_W * SCREEN_H)
#define NBG0_BASE VDP2_VRAM_ADDR(0, 0)
#define BG_CPD_BASE VDP2_VRAM_ADDR(2, 0)
#define BG_MAP_BASE VDP2_VRAM_ADDR(3, 0)
#define PALETTE_BASE VDP2_CRAM_MODE_1_OFFSET(0, 0, 0)
#define SELECT_ENTRY_WAIT 17U
#define LEFT_SCROLL 0x0140U
#define RIGHT_SCROLL 0x0210U

extern uint8_t asset_modex[];
extern uint8_t asset_modepal[];
extern uint8_t asset_cursor0[];
extern uint8_t asset_cursor0pal[];
extern uint8_t asset_selbg[];
extern uint8_t asset_selworld[];
extern uint8_t asset_selpal[];

static smpc_peripheral_digital_t pad;
static uint8_t mode_bitmap[MODE_BITMAP_BYTES] __attribute__((aligned(4)));
static uint8_t select_bg_bitmap[SRC_BG_BYTES] __attribute__((aligned(4)));
static uint8_t select_world[SRC_WORLD_BYTES] __attribute__((aligned(4)));
static uint16_t base_palette[256] __attribute__((aligned(4)));
static uint8_t patch96[96U * 96U] __attribute__((aligned(4)));
static unsigned arcade_logic_phase;
static uint8_t previous_direction;

static void intback(void *work __unused){smpc_peripheral_intback_issue();}
static void sync_n(unsigned n){while(n--){vdp2_sync();vdp2_sync_wait();}}
static void poll_pad(void){smpc_peripheral_process();smpc_peripheral_digital_port(1,&pad);}
static void dma_copy(void *dst,const void *src,uint32_t n){scu_dma_transfer(0,dst,src,n);scu_dma_transfer_wait(0);}
static bool unpack_rle(const uint8_t *src,uint32_t size,uint8_t *dst,uint32_t cap){
 uint32_t in=0,out=0;while(in<size){unsigned tag=src[in++];uint32_t count=(tag&127U)+1U;if(count>cap-out)return false;
 if(tag&128U){if(in>=size)return false;memset(dst+out,src[in++],count);}else{if(count>size-in)return false;memcpy(dst+out,src+in,count);in+=count;}out+=count;}return out==cap;
}
static void arcade_logic_wait(void){for(;;){sync_n(1);arcade_logic_phase+=57445U;if(arcade_logic_phase>=60000U){arcade_logic_phase-=60000U;return;}}}

static const uint8_t mode_pulse[MODE_PULSE_PHASES]={1,2,3,4,5,4,3,2,1,0};

/* ROM $005518: TAG / Saturday Night's Main Event, six 15-word phases. */
static const uint16_t mode_tag_rgb444[6][15]={
 {0x09AA,0x0A60,0x0DA0,0x0706,0x0090,0x00F0,0x0AFF,0x0FFF,0x0577,0x00BB,0x000F,0x0FF0,0x00DD,0x000A,0x00FF},
 {0x0BCC,0x0B82,0x0FC2,0x0928,0x0090,0x00F0,0x0CFF,0x0FFF,0x0799,0x02DD,0x022F,0x0FF3,0x02FF,0x022C,0x03FF},
 {0x0DEE,0x0DA4,0x0FE4,0x0B4A,0x0090,0x00F0,0x0EFF,0x0FFF,0x09BB,0x04FF,0x044F,0x0FF6,0x04FF,0x044F,0x06FF},
 {0x0FFF,0x0FC6,0x0FF6,0x0D6C,0x0090,0x00F0,0x0FFF,0x0FFF,0x0BDD,0x06FF,0x066F,0x0FF9,0x06FF,0x066F,0x09FF},
 {0x0FFF,0x0FE8,0x0FF8,0x0F8E,0x0090,0x00F0,0x0FFF,0x0FFF,0x0DFF,0x08FF,0x088F,0x0FFC,0x08FF,0x088F,0x0CFF},
 {0x0FFF,0x0FFA,0x0FFA,0x0FAF,0x0090,0x00F0,0x0FFF,0x0FFF,0x0FFF,0x0AFF,0x0AAF,0x0FFF,0x0AFF,0x0AAF,0x0FFF}
};
/* ROM $0055D8: ROYAL RUMBLE, six 15-word phases. */
static const uint16_t mode_royal_rgb444[6][15]={
 {0x09AA,0x0048,0x006C,0x009E,0x02BF,0x06DF,0x0AFF,0x0FFF,0x0000,0x0204,0x0090,0x00F0,0x00BB,0x00DD,0x00FF},
 {0x0BCC,0x026A,0x028E,0x02BF,0x04DF,0x08FF,0x0CFF,0x0FFF,0x0002,0x0426,0x0090,0x02F2,0x02DD,0x02FF,0x02FF},
 {0x0DEE,0x048C,0x04AF,0x04DF,0x06FF,0x0AFF,0x0EFF,0x0FFF,0x0024,0x0648,0x0090,0x04F4,0x04FF,0x04FF,0x04FF},
 {0x0FFF,0x06AE,0x06CF,0x06FF,0x08FF,0x0CFF,0x0FFF,0x0FFF,0x0246,0x086A,0x0090,0x06F6,0x06FF,0x06FF,0x08FF},
 {0x0FFF,0x08CF,0x08EF,0x08FF,0x0AFF,0x0EFF,0x0FFF,0x0FFF,0x0468,0x0A8C,0x0090,0x08F8,0x08FF,0x08FF,0x0AFF},
 {0x0FFF,0x0AEF,0x0AFF,0x0BFF,0x0CFF,0x0FFF,0x0FFF,0x0FFF,0x068A,0x0CAE,0x0090,0x0AFA,0x0AFF,0x0AFF,0x0CFF}
};


static uint16_t rgb444_to_saturn(uint16_t v){
 unsigned r=v&15U,g=(v>>4)&15U,b=(v>>8)&15U;
 r=(r<<1)|(r>>3);g=(g<<1)|(g>>3);b=(b<<1)|(b>>3);
 return (uint16_t)(0x8000U|r|(g<<5)|(b<<10));
}
static void mode_set_family_palette(bool royal,unsigned phase){
 const uint16_t (*src)[15]=royal?mode_royal_rgb444:mode_tag_rgb444;
 /* Exact FG palette destinations: Royal bank9 $188482, TAG bank12 $188602.
    Synthetic bitmap indices preserve those bank/pen identities one-for-one. */
 unsigned base=royal?200U:216U;
 if(phase>5U)phase=0U;
 for(unsigned i=0;i<15U;i++)base_palette[base+i]=rgb444_to_saturn(src[phase][i]);
 dma_copy((void*)(PALETTE_BASE+base*2U),&base_palette[base],30U);
}
static unsigned direction_code(void){
 unsigned d=0U;if(pad.pressed.button.right)d|=1U;if(pad.pressed.button.left)d|=2U;if(pad.pressed.button.up)d|=4U;if(pad.pressed.button.down)d|=8U;return d;
}
static bool code_sets_tag(unsigned d){return d==1U||d==5U||d==9U;}
static bool code_sets_royal(unsigned d){return d==2U||d==6U||d==10U;}


/* Exact relevant $0064E0 SELECT PLAYERS direction normalization. */
static unsigned selector_direction_edge(void){
 unsigned d=0U;
 if(pad.pressed.button.right)d|=1U;if(pad.pressed.button.left)d|=2U;
 if(pad.pressed.button.up)d|=4U;if(pad.pressed.button.down)d|=8U;
 d&=0x0FU;
 if(d==previous_direction)return 0U;
 previous_direction=(uint8_t)d;
 return d;
}

static void vram_clear32(volatile uint8_t *dst,uint32_t bytes){volatile uint32_t *p=(volatile uint32_t*)dst;for(uint32_t i=0;i<bytes/4U;i++)p[i]=0U;}

static void palette_upload(void){
 memcpy(base_palette,asset_selpal,512U);
 for(unsigned i=0;i<16U;i++)base_palette[232U+i]=((uint16_t*)asset_cursor0pal)[i];
 base_palette[253]=0xFFA0U;base_palette[255]=0x83E0U;
 dma_copy((void*)PALETTE_BASE,base_palette,512U);
}

/* Exact fixed background, represented as 8x8 cells on NBG2.  This avoids a
   second bitmap plane competing with the resident 1024x256 NBG0 bitmap while
   preserving every source pixel exactly. */
static void upload_bg_once(void){
 volatile uint8_t *cpd=(volatile uint8_t*)BG_CPD_BASE;
 volatile uint16_t *map=(volatile uint16_t*)BG_MAP_BASE;
 vram_clear32(cpd,0x20000U);
 vram_clear32((volatile uint8_t*)map,0x2000U);
 unsigned tile=0U;
 for(unsigned ty=0;ty<30U;ty++){
  for(unsigned tx=0;tx<64U;tx++,tile++){
   uint8_t buf[64];
   for(unsigned py=0;py<8U;py++)
    memcpy(buf+py*8U,select_bg_bitmap+(ty*8U+py)*SRC_BG_W+tx*8U,8U);
   vdp2_vram_t addr=BG_CPD_BASE+tile*64U;
   dma_copy((void*)addr,buf,64U);
   map[ty*64U+tx]=(uint16_t)VDP2_SCRN_PND_CONFIG_1(1,addr,PALETTE_BASE);
  }
 }
}

/* Load the 528-pixel transparent roster foreground once into a 1024x256 NBG0 bitmap.
   The remaining pixels stay transparent index 0. No framebuffer rebuild is performed
   during the 26-step page transition. */
static void upload_world_once(void){
 volatile uint8_t *dst=(volatile uint8_t*)NBG0_BASE;
 vram_clear32(dst,VDP_FG_W*VDP_H);
 for(unsigned y=0;y<SCREEN_H;y++)
  dma_copy((void*)(dst+y*VDP_FG_W),select_world+y*SRC_WORLD_W,SRC_WORLD_W);
}

static void cursor_geometry(unsigned slot,int *x0,int *y0){
 unsigned v=(slot==9U)?8U:slot;unsigned col=v%5U,row=v/5U;
 int machine_x=384+96*(int)col;int machine_y=616-96*(int)row;
 int world_anchor_x=machine_x-(int)LEFT_SCROLL; /* screen x + hardware-scroll offset */
 int anchor_y=685-machine_y;
 *x0=world_anchor_x-40;*y0=anchor_y-35;
}

/* Restore/draw only the 96x96 cursor rectangle. This happens only on a selection edge,
   never in the scroll loop. The foreground itself remains resident in VDP2 VRAM. */
static void cursor_patch_upload(unsigned slot,bool draw){
 int x0,y0;cursor_geometry(slot,&x0,&y0);
 for(unsigned y=0;y<96U;y++)for(unsigned x=0;x<96U;x++){
  int wx=x0+(int)x,wy=y0+(int)y;uint8_t p=0;
  if(wx>=0&&wx<(int)SRC_WORLD_W&&wy>=0&&wy<(int)SCREEN_H)p=select_world[(unsigned)wy*SRC_WORLD_W+(unsigned)wx];
  if(draw){uint8_t cp=asset_cursor0[y*96U+x];if(cp)p=(uint8_t)(232U+cp);}
  patch96[y*96U+x]=p;
 }
 volatile uint8_t *dst=(volatile uint8_t*)NBG0_BASE;
 for(unsigned y=0;y<96U;y++){
  int wy=y0+(int)y;if(wy<0||wy>=(int)VDP_H)continue;
  unsigned sx=0U,w=96U;if(x0<0){sx=(unsigned)(-x0);w-=sx;}if(x0+(int)96>=(int)VDP_FG_W){unsigned cut=(unsigned)(x0+(int)96-(int)VDP_FG_W);if(cut<w)w-=cut;else w=0U;}
  if(w)dma_copy((void*)(dst+(unsigned)wy*VDP_FG_W+(unsigned)(x0+(int)sx)),patch96+y*96U+sx,w);
 }
}

static void set_fg_scroll(unsigned logical_scroll){
 int32_t offset=(int32_t)logical_scroll-(int32_t)LEFT_SCROLL;
 vdp2_scrn_scroll_x_set(VDP2_SCRN_NBG0,fix16_int32_from(offset));
}

static void confirmed_rebuild(unsigned page,unsigned slot){
 /* The roster foreground is already resident. Never clear/re-upload the full NBG0 here:
    doing so exposes NBG2 alone for a frame on real Saturn (the confirmation flash seen
    in TEST02). Compose only the selected 96x96 patch, preserving the green highlight
    and cursor while leaving every other foreground pixel untouched. */
 unsigned visual=(slot==9U)?8U:slot;unsigned col=visual%5U,row=visual/5U;unsigned cidx=99U;
 if(page==0U){if(col<=2U)cidx=row*3U+col;}else if(col>=2U){unsigned pc=col-2U;if(pc<3U)cidx=page*6U+row*3U+pc;}

 int x0,y0;cursor_geometry(slot,&x0,&y0);
 for(unsigned y=0;y<96U;y++)for(unsigned x=0;x<96U;x++){
  int wx=x0+(int)x,wy=y0+(int)y;uint8_t p=0;
  if(wx>=0&&wx<(int)SRC_WORLD_W&&wy>=0&&wy<(int)SCREEN_H)p=select_world[(unsigned)wy*SRC_WORLD_W+(unsigned)wx];
  patch96[y*96U+x]=p;
 }

 if(cidx<12U){
  const uint8_t *mask=selection_masks+cidx*800U;
  unsigned world_col=visual%5U;
  unsigned hx=32U+world_col*96U,hy=40U+row*96U;
  for(unsigned py=0;py<80U;py++)for(unsigned px=0;px<80U;px++){
   unsigned bit=py*80U+px;
   if(mask[bit/8U]&(128U>>(bit%8U))){
    int lx=(int)(hx+px)-x0,ly=(int)(hy+py)-y0;
    if(lx>=0&&lx<96&&ly>=0&&ly<96)patch96[(unsigned)ly*96U+(unsigned)lx]=255U;
   }
  }
 }

 /* Cursor is the final layer, so the bright-green portrait/background never destroys
    the selection frame and the frame never destroys the green confirmation fill. */
 for(unsigned y=0;y<96U;y++)for(unsigned x=0;x<96U;x++){
  uint8_t cp=asset_cursor0[y*96U+x];if(cp)patch96[y*96U+x]=(uint8_t)(232U+cp);
 }
 volatile uint8_t *dst=(volatile uint8_t*)NBG0_BASE;
 vdp2_tvmd_vblank_in_wait();
 for(unsigned y=0;y<96U;y++){
  int wy=y0+(int)y;if(wy<0||wy>=(int)VDP_H)continue;
  unsigned sx=0U,w=96U;if(x0<0){sx=(unsigned)(-x0);w-=sx;}if(x0+(int)96>=(int)VDP_FG_W){unsigned cut=(unsigned)(x0+(int)96-(int)VDP_FG_W);if(cut<w)w-=cut;else w=0U;}
  if(w)dma_copy((void*)(dst+(unsigned)wy*VDP_FG_W+(unsigned)(x0+(int)sx)),patch96+y*96U+sx,w);
 }
}

static void roster_init(void){
 /* SMPC/vblank ownership persists from boot; only VDP2 is reconfigured at handoff.
    Commit display blanking BEFORE touching CRAM/VRAM. TEST01 changed the roster palette
    while the previous MODE bitmap was still being scanned out on real hardware, creating
    the observed yellow/orange flash. */
 vdp2_tvmd_display_clear();vdp2_sync();vdp2_sync_wait();
 vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE,VDP2_TVMD_HORZ_NORMAL_A,VDP2_TVMD_VERT_240);vdp2_cram_mode_set(1);
 vdp2_scrn_back_color_set(VDP2_VRAM_ADDR(3,0x1FFFE),RGB1555(1,0,0,0));
 const vdp2_scrn_bitmap_format_t fg={.scroll_screen=VDP2_SCRN_NBG0,.ccc=VDP2_SCRN_CCC_PALETTE_256,.bitmap_size=VDP2_SCRN_BITMAP_SIZE_1024X256,.palette_base=PALETTE_BASE,.bitmap_base=NBG0_BASE};
 const vdp2_scrn_cell_format_t bg={.scroll_screen=VDP2_SCRN_NBG2,.ccc=VDP2_SCRN_CCC_PALETTE_256,.char_size=VDP2_SCRN_CHAR_SIZE_1X1,.pnd_size=1,.aux_mode=VDP2_SCRN_AUX_MODE_1,.plane_size=VDP2_SCRN_PLANE_SIZE_1X1,.cpd_base=BG_CPD_BASE,.palette_base=PALETTE_BASE};
 const vdp2_scrn_normal_map_t bgmap={.base_addr={BG_MAP_BASE,BG_MAP_BASE,BG_MAP_BASE,BG_MAP_BASE}};
 vdp2_scrn_bitmap_format_set(&fg);vdp2_scrn_cell_format_set(&bg,&bgmap);
 set_fg_scroll(LEFT_SCROLL);vdp2_scrn_scroll_y_set(VDP2_SCRN_NBG0,fix16_int32_from(0));
 vdp2_scrn_scroll_x_set(VDP2_SCRN_NBG2,fix16_int32_from(0));vdp2_scrn_scroll_y_set(VDP2_SCRN_NBG2,fix16_int32_from(0));
 vdp2_scrn_priority_set(VDP2_SCRN_NBG0,3);vdp2_scrn_priority_set(VDP2_SCRN_NBG2,2);
 vdp2_vram_cycp_t cycles=*vdp2_vram_cycp_get();
 for(unsigned bank=0;bank<4;bank++)cycles.pt[bank].raw=0xFFFFFFFFU;
 for(unsigned bank=0;bank<2;bank++){
  cycles.pt[bank].t0=VDP2_VRAM_CYCP_CHPNDR_NBG0;cycles.pt[bank].t1=VDP2_VRAM_CYCP_CHPNDR_NBG0;cycles.pt[bank].t2=VDP2_VRAM_CYCP_CHPNDR_NBG0;cycles.pt[bank].t3=VDP2_VRAM_CYCP_CHPNDR_NBG0;cycles.pt[bank].t7=VDP2_VRAM_CYCP_CPU_RW;
 }
 cycles.pt[2].t0=VDP2_VRAM_CYCP_CHPNDR_NBG2;cycles.pt[2].t1=VDP2_VRAM_CYCP_CHPNDR_NBG2;cycles.pt[2].t2=VDP2_VRAM_CYCP_CHPNDR_NBG2;cycles.pt[2].t3=VDP2_VRAM_CYCP_CHPNDR_NBG2;cycles.pt[2].t7=VDP2_VRAM_CYCP_CPU_RW;
 cycles.pt[3].t0=VDP2_VRAM_CYCP_PNDR_NBG2;cycles.pt[3].t1=VDP2_VRAM_CYCP_PNDR_NBG2;cycles.pt[3].t7=VDP2_VRAM_CYCP_CPU_RW;vdp2_vram_cycp_set(&cycles);
 if(!unpack_rle(asset_selbg,37910U,select_bg_bitmap,SRC_BG_BYTES))while(true)sync_n(1);
 if(!unpack_rle(asset_selworld,39619U,select_world,SRC_WORLD_BYTES))while(true)sync_n(1);
 palette_upload();upload_bg_once();upload_world_once();cursor_patch_upload(0U,true);
 vdp2_scrn_display_set(VDP2_SCRN_DISPTP_NBG0|VDP2_SCRN_DISP_NBG2);vdp2_sync();vdp2_sync_wait();vdp2_tvmd_display_set();
}


void user_init(void){
 smpc_peripheral_init();vdp_sync_vblank_out_set(intback,NULL);
 vdp2_tvmd_display_clear();vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE,VDP2_TVMD_HORZ_NORMAL_A,VDP2_TVMD_VERT_240);vdp2_cram_mode_set(1);
 vdp2_scrn_back_color_set(VDP2_VRAM_ADDR(3,0x1FFFE),RGB1555(1,0,0,0));
 const vdp2_scrn_bitmap_format_t f={.scroll_screen=VDP2_SCRN_NBG0,.ccc=VDP2_SCRN_CCC_PALETTE_256,.bitmap_size=VDP2_SCRN_BITMAP_SIZE_512X512,.palette_base=PALETTE_BASE,.bitmap_base=MODE_BITMAP_BASE};
 vdp2_scrn_bitmap_format_set(&f);vdp2_scrn_scroll_x_set(VDP2_SCRN_NBG0,0);vdp2_scrn_scroll_y_set(VDP2_SCRN_NBG0,0);vdp2_scrn_priority_set(VDP2_SCRN_NBG0,3);
 vdp2_vram_cycp_t cycles=*vdp2_vram_cycp_get();for(unsigned bank=0;bank<2;bank++){cycles.pt[bank].t0=VDP2_VRAM_CYCP_CHPNDR_NBG0;cycles.pt[bank].t1=VDP2_VRAM_CYCP_CHPNDR_NBG0;cycles.pt[bank].t2=VDP2_VRAM_CYCP_CHPNDR_NBG0;cycles.pt[bank].t3=VDP2_VRAM_CYCP_CHPNDR_NBG0;cycles.pt[bank].t4=VDP2_VRAM_CYCP_NO_ACCESS;cycles.pt[bank].t5=VDP2_VRAM_CYCP_NO_ACCESS;cycles.pt[bank].t6=VDP2_VRAM_CYCP_CPU_RW;cycles.pt[bank].t7=VDP2_VRAM_CYCP_CPU_RW;}vdp2_vram_cycp_set(&cycles);
 if(!unpack_rle(asset_modex,MODE_RLE_SIZE,mode_bitmap,MODE_BITMAP_BYTES))while(true)sync_n(1);
 memcpy(base_palette,asset_modepal,512U);
 /* $2A06 world palette mode3 initializes BOTH descriptor families at phase0. */
 mode_set_family_palette(true,0U);mode_set_family_palette(false,0U);
 dma_copy((void*)MODE_BITMAP_BASE,mode_bitmap,MODE_BITMAP_BYTES);dma_copy((void*)PALETTE_BASE,base_palette,512U);
 vdp2_scrn_display_set(VDP2_SCRN_DISP_NBG0);vdp2_sync();vdp2_sync_wait();vdp2_tvmd_display_set();
}


int main(void){
 bool royal=false;
 unsigned phase_index=0U,dwell=0U;
 arcade_logic_phase=0U;
 /* Integrated MODE -> ROSTER handoff. The isolated MODE timeout/freeze is intentionally removed. */
 while(true){
  poll_pad();
  if(pad.held.button.a||pad.held.button.b)break;
  unsigned d=direction_code();bool changed=false;
  if(royal&&code_sets_tag(d)){royal=false;changed=true;}
  else if(!royal&&code_sets_royal(d)){royal=true;changed=true;}
  if(changed){
    mode_set_family_palette(!royal,0U);phase_index=0U;dwell=0U;
  }else{
    if(++dwell>=MODE_PULSE_DWELL){
      dwell=0U;unsigned phase=mode_pulse[phase_index];phase_index=(phase_index+1U)%MODE_PULSE_PHASES;
      mode_set_family_palette(royal,phase);
    }
  }
  arcade_logic_wait();
 }

 roster_init();
 unsigned slot=0U,page=0U,scroll=LEFT_SCROLL;bool confirmed=false;
 uint8_t occupied[10]={0};occupied[0]=1U;previous_direction=0U;arcade_logic_phase=0U;
 sync_n(SELECT_ENTRY_WAIT);
 while(true){
  poll_pad();
  if(confirmed){arcade_logic_wait();continue;}
  if(pad.held.button.a||pad.held.button.b){confirmed=true;confirmed_rebuild(page,slot);arcade_logic_wait();continue;}
  unsigned code=selector_direction_edge();
  if(code){
   unsigned dest=slot;
   if(page==0U){if(slot<8U)dest=nav_left[slot][code];}
   else {const uint8_t (*matrix)[11]=royal?nav_right_royal:nav_right_tag;dest=matrix[slot][code];}
   if(dest<10U&&dest!=slot){
    occupied[slot]=0U;
    if(!occupied[dest]){
     unsigned old=slot;cursor_patch_upload(old,false);slot=dest;occupied[slot]=1U;cursor_patch_upload(slot,true);
     bool to_right=(page==0U&&((old==2U&&slot==3U)||(old==7U&&slot==8U)));
     bool to_left=(page==1U&&((old==2U&&slot==1U)||(old==7U&&slot==6U)));
     if(to_right||to_left){
      for(unsigned i=0;i<26U;i++){scroll=(unsigned)((int32_t)scroll+(to_right?8:-8));set_fg_scroll(scroll);arcade_logic_wait();}
      page=to_right?1U:0U;
     }
    }else occupied[slot]=1U;
   }
  }
  arcade_logic_wait();
 }
}
