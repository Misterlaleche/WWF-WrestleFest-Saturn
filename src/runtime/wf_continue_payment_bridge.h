#ifndef WF_CONTINUE_PAYMENT_BRIDGE_H
#define WF_CONTINUE_PAYMENT_BRIDGE_H
#include <stdbool.h>
#include <stdint.h>
#include "wf_continue_gameover.h"
#include "wf_operator_credits.h"
typedef struct wf_continue_payment_context {wf_credit_state_t *credits;uint16_t operator_config;} wf_continue_payment_context_t;
bool wf_continue_payment_available(uint8_t request_code,void *user);
bool wf_continue_payment_spend(uint8_t request_code,void *user);
bool wf_continue_payment_ops_init(wf_continue_credit_ops_t *ops,wf_continue_payment_context_t *ctx,wf_credit_state_t *credits,uint16_t operator_config);
#endif
