#include "wf_continue_payment_bridge.h"
bool wf_continue_payment_available(uint8_t request_code,void *user){
 wf_continue_payment_context_t *ctx=(wf_continue_payment_context_t*)user;
 if(!ctx||!ctx->credits||(request_code!=1u&&request_code!=3u))return false;
 return wf_credit_available(ctx->credits,ctx->operator_config,request_code);
}
bool wf_continue_payment_spend(uint8_t request_code,void *user){
 wf_continue_payment_context_t *ctx=(wf_continue_payment_context_t*)user;
 if(!ctx||!ctx->credits||(request_code!=1u&&request_code!=3u))return false;
 return wf_credit_pay(ctx->credits,ctx->operator_config,request_code);
}
bool wf_continue_payment_ops_init(wf_continue_credit_ops_t *ops,wf_continue_payment_context_t *ctx,wf_credit_state_t *credits,uint16_t operator_config){
 if(!ops||!ctx||!credits)return false;
 ctx->credits=credits;ctx->operator_config=operator_config;
 ops->available=wf_continue_payment_available;ops->spend=wf_continue_payment_spend;ops->user=ctx;return true;
}
