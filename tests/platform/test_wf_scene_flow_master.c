#include <assert.h>
#include "../../src/runtime/wf_scene_flow_master.h"

int main(void)
{
    wf_scene_flow_state_t s;
    wf_scene_flow_reset(&s);
    assert(s.current == WF_SCENE_UNBOUND);
    assert(wf_scene_flow_bind_initial(&s, WF_SCENE_BOOT_PRESENTATION) == WF_FLOW_OK);
    assert(wf_scene_flow_request(&s, WF_SCENE_BOOT_PRESENTATION, WF_SCENE_TITLE, WF_FLOW_T03_PRESENTATION_TO_TITLE) == WF_FLOW_OK);
    assert(wf_scene_flow_request(&s, WF_SCENE_TITLE, WF_SCENE_MODE_SELECT, WF_FLOW_T09_CREDITED_START_TO_MODE) == WF_FLOW_OK);
    assert(wf_scene_flow_request(&s, WF_SCENE_MODE_SELECT, WF_SCENE_ROSTER_SELECT, WF_FLOW_T12_T13_MODE_TO_ROSTER) == WF_FLOW_OK);
    assert(wf_scene_flow_request(&s, WF_SCENE_ROSTER_SELECT, WF_SCENE_PREMATCH, WF_FLOW_T16_ROSTER_TO_PREMATCH) == WF_FLOW_OK);
    assert(wf_scene_flow_request(&s, WF_SCENE_PREMATCH, WF_SCENE_GAMEPLAY, WF_FLOW_T18_PREMATCH_TO_GAMEPLAY) == WF_FLOW_OK);
    assert(wf_scene_flow_request(&s, WF_SCENE_GAMEPLAY, WF_SCENE_POSTMATCH_ROOT, WF_FLOW_T19_GAMEPLAY_TO_POSTMATCH) == WF_FLOW_OK);
    assert(wf_scene_flow_request(&s, WF_SCENE_POSTMATCH_ROOT, WF_SCENE_PREMATCH, WF_FLOW_WP14_CONTINUE_ACCEPT_TO_PREMATCH) == WF_FLOW_OK);

    wf_scene_flow_reset(&s);
    assert(wf_scene_flow_bind_initial(&s, WF_SCENE_TITLE) == WF_FLOW_OK);
    assert(wf_scene_flow_request(&s, WF_SCENE_TITLE, WF_SCENE_ATTRACT_ROOT, WF_FLOW_T04_TITLE_TO_ATTRACT) == WF_FLOW_OK);
    assert(wf_scene_flow_request(&s, WF_SCENE_ATTRACT_ROOT, WF_SCENE_MODE_SELECT, WF_FLOW_T09_CREDITED_START_TO_MODE) == WF_FLOW_OK);

    wf_scene_flow_reset(&s);
    assert(wf_scene_flow_bind_initial(&s, WF_SCENE_CAMPAIGN_PROGRESSION) == WF_FLOW_OK);
    assert(wf_scene_flow_request(&s, WF_SCENE_CAMPAIGN_PROGRESSION, WF_SCENE_CREDITS, WF_FLOW_T23_CAMPAIGN_TO_CREDITS) == WF_FLOW_OK);
    assert(wf_scene_flow_request(&s, WF_SCENE_CREDITS, WF_SCENE_BOOT_PRESENTATION, WF_FLOW_T24_CREDITS_TO_PRESENTATION) == WF_FLOW_OK);

    wf_scene_flow_reset(&s);
    assert(wf_scene_flow_bind_initial(&s, WF_SCENE_TITLE) == WF_FLOW_OK);
    assert(wf_scene_flow_request(&s, WF_SCENE_TITLE, WF_SCENE_GAMEPLAY, WF_FLOW_T18_PREMATCH_TO_GAMEPLAY) == WF_FLOW_EDGE_NOT_PROVEN);
    assert(wf_scene_flow_request(&s, WF_SCENE_TITLE, WF_SCENE_MODE_SELECT, WF_FLOW_T04_TITLE_TO_ATTRACT) == WF_FLOW_EVIDENCE_MISMATCH);
    assert(wf_scene_flow_request(&s, WF_SCENE_ATTRACT_ROOT, WF_SCENE_MODE_SELECT, WF_FLOW_T09_CREDITED_START_TO_MODE) == WF_FLOW_CURRENT_MISMATCH);
    return 0;
}
