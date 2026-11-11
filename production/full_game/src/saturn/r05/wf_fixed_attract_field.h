#ifndef WF_FIXED_ATTRACT_FIELD_H
#define WF_FIXED_ATTRACT_FIELD_H
#include <stdbool.h>
#include <stdint.h>
/* One real input/arcade-clock sample per observed physical field, shared by
 * both prefill targets. Never replay an old pad sample at CD busy-loop rate. */
typedef struct {uint32_t last_field;int result;} wf_fixed_attract_field_t;
static inline int wf_fixed_attract_field_service(wf_fixed_attract_field_t*g,
 uint32_t field,int(*on_field)(void*),void*user){
 if(g->result||field==g->last_field)return g->result;
 g->last_field=field;g->result=on_field?on_field(user):0;return g->result;
}
/* Accepted credit/failure is terminal: no further prefetch burst may start.
 * The caller retains the current transfer and enters cooperative stop. */
static inline int wf_fixed_attract_prefill_step(wf_fixed_attract_field_t*g,
 uint32_t field,int(*on_field)(void*),void*user,bool(*io_service)(void)){
 int result=wf_fixed_attract_field_service(g,field,on_field,user);
 if(result)return result;
 if(!io_service()){g->result=-1;return -1;}return 0;
}
#endif
