#ifndef WF_R18_RANKING_GLYPH_H
#define WF_R18_RANKING_GLYPH_H
#include "wf_r18_prematch.h"
typedef wf_status_t (*wf_r18_ranking_glyph_store_fn)(void *,uint32_t,uint32_t,unsigned);
typedef struct wf_r18_ranking_glyph_context {
 wf_r18_prematch_t *machine;
 wf_r18_ranking_glyph_store_fn store_bus;
 void *user;
 uint32_t blocked_dependency,blocked_address;
} wf_r18_ranking_glyph_context_t;
/* Original6CFA and its literal6D76 helper. BYTE/LONG stores use physical24bit
 * addresses and their actual width. The callback transports real bus writes
 * without changing D/A/rawRAM. No ROM/input/IRQ/default glyph is supplied.
 * 6CFA restores D0-D4/A2 only at its successful MOVEM; failures retain the
 * exact executed prefix. 6D76 modifies lowWORD D3 even when entered directly.
 * CCR/stack are outside this ABI. Stop or restore a checkpoint after failure.
 */
wf_status_t wf_r18_ranking_glyph_call(wf_r18_ranking_glyph_context_t *,uint32_t owner);
#endif
