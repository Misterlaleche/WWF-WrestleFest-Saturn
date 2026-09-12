#include "wf_hud_core.h"
#include <string.h>
const wf_hud_slot_meta_t wf_hud_slots[WF_HUD_SLOT_COUNT]={
 {0x0104,0x0104,0x0204,0x0310,0x0410,1,1},{0x0128,0x0128,0x0228,0x0334,0x0434,1,10},
 {0x0154,0x0154,0x0254,0x0360,0x0460,1,21},{0x0178,0x0178,0x0278,0x0384,0x0484,1,30}
};
const uint16_t wf_hud_player_indicator_tiles[WF_HUD_PLAYER_INDICATOR_COUNT]={0x0B0F,0x0B18,0x0B21,0x0B2A};
const uint16_t wf_hud_portrait_descriptors[WF_HUD_PORTRAIT_COUNT]={0x21D0,0x31D9,0x41EB,0x51E2,0xD1F4,0x71FD,0x8206,0x920F,0xA218,0xB221,0xC22A,0xD233};
void wf_hud_stamina_init(wf_hud_stamina_t*s,uint16_t live,uint16_t max){if(!s)return;s->queued_delta=0;s->accumulator=0;s->maximum=max;s->displayed=(int16_t)live;if(s->displayed<0)s->displayed=0;if((uint16_t)s->displayed>=max)s->displayed=(int16_t)max;}
void wf_hud_stamina_queue(wf_hud_stamina_t*s,int16_t delta){if(s)s->queued_delta=(int16_t)(s->queued_delta+delta);}
void wf_hud_stamina_update(wf_hud_stamina_t*s){int16_t step;if(!s)return;s->accumulator=(int16_t)(s->accumulator+s->queued_delta);s->queued_delta=0;if(s->accumulator>0)step=s->accumulator>1?2:1;else if(s->accumulator<0)step=s->accumulator<-1?-2:-1;else step=0;s->displayed=(int16_t)(s->displayed+step);s->accumulator=(int16_t)(s->accumulator-step);if(s->displayed<0){s->displayed=0;s->accumulator=0;}else if((uint16_t)s->displayed>=s->maximum){s->displayed=(int16_t)s->maximum;s->accumulator=0;}}
void wf_hud_stamina_cells(uint8_t v,unsigned slot,wf_hud_bar_cell_t out[WF_HUD_BAR_COLUMNS]){unsigned q=v/16u,r=v%16u,i=0;if(!out)return;if(q>6u)q=6u;for(;i<q;i++){const unsigned f=8u;uint16_t u=(slot&2u)?(uint16_t)(0x0AF2u+f):(uint16_t)(0x0AD9u+f);out[i].upper_tile=u;out[i].lower_tile=(uint16_t)(u+((slot&2u)?14u:11u));}if(i<6u&&r){unsigned f=(r-(r&1u))/2u;uint16_t u;if(f==0u){out[i].upper_tile=0x0B33u;out[i].lower_tile=0x0B3Au;}else{u=(slot&2u)?(uint16_t)(0x0AF2u+f):(uint16_t)(0x0AD9u+f);out[i].upper_tile=u;out[i].lower_tile=(uint16_t)(u+((slot&2u)?14u:11u));}i++;}for(;i<6u;i++){out[i].upper_tile=0x0B33u;out[i].lower_tile=0x0B3Au;}}
static uint8_t bcd_dec(uint8_t v,bool *borrow){uint8_t lo=v&0x0fu,hi=(v>>4)&0x0fu;if(lo){*borrow=false;return(uint8_t)(v-1u);}if(hi){*borrow=false;return(uint8_t)(((hi-1u)<<4)|9u);}*borrow=true;return 0x99u;}
void wf_hud_timer_reset(wf_hud_timer_t*t){if(t)memset(t,0,sizeof(*t));}
void wf_hud_timer_step(wf_hud_timer_t*t,bool gate,bool enabled,wf_hud_timer_event_t*e){bool b,bm;if(e)memset(e,0,sizeof(*e));if(!t||!gate||!enabled||t->time_up)return;if(!t->initialized){t->initialized=true;t->minutes_bcd=WF_HUD_TIMER_SEED_MINUTES_BCD;t->seconds_bcd=WF_HUD_TIMER_SEED_SECONDS_BCD;t->subcounter=WF_HUD_TIMER_SUBCOUNTER_SEED;}if(t->subcounter>0u)t->subcounter--;if(t->subcounter!=0u)return;t->subcounter=WF_HUD_TIMER_SUBCOUNTER_RELOAD;t->seconds_bcd=bcd_dec(t->seconds_bcd,&b);if(b){t->seconds_bcd=0x59u;t->minutes_bcd=bcd_dec(t->minutes_bcd,&bm);if(bm){t->minutes_bcd=0;t->seconds_bcd=0;t->time_up=true;if(e){e->redraw=true;e->time_up=true;e->transition_gate_value=WF_HUD_TIMER_TIMEUP_TRANSITION;}return;}if(t->minutes_bcd==WF_HUD_TIMER_FIVE_MINUTES_BCD&&!t->five_minute_event_seen){t->five_minute_event_seen=true;if(e){e->five_minute_event=true;e->command_byte=WF_HUD_TIMER_FIVE_MINUTE_COMMAND;}}}if(e)e->redraw=true;}


