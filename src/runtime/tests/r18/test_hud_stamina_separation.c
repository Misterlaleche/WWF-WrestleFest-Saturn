#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "wf_r18_hud.h"

static wf_runtime_t rt;
static wf_r18_video_t video;
static uint16_t names[4096];

static uint16_t cell(uint32_t address)
{
    const uint8_t *p = video.fg0 + (address - 0xc0000u);
    return (uint16_t)((p[3] << 8) | p[1]);
}

static void put_cell(uint32_t address, uint16_t tile)
{
    wf_r17_fg0_write_t w[2] = {
        { address + 1u, tile & 255u, 1 },
        { address + 3u, tile >> 8, 1 }
    };
    assert(wf_r18_video_batch(&rt, w, 2, &video) == WF_OK);
}

static void team(unsigned first)
{
    /* $76A2 bases, $782C portrait offset and $7628 bar offset, read
     * independently from ROM. Filling after the six-cell clear must return
     * to the original bar position, preserving the adjacent portrait. */
    static const uint32_t bases[4] = {0xc0104u, 0xc0128u, 0xc0154u, 0xc0178u};
    memset(&rt, 0, sizeof rt);
    memset(&video, 0, sizeof video);
    rt.m68k_regs.valid = 1;
    rt.g.credited_route = 1;
    wf_r17_post7_ctx_t ctx = {0};
    ctx.fg0_batch = wf_r18_video_batch;
    ctx.user = &video;
    for (unsigned slot = first; slot < first + 2; ++slot) {
        wf_actor_raw_t *a = &rt.actors[slot];
        wf_set_u8(a, WF_A_ACTIVE, 0x80);
        wf_set_u8(a, WF_A_ROLE, slot & 1u ? 0x40 : 0);
        wf_set_be32(a, WF_A_INPUT_SOURCE, 0x140020u + slot * 2u);
        wf_set_be16(a, WF_A_STAMINA_MAX, 110);
        wf_set_be16(a, 0x6e, 96);
        wf_set_be16(a, WF_A_VISIBLE_DELTA, 0xfffe);
        for (unsigned y = 0; y < 3; ++y)
            for (unsigned x = 0; x < 3; ++x)
                put_cell(bases[slot] + 0x100u + y * 0x100u + x * 4u,
                         (uint16_t)(0x31d9u + y * 3u + x));
    }
    assert(wf_r17_owner_7548(&rt, &ctx) == WF_OK);
    for (unsigned slot = first; slot < first + 2; ++slot) {
        assert(wf_be16(&rt.actors[slot], 0x6e) == 94);
        for (unsigned y = 0; y < 3; ++y)
            for (unsigned x = 0; x < 3; ++x)
                assert(cell(bases[slot] + 0x100u + y * 0x100u + x * 4u) ==
                       (uint16_t)(0x31d9u + y * 3u + x));
        /* 94 points: five full 16-point cells and one 14-point remainder.
         * $76AA uses another glyph family for slots 2/3. */
        const uint16_t full = slot < 2 ? 0x1ae1u : 0x1afau;
        const uint16_t partial = slot < 2 ? 0x1ae0u : 0x1af9u;
        for (unsigned x = 0; x < 6; ++x)
            assert(cell(bases[slot] + 0x20cu + x * 4u) == (x < 5 ? full : partial));
    }
    wf_r18_hud_reset(&video);
    assert(wf_r18_hud_map_update(&video, names, NULL) == WF_OK);
    for (unsigned slot = first; slot < first + 2; ++slot)
        for (unsigned y = 0; y < 3; ++y)
            for (unsigned x = 0; x < 3; ++x) {
                unsigned index = (bases[slot] - 0xc0000u + 0x100u + y * 0x100u + x * 4u) / 4u;
                assert(names[index] == (uint16_t)(0x31d9u + y * 3u + x));
                assert(names[index + 2048u] == names[index]);
            }
}

int main(void)
{
    team(0);
    team(2);
    puts("PASS stamina updates preserve both portraits and fill the original six-cell bars on both HUD sides, through native VDP2 names");
    return 0;
}
