#include "wf_r17_transient_reset_10e6a.h"

wf_status_t wf_r17_transient_reset_10e6a(wf_runtime_t *rt){
 unsigned i;
 if(!rt)return WF_BAD_STATE;
 for(i=0u;i<WF_R17_TRANSIENT_RECORDS;i++)
  rt->transient_1258[i][0]=(uint8_t)(rt->transient_1258[i][0]&0x7fu);
 /* $10E70 MOVE.W #11,D0; twelve DBF iterations end at $FFFF. */
 wf_r17_m68k_move_w(&rt->m68k_regs,0u,0xffffu);
 return WF_OK;
}
