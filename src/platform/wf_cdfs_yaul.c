#include "wf_cdfs_yaul.h"

#include <string.h>

static bool
lookup_file(const char *name, wf_cdfs_file_info_t *out, void *user)
{
    wf_cdfs_yaul_t *state = (wf_cdfs_yaul_t *)user;
    uint32_t i;

    if (!state || !name || !out)
        return false;

    for (i = 0; i < state->filelist.entries_count; ++i) {
        const cdfs_filelist_entry_t *entry = &state->filelist.entries[i];
        if (entry->type != CDFS_ENTRY_TYPE_FILE)
            continue;
        if (strcmp(entry->name, name) != 0)
            continue;
        out->starting_fad = (uint32_t)entry->starting_fad;
        out->size = (uint32_t)entry->size;
        return true;
    }
    return false;
}

static bool
read_file(uint32_t starting_fad, void *dst, uint32_t bytes, void *user)
{
    (void)user;
    if (!dst || bytes == 0u || starting_fad < 150u)
        return false;
    return cd_block_sectors_read((fad_t)starting_fad, dst, bytes) == 0;
}

bool
wf_cdfs_yaul_mount(wf_cdfs_yaul_t *state,
                   cdfs_filelist_entry_t *entries,
                   int32_t entry_capacity)
{
    if (!state || !entries || entry_capacity <= 0 ||
        entry_capacity > (int32_t)WF_CDFS_FILE_CAPACITY_CURRENT)
        return false;

    state->entries = entries;
    state->entry_capacity = entry_capacity;
    cdfs_config_default_set();
    cdfs_filelist_init(&state->filelist, entries, entry_capacity);
    cdfs_filelist_root_read(&state->filelist);

    return state->filelist.entries_count <= (uint32_t)entry_capacity;
}

void
wf_cdfs_yaul_ops(wf_cdfs_yaul_t *state, wf_cdfs_ops_t *ops_out)
{
    if (!ops_out)
        return;
    ops_out->lookup = lookup_file;
    ops_out->read = read_file;
    ops_out->user = state;
}
