#ifndef WF_FRONTEND_RESUME_H
#define WF_FRONTEND_RESUME_H
#include "wf_r18_frontend_driver.h"

/* Reconnect a restored, quiescent AC0 frontend to this executable's original
 * callbacks. Changes only host bindings, never runs RESET or another owner.
 * The restored D/A, SSP/USP/SR, PC, phase, generation and visible state remain
 * authoritative; config.ssp/usp/sr are cold-entry fields and are ignored.
 * Invalid checkpoints/configurations return BAD_STATE without mutation. */
wf_status_t wf_frontend_resume_bind(wf_r18_frontend_driver_t *,
                                  const wf_r18_frontend_driver_config_t *);
#endif
