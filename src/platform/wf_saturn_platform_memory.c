#include "wf_saturn_platform_memory.h"

static bool
range_end(uint32_t addr, uint32_t len, uint32_t *end)
{
    if (!end)
        return false;
    if (len == 0u) {
        *end = addr;
        return true;
    }
    if (addr > UINT32_MAX - len)
        return false;
    *end = addr + len;
    return true;
}

bool
wf_mem_range_contains(uint32_t base, uint32_t bytes,
                      uint32_t addr, uint32_t len)
{
    uint32_t outer_end;
    uint32_t inner_end;

    if (!range_end(base, bytes, &outer_end) ||
        !range_end(addr, len, &inner_end))
        return false;

    return addr >= base && inner_end <= outer_end;
}

bool
wf_mem_is_hwram_range(uint32_t addr, uint32_t len)
{
    return wf_mem_range_contains(WF_HWRAM_BASE, WF_HWRAM_BYTES, addr, len) ||
           wf_mem_range_contains(WF_HWRAM_UNCACHED_BASE, WF_HWRAM_BYTES,
                                 addr, len);
}

bool
wf_mem_is_lwram_range(uint32_t addr, uint32_t len)
{
    return wf_mem_range_contains(WF_LWRAM_BASE, WF_LWRAM_BYTES, addr, len) ||
           wf_mem_range_contains(WF_LWRAM_UNCACHED_BASE, WF_LWRAM_BYTES,
                                 addr, len);
}

wf_mem_contract_result_t
wf_graphics_scu_dma_source_validate(uint32_t src, uint32_t len)
{
    if (len == 0u)
        return WF_MEM_BAD_ARGUMENT;
    if (wf_mem_is_lwram_range(src, len))
        return WF_MEM_LWRAM_DMA_FORBIDDEN;

    return wf_mem_is_hwram_range(src, len) ? WF_MEM_OK : WF_MEM_OUT_OF_RANGE;
}

wf_mem_contract_result_t
wf_scu_dma_reject_lwram(uint32_t src, uint32_t dst, uint32_t len)
{
    if (len == 0u)
        return WF_MEM_BAD_ARGUMENT;
    if (wf_mem_is_lwram_range(src, len) || wf_mem_is_lwram_range(dst, len))
        return WF_MEM_LWRAM_DMA_FORBIDDEN;

    return WF_MEM_OK;
}

uint32_t
wf_cdfs_round_sector(uint32_t bytes)
{
    const uint32_t mask = WF_CDFS_SECTOR_BYTES - 1u;

    if (bytes > UINT32_MAX - mask)
        return 0u;

    return (bytes + mask) & ~mask;
}

wf_mem_contract_result_t
wf_cdfs_entry_count_validate(uint32_t entries)
{
    return entries <= WF_CDFS_FILE_CAPACITY_CURRENT ?
        WF_MEM_OK : WF_MEM_OVERFLOW;
}

wf_mem_contract_result_t
wf_saturn_platform_memory_contract_validate(void)
{
    if (WF_CRAM_BG_FIRST + WF_CRAM_WINDOW_ENTRIES != 0x400u)
        return WF_MEM_OVERFLOW;
    if ((WF_CRAM_BG_FIRST + WF_CRAM_WINDOW_ENTRIES) *
            WF_CRAM_ENTRY_BYTES_MODE2 != WF_VDP2_CRAM_BYTES)
        return WF_MEM_OVERFLOW;

    if (WF_VDP2_FG_CPD_OFF + WF_VDP2_FG_CPD_BYTES > WF_VDP2_BG_CPD_OFF)
        return WF_MEM_OVERFLOW;
    if (WF_VDP2_BG_CPD_OFF + WF_VDP2_BG_CPD_BYTES > WF_VDP2_TEXT_CPD_OFF)
        return WF_MEM_OVERFLOW;
    if (WF_VDP2_TEXT_CPD_OFF + WF_VDP2_TEXT_CPD_BYTES > WF_VDP2_FG_PND_OFF)
        return WF_MEM_OVERFLOW;
    if (WF_VDP2_FG_PND_OFF + WF_VDP2_FG_PND_BYTES > WF_VDP2_BG_PND_OFF)
        return WF_MEM_OVERFLOW;
    if (WF_VDP2_BG_PND_OFF + WF_VDP2_BG_PND_BYTES > WF_VDP2_TEXT_PND_OFF)
        return WF_MEM_OVERFLOW;
    if (WF_VDP2_TEXT_PND_OFF + WF_VDP2_TEXT_PND_BYTES > WF_VDP2_RESERVED_OFF)
        return WF_MEM_OVERFLOW;
    if (WF_VDP2_RESERVED_OFF > WF_VDP2_BACK_COLOR_OFF)
        return WF_MEM_OVERFLOW;
    if (WF_VDP2_BACK_COLOR_OFF + WF_VDP2_BACK_COLOR_BYTES !=
        WF_VDP2_VRAM_BYTES)
        return WF_MEM_OVERFLOW;

    if (WF_SCSP_SFX_RESIDENT_END > WF_SCSP_SFX_DYNAMIC_OFF)
        return WF_MEM_OVERFLOW;
    if (WF_SCSP_SFX_DYNAMIC_END > WF_SCSP_MUSIC_RING_OFF)
        return WF_MEM_OVERFLOW;
    if (WF_SCSP_MUSIC_RING_OFF + WF_SCSP_MUSIC_RING_BYTES >
        WF_SCSP_RAM_BYTES)
        return WF_MEM_OVERFLOW;
    if ((uint32_t)WF_LWRAM_COMBAT_6P_PROVEN_PEAK >
        (uint32_t)WF_LWRAM_BYTES)
        return WF_MEM_OVERFLOW;

    return WF_MEM_OK;
}
