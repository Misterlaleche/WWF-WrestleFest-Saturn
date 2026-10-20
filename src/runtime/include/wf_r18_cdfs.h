#ifndef WF_R18_CDFS_H
#define WF_R18_CDFS_H
#include <stddef.h>
#include <stdint.h>
/* 0 = success; -1 = transport, format, bounds or missing-file error.
 * Single master-CPU owner; not reentrant. CD block must already be initialized. */
int wf_r18_cdfs_init(void);
int wf_r18_file_size(const char *name, uint32_t *out);
int wf_r18_file_read(const char *name, uint32_t offset, void *dst, size_t bytes);
#endif
