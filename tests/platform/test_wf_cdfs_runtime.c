#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "../../src/platform/wf_cdfs_runtime.h"

typedef struct mock_file {
    const char *name;
    uint32_t fad;
    uint32_t size;
    uint8_t fill;
} mock_file_t;

typedef struct mock_fs {
    const mock_file_t *files;
    uint32_t count;
    uint32_t reads;
    uint32_t fail_fad;
} mock_fs_t;

static bool lookup_cb(const char *name, wf_cdfs_file_info_t *out, void *user)
{
    mock_fs_t *fs = (mock_fs_t *)user;
    uint32_t i;
    for (i = 0; i < fs->count; ++i) {
        if (strcmp(name, fs->files[i].name) == 0) {
            out->starting_fad = fs->files[i].fad;
            out->size = fs->files[i].size;
            return true;
        }
    }
    return false;
}

static bool read_cb(uint32_t fad, void *dst, uint32_t bytes, void *user)
{
    mock_fs_t *fs = (mock_fs_t *)user;
    uint32_t i;
    if (fad == fs->fail_fad)
        return false;
    for (i = 0; i < fs->count; ++i) {
        if (fad == fs->files[i].fad && bytes == fs->files[i].size) {
            memset(dst, fs->files[i].fill, bytes);
            fs->reads++;
            return true;
        }
    }
    return false;
}

int main(void)
{
    static const mock_file_t files[] = {
        {"RING.PAK", 150u, 3000u, 0x11u},
        {"OBJ.PAK", 152u, 4096u, 0x22u},
        {"FIGHT.PAK", 154u, 2048u, 0x33u}
    };
    uint8_t a[4096];
    uint8_t b[4096];
    mock_fs_t fs = {files, 3u, 0u, 0u};
    wf_cdfs_ops_t ops = {lookup_cb, read_cb, &fs};
    wf_cdfs_runtime_t rt;
    wf_cdfs_file_info_t info;
    wf_cdfs_load_request_t one = {
        "RING.PAK", a, 0x06010000u, sizeof(a), WF_CDFS_DEST_HWRAM_STAGING
    };
    wf_cdfs_load_request_t reqs[2] = {
        {"RING.PAK", a, 0x06010000u, sizeof(a), WF_CDFS_DEST_HWRAM_STAGING},
        {"OBJ.PAK", b, 0x00220000u, sizeof(b), WF_CDFS_DEST_LWRAM_BULK}
    };
    wf_cdfs_scene_transaction_t txn = {reqs, 2u, false};

    assert(wf_cdfs_runtime_init(&rt, &ops));
    assert(wf_cdfs_request_preflight(&rt, &one, &info) == WF_CDFS_OK);
    assert(info.starting_fad == 150u && info.size == 3000u);
    assert(wf_cdfs_load_one(&rt, &one) == WF_CDFS_OK);
    assert(fs.reads == 1u && a[0] == 0x11u && a[2999] == 0x11u);

    one.name = "TOO-LONG-NAME.PAK";
    assert(wf_cdfs_request_preflight(&rt, &one, NULL) == WF_CDFS_BAD_NAME);
    one.name = "MISS.PAK";
    assert(wf_cdfs_request_preflight(&rt, &one, NULL) == WF_CDFS_NOT_FOUND);
    one.name = "RING.PAK";
    one.dst_capacity = 2999u;
    assert(wf_cdfs_request_preflight(&rt, &one, NULL) == WF_CDFS_DEST_CAPACITY);
    one.dst_capacity = sizeof(a);
    one.dst_bus_addr = 0x00210000u;
    assert(wf_cdfs_request_preflight(&rt, &one, NULL) == WF_CDFS_DEST_RANGE);

    assert(wf_cdfs_scene_preflight(&rt, &txn) == WF_CDFS_OK);
    assert(!txn.ready);
    assert(wf_cdfs_scene_load(&rt, &txn) == WF_CDFS_OK);
    assert(txn.ready && fs.reads == 3u);
    assert(b[0] == 0x22u && b[4095] == 0x22u);

    reqs[1].dst_bus_addr = 0x06010800u;
    reqs[1].dest_kind = WF_CDFS_DEST_HWRAM_STAGING;
    assert(wf_cdfs_scene_preflight(&rt, &txn) == WF_CDFS_DEST_OVERLAP);

    reqs[1].dst_bus_addr = 0x00220000u;
    reqs[1].dest_kind = WF_CDFS_DEST_LWRAM_BULK;
    fs.fail_fad = 152u;
    assert(wf_cdfs_scene_load(&rt, &txn) == WF_CDFS_IO);
    assert(!txn.ready);

    return 0;
}
