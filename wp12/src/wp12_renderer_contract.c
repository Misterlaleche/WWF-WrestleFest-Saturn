#include "wp12_renderer_contract.h"
bool wp12_renderer_project_actor(const wp12_renderer_contract_t *r,const wp12_actor_t *a,wp12_screen_point_t *out){
 if(!r||!r->machine_to_screen||!a||!out)return false;
 return r->machine_to_screen(a->x_machine,a->y_machine,a->z_machine,out,r->user);
}
