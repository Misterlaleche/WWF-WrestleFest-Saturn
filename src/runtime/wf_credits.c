#include "wf_credits.h"
#include <string.h>

const wf_credits_page_t wf_credits_pages[WF_CREDITS_FIXED_PAGE_COUNT] = {
    {0x58u,0x00u,0x4Du,160,80,120,64,0,0},
    {0x59u,0x01u,0x4Du,160,80,88,64,0,0},
    {0x5Au,0x02u,0x4Du,160,80,64,64,0,0},
    {0x5Bu,0x03u,0x4Du,160,80,96,64,0,0},
    {0x5Cu,0x06u,0x4Du,160,80,112,64,0,0},
    {0x5Du,0x07u,0x4Du,160,80,112,64,0,0},
    {0x5Eu,0x08u,0x4Du,160,80,128,64,0,0},
    {0x5Fu,0x09u,0x4Du,160,80,64,72,112,88},
    {0x60u,0x0Au,0x4Du,160,80,104,64,0,0}
};

const wf_credits_staff_block_t wf_credits_staff_blocks[WF_CREDITS_STAFF_BLOCK_COUNT] = {
    {0u,0x50u,768,384,128,-128,0x3Bu},
    {1u,0x50u,896,384,256,-128,0x3Bu},
    {2u,0x50u,768,128,128,-384,0x3Bu},
    {3u,0x50u,896,128,256,-384,0x3Bu},
    {4u,0x50u,800,-16,160,-528,0x3Bu},
    {5u,0x50u,800,-176,160,-688,0x3Bu}
};

static void clear_events(wf_credits_events_t *e)
{
    if (e) memset(e,0,sizeof(*e));
}

bool wf_credits_skip_from_raw(const uint16_t raw[4])
{
    unsigned i;
    if (!raw) return false;
    for (i=0;i<4u;++i) {
        uint16_t normalized=(uint16_t)~raw[i];
        if ((normalized & WF_CREDITS_SKIP_MASK)!=0u) return true;
    }
    return false;
}

void wf_credits_begin(wf_credits_runtime_t *rt)
{
    if (!rt) return;
    memset(rt,0,sizeof(*rt));
    rt->state=WF_CREDITS_ENTRY_FADE;
    rt->fade_remaining=WF_CREDITS_COMMON_FADE_VBLANKS;
}

static void enter_exit(wf_credits_runtime_t *rt,wf_credits_events_t *e)
{
    rt->state=WF_CREDITS_EXIT_FADE;
    rt->fade_remaining=WF_CREDITS_COMMON_FADE_VBLANKS;
    if (!rt->exit_command_sent) {
        rt->exit_command_sent=true;
        if (e) { e->audio_command=true; e->audio_command_byte=WF_CREDITS_EXIT_SOUND_CMD; }
    }
}

void wf_credits_step_vblank(wf_credits_runtime_t *rt,
                            const uint16_t player_words_active_low[4],
                            wf_credits_events_t *e)
{
    bool skip;
    clear_events(e);
    if (!rt) return;
    skip=wf_credits_skip_from_raw(player_words_active_low);

    switch (rt->state) {
    case WF_CREDITS_IDLE:
    case WF_CREDITS_DONE:
        return;

    case WF_CREDITS_ENTRY_FADE:
        if (rt->fade_remaining>0u) --rt->fade_remaining;
        if (rt->fade_remaining==0u) {
            rt->state=WF_CREDITS_FIXED_PAGES;
            rt->page_index=0u;
            rt->page_sync_remaining=1u;
            rt->page_hold_remaining=WF_CREDITS_FIXED_HOLD_VBLANKS;
            if (e) {
                e->setup_background=true;
                e->audio_command=true;
                e->audio_command_byte=WF_CREDITS_ENTRY_SOUND_CMD;
                e->load_fixed_page=true;
                e->fixed_page_index=0u;
            }
        }
        return;

    case WF_CREDITS_FIXED_PAGES:
        if (skip) { enter_exit(rt,e); return; }
        if (rt->page_sync_remaining>0u) {
            --rt->page_sync_remaining;
            return;
        }
        if (rt->page_hold_remaining>0u) --rt->page_hold_remaining;
        if (rt->page_hold_remaining==0u) {
            if ((unsigned)rt->page_index + 1u < WF_CREDITS_FIXED_PAGE_COUNT) {
                ++rt->page_index;
                rt->page_sync_remaining=1u;
                rt->page_hold_remaining=WF_CREDITS_FIXED_HOLD_VBLANKS;
                if (e) { e->load_fixed_page=true; e->fixed_page_index=rt->page_index; }
            } else {
                rt->state=WF_CREDITS_STAFF_SCROLL;
                rt->staff_updates_remaining=WF_CREDITS_STAFF_UPDATES;
                rt->staff_scroll_accum_16_16=0;
                if (e) e->setup_staff=true;
            }
        }
        return;

    case WF_CREDITS_STAFF_SCROLL:
        if (skip) { enter_exit(rt,e); return; }
        if (rt->staff_updates_remaining>0u) {
            rt->staff_scroll_accum_16_16 += WF_CREDITS_SCROLL_DY_16_16;
            --rt->staff_updates_remaining;
            if (e) {
                e->staff_scroll_update=true;
                e->staff_scroll_accum_16_16=rt->staff_scroll_accum_16_16;
            }
        }
        if (rt->staff_updates_remaining==0u) enter_exit(rt,e);
        return;

    case WF_CREDITS_EXIT_FADE:
        if (rt->fade_remaining>0u) --rt->fade_remaining;
        if (rt->fade_remaining==0u) {
            rt->state=WF_CREDITS_FRONTEND_RETURN;
            if (e) {
                e->cleanup=true;
                e->frontend_return=true;
                e->frontend_return_rom=WF_CREDITS_FRONTEND_RETURN_ROM;
            }
        }
        return;

    case WF_CREDITS_FRONTEND_RETURN:
        rt->state=WF_CREDITS_DONE;
        return;
    }
}
