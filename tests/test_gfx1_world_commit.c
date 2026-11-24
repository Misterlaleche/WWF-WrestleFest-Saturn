#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "wf_r18_world.h"

static uint8_t rom[0x80000];
static uint8_t overlay[2][1024];
static unsigned patch_calls;
static unsigned restore_calls;
static char call_order[128];
static unsigned call_order_len;

static void be32_store(uint32_t at, uint32_t value)
{
    rom[at + 0] = (uint8_t)(value >> 24);
    rom[at + 1] = (uint8_t)(value >> 16);
    rom[at + 2] = (uint8_t)(value >> 8);
    rom[at + 3] = (uint8_t)value;
}

static wf_status_t read_rom(void *user, uint32_t at, void *dst, size_t bytes)
{
    (void)user;
    if (at > sizeof rom || bytes > sizeof rom - at)
        return WF_BLOCKED;
    memcpy(dst, rom + at, bytes);
    return WF_OK;
}

int wf_r18_ring_patch_write(unsigned layer, unsigned first_cell,
                            const uint16_t cells[32])
{
    (void)cells;
    if (layer >= 2 || first_cell + 99 >= 1024)
        return -1;
    patch_calls++;
    call_order[call_order_len++] = 'P';
    for (unsigned row = 0; row < 4; row++)
        for (unsigned col = 0; col < 4; col++)
            overlay[layer][first_cell + row * 32 + col] = 1;
    return 0;
}

void wf_r18_ring_tile_restore(unsigned layer, unsigned physical_cell)
{
    restore_calls++;
    call_order[call_order_len++] = 'R';
    if (layer < 2 && physical_cell < 1024)
        overlay[layer][physical_cell] = 0;
}

int main(void)
{
    wf_runtime_t rt;
    wf_r18_world_ctx_t world;
    const unsigned cell = 100;
    memset(&rt, 0, sizeof rt);
    memset(&world, 0, sizeof world);
    memset(rom, 0, sizeof rom);
    memset(overlay, 0, sizeof overlay);

    /* Arena 0, layer 0 destination/source pointer table. */
    be32_store(0x295a4, 0x100);
    be32_store(0x295a8, 0x200);
    /* Frame 0 starts at physical cell 100. */
    rom[0x100] = (uint8_t)((cell * 4) >> 8);
    rom[0x101] = (uint8_t)(cell * 4);

    rt.g.global_007e = 0;
    rt.g.global_16d2 = 0x8000;
    world.read_rom = read_rom;
    world.animation_queue[0][0] = 0;
    world.animation_count[0] = 1;
    for (unsigned i = 0; i < 16; i++)
        world.cache[0][i].destination = 0x80000u + cell * 4u;

    if (wf_r18_world_commit(&rt, &world) != WF_OK) {
        fputs("world commit was blocked\n", stderr);
        return 2;
    }
    if (patch_calls != 1 || restore_calls != 64) {
        fprintf(stderr, "unexpected calls: patch=%u restore=%u\n",
                patch_calls, restore_calls);
        return 3;
    }
    if (call_order_len == 0 || call_order[0] != 'P') {
        fputs("GFX1 regression: transport restore ran before animation patch\n", stderr);
        return 4;
    }
    if (overlay[0][cell] != 0 || overlay[0][cell + 1] != 0 ||
        overlay[0][cell + 32] != 0 || overlay[0][cell + 33] != 0) {
        fputs("GFX1 regression: stale animated overlay survived recycled cells\n", stderr);
        return 5;
    }
    if ((rt.g.global_16d2 & 0x8000) != 0 || world.animation_count[0] != 0) {
        fputs("commit flags were not consumed\n", stderr);
        return 6;
    }
    puts("PASS GFX1 world commit ordering");
    return 0;
}
