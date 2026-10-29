#ifndef WF_R18_CDFS_H
#define WF_R18_CDFS_H
#include <stddef.h>
#include <stdint.h>
/* 0 = success; -1 = transport, format, bounds or missing-file error.
 * Single master-CPU owner; not reentrant. CD block must already be initialized. */
int wf_r18_cdfs_init(void);
void wf_r18_cdfs_combat_window_set(unsigned combat);
int wf_r18_file_size(const char *name, uint32_t *out);
int wf_r18_file_read(const char *name, uint32_t offset, void *dst, size_t bytes);
/* Read-only counters for a frozen fault report. Bulk sector reads contribute
 * to sectors, while windows counts only cache-window refills. */
void wf_r18_cdfs_stats(uint32_t *windows, uint32_t *sectors);
/* Cache refills and bulk sectors by source: ROM, raw sprites, resident
 * sprite packs, scenes, audio, other. Counters change only on CD commands. */
#define WF_R18_CDFS_FILE_CLASSES 6u
void wf_r18_cdfs_file_stats(uint32_t refills[WF_R18_CDFS_FILE_CLASSES],
                             uint32_t bulk_sectors[WF_R18_CDFS_FILE_CLASSES]);
#endif
