/* WP02 R02 hardware-visible failure display.
 * This deliberately uses no font texture, heap allocation, filesystem, audio,
 * or timer service. The existing R01 VDP1/VDP2 setup must have completed.
 *
 * Four rows of eight hexadecimal digits. Small left-hand digits number rows.
 * Caller chooses the four words and must snapshot fault state before stopping
 * audio or calling any helper which clears its own diagnostics.
 *
 * Suggested first hardware run:
 *   words[0] = (fatal_code << 24) | (bgm_reason << 16) | raw_monitor;
 *   words[1] = (fill_bytes << 16) | (bgm_refills & 0xFFFF);
 *   words[2] = (renders << 16) | (missed_vblanks & 0xFFFF);
 *   words[3] = max_bgm_service_us;
 * Document the exact packing on the hardware test instructions.
 *
 * 256 static commands consume 8 KiB HWRAM, no VDP1 texture space. Worst-case
 * drawing uses 244 commands including clip, origin, clear, and END. All
 * coordinates are within 320x240; no digit extends beyond x=295 or y=218.
 * This was compiled against the supplied SH2 SDK; actual display behavior still
 * requires emulator/physical validation by the integrating coordinator.
 */
#ifndef WP02_DIAG_H
#define WP02_DIAG_H
#include <yaul.h>
#include <stdint.h>
#include <string.h>

static vdp1_cmdt_t wp02_diag_commands[256] __aligned(32);

static void wp02_diag_rect(unsigned *n, int x, int y, int w, int h,
                          uint16_t color)
{
    vdp1_cmdt_t *c = &wp02_diag_commands[(*n)++];
    const vdp1_cmdt_draw_mode_t mode = {
        .pre_clipping_disable = true,
        .trans_pixel_disable = true,
        .end_code_disable = true
    };
    const int16_vec2_t corners[4] = {
        INT16_VEC2_INITIALIZER(x, y),
        INT16_VEC2_INITIALIZER(x+w-1, y),
        INT16_VEC2_INITIALIZER(x+w-1, y+h-1),
        INT16_VEC2_INITIALIZER(x, y+h-1)
    };
    vdp1_cmdt_polygon_set(c);
    vdp1_cmdt_draw_mode_set(c, mode);
    c->cmd_colr = color;
    vdp1_cmdt_vtx_set(c, corners);
}

static void wp02_diag_digit(unsigned *n, unsigned value, int x, int y)
{
    /* a,b,c,d,e,f,g, hexadecimal A,b,C,d,E,F. */
    static const uint8_t segments[16] = {
        0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,
        0x7F,0x6F,0x77,0x7C,0x39,0x5E,0x79,0x71
    };
    static const uint8_t rect[7][4] = {
        {4,0,14,4}, {18,4,4,12}, {18,20,4,12}, {4,32,14,4},
        {0,20,4,12}, {0,4,4,12}, {4,16,14,4}
    };
    const unsigned mask = segments[value & 15u];
    for (unsigned bit = 0; bit < 7u; ++bit)
        if (mask & (1u << bit))
            wp02_diag_rect(n, x+rect[bit][0], y+rect[bit][1],
                           rect[bit][2], rect[bit][3], 1023u);
}

static void wp02_diag_show(const uint32_t words[4])
{
    vdp1_sync_wait();
    vdp2_sync_wait();
    memset(wp02_diag_commands, 0, sizeof(wp02_diag_commands));
    vdp1_cmdt_system_clip_coord_set(&wp02_diag_commands[0]);
    vdp1_cmdt_vtx_system_clip_coord_set(&wp02_diag_commands[0],
                                     INT16_VEC2(319,239));
    vdp1_cmdt_local_coord_set(&wp02_diag_commands[1]);
    vdp1_cmdt_vtx_local_coord_set(&wp02_diag_commands[1], INT16_VEC2(0,0));
    unsigned n = 2;
    /* Transparent framebuffer clear allows the red back-screen to show. */
    wp02_diag_rect(&n, 0, 0, 320, 240, 0);
    for (unsigned row = 0; row < 4u; ++row) {
        const int y = 24 + (int)row*53;
        wp02_diag_digit(&n, row+1u, 4, y);
        for (unsigned col = 0; col < 8u; ++col)
            wp02_diag_digit(&n, words[row] >> (28u-col*4u),
                            36+(int)col*34, y);
    }
    vdp1_cmdt_end_set(&wp02_diag_commands[n++]);
    /* R01 uses mode-2 CRAM and palette-only sprite type 0. Entry 1023 is
     * reclaimed only after disabling the ring; no RGB/palette mode change. */
    vdp2_scrn_display_set(0);
    MEMORY_WRITE(32, 0x25F00FFCu, 0x00FFFFFFu);
    vdp2_sprite_priority_set(0,6);
    vdp2_scrn_back_color_set(VDP2_VRAM_ADDR(3,0x1FFFE),
                            RGB1555(1,31,0,0));
    vdp1_sync_cmdt_put(wp02_diag_commands, (uint16_t)n, 0);
    vdp1_sync_put_wait();
    vdp1_sync_render();
    vdp1_sync();
    vdp2_tvmd_display_set();
    vdp2_sync();
    vdp1_sync_wait();
    vdp2_sync_wait();
}
#endif
