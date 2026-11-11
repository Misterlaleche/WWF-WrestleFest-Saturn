#ifndef WF_FRONTEND_SAVE_H
#define WF_FRONTEND_SAVE_H
#include <stddef.h>
#include <stdint.h>
#include "wf_r18_save_saturn.h"

/* Frontend executable only. Backup absence/full/corruption is a nonfatal
 * status. No extra state is carried through the combat capsule. */
wf_r18_save_saturn_status_t wf_frontend_save_init(void);
/* Cold RESET ready callback only, after original $6F6 installed defaults. */
wf_r18_save_saturn_status_t wf_frontend_save_load(uint8_t *ram,size_t bytes);
/* After frontend CRT/reload: inspect BUP through a local ranking copy, keeping
 * restored RAM authoritative. Original load creates a missing file. */
wf_r18_save_saturn_status_t wf_frontend_save_rebind(const uint8_t *ram,size_t bytes);
/* Original post owner completion/cancellation boundary. An uninspected new
 * executable rebinds first, so CORRUPT protection survives a CRT reset. */
wf_r18_save_saturn_status_t wf_frontend_save_store(const uint8_t *ram,size_t bytes);
#endif
