#ifndef WF_ALLIES_TRACE_H
#define WF_ALLIES_TRACE_H
#include "wf_runtime.h"
void allies_trace_begin(wf_runtime_t *,unsigned,unsigned,uint32_t);
void allies_trace_end(wf_runtime_t *,unsigned,unsigned,uint32_t,wf_status_t);
#endif
