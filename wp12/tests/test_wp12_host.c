#include <assert.h>
#include <string.h>
#include "wp12_prematch.h"
static wp12_actor_t a(uint8_t p56,uint8_t p57){wp12_actor_t x;memset(&x,0,sizeof(x));x.active=true;x.plus56_identity_program=p56;x.plus57_voice_identity=p57;return x;}
int main(void){
 assert(wp12_tag_identity_window_updates(0)==65&&wp12_tag_identity_window_updates(9)==257);
 wp12_actor_t t[6];memset(t,0,sizeof(t));for(unsigned i=0;i<4;i++)t[i]=a(0,0);
 wp12_prematch_t s;wp12_init_tag(&s,t);while(s.phase!=WP12_PHASE_HANDOFF)wp12_step(&s,(wp12_input_t){0});assert(s.total_updates==1249);
 memset(t,0,sizeof(t));for(unsigned i=0;i<6;i++)t[i]=a(0,0);wp12_init_royal(&s,t);while(s.phase==WP12_PHASE_VISIBLE)wp12_step(&s,(wp12_input_t){0});assert(s.total_updates==64);
 return 0;
}