wf_hud_cell_t wf_hud_text_cell_from_raw(uint16_t raw_word){
 const uint8_t lo=(uint8_t)(raw_word&0x00ffu);
 const uint8_t hi=(uint8_t)(raw_word>>8);
 return (wf_hud_cell_t){(uint16_t)((uint16_t)lo|((uint16_t)(hi&0x0fu)<<8)),(uint8_t)(hi>>4),raw_word};
}
bool wf_hud_portrait_descriptor(uint8_t selector,uint16_t*raw_descriptor){
 if(!raw_descriptor||selector>=WF_HUD_PORTRAIT_COUNT||selector==5u)return false;
 *raw_descriptor=wf_hud_portrait_descriptors[selector];return true;
}
bool wf_hud_portrait_cell(uint8_t selector,wf_hud_cell_t*cell){
 uint16_t raw=0u;if(!cell||!wf_hud_portrait_descriptor(selector,&raw))return false;
 *cell=wf_hud_text_cell_from_raw(raw);return true;
}
wf_hud_cell_t wf_hud_portrait_grid_cell(uint8_t selector,uint8_t x,uint8_t y){
 uint16_t raw=0u;if(x>=3u||y>=3u||!wf_hud_portrait_descriptor(selector,&raw))return (wf_hud_cell_t){0u,0u,0u};
 raw=(uint16_t)(raw+(uint16_t)y*3u+(uint16_t)x);return wf_hud_text_cell_from_raw(raw);
}
wf_hud_cell_t wf_hud_stamina_cell(uint8_t v,uint8_t slot,uint8_t column,bool lower){
 uint8_t q=(uint8_t)(v/16u),r=(uint8_t)(v%16u),fill=0u;uint16_t tile;
 if(q>6u)q=6u;
 if(column<q)fill=8u;else if(column==q&&q<6u&&r!=0u)fill=(uint8_t)((r&0x0eu)/2u);
 if(fill==0u)tile=lower?0x0b3au:0x0b33u;
 else if((slot&2u)!=0u)tile=(uint16_t)(0x0af2u+fill+(lower?14u:0u));
 else tile=(uint16_t)(0x0ad9u+fill+(lower?11u:0u));
 return (wf_hud_cell_t){tile,1u,0u};
}
wf_hud_cell_t wf_hud_timer_digit_cell(uint8_t digit,bool lower){
 if(digit>9u)digit=0u;
 return (wf_hud_cell_t){(uint16_t)((lower?0x00b3u:0x00a8u)+digit),0x0eu,0u};
}
wf_hud_count_layout_t wf_hud_count_layout(uint8_t value,bool presentation){
 if(presentation)return (wf_hud_count_layout_t){17u,34u,2u,4u,0x00eau};
 if(value<10u)return (wf_hud_count_layout_t){7u,18u,2u,4u,0x00eau};
 return (wf_hud_count_layout_t){7u,17u,4u,4u,0x00eau};
}
uint32_t wf_hud_xbgr444_to_rgb888(uint16_t c){
 const uint32_t r4=(uint32_t)c&0x0fu,g4=((uint32_t)c>>4)&0x0fu,b4=((uint32_t)c>>8)&0x0fu;
 const uint32_t r8=(r4<<4)|r4,g8=(g4<<4)|g4,b8=(b4<<4)|b4;
 return (r8<<16)|(g8<<8)|b8;
}
