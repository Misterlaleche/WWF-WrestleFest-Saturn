# Saturn platform adaptations

These are transport adaptations only; they do not change the proven arcade sequence.

- The isolated harness uses VDP2 NBG0 as a 512x256 8-bit indexed bitmap plane and CRAM mode 1. Only 512x240 visible pixels are populated.
- The three validated plate bitmaps are preloaded into separate VDP2 VRAM pages before presentation begins. This avoids introducing CDFS latency into ROM-timed holds/fades. This triple-page residency is harness-only and is not a master-runtime ABI.
- ROM-derived indexed palettes remain indexed. Fade levels are rebuilt from the exact arcade 4-bit transfer table and published to CRAM at synchronized VBlank boundaries.
- HWRAM aligned staging is used for SCU DMA to VDP2 VRAM. LWRAM is not used as an SCU-DMA endpoint.
- The isolated test image links the validated INTRO.RLE asset locally. The master may later bind this resource to WP32/WP33 ownership without changing scene logic or timing.
- The harness enters a permanent black hold after TITLE_HANDOFF because TITLE belongs to a separate owner. This black hold is diagnostic harness behavior after the block has already completed; it is not claimed as an arcade TITLE delay.
- No private SCSP backend or private SMPC input loop is introduced.
