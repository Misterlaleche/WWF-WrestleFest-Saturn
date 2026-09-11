#include "wf_scene_flow_master.h"

typedef struct wf_flow_edge {
    wf_scene_id_t from;
    wf_scene_id_t to;
    wf_scene_flow_evidence_t evidence;
} wf_flow_edge_t;

static const wf_flow_edge_t k_edges[] = {
    { WF_SCENE_BOOT_PRESENTATION, WF_SCENE_TITLE, WF_FLOW_T03_PRESENTATION_TO_TITLE },
    { WF_SCENE_TITLE, WF_SCENE_ATTRACT_ROOT, WF_FLOW_T04_TITLE_TO_ATTRACT },
    { WF_SCENE_TITLE, WF_SCENE_MODE_SELECT, WF_FLOW_T09_CREDITED_START_TO_MODE },
    { WF_SCENE_ATTRACT_ROOT, WF_SCENE_MODE_SELECT, WF_FLOW_T09_CREDITED_START_TO_MODE },
    { WF_SCENE_MODE_SELECT, WF_SCENE_ROSTER_SELECT, WF_FLOW_T12_T13_MODE_TO_ROSTER },
    { WF_SCENE_ROSTER_SELECT, WF_SCENE_PREMATCH, WF_FLOW_T16_ROSTER_TO_PREMATCH },
    { WF_SCENE_PREMATCH, WF_SCENE_GAMEPLAY, WF_FLOW_T18_PREMATCH_TO_GAMEPLAY },
    { WF_SCENE_GAMEPLAY, WF_SCENE_POSTMATCH_ROOT, WF_FLOW_T19_GAMEPLAY_TO_POSTMATCH },
    { WF_SCENE_TOP_RANKING, WF_SCENE_ATTRACT_ROOT, WF_FLOW_T21_RANKING_TO_ATTRACT },
    { WF_SCENE_CAMPAIGN_PROGRESSION, WF_SCENE_CREDITS, WF_FLOW_T23_CAMPAIGN_TO_CREDITS },
    { WF_SCENE_CREDITS, WF_SCENE_BOOT_PRESENTATION, WF_FLOW_T24_CREDITS_TO_PRESENTATION },
    { WF_SCENE_POSTMATCH_ROOT, WF_SCENE_CAMPAIGN_PROGRESSION, WF_FLOW_WP13_POSTWIN_TO_PROGRESSION },
    { WF_SCENE_POSTMATCH_ROOT, WF_SCENE_PREMATCH, WF_FLOW_WP14_CONTINUE_ACCEPT_TO_PREMATCH }
};

void wf_scene_flow_reset(wf_scene_flow_state_t *state)
{
    if (!state) return;
    state->current = WF_SCENE_UNBOUND;
    state->previous = WF_SCENE_UNBOUND;
    state->last_evidence = WF_FLOW_EVIDENCE_NONE;
    state->transition_serial = 0u;
}

wf_scene_flow_result_t wf_scene_flow_bind_initial(wf_scene_flow_state_t *state, wf_scene_id_t initial_scene)
{
    if (!state || initial_scene == WF_SCENE_UNBOUND) return WF_FLOW_BAD_ARGUMENT;
    if (state->current != WF_SCENE_UNBOUND) return WF_FLOW_CURRENT_MISMATCH;
    state->current = initial_scene;
    state->previous = WF_SCENE_UNBOUND;
    state->last_evidence = WF_FLOW_EVIDENCE_NONE;
    return WF_FLOW_OK;
}

wf_bool_t wf_scene_flow_edge_proven(wf_scene_id_t from, wf_scene_id_t to, wf_scene_flow_evidence_t evidence)
{
    wf_u32_t i;
    for (i = 0u; i < (wf_u32_t)(sizeof(k_edges) / sizeof(k_edges[0])); ++i) {
        if (k_edges[i].from == from && k_edges[i].to == to && k_edges[i].evidence == evidence) return 1;
    }
    return 0;
}

wf_scene_flow_result_t wf_scene_flow_request(wf_scene_flow_state_t *state,
                                             wf_scene_id_t expected_from,
                                             wf_scene_id_t to,
                                             wf_scene_flow_evidence_t evidence)
{
    wf_u32_t i;
    if (!state || expected_from == WF_SCENE_UNBOUND || to == WF_SCENE_UNBOUND || evidence == WF_FLOW_EVIDENCE_NONE)
        return WF_FLOW_BAD_ARGUMENT;
    if (state->current != expected_from) return WF_FLOW_CURRENT_MISMATCH;
    if (!wf_scene_flow_edge_proven(expected_from, to, evidence)) {
        for (i = 0u; i < (wf_u32_t)(sizeof(k_edges) / sizeof(k_edges[0])); ++i) {
            if (k_edges[i].from == expected_from && k_edges[i].to == to) return WF_FLOW_EVIDENCE_MISMATCH;
        }
        return WF_FLOW_EDGE_NOT_PROVEN;
    }
    state->previous = state->current;
    state->current = to;
    state->last_evidence = evidence;
    state->transition_serial++;
    return WF_FLOW_OK;
}
