#include <yaul.h>
#include <stdint.h>
#include "wp0_error_screen.h"
#include "debug_font_1bpp.h"

/* Fatal-screen-only ownership: texture 0000..1FFF, map2000..2FFF.
 * All original video/audio resources are left alone during normal playback. */
#define DIAG_PATTERNS ((volatile uint16_t *)0x25E00000UL)
#define DIAG_MAP ((volatile uint32_t *)0x25E02000UL)

static void glyphs_load(void)
{
    for (unsigned ch=0;ch<64;ch++) {
        for (unsigned tile=0;tile<4;tile++) {
            for (unsigned y=0;y<8;y++) {
                for (unsigned x=0;x<8;x+=4) {
                    uint16_t packed=0;
                    for (unsigned pixel=0;pixel<4;pixel++) {
                        const unsigned source_x=((tile&1U)*8U+x+pixel)/2U;
                        const unsigned source_y=((tile>>1)*8U+y)/2U;
                        packed=(uint16_t)((packed<<4)|
                            ((debug_font_1bpp[ch][source_y]&(1U<<source_x))?15U:0U));
                    }
                    DIAG_PATTERNS[ch*64U+tile*16U+y*2U+x/4U]=packed;
                }
            }
        }
    }
}

static void text_put(unsigned column,unsigned row,const char *text)
{
    while (*text && column<20U) {
        unsigned ch=(unsigned char)*text++;
        if (ch<32U || ch>=96U) ch=32U;
        DIAG_MAP[row*32U+column++]=(ch-32U)*4U;
    }
}

static void number_put(unsigned column,unsigned row,uint32_t value)
{
    if (value==UINT32_MAX) {
        text_put(column,row,"UNKNOWN");
        return;
    }
    char digits[9];
    digits[8]='\0';
    for (unsigned i=8;i>0;i--) {
        digits[i-1]=(char)('0'+value%10U);
        value/=10U;
    }
    if (value) {
        text_put(column,row,"OVERFLOW");
        return;
    }
    for (unsigned i=0;i<7 && digits[i]=='0';i++) digits[i]=' ';
    text_put(column,row,digits);
}

void wp0_error_screen(const uint32_t values[12])
{
    static const char *const labels[12]={
        "CODE","AUDIO","FRAME","MISS","CPU","DRAW",
        "REG","GAPUS","QUEUE","PLAY","BLOCK","FLAGS"
    };
    vdp2_tvmd_display_clear();
    vdp2_scrn_display_set(0);
    vdp2_vram_cycp_clear();
    vdp2_sync();
    vdp2_sync_wait();
    glyphs_load();
    for (unsigned i=0;i<1024;i++) DIAG_MAP[i]=0;
    vdp2_cram_mode_set(2);
    ((volatile uint32_t *)0x25F00000UL)[0]=0x00000030U;
    ((volatile uint32_t *)0x25F00000UL)[15]=0x00FFFFFFU;
    const vdp2_scrn_cell_format_t format={
        .scroll_screen=VDP2_SCRN_NBG0,.ccc=VDP2_SCRN_CCC_PALETTE_16,
        .char_size=VDP2_SCRN_CHAR_SIZE_2X2,.pnd_size=2,
        .aux_mode=VDP2_SCRN_AUX_MODE_0,.plane_size=VDP2_SCRN_PLANE_SIZE_1X1,
        .cpd_base=VDP2_VRAM_ADDR(0,0),.palette_base=0x25F00000UL
    };
    const vdp2_scrn_normal_map_t map={
        .base_addr={0x25E02000UL,0x25E02000UL,0x25E02000UL,0x25E02000UL}
    };
    vdp2_scrn_cell_format_set(&format,&map);
    vdp2_scrn_scroll_x_set(VDP2_SCRN_NBG0,0);
    vdp2_scrn_scroll_y_set(VDP2_SCRN_NBG0,0);
    vdp2_scrn_priority_set(VDP2_SCRN_NBG0,7);
    for (unsigned i=0;i<8;i++) vdp2_sprite_priority_set(i,0);
    vdp2_vram_cycp_t cycles;
    for (unsigned i=0;i<4;i++) cycles.pt[i].raw=0xFFFFFFFFU;
    /* Pattern-name fetch atT0, character fetch atT4. No runtime VRAM
     * access is needed once this static diagnostic screen is displayed. */
    cycles.pt[0].raw=0x0FFF4FFFU;
    vdp2_vram_cycp_set(&cycles);
    text_put(1,1,"WP0 R04 ERROR");
    for (unsigned i=0;i<12;i++) {
        text_put(1,i+2,labels[i]);
        number_put(8,i+2,values[i]);
    }
    vdp2_scrn_display_set(VDP2_SCRN_DISP_NBG0);
    vdp2_tvmd_display_set();
    vdp2_sync();
    vdp2_sync_wait();
}
