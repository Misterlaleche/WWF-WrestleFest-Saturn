#ifndef WF_SATURN_PLATFORM_MEMORY_H
#define WF_SATURN_PLATFORM_MEMORY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
    WF_HWRAM_BASE = 0x06000000u,
    WF_HWRAM_UNCACHED_BASE = 0x26000000u,
    WF_HWRAM_BYTES = 0x00100000u,
    WF_LWRAM_BASE = 0x00200000u,
    WF_LWRAM_UNCACHED_BASE = 0x20200000u,
    WF_LWRAM_BYTES = 0x00100000u,
    WF_VDP1_VRAM_BASE = 0x25C00000u,
    WF_VDP1_VRAM_BYTES = 0x00080000u,
    WF_VDP2_VRAM_BASE = 0x25E00000u,
    WF_VDP2_VRAM_BYTES = 0x00080000u,
    WF_VDP2_CRAM_BASE = 0x25F00000u,
    WF_VDP2_CRAM_BYTES = 0x00001000u,
    WF_SCSP_RAM_BASE = 0x25A00000u,
    WF_SCSP_RAM_BYTES = 0x00080000u,
    WF_CDFS_SECTOR_BYTES = 2048u,
    WF_CDFS_FILE_CAPACITY_CURRENT = 384u
};

enum {
    WF_CRAM_MODE = 2u,
    WF_CRAM_TEXT_FIRST = 0x000u,
    WF_CRAM_SPRITES_FIRST = 0x100u,
    WF_CRAM_FG_FIRST = 0x200u,
    WF_CRAM_BG_FIRST = 0x300u,
    WF_CRAM_WINDOW_ENTRIES = 0x100u,
    WF_CRAM_ENTRY_BYTES_MODE2 = 4u
};

enum {
    WF_VDP2_FG_CPD_OFF = 0x00000u,
    WF_VDP2_FG_CPD_BYTES = 0x20000u,
    WF_VDP2_BG_CPD_OFF = 0x20000u,
    WF_VDP2_BG_CPD_BYTES = 0x20000u,
    WF_VDP2_TEXT_CPD_OFF = 0x40000u,
    WF_VDP2_TEXT_CPD_BYTES = 0x10000u,
    WF_VDP2_FG_PND_OFF = 0x50000u,
    WF_VDP2_FG_PND_BYTES = 0x01000u,
    WF_VDP2_BG_PND_OFF = 0x51000u,
    WF_VDP2_BG_PND_BYTES = 0x01000u,
    WF_VDP2_TEXT_PND_OFF = 0x52000u,
    WF_VDP2_TEXT_PND_BYTES = 0x04000u,
    WF_VDP2_RESERVED_OFF = 0x56000u,
    WF_VDP2_BACK_COLOR_OFF = 0x7FFFEu,
    WF_VDP2_BACK_COLOR_BYTES = 2u
};

enum {
    WF_SCSP_DRIVER_END = 0x14000u,
    WF_SCSP_SFX_RESIDENT_OFF = 0x14000u,
    WF_SCSP_SFX_RESIDENT_END = 0x3A800u,
    WF_SCSP_SFX_DYNAMIC_OFF = 0x3A800u,
    WF_SCSP_SFX_DYNAMIC_END = 0x58800u,
    WF_SCSP_MUSIC_RING_OFF = 0x60000u,
    WF_SCSP_MUSIC_RING_BYTES = 0x10000u,
    WF_SCSP_MUSIC_HALF_BYTES = 0x08000u,
    WF_SCSP_MUSIC_SLOT = 31u
};

enum {
    WF_LWRAM_COMBAT_6P_PROVEN_PEAK = 903168u,
    WF_LWRAM_COMBAT_OVERLAY_RESERVE = 131072u
};

typedef enum wf_mem_contract_result {
    WF_MEM_OK = 0,
    WF_MEM_BAD_ARGUMENT,
    WF_MEM_OVERFLOW,
    WF_MEM_LWRAM_DMA_FORBIDDEN,
    WF_MEM_OUT_OF_RANGE
} wf_mem_contract_result_t;

bool wf_mem_range_contains(uint32_t base, uint32_t bytes,
                           uint32_t addr, uint32_t len);
bool wf_mem_is_hwram_range(uint32_t addr, uint32_t len);
bool wf_mem_is_lwram_range(uint32_t addr, uint32_t len);
wf_mem_contract_result_t wf_graphics_scu_dma_source_validate(uint32_t src,
                                                             uint32_t len);
wf_mem_contract_result_t wf_scu_dma_reject_lwram(uint32_t src,
                                                 uint32_t dst,
                                                 uint32_t len);
uint32_t wf_cdfs_round_sector(uint32_t bytes);
wf_mem_contract_result_t wf_cdfs_entry_count_validate(uint32_t entries);
wf_mem_contract_result_t wf_saturn_platform_memory_contract_validate(void);

#endif
