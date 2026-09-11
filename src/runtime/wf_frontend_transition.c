#include "wf_frontend_transition.h"
#include <string.h>

static const uint8_t k_fade[16][8] = {
 {0,0,0,0,0,0,0,0},
 {0,0,0,0,1,1,1,1},
 {0,0,0,1,1,1,2,2},
 {0,0,1,1,2,2,3,3},
 {0,0,1,2,3,3,4,4},
 {0,1,1,2,3,3,4,5},
 {0,1,2,3,3,4,5,6},
 {0,1,2,3,4,5,6,7},
 {0,1,2,3,4,5,6,8},
 {0,2,3,4,5,6,7,9},
 {0,1,2,4,5,7,8,10},
 {0,1,3,4,5,6,8,11},
 {0,2,3,4,5,7,9,12},
 {0,2,4,5,7,8,10,13},
 {0,1,3,5,7,9,11,14},
 {0,2,4,6,8,10,12,15}
};

static void clear_event(wf_frontend_transition_event_t *event)
{
    if (event) memset(event,0,sizeof(*event));
}

uint8_t wf_frontend_fade_map_nibble(uint8_t input_nibble,uint8_t level)
{
    return k_fade[input_nibble & 0x0Fu][level & 0x07u];
}

static void begin(wf_frontend_transition_t *rt,
                  wf_frontend_transition_kind_t kind,
                  uint16_t total,
                  uint8_t start_level)
{
    if (!rt) return;
    memset(rt,0,sizeof(*rt));
    rt->kind=kind;
    rt->remaining_vblanks=total;
    rt->total_vblanks=total;
    rt->current_level=start_level;
    rt->active=total!=0u;
}

void wf_frontend_transition_begin_fade(wf_frontend_transition_t *rt)
{
    begin(rt,WF_FRONTEND_TRANSITION_PALETTE_FADE_BLACK,WF_FRONTEND_FADE_TOTAL_VBLANKS,7u);
}

void wf_frontend_transition_begin_stripe_close(wf_frontend_transition_t *rt)
{
    begin(rt,WF_FRONTEND_TRANSITION_STRIPE_CLOSE,WF_FRONTEND_STRIPE_TOTAL_VBLANKS,0u);
}

void wf_frontend_transition_begin_stripe_open(wf_frontend_transition_t *rt)
{
    begin(rt,WF_FRONTEND_TRANSITION_STRIPE_OPEN,WF_FRONTEND_STRIPE_TOTAL_VBLANKS,7u);
}

void wf_frontend_transition_begin_wait(wf_frontend_transition_t *rt,uint16_t vblanks)
{
    begin(rt,WF_FRONTEND_TRANSITION_WAIT,vblanks,0u);
}

void wf_frontend_transition_step_vblank(wf_frontend_transition_t *rt,
                                        wf_frontend_transition_event_t *event)
{
    clear_event(event);
    if (!rt || !rt->active) return;

    if (event) {
        event->valid=true;
        if (rt->kind==WF_FRONTEND_TRANSITION_PALETTE_FADE_BLACK)
            event->fade_level=rt->current_level;
        else if (rt->kind==WF_FRONTEND_TRANSITION_STRIPE_CLOSE) {
            event->stripe_write=true;
            event->stripe_tile_code=(uint16_t)(WF_FRONTEND_STRIPE_TILE_FIRST+rt->current_level);
            event->stripe_palette_bank=WF_FRONTEND_STRIPE_PALETTE_BANK;
        } else if (rt->kind==WF_FRONTEND_TRANSITION_STRIPE_OPEN) {
            event->stripe_write=true;
            event->stripe_tile_code=(uint16_t)(WF_FRONTEND_STRIPE_TILE_FIRST+rt->current_level);
            event->stripe_palette_bank=WF_FRONTEND_STRIPE_PALETTE_BANK;
        }
    }

    if (rt->remaining_vblanks>0u) rt->remaining_vblanks--;
    rt->level_vblank_index++;

    if (rt->level_vblank_index==3u && rt->remaining_vblanks>0u) {
        rt->level_vblank_index=0u;
        if (rt->kind==WF_FRONTEND_TRANSITION_PALETTE_FADE_BLACK) rt->current_level--;
        else if (rt->kind==WF_FRONTEND_TRANSITION_STRIPE_CLOSE) rt->current_level++;
        else if (rt->kind==WF_FRONTEND_TRANSITION_STRIPE_OPEN) rt->current_level--;
    }

    if (rt->remaining_vblanks==0u) {
        rt->active=false;
        if (event) {
            event->complete=true;
            if (rt->kind==WF_FRONTEND_TRANSITION_STRIPE_OPEN)
                event->stripe_clear_fg0=true;
        }
    }
}

bool wf_frontend_transition_active(const wf_frontend_transition_t *rt)
{
    return rt && rt->active;
}

bool wf_frontend_select_slide_step(uint16_t *scroll_x,bool toward_right)
{
    uint16_t target;
    if (!scroll_x) return true;
    target=toward_right?WF_FRONTEND_SELECT_RIGHT_X:WF_FRONTEND_SELECT_LEFT_X;
    if (*scroll_x==target) return true;
    if (toward_right) {
        if (*scroll_x<target) *scroll_x=(uint16_t)(*scroll_x+WF_FRONTEND_SELECT_SLIDE_STEP);
        if (*scroll_x>target) *scroll_x=target;
    } else {
        if (*scroll_x>target) *scroll_x=(uint16_t)(*scroll_x-WF_FRONTEND_SELECT_SLIDE_STEP);
        if (*scroll_x<target) *scroll_x=target;
    }
    return *scroll_x==target;
}
