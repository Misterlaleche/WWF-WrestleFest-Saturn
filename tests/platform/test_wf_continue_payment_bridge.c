#include <assert.h>
#include "../../src/runtime/wf_continue_payment_bridge.h"

int main(void){
 wf_credit_state_t credits;wf_continue_payment_context_t ctx;wf_continue_credit_ops_t ops;wf_continue_runtime_t rt;wf_continue_event_t ev;wf_continue_pair_inputs_t in={0xff7f,0xffff};
 wf_credit_boot_init(&credits,3u);
 assert(wf_continue_payment_ops_init(&ops,&ctx,&credits,3u));
 assert(wf_continue_begin(&rt,WF_CONTINUE_PAIR_A,1u));
 wf_continue_step(&rt,&in,&ops,&ev);assert(!ev.credit_available&&!ev.accepted&&credits.credits==0u);
 wf_coin_event_t coin;wf_credit_accept_pulse(&credits,&coin);wf_credit_accept_pulse(&credits,&coin);assert(credits.credits==0u&&credits.partial==2u);
 wf_continue_step(&rt,&in,&ops,&ev);assert(ev.credit_available&&ev.accepted);assert(credits.partial==1u&&credits.credits==0u);
 wf_credit_boot_init(&credits,0u);credits.credits=2u;assert(wf_continue_begin(&rt,WF_CONTINUE_PAIR_B,3u));in.player_a_active_low=0xffffu;in.player_b_active_low=0xff7fu;
 assert(wf_continue_payment_ops_init(&ops,&ctx,&credits,WF_OPERATOR_CONFIG_CONTINUE_AS_START));wf_continue_step(&rt,&in,&ops,&ev);assert(ev.accepted&&credits.credits==0u);
 return 0;
}
