#include <assert.h>
#include "../../src/runtime/wf_hud_core.h"
static void stamina(void){wf_hud_stamina_t s;wf_hud_bar_cell_t c[6];wf_hud_stamina_init(&s,100,120);wf_hud_stamina_queue(&s,-5);wf_hud_stamina_update(&s);assert(s.displayed==98&&s.accumulator==-3&&s.queued_delta==0);wf_hud_stamina_update(&s);assert(s.displayed==96&&s.accumulator==-1);wf_hud_stamina_update(&s);assert(s.displayed==95&&s.accumulator==0);wf_hud_stamina_cells(0,0,c);for(int i=0;i<6;i++)assert(c[i].upper_tile==0x0b33&&c[i].lower_tile==0x0b3a);wf_hud_stamina_cells(96,0,c);for(int i=0;i<6;i++)assert(c[i].upper_tile==0x0ae1&&c[i].lower_tile==0x0aec);wf_hud_stamina_cells(95,2,c);assert(c[5].upper_tile==0x0af9&&c[5].lower_tile==0x0b07);}
static void timer(void){wf_hud_timer_t t;wf_hud_timer_event_t e;wf_hud_timer_reset(&t);wf_hud_timer_step(&t,true,true,&e);assert(t.minutes_bcd==0x30&&t.seconds_bcd==0x00&&e.redraw);for(int i=0;i<3;i++){wf_hud_timer_step(&t,true,true,&e);assert(!e.redraw);}wf_hud_timer_step(&t,true,true,&e);assert(t.minutes_bcd==0x29&&t.seconds_bcd==0x59&&e.redraw);t.initialized=true;t.minutes_bcd=0x06;t.seconds_bcd=0;t.subcounter=1;t.five_minute_event_seen=false;wf_hud_timer_step(&t,true,true,&e);assert(t.minutes_bcd==0x05&&t.seconds_bcd==0x59&&e.five_minute_event&&e.command_byte==0x0b);t.minutes_bcd=0;t.seconds_bcd=0;t.subcounter=1;wf_hud_timer_step(&t,true,true,&e);assert(t.time_up&&e.time_up&&t.minutes_bcd==0&&t.seconds_bcd==0&&e.transition_gate_value==3);}
static void renderer_contract(void){
 wf_hud_cell_t c;uint16_t raw=0;
 assert(wf_hud_xbgr444_to_rgb888(0x000f)==0x00ff0000u);
 assert(wf_hud_xbgr444_to_rgb888(0x00f0)==0x0000ff00u);
 assert(wf_hud_xbgr444_to_rgb888(0x0f00)==0x000000ffu);
 for(unsigned v=0;v<4096u;v++){unsigned r=v&15u,g=(v>>4)&15u,b=(v>>8)&15u;unsigned exp=(((r<<4)|r)<<16)|(((g<<4)|g)<<8)|((b<<4)|b);assert(wf_hud_xbgr444_to_rgb888((uint16_t)v)==exp);}
 assert(wf_hud_portrait_descriptor(0,&raw)&&raw==0x21d0u);
 assert(!wf_hud_portrait_descriptor(5,&raw));
 assert(wf_hud_portrait_cell(4,&c)&&c.tile==0x01f4u&&c.palette==0x0du&&c.raw_word==0xd1f4u);
 c=wf_hud_portrait_grid_cell(0,2,2);assert(c.raw_word==0x21d8u);
 c=wf_hud_stamina_cell(95,2,5,false);assert(c.tile==0x0af9u&&c.palette==1u);
 c=wf_hud_timer_digit_cell(5,false);assert(c.tile==0x00adu&&c.palette==0x0eu);
 wf_hud_count_layout_t n=wf_hud_count_layout(10,false);assert(n.row==7u&&n.col==17u&&n.width_cells==4u&&n.raw_attribute==0x00eau);
 assert(WF_HUD_CRAM_MODE==2u&&WF_HUD_TEXT_CRAM_FIRST==0x000u&&WF_HUD_TEXT_CRAM_LAST==0x0ffu&&WF_HUD_VDP2_PLANE_NBG2==2u);
}
int main(void){stamina();timer();renderer_contract();assert(wf_hud_slots[3].fg_offset==0x0178);assert(wf_hud_player_indicator_tiles[3]==0x0b2a);assert(wf_hud_portrait_descriptors[11]==0xd233);return 0;}
