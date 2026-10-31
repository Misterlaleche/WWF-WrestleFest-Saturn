#include "wf_frontend.h"
#include <assert.h>
#include <stdio.h>
static wf_frontend_effect_t step(wf_frontend_t*f,unsigned d,int a){wf_frontend_effect_t e;assert(wf_frontend_tick(f,d,a,&e));return e;}
static wf_frontend_effect_t press(wf_frontend_t*f,unsigned d){step(f,0,0);return step(f,d,0);}
int main(void){
 wf_frontend_t f;wf_frontend_effect_t e;
 wf_frontend_init(&f);assert(f.state==WF_FRONTEND_MODE&&!f.royal);
 for(unsigned i=0;i<4;i++){e=step(&f,0,0);assert(!e.palette);}
 e=step(&f,0,0);assert(e.palette&&e.phase==1&&!e.family);
 e=step(&f,2,0);assert(f.royal&&e.palette&&!e.family&&e.phase==0);
 step(&f,0,1);assert(f.state==WF_FRONTEND_LOAD_ROSTER);
 assert(!wf_frontend_tick(&f,1,0,&e)); /* loading cannot advance */
 wf_frontend_roster_ready(&f);assert(f.state==WF_FRONTEND_ROSTER);
 e=press(&f,1);assert(e.cursor&&e.old_slot==0&&f.slot==1);
 step(&f,1,0);assert(f.slot==1); /* held direction is not a fresh edge */
 press(&f,1);assert(f.slot==2);
 e=press(&f,1);assert(f.slot==3&&f.state==WF_FRONTEND_SCROLL&&e.scroll&&f.scroll==328);
 for(unsigned i=1;i<26;i++){e=step(&f,0,1);assert(e.scroll&&f.scroll==320+8*(i+1));}
 assert(f.state==WF_FRONTEND_SCROLL);
 e=step(&f,0,1);assert(!e.scroll&&!e.confirm);
 assert(f.page==1&&f.state==WF_FRONTEND_ROSTER); /* confirm ignored during original scroll loop */
 press(&f,2);assert(f.slot==2&&f.page==1);
 press(&f,2);assert(f.slot==1&&f.state==WF_FRONTEND_SCROLL);
 for(unsigned i=1;i<26;i++)step(&f,0,0);
 e=step(&f,0,0);assert(!e.scroll);
 assert(f.scroll==320&&f.page==0);
 e=step(&f,0,1);assert(e.confirm&&f.state==WF_FRONTEND_DONE&&f.slot==1&&f.royal);
 assert(!wf_frontend_tick(&f,0,0,&e)); /* terminal does not restart */
 wf_frontend_init(&f);step(&f,0,1);wf_frontend_roster_ready(&f);
 assert(!wf_frontend_tick(&f,12,0,&e));assert(f.state==WF_FRONTEND_ERROR); /* no OOB table */
 /* The approved right-page tables differ at the tenth wrestler slot. */
 for(unsigned royal=0;royal<2;royal++){
  wf_frontend_init(&f);if(royal)step(&f,2,0);step(&f,0,1);wf_frontend_roster_ready(&f);
  press(&f,8);assert(f.slot==5);press(&f,1);assert(f.slot==6);press(&f,1);assert(f.slot==7);
  press(&f,1);for(unsigned i=1;i<27;i++)step(&f,0,0);
  assert(f.slot==8&&f.page==1);
  press(&f,1);assert(f.slot==(royal?9u:8u));
  e=step(&f,0,1);assert(e.confirm&&f.state==WF_FRONTEND_DONE);
 }
 puts("frontend: palette, load barrier, edges, both26-step scrolls, terminal, invalid input PASS");
}
