#include <assert.h>
#include <stdint.h>

#include "../../src/platform/wf_saturn_platform_memory.h"

int
main(void)
{
    assert(wf_saturn_platform_memory_contract_validate() == WF_MEM_OK);

    assert(wf_mem_is_hwram_range(0x06000000u, 32u));
    assert(wf_mem_is_hwram_range(0x260FFFF0u, 16u));
    assert(!wf_mem_is_hwram_range(0x060FFFF0u, 32u));
    assert(wf_mem_is_lwram_range(0x00200000u, 2048u));

    assert(wf_graphics_scu_dma_source_validate(0x06010000u, 128u) ==
           WF_MEM_OK);
    assert(wf_graphics_scu_dma_source_validate(0x00210000u, 128u) ==
           WF_MEM_LWRAM_DMA_FORBIDDEN);
    assert(wf_graphics_scu_dma_source_validate(0x25C00000u, 128u) ==
           WF_MEM_OUT_OF_RANGE);

    assert(wf_scu_dma_reject_lwram(0x06010000u, 0x25C00000u, 128u) ==
           WF_MEM_OK);
    assert(wf_scu_dma_reject_lwram(0x00210000u, 0x25C00000u, 128u) ==
           WF_MEM_LWRAM_DMA_FORBIDDEN);
    assert(wf_scu_dma_reject_lwram(0x06010000u, 0x00210000u, 128u) ==
           WF_MEM_LWRAM_DMA_FORBIDDEN);

    assert(wf_cdfs_round_sector(1u) == 2048u);
    assert(wf_cdfs_round_sector(2048u) == 2048u);
    assert(wf_cdfs_round_sector(2049u) == 4096u);
    assert(wf_cdfs_entry_count_validate(384u) == WF_MEM_OK);
    assert(wf_cdfs_entry_count_validate(385u) == WF_MEM_OVERFLOW);

    return 0;
}
