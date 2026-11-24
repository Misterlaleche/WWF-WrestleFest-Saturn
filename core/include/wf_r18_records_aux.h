#ifndef WF_R18_RECORDS_AUX_H
#define WF_R18_RECORDS_AUX_H
#include "wf_r18_prematch.h"
#include "wf_r18_metasprite.h"

typedef wf_status_t (*wf_r18_records_aux_store_fn)(void *user,uint32_t address,
                                                uint32_t value,unsigned width);
typedef struct wf_r18_records_aux_context {
 wf_r18_prematch_t *machine;
 wf_r18_read_rom_fn read_rom;
 void *user;
 wf_r18_records_aux_store_fn store_bus;
 uint32_t blocked_dependency,blocked_address;
} wf_r18_records_aux_context_t;
/* Original A4D4 record identity graphics (private77D8/A5BE) and7874 frames.
 * Reads canonical ROM tables and raw16KB RAM; emits literal FG stores of
 * width1/4 through the required real bus adapter. Padding is not rewritten.
 * Context callbacks transport bytes only and must preserve machine D/A/RAM.
 * Successful calls preserve original MOVEM D/A sets. A failing callback or
 * unsupported table read retains the exact executed prefix in live D/A and
 * emitted writes; stop or restore a checkpoint before retrying.
 * Portrait identities0..11 and badge table0..12 are represented. Others
 * block at the first unsupported table-read instruction. TAG second IDs
 * retain D2.W bits15..8 exactly; they are not normalized to a BYTE.
 * A valid entry clears prior diagnostic fields. Invalid context returns
 * WF_BAD_STATE without mutating the context, machine or output.
 * No IDs/defaults,
 * credits, clock, palette conversion or input are substituted.
 * blocked_dependency/machine.dependency are original failing PCs; address
 * is physical24bit. Stack/CCR and hardware timing are outside this ABI.
 */
wf_status_t wf_r18_records_aux_call(wf_r18_records_aux_context_t *,uint32_t owner);
#endif
