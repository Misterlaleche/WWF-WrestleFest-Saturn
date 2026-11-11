/* Included by the projected HW1 banked backend after its scalar ROM upload.
 * Requests consume sequential logical slots beginning at emitted_tiles;
 * only source_tile is read from each descriptor, as in the original caller.
 * Callbacks must not mutate the backend. They observe every completed prefix.
 */
wf_status_t wf_saturn_yaul_upload_rom_tiles(wf_saturn_yaul_backend_t *b,
        const wf_vdp1_tile_upload_t *tiles, unsigned count,
        wf_status_t (*read_tile)(uint16_t, const uint8_t **)) {
    if (!b || !b->active || !read_tile || (!tiles && count) ||
        b->expected_tiles > WF_VDP1_STREAM_TILES_MAX ||
        b->emitted_tiles > b->expected_tiles ||
        count > b->expected_tiles - b->emitted_tiles) return WF_BAD_STATE;
#ifdef WF_PLATFORM_SATURN
    if (b->rom_frame) {
        unsigned first = b->emitted_tiles, slot = first, limit = first + count;
        uint32_t reused = 0;
        while (slot < limit) {
            unsigned byte = slot >> 3, end = (slot | 7u) + 1u;
            if (end > limit) end = limit;
            uint8_t identity = b->rom_identity[byte], resident = b->resident[byte];
            while (slot < end) {
                uint16_t source = tiles[slot-first].source_tile;
                unsigned physical = g_rom_cache.frame_slot[slot];
                unsigned mask = byte_mask(slot);
                if (physical >= WP1980_VRAM_CACHE_SLOTS ||
                    g_rom_cache.source[physical] != source) {
                    b->rom_identity[byte] = identity; b->resident[byte] = resident;
                    b->emitted_tiles = slot; b->texture_reused += reused;
                    return WF_BAD_STATE;
                }
                if (g_rom_cache.flags[physical] & WP1980_VRAM_COMMITTED) {
                    b->crop_meta[slot] = g_rom_cache.crop_meta[physical];
                    b->crop_height[slot] = g_rom_cache.crop_height[physical];
                    b->rom_source[slot] = source;
                    identity |= (uint8_t)mask; resident |= (uint8_t)mask;
                    ++slot; ++reused;
                } else {
                    /* Publish the same prefix before the unchanged provider,
                     * crop/copy and error path can observe the backend. */
                    b->rom_identity[byte] = identity; b->resident[byte] = resident;
                    b->emitted_tiles = slot; b->texture_reused += reused; reused = 0;
                    wf_status_t status = wf_saturn_yaul_upload_rom_tile(
                        b, (uint16_t)slot, source, read_tile);
                    if (status != WF_OK) return status;
                    ++slot;
                    identity = b->rom_identity[byte]; resident = b->resident[byte];
                }
            }
            b->rom_identity[byte] = identity; b->resident[byte] = resident;
        }
        b->emitted_tiles = slot; b->texture_reused += reused;
        return WF_OK;
    }
#endif
    for (unsigned i = 0; i < count; ++i) {
        wf_status_t status = wf_saturn_yaul_upload_rom_tile(
            b, (uint16_t)b->emitted_tiles, tiles[i].source_tile, read_tile);
        if (status != WF_OK) return status;
    }
    return WF_OK;
}
