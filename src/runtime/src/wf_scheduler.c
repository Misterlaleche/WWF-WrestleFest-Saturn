#include <string.h>
#include "wf_runtime.h"

typedef struct { uint32_t addr; } phase_t;
static const phase_t branch_a[] = {
 {0x00514E},{0x008B3A},{0x02088E},{0x020958},{0x026936},{0x02983C},{0x00F560},{0x00F18A},{0x01C150},{0x01F914},{0x00F4C2},{0x024062},{0x024E58},{0x00FDEE},{0x010120},{0x010E6A},{0x007A00},{0x0088A2},{0x008710},{0x0262D2},{0x00BB14},{0x007366},{0x008CF8},{0x009052},{0x00899C},{0x008ECC},{0x002836},{0x0018C4},{0x0298EC}
};
static const phase_t branch_b[] = {
 {0x0285DA},{0x026A42},{0x026936},{0x02983C},{0x00514E},{0x008B3A},{0x02088E},{0x020958},{0x00F560},{0x00F18A},{0x01C150},{0x01F914},{0x00F4C2},{0x00FDEE},{0x010120},{0x010E6A},{0x007548},{0x008710},{0x007A00},{0x007366},{0x008CF8},{0x00BCFE},{0x00C04A},{0x00C710},{0x002836},{0x0298EC}
};
void wf_runtime_init(wf_runtime_t *rt, wf_phase_binding_t *b,size_t n){ memset(rt,0,sizeof(*rt));rt->bindings=b;rt->binding_count=n; }
wf_status_t wf_dispatch_phase(wf_runtime_t *rt,uint32_t addr){
 if(rt->trace_count<64) rt->trace[rt->trace_count++]=addr;
 for(size_t i=0;i<rt->binding_count;i++) if(rt->bindings[i].rom_addr==addr) return rt->bindings[i].fn(rt,addr,rt->bindings[i].user);
 return WF_BLOCKED; /* Never silently skip an unknown ROM phase. */
}
wf_status_t wf_runtime_frame(wf_runtime_t *rt){
 rt->trace_count=0; rt->frame_ready=0; rt->vblank_ack=0; rt->g.frame_handshake[0]=rt->g.frame_handshake[1]=0;
 const phase_t *p; size_t n;
 if((rt->g.pipeline_mode&1u)==0){ p=branch_a;n=sizeof(branch_a)/sizeof(branch_a[0]); }
 else { p=branch_b;n=sizeof(branch_b)/sizeof(branch_b[0]); }
 for(size_t i=0;i<n;i++){ wf_status_t s=wf_dispatch_phase(rt,p[i].addr); if(s!=WF_OK) return s; }
 rt->frame_ready=1;
 if(!rt->vblank_ack) return WF_BLOCKED; /* platform VBlank owner must acknowledge */
 rt->g.frame_counter++;
 return WF_OK;
}
