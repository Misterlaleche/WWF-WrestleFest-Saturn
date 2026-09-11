#ifndef WF_SCENE_FLOW_MASTER_H
#define WF_SCENE_FLOW_MASTER_H

typedef unsigned int wf_u32_t;
typedef int wf_bool_t;

typedef enum wf_scene_id {
    WF_SCENE_UNBOUND = 0,
    WF_SCENE_BOOT_PRESENTATION,
    WF_SCENE_TITLE,
    WF_SCENE_ATTRACT_ROOT,
    WF_SCENE_MODE_SELECT,
    WF_SCENE_ROSTER_SELECT,
    WF_SCENE_PREMATCH,
    WF_SCENE_GAMEPLAY,
    WF_SCENE_POSTMATCH_ROOT,
    WF_SCENE_CAMPAIGN_PROGRESSION,
    WF_SCENE_TOP_RANKING,
    WF_SCENE_CREDITS,
    WF_SCENE_FRONTEND_RETURN
} wf_scene_id_t;

typedef enum wf_scene_flow_evidence {
    WF_FLOW_EVIDENCE_NONE = 0,
    WF_FLOW_T03_PRESENTATION_TO_TITLE,
    WF_FLOW_T04_TITLE_TO_ATTRACT,
    WF_FLOW_T09_CREDITED_START_TO_MODE,
    WF_FLOW_T12_T13_MODE_TO_ROSTER,
    WF_FLOW_T16_ROSTER_TO_PREMATCH,
    WF_FLOW_T18_PREMATCH_TO_GAMEPLAY,
    WF_FLOW_T19_GAMEPLAY_TO_POSTMATCH,
    WF_FLOW_T21_RANKING_TO_ATTRACT,
    WF_FLOW_T23_CAMPAIGN_TO_CREDITS,
    WF_FLOW_T24_CREDITS_TO_PRESENTATION,
    WF_FLOW_WP13_POSTWIN_TO_PROGRESSION,
    WF_FLOW_WP14_CONTINUE_ACCEPT_TO_PREMATCH
} wf_scene_flow_evidence_t;

typedef enum wf_scene_flow_result {
    WF_FLOW_OK = 0,
    WF_FLOW_BAD_ARGUMENT,
    WF_FLOW_CURRENT_MISMATCH,
    WF_FLOW_EDGE_NOT_PROVEN,
    WF_FLOW_EVIDENCE_MISMATCH
} wf_scene_flow_result_t;

typedef struct wf_scene_flow_state {
    wf_scene_id_t current;
    wf_scene_id_t previous;
    wf_scene_flow_evidence_t last_evidence;
    wf_u32_t transition_serial;
} wf_scene_flow_state_t;

void wf_scene_flow_reset(wf_scene_flow_state_t *state);
wf_scene_flow_result_t wf_scene_flow_bind_initial(wf_scene_flow_state_t *state,
                                                  wf_scene_id_t initial_scene);
wf_bool_t wf_scene_flow_edge_proven(wf_scene_id_t from,
                                    wf_scene_id_t to,
                                    wf_scene_flow_evidence_t evidence);
wf_scene_flow_result_t wf_scene_flow_request(wf_scene_flow_state_t *state,
                                             wf_scene_id_t expected_from,
                                             wf_scene_id_t to,
                                             wf_scene_flow_evidence_t evidence);

#endif
