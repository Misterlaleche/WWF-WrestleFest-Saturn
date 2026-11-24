#ifndef WF_JO_SAVE_H
#define WF_JO_SAVE_H
#include <stddef.h>
#include <stdint.h>
#include "wf_r18_save.h"

/* Saturn BIOS library (16 KiB) plus 8312-byte work area (Sega STN-52).
 * The lease rounds up to complete slots in both the supported 4 KiB and
 * 8 KiB sprite packs. The caller lends
 * this memory only after video/DMA users have drained, outside combat.
 * No pointer into the lease is retained after load/store returns. */
#define WF_JO_SAVE_LIBRARY_BYTES 16384u
#define WF_JO_SAVE_BIOS_WORK_BYTES 8312u
#define WF_JO_SAVE_WORKSPACE_BYTES 32768u
typedef enum {
    WF_JO_SAVE_UNAVAILABLE=0, WF_JO_SAVE_DEFAULTS=1, WF_JO_SAVE_LOADED=2,
    WF_JO_SAVE_CORRUPT=3, WF_JO_SAVE_WRITE_ERROR=4, WF_JO_SAVE_SAVED=5
} wf_jo_save_status_t;

/* A successful open must be matched by close, including failure paths.
 * find returns 1/exact file, 0/missing, -1/error; other operations return
 * nonzero on success. No backend operation formats or deletes storage.
 * This small transport boundary also supports ordinary host device tests. */
typedef struct {
    int (*open)(void *,void *,size_t);
    void (*close)(void *);
    int (*find)(void *,const char *,size_t *);
    int (*read)(void *,const char *,void *,size_t);
    int (*write)(void *,const char *,const void *,size_t);
    int (*verify)(void *,const char *,const void *,size_t);
} wf_jo_save_backend_t;
typedef struct {
    const wf_jo_save_backend_t *backend;
    void *user;
    wf_r18_save_data_t data;
    wf_jo_save_status_t status;
    uint8_t writable,cached;
} wf_jo_save_t;

/* NULL backend selects the Jo-owned internal-memory BIOS transport on SH-2.
 * Call init once at boot, load after original RESET creates factory tables,
 * and store only after the original ranking owner has completed. Corrupt or
 * unreadable existing files remain untouched. Missing/unavailable storage
 * never changes the supplied live factory rankings. */
void wf_jo_save_init(wf_jo_save_t *,const wf_jo_save_backend_t *,void *);
wf_jo_save_status_t wf_jo_save_load(wf_jo_save_t *,uint8_t *,void *,size_t);
/* Cheap exact comparison, no BIOS/workspace access. Check before draining
 * rendering or borrowing cache memory; factory-only sessions never write. */
int wf_jo_save_needs_store(const wf_jo_save_t *,const uint8_t *);
wf_jo_save_status_t wf_jo_save_store(wf_jo_save_t *,const uint8_t *,void *,size_t);
#endif
