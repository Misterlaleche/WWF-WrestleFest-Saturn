#include "wf_payment.h"

static int spend_whole(wf_runtime_t *rt,uint8_t n){
 if(rt->g.credit_count_004f<n)return 0;
 rt->g.credit_count_004f=(uint8_t)(rt->g.credit_count_004f-n);
 return 1;
}
static int spend_fixed_one(wf_runtime_t *rt){
 if(rt->g.credit_remainder_004e!=0u){rt->g.credit_remainder_004e--;return 1;}
 if(rt->g.credit_count_004f==0u)return 0;
 rt->g.credit_count_004f--;
 rt->g.credit_remainder_004e=rt->g.credit_cycle_units_004b;
 rt->g.credit_remainder_004e--;
 return 1;
}
static int spend_fixed_two(wf_runtime_t *rt){
 if(rt->g.credit_count_004f==0u && rt->g.credit_remainder_004e<2u)return 0;
 if(rt->g.credit_remainder_004e>=2u){rt->g.credit_remainder_004e=(uint8_t)(rt->g.credit_remainder_004e-2u);return 1;}
 rt->g.credit_count_004f--;
 rt->g.credit_remainder_004e=(uint8_t)(rt->g.credit_remainder_004e+rt->g.credit_cycle_units_004b);
 rt->g.credit_remainder_004e=(uint8_t)(rt->g.credit_remainder_004e-2u);
 return 1;
}
int wf_payment_consume_055a(wf_runtime_t *rt,uint8_t mode){
 if(!rt)return 0;
 switch(mode){
  case 0u:return spend_whole(rt,1u); /* $5CE */
  case 1u:return (rt->g.config_0066&0x0010u)?spend_whole(rt,1u):spend_fixed_one(rt);
  case 2u:return (rt->g.config_0066&0x0008u)?spend_whole(rt,1u):spend_fixed_one(rt);
  case 3u:
   if((rt->g.config_0066&0x0010u)!=0u)return spend_whole(rt,2u);
   if((rt->g.config_0066&0x0003u)==0u)return spend_whole(rt,2u);
   return spend_fixed_two(rt);
  case 4u:return spend_whole(rt,2u);
  case 5u:return (rt->g.config_0066&0x0004u)?spend_whole(rt,1u):spend_fixed_one(rt);
  default:return 0;
 }
}
