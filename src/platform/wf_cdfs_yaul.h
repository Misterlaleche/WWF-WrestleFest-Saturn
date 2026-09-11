#ifndef WF_CDFS_YAUL_H
#define WF_CDFS_YAUL_H

#include <stdbool.h>
#include <stdint.h>

#include <yaul.h>

#include "wf_cdfs_runtime.h"

typedef struct wf_cdfs_yaul {
    cdfs_filelist_t filelist;
    cdfs_filelist_entry_t *entries;
    int32_t entry_capacity;
} wf_cdfs_yaul_t;

bool wf_cdfs_yaul_mount(wf_cdfs_yaul_t *state,
                        cdfs_filelist_entry_t *entries,
                        int32_t entry_capacity);
void wf_cdfs_yaul_ops(wf_cdfs_yaul_t *state, wf_cdfs_ops_t *ops_out);

#endif
