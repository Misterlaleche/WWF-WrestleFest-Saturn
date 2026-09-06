ifeq ($(strip $(YAUL_INSTALL_ROOT)),)
  $(error Undefined YAUL_INSTALL_ROOT)
endif
include $(YAUL_INSTALL_ROOT)/share/build.pre.mk
BUILTIN_ASSETS+= assets/ENV.PAL;asset_pal
BUILTIN_ASSETS+= assets/ROSTER.PAL;asset_rosterpal
BUILTIN_ASSETS+= assets/OBJECT15.PAL;asset_objectpal
BUILTIN_ASSETS+= assets/RING14.PAL;asset_ring14pal
BUILTIN_ASSETS+= assets/SDRV.BIN;asset_sound_driver
BUILTIN_ASSETS+= assets/ENTRY.PAL;asset_entrypal
BUILTIN_ASSETS+= assets/INTRO.RLE;asset_intro
BUILTIN_ASSETS+= assets/PROMPT.TEX;asset_prompt
BUILTIN_ASSETS+= assets/PROMPT.PAL;asset_promptpal
BUILTIN_ASSETS+= assets/ANNOUNCE.PAL;asset_announcepal
BUILTIN_ASSETS+= assets/LOGOS.PAL;asset_logospal
BUILTIN_ASSETS+= assets/UI.TEX;asset_ui
BUILTIN_ASSETS+= assets/UI.PAL;asset_uipal
BUILTIN_ASSETS+= assets/ENTRYUI.TEX;asset_entryui
BUILTIN_ASSETS+= assets/ENTRYUI.PAL;asset_entryuipal
SH_PROGRAM:= wrestlefest-saturn-test254-v0269
SH_SRCS:= main.c game.c intro.c arcade_attract.c arcade_attract_bio.c arcade_attract_records.c arcade_attract_transition.c arcade_presentation_audio.c arcade_interstitial.c session.c codec.c arcade_animation.c arcade_action_map.c arcade_input.c arcade_pair_state.c arcade_behind_pair.c hud.c match_flow.c match_result.c roster_pack.c arcade_stamina.c campaign.c campaign_presentation.c campaign_runtime.c arcade_ringside.c arcade_moves.c arcade_rumble.c arcade_ai.c wf_audio_event.c wf_audio_sfx.c wf_audio_adpcm_index.c wf_oki_decode.c wf_music_cdda.c wf_audio_saturn.c ponesound_min.c
SH_CFLAGS+= -Os -I. -Wall -Wextra
IP_VERSION:= V0.269
IP_RELEASE_DATE:= 20260906
IP_AREAS:= JTUBKAEL
IP_PERIPHERALS:= J
IP_TITLE:= WWF WrestleFest Test254
IP_MASTER_STACK_ADDR:= 0x06004000
IP_SLAVE_STACK_ADDR:= 0x06001E00
IP_1ST_READ_ADDR:= 0x06004000
IP_1ST_READ_SIZE:= 0
include $(YAUL_INSTALL_ROOT)/share/build.post.iso-cue.mk
