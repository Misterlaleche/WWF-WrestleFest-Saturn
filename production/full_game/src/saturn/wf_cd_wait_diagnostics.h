#ifndef WF_CD_WAIT_DIAGNOSTICS_H
#define WF_CD_WAIT_DIAGNOSTICS_H
#include <stdint.h>
/* Separate observer ABI: never extend wp1979_cd_diagnostics_t. Reasons1..3
 * retain the terminal trigger even when a final query returns another error. */
enum { WF_CD_WAIT_NONE, WF_CD_WAIT_PEND, WF_CD_WAIT_POLLS,
       WF_CD_WAIT_NOTICES, WF_CD_WAIT_COMMAND, WF_CD_WAIT_COUNT };
typedef struct {
 uint32_t version,reason,last_hirq,seen_hirq,count,notices,polls;
 uint32_t remaining_sectors,fields_elapsed,fields_available;
 uint32_t requery_attempt;
 int32_t requery_result;
 uint32_t requery_recovered,read_requeries,read_recoveries;
 int32_t result;
} wf_cd_wait_diagnostics_t;
extern volatile wf_cd_wait_diagnostics_t wf_cd_wait_diagnostics;
/* fields_elapsed is read-only VBlank delta, not a wall-clock deadline. It can
 * undercount when interrupts are masked. polls saturates at UINT32_MAX. */
#endif
