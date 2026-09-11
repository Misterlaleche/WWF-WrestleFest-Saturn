#ifndef WF_CDFS_RUNTIME_H
#define WF_CDFS_RUNTIME_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "wf_saturn_platform_memory.h"

typedef struct wf_cdfs_file_info {
    uint32_t starting_fad;
    uint32_t size;
} wf_cdfs_file_info_t;

typedef struct wf_cdfs_ops {
    bool (*lookup)(const char *name, wf_cdfs_file_info_t *out, void *user);
    bool (*read)(uint32_t starting_fad, void *dst, uint32_t bytes, void *user);
    void *user;
} wf_cdfs_ops_t;

typedef enum wf_cdfs_dest_kind {
    WF_CDFS_DEST_HWRAM_STAGING = 1,
    WF_CDFS_DEST_LWRAM_BULK = 2
} wf_cdfs_dest_kind_t;

typedef enum wf_cdfs_result {
    WF_CDFS_OK = 0,
    WF_CDFS_BAD_ARGUMENT,
    WF_CDFS_BAD_NAME,
    WF_CDFS_NOT_FOUND,
    WF_CDFS_ZERO_FILE,
    WF_CDFS_DEST_RANGE,
    WF_CDFS_DEST_CAPACITY,
    WF_CDFS_DEST_OVERLAP,
    WF_CDFS_IO
} wf_cdfs_result_t;

typedef struct wf_cdfs_runtime {
    wf_cdfs_ops_t ops;
} wf_cdfs_runtime_t;

typedef struct wf_cdfs_load_request {
    const char *name;
    void *dst;
    uint32_t dst_bus_addr;
    uint32_t dst_capacity;
    wf_cdfs_dest_kind_t dest_kind;
} wf_cdfs_load_request_t;

typedef struct wf_cdfs_scene_transaction {
    const wf_cdfs_load_request_t *requests;
    uint32_t request_count;
    bool ready;
} wf_cdfs_scene_transaction_t;

bool wf_cdfs_runtime_init(wf_cdfs_runtime_t *runtime,
                          const wf_cdfs_ops_t *ops);
wf_cdfs_result_t wf_cdfs_request_preflight(const wf_cdfs_runtime_t *runtime,
                                           const wf_cdfs_load_request_t *request,
                                           wf_cdfs_file_info_t *file_out);
wf_cdfs_result_t wf_cdfs_load_one(const wf_cdfs_runtime_t *runtime,
                                  const wf_cdfs_load_request_t *request);
wf_cdfs_result_t wf_cdfs_scene_preflight(const wf_cdfs_runtime_t *runtime,
                                         wf_cdfs_scene_transaction_t *txn);
wf_cdfs_result_t wf_cdfs_scene_load(wf_cdfs_runtime_t *runtime,
                                    wf_cdfs_scene_transaction_t *txn);

#endif
