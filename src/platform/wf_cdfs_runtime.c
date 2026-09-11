#include "wf_cdfs_runtime.h"

#include <string.h>

static bool
valid_root_name(const char *name)
{
    size_t len;

    if (!name)
        return false;
    len = strlen(name);
    if (len == 0u || len > 11u)
        return false;
    return strchr(name, '/') == NULL && strchr(name, '\\') == NULL;
}

static bool
range_overlap(uint32_t a, uint32_t alen, uint32_t b, uint32_t blen)
{
    uint32_t ae;
    uint32_t be;

    if (alen == 0u || blen == 0u)
        return false;
    if (a > UINT32_MAX - alen || b > UINT32_MAX - blen)
        return true;
    ae = a + alen;
    be = b + blen;
    return a < be && b < ae;
}

bool
wf_cdfs_runtime_init(wf_cdfs_runtime_t *runtime, const wf_cdfs_ops_t *ops)
{
    if (!runtime || !ops || !ops->lookup || !ops->read)
        return false;
    runtime->ops = *ops;
    return true;
}

wf_cdfs_result_t
wf_cdfs_request_preflight(const wf_cdfs_runtime_t *runtime,
                          const wf_cdfs_load_request_t *request,
                          wf_cdfs_file_info_t *file_out)
{
    wf_cdfs_file_info_t file;

    if (!runtime || !request || !request->dst)
        return WF_CDFS_BAD_ARGUMENT;
    if (!valid_root_name(request->name))
        return WF_CDFS_BAD_NAME;
    if (!runtime->ops.lookup(request->name, &file, runtime->ops.user))
        return WF_CDFS_NOT_FOUND;
    if (file.size == 0u)
        return WF_CDFS_ZERO_FILE;
    if (request->dst_capacity < file.size)
        return WF_CDFS_DEST_CAPACITY;

    switch (request->dest_kind) {
    case WF_CDFS_DEST_HWRAM_STAGING:
        if (!wf_mem_is_hwram_range(request->dst_bus_addr, file.size))
            return WF_CDFS_DEST_RANGE;
        break;
    case WF_CDFS_DEST_LWRAM_BULK:
        if (!wf_mem_is_lwram_range(request->dst_bus_addr, file.size))
            return WF_CDFS_DEST_RANGE;
        break;
    default:
        return WF_CDFS_BAD_ARGUMENT;
    }

    if (file_out)
        *file_out = file;
    return WF_CDFS_OK;
}

wf_cdfs_result_t
wf_cdfs_load_one(const wf_cdfs_runtime_t *runtime,
                 const wf_cdfs_load_request_t *request)
{
    wf_cdfs_file_info_t file;
    wf_cdfs_result_t result;

    result = wf_cdfs_request_preflight(runtime, request, &file);
    if (result != WF_CDFS_OK)
        return result;

    if (!runtime->ops.read(file.starting_fad, request->dst, file.size,
                           runtime->ops.user))
        return WF_CDFS_IO;
    return WF_CDFS_OK;
}

wf_cdfs_result_t
wf_cdfs_scene_preflight(const wf_cdfs_runtime_t *runtime,
                        wf_cdfs_scene_transaction_t *txn)
{
    uint32_t i;
    uint32_t j;

    if (!runtime || !txn || !txn->requests || txn->request_count == 0u)
        return WF_CDFS_BAD_ARGUMENT;

    txn->ready = false;

    for (i = 0; i < txn->request_count; ++i) {
        wf_cdfs_file_info_t fi;
        wf_cdfs_result_t r = wf_cdfs_request_preflight(
            runtime, &txn->requests[i], &fi);
        if (r != WF_CDFS_OK)
            return r;

        for (j = 0; j < i; ++j) {
            wf_cdfs_file_info_t fj;
            r = wf_cdfs_request_preflight(runtime, &txn->requests[j], &fj);
            if (r != WF_CDFS_OK)
                return r;
            if (range_overlap(txn->requests[i].dst_bus_addr, fi.size,
                              txn->requests[j].dst_bus_addr, fj.size))
                return WF_CDFS_DEST_OVERLAP;
        }
    }

    return WF_CDFS_OK;
}

wf_cdfs_result_t
wf_cdfs_scene_load(wf_cdfs_runtime_t *runtime,
                   wf_cdfs_scene_transaction_t *txn)
{
    uint32_t i;
    wf_cdfs_result_t r;

    r = wf_cdfs_scene_preflight(runtime, txn);
    if (r != WF_CDFS_OK)
        return r;

    for (i = 0; i < txn->request_count; ++i) {
        r = wf_cdfs_load_one(runtime, &txn->requests[i]);
        if (r != WF_CDFS_OK) {
            txn->ready = false;
            return r;
        }
    }

    txn->ready = true;
    return WF_CDFS_OK;
}
