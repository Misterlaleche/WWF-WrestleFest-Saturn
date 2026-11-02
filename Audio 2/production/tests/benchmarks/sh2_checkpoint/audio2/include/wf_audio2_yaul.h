#ifndef WF_AUDIO2_YAUL_H
#define WF_AUDIO2_YAUL_H
#include <stdbool.h>
#include <stdint.h>
/* Static-linked profile only. The game must explicitly reserve the slave SH2
 * for this worker. This adapter cannot coexist with another slave entry owner.
 */
bool wf_audio2_yaul_worker_start(uint32_t workspace,void *user);
bool wf_audio2_yaul_worker_quiescent(void *user);
#endif
