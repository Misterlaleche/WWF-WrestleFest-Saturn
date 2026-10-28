/* Deterministic reset seed for a live-input hardware-path validation.
 * The regular SMPC pad reader remains active; no input replay is compiled. */
#define WF_R18_BOOT_SEED_ZERO 1
#include "../../src/wf_saturn_entry.c"
