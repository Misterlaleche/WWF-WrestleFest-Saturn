#ifndef WF_JO_VIDEO_TEST_PLATFORM_H
#define WF_JO_VIDEO_TEST_PLATFORM_H
#include <stdbool.h>
#include <string.h>
#include "wf_jo_video.h"
typedef wf_jo_command_t jo_vdp1_command;
#define JO_WF_VIDEO_BANKS 4u
#define JO_VDP1_TEXTURE_DEF_BASE_ADDRESS 0x10000u
#define JO_VDP1_VRAM 0x25c00000u
#define JO_VDP2_VRAM_A0 0x25e00000u
#define JO_VDP2_VRAM_A1 0x25e20000u
#define JO_VDP2_VRAM_B0 0x25e40000u
#define JO_VDP2_VRAM_B1 0x25e60000u
#define JO_VDP2_CRAM 0x25f00000u
#define jo_memcpy memcpy
bool __jo_vdp1_displayed_frame_changed(unsigned *tag);
unsigned __jo_vdp1_frame_tag(void);
unsigned jo_core_get_vblank_count(void);
int jo_core_add_vblank_callback(void (*callback)(void));
void __jo_core_pause(void);
unsigned __jo_core_disable_interrupts(void);
void __jo_core_restore_interrupts(unsigned sr);
jo_vdp1_command *__jo_vdp1_reserve_commands(unsigned n,unsigned short **keys,unsigned *reserved);
void __jo_vdp1_commit_commands(unsigned n);
int jo_vdp1_frame_command_count(void);
int jo_vdp1_command_max(void);
void jo_dma_copy(const void *source,void *dest,unsigned bytes);
void jo_wf_vdp2_init(void);
void jo_wf_vdp2_blank(void);
void jo_wf_vdp2_gate(void);
void jo_wf_vdp2_diagnostics(void);
void jo_wf_vdp2_present(unsigned bank,const unsigned short *scroll,unsigned char priority,
                       unsigned layers,unsigned opaque,unsigned sprites);
#endif
