#ifndef WF_LIVE_VIDEO_H
#define WF_LIVE_VIDEO_H
#include "wf_session.h"
#include "wf_board.h"
bool wf_live_video_init(void);
/* Reload resources after CRT reset without replaying the startup notice. */
bool wf_live_video_init_return(void);
bool wf_live_video_atlas(unsigned which);
bool wf_live_video_prepare(wf_session_t *);
/* After a clean frontend reload and canonical scene export. Caller owns
 * the load window; the verified final picture remains hidden on return. */
enum {WF_LIVE_VIDEO_POST_ATLAS=3};
bool wf_live_video_return_prepare(wf_session_t *,const wf_board_t *);
/* Within original world1BA6's load window, before any final/credit draw. */
bool wf_live_video_finale_prepare(wf_session_t *);
bool wf_live_video_commit(wf_session_t *,const wf_board_t *,const uint8_t *,unsigned);
void wf_live_video_hide(void);
void wf_live_video_finish(void);
void wf_live_video_reveal(void);
extern volatile uint32_t wf_video_error,wf_video_commits,wf_video_sprites,wf_video_tile;
#if defined(WF_FIXED_ATTRACT) && WF_FIXED_ATTRACT
bool wf_live_video_program_load(void);
bool wf_live_video_restore_attract(wf_session_t *);
#endif
#endif
