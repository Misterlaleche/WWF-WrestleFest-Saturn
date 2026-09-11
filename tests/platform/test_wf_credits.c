#include <assert.h>
#include <stdint.h>
#include "../../src/runtime/wf_credits.h"

static void idle_inputs(uint16_t p[4]) { for(unsigned i=0;i<4u;i++) p[i]=0xffffu; }

int main(void)
{
    wf_credits_runtime_t rt;
    wf_credits_events_t ev;
    uint16_t p[4];
    unsigned vblank=0;
    idle_inputs(p);

    assert(WF_CREDITS_FIXED_PAGE_COUNT==9u);
    assert(WF_CREDITS_STAFF_BLOCK_COUNT==6u);
    assert(wf_credits_pages[7].text_id==0x5fu && wf_credits_pages[7].text_x2==112 && wf_credits_pages[7].text_y2==88);
    assert(wf_credits_staff_blocks[5].initial_screen_y==-688);
    assert(!wf_credits_skip_from_raw(p));
    p[2]=(uint16_t)~0x0010u;
    assert(wf_credits_skip_from_raw(p));
    idle_inputs(p);

    wf_credits_begin(&rt);
    for(unsigned i=0;i<23u;i++){wf_credits_step_vblank(&rt,p,&ev);vblank++;assert(rt.state==WF_CREDITS_ENTRY_FADE);assert(!ev.audio_command);}
    wf_credits_step_vblank(&rt,p,&ev);vblank++;
    assert(rt.state==WF_CREDITS_FIXED_PAGES && ev.setup_background && ev.audio_command && ev.audio_command_byte==0x11u && ev.load_fixed_page && ev.fixed_page_index==0u);

    for(unsigned page=0;page<9u;page++){
        wf_credits_step_vblank(&rt,p,&ev);vblank++;
        assert(!ev.load_fixed_page);
        for(unsigned i=0;i<128u;i++){wf_credits_step_vblank(&rt,p,&ev);vblank++;assert(rt.state==WF_CREDITS_FIXED_PAGES);}
        wf_credits_step_vblank(&rt,p,&ev);vblank++;
        if(page<8u){assert(rt.state==WF_CREDITS_FIXED_PAGES);assert(ev.load_fixed_page);assert(ev.fixed_page_index==page+1u);}
        else {assert(rt.state==WF_CREDITS_STAFF_SCROLL);assert(ev.setup_staff);}
    }

    for(unsigned i=0;i<1280u;i++){wf_credits_step_vblank(&rt,p,&ev);vblank++;assert(ev.staff_scroll_update);assert(rt.state==WF_CREDITS_STAFF_SCROLL);}
    wf_credits_step_vblank(&rt,p,&ev);vblank++;
    assert(ev.staff_scroll_update);
    assert(ev.audio_command && ev.audio_command_byte==0x1fu);
    assert(rt.state==WF_CREDITS_EXIT_FADE);
    assert(rt.staff_scroll_accum_16_16==(int32_t)(1281u*0x0000c000u));

    for(unsigned i=0;i<23u;i++){wf_credits_step_vblank(&rt,p,&ev);vblank++;assert(rt.state==WF_CREDITS_EXIT_FADE);}
    wf_credits_step_vblank(&rt,p,&ev);vblank++;
    assert(rt.state==WF_CREDITS_FRONTEND_RETURN && ev.cleanup && ev.frontend_return && ev.frontend_return_rom==0x000006fcu);
    wf_credits_step_vblank(&rt,p,&ev);vblank++;
    assert(rt.state==WF_CREDITS_DONE);

    wf_credits_begin(&rt);
    for(unsigned i=0;i<24u;i++) wf_credits_step_vblank(&rt,p,&ev);
    p[3]=(uint16_t)~0x0020u;
    wf_credits_step_vblank(&rt,p,&ev);
    assert(rt.state==WF_CREDITS_EXIT_FADE && ev.audio_command && ev.audio_command_byte==0x1fu);
    return 0;
}
