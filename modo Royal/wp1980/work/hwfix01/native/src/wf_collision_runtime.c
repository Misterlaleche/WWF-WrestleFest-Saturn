#include "wf_collision_runtime.h"
const wf_box4_t wf_source_boxes[11]={{-12,0,-1,96},{-24,0,-1,96},{-32,0,-1,96},{-40,0,-1,96},{-48,0,-1,96},{-56,0,-1,96},{-64,0,-1,96},{-72,0,-1,96},{-80,0,-1,96},{-6,0,-1,96},{-16,0,-1,96}};
const wf_box4_t wf_target_boxes[11]={{-16,0,32,96},{-12,0,12,96},{-8,0,8,48},{-16,48,-32,96},{16,0,48,96},{-16,64,-1,127},{-28,0,16,80},{8,0,-32,127},{16,0,40,96},{-28,0,-1,96},{0,0,64,96}};
static uint16_t addw(uint16_t a,int8_t b){return (uint16_t)(a+(int16_t)b);} static int8_t negb(int8_t v){return (int8_t)(uint8_t)(-(uint8_t)v);}
static int uov4(uint16_t s0,uint16_t s1,uint16_t t0,uint16_t t1){return (t0>=s0&&s1>=t0)||(t1>=s0&&s1>=t1)||(s0>=t0&&t1>=s0)||(s1>=t0&&t1>=s1);}
static int uov3(uint16_t s0,uint16_t s1,uint16_t t0,uint16_t t1){return (t0>=s0&&s1>=t0)||(t1>=s0&&s1>=t1)||(s0>=t0&&t1>=s0);}
int wf_collision_axis06_overlap(const wf_actor_raw_t*s,const wf_box4_t*sb,const wf_actor_raw_t*t,const wf_box4_t*tb){
 int8_t sx0=sb->x0,sx1=sb->x1,tx0=tb->x0,tx1=tb->x1;
 /* $24278/$2427C and $2429A/$2429E negate BYTE endpoints, then EXG them. */
 if(wf_u8(s,WF_A_MIRROR)&0x80u){int8_t old=sx0;sx0=negb(sx1);sx1=negb(old);}
 if(wf_u8(t,WF_A_MIRROR)&0x80u){int8_t old=tx0;tx0=negb(tx1);tx1=negb(old);}
 return uov4(addw(wf_be16(s,WF_A_X),sx0),addw(wf_be16(s,WF_A_X),sx1),addw(wf_be16(t,WF_A_X),tx0),addw(wf_be16(t,WF_A_X),tx1));
}
int wf_collision_axis0e_overlap(const wf_actor_raw_t*s,const wf_box4_t*sb,const wf_actor_raw_t*t,const wf_box4_t*tb){return uov3(addw(wf_be16(s,WF_A_Z),sb->z0),addw(wf_be16(s,WF_A_Z),sb->z1),addw(wf_be16(t,WF_A_Z),tb->z0),addw(wf_be16(t,WF_A_Z),tb->z1));}
void wf_collision_write_profile_damage(wf_actor_raw_t*d,uint8_t amount){wf_set_u8(d,WF_A_PENDING_DAMAGE+1u,amount);}
void wf_collision_success_tail(wf_actor_raw_t*s,unsigned ss,wf_actor_raw_t*d,unsigned ds){
 static const uint32_t base[9]={0x001C05B0,0x001C06BC,0x001C07C8,0x001C08D4,0x001C09E0,0x001C0AEC,0x001C0BF8,0x001C0D04,0x001C0E10};
 uint16_t mask=(uint16_t)(0x8000u>>ds); wf_set_be16(s,WF_A_COLL_CACHE,(uint16_t)(wf_be16(s,WF_A_COLL_CACHE)|mask));
 wf_set_be16(s,WF_A_TIMER44,0);wf_set_be16(s,WF_A_TIMER46,0);wf_set_be16(s,WF_A_PROFILE,0);wf_set_be32(s,WF_A_RECIP_LINK,base[ds]);
 wf_set_be16(d,WF_A_PROFILE,0);wf_set_be16(d,WF_A_TIMER46,0);wf_set_be16(d,0x18,0);wf_set_be16(d,0x1A,0);wf_set_u8(d,WF_A_FLAGS33,(uint8_t)(wf_u8(d,WF_A_FLAGS33)&~0x10u));wf_set_be32(d,WF_A_RECIP_LINK,base[ss]);wf_set_be16(d,WF_A_COLL_COUNT,(uint16_t)(wf_be16(d,WF_A_COLL_COUNT)+1u));
 if(wf_be16(d,WF_A_TIER)==2u)wf_set_u8(d,WF_A_STATE4,(uint8_t)(wf_state4_flags(d)|0x80u));
}
