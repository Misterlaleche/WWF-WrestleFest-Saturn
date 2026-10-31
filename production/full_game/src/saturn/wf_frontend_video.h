#ifndef WF_FRONTEND_VIDEO_H
#define WF_FRONTEND_VIDEO_H
#include "wf_frontend.h"
bool wf_frontend_video_mode(void);
bool wf_frontend_video_roster(void);
void wf_frontend_video_apply(const wf_frontend_t *f,const wf_frontend_effect_t *effect);
#endif
