/* Capture the original terminal result rather than the replay's diagnostic
 * green marker. Inputs, RNG fixture, native gameplay and $11B6 condition
 * are the same as the corresponding replay; no artificial early stop. */
#define WF_R18_CAPTURE_RESULT 1
#include "../../src/wf_saturn_entry.c"
