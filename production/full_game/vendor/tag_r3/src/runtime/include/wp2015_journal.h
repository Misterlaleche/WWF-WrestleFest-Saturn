#ifndef WP2015_JOURNAL_H
#define WP2015_JOURNAL_H
#include "wf_runtime.h"
#define WP2015_JOURNAL_ADDRESS 0x05c5c000u
#define WP2015_JOURNAL_BYTES 0x0d320u
#define WP2015_JOURNAL_CAPACITY 9000u
#define WP2015_JOURNAL_MAGIC 0x57504a32u
void wp2015_journal_begin(const wf_runtime_t *rt,uint32_t vblank);
void wp2015_journal_record(const wf_runtime_t *rt,uint32_t vblank);
#endif
