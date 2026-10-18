#ifndef WF_R17_PARTICIPANT_IO_H
#define WF_R17_PARTICIPANT_IO_H
#include "wf_r17_post7a00.h"
/* Read the hardware WORD at the precise participant-owner sampling boundary.
 * The platform supplies values, not gameplay. Non-OK must leave output and
 * externally committed state unchanged. The callback does not own D regs. */
typedef wf_status_t (*wf_r17_participant_input_fn)(wf_runtime_t *rt,
 uint32_t address,uint16_t *value,void *user);
/* A single transaction for owners that update FG0 and palette/video together.
 * On failure neither batch may be committed. */
typedef wf_status_t (*wf_r17_participant_commit_fn)(wf_runtime_t *rt,
 const wf_r17_fg0_write_t *fg0,size_t fg0_count,
 const wf_r17_fg0_write_t *video,size_t video_count,void *user);
typedef struct wf_r17_participant_io {
 wf_r17_participant_input_fn input_word;
 wf_r17_fg0_batch_fn fg0_batch;
 wf_r17_fg0_batch_fn video_batch; /* raw non-FG0 transfers, including palette RAM */
 wf_r17_participant_commit_fn commit_batch;
 void *user;
 uint32_t blocked_dependency_rom;
} wf_r17_participant_io_t;
#endif
