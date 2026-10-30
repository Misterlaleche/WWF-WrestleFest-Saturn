#ifndef WF_R18_ROYAL_SETUP_H
#define WF_R18_ROYAL_SETUP_H
#include "wf_r18_prematch.h"
/* Complete Royal branch of original $1086C, including $24CC/$21B4 live RNG,
 * first-free fallbacks, six anchor placements, $10782 health and $10718 referee.
 * All 16KB RAM, live D/A and pre-existing identities/occupancy are inputs.
 * No reset, forced seed, generic roster or TAG setup is substituted.
 * Only $2AEA palette allocation is an external effect in this owner.
 * Success preserves D0-D3/A0-A2 as the original MOVEM does; D4-D7/A3-A6 remain
 * live. Non-Royal, IDs outside the proven twelve-entry window, or topology
 * outside the six-anchor/three-per-half contract block with the owning PC.
 * Failed dependencies retain the executed prefix and are fatal until restored.
 */
wf_status_t wf_r18_royal_setup_1086c(wf_r18_prematch_t *);
/* Shared mode-independent referee owner, also used by the raw TAG setup. */
wf_status_t wf_r18_setup_referee_10718(wf_r18_prematch_t *);
#endif
