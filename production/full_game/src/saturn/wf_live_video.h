#ifndef WF_LIVE_VIDEO_H
#define WF_LIVE_VIDEO_H
#include "wf_session.h"
#include "wf_board.h"
bool wf_live_video_init(void);
bool wf_live_video_commit(wf_session_t *,const wf_board_t *,const uint8_t *,unsigned);
void wf_live_video_hide(void);
void wf_live_video_reveal(void);
extern volatile uint32_t wf_video_error,wf_video_commits,wf_video_sprites,wf_video_tile;
#endif
