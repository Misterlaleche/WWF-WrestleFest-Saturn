SH_PROGRAM:= wf-combat09
SH_BUILD_DIR:= /work/session288-royal-retained-raw-r01/build
SH_OUTPUT_DIR:= /work/session288-royal-retained-raw-r01/build/unused-disc
include $(YAUL_INSTALL_ROOT)/share/build.pre.mk
SH_SRCS:= /work/session27-incremental-candidate/overlay/src/saturn/combat_main.c /work/session288-royal-retained-raw-r01/wf_combat_saturn.c /work/repo/production/full_game/src/saturn/wf_workspace.c /work/repo/production/full_game/src/wf_combat_diagnostic.c /work/session27-incremental-candidate/overlay/src/saturn/wf_platform.c /work/repo/production/full_game/src/saturn/wf_failure_screen.c /work/session27-incremental-candidate/overlay/src/saturn/wf_cd.c /work/repo/production/full_game/src/wf_cart.c /work/repo/production/full_game/src/wf_arcade_clock.c /work/repo/production/full_game/src/wf_pacer.c /work/repo/production/full_game/src/wf_board.c /work/session27-incremental-candidate/overlay/src/wf_combat_handoff.c /work/session131-royal-bulk256-candidate/candidate/cdfs-cart.c /work/session15-tag-cd-hardware-r24/build-r01/cd-wait/wp1979_cd_read.c /work/session15-tag-cd-hardware-r24/build-r01/cd-wait/wp2015_cd_rate.c
SH_SRCS+= /work/repo/production/full_game/vendor/tag_r3/src/runtime/src/wp2015_irq_mac.c /work/repo/production/full_game/vendor/tag_r3/src/runtime/src/wp2015_irq_mac_guard.sx
SH_SRCS+= /work/session46-restored-baseline/return-rom-r01/candidate/wf_combat_loader.c /work/repo/production/full_game/src/saturn/wf_combat_jump.sx /work/repo/production/full_game/src/wf_scene_checkpoint.c /work/repo/production/full_game/src/saturn/wf_alpha_allocator.c
SH_CFLAGS+= -DWF_COMBAT_ROYAL_FULL_CROP=1 -DWF_DISABLE_ROYAL_FULL_CROP_EXPERIMENT=1 -DWF_COMBAT_SHARED_CART_ROYAL=1 -I/work/session27-incremental-candidate/royal-2k -DWF_COMBAT_TAG_MATCH_SCENES=1 -DWF_COMBAT_TAG_STARTUP_LZ4=1 -DWF_COMBAT_TAG_STARTUP_STAGE=1 -DWF_ALPHA_RETURN=1 -DWF_COMBAT_IMAGE_RAM_END=0x060fffc0u -DWF_COMBAT_TAG_SCENE6=1 -DWF_COMBAT_TAG_CROP_STAGE=1 -DWF_COMBAT_TAG_ATLAS_ARCHIVE=1 -DWF_CD_RATE_GUARDED=1 -DWF_COMBAT_CROPPED_CART=1 -DWF_COMBAT_RETAINED_CROP_POOL=1 -DWF_COMBAT_PROFILE_ARCHIVE=1 -DWF_COMBAT_HUD_WINDOW=1 -DWF_COMBAT_BANKED_TRANSPORT=1 -DWF_COMBAT_HW1_BANKED=1 -DWF_PLATFORM_SATURN=1 -I/work/session27-incremental-candidate/core-2k-index/include -DWF_R18_ROM_EXTRA_CAPACITY=28u -O2 -DWF_COMBAT_MODULE=1 -DWF_NATIVE_PROFILE=0 -DWF_SATURN_HWRAM_HOT_INDEX_BYTES=0x14000u -I/work/repo/production/full_game/src -I/work/repo/production/full_game/src/saturn -I/work/session15-tag-cd-hardware-r24/build-r01/cd-wait -I/work/repo/production/full_game/build/frontend -I/work/repo/production/full_game/build/session-include -I/work/repo/production/full_game/vendor/tag_r3/src/runtime/include -Wall -Wextra -Werror -Wno-misleading-indentation
SH_SPECS:= yaul.specs yaul-main.specs /work/session13-alpha/overlays-r25b/preliminary-lto.specs
SH_LDFLAGS+= /work/session13-alpha/overlays-r25b/sdk_mm_guard.o -flto -O2 /work/session27-incremental-candidate/core-2k-index/libwf_session.a
SH_DEFSYMS+= -Wl,--defsym=___master_stack=0x06004000 -Wl,--defsym=___slave_stack=0x06001E00
SH_CFLAGS+= -I/work/session27-incremental-candidate/candidate -I/work/session27-incremental-candidate -I/work/session27-incremental-candidate/overlay/src/saturn -fstack-usage
SH_SRCS+= /work/session27-incremental-candidate/candidate/wf_result_service.c /work/session27-incremental-candidate/wf_result_prefix.c
SH_CFLAGS+= -DWF_RESULT_PRELOAD=1
SH_CFLAGS+= -DWF_ROM_HANDOFF=1 -I/work/session27-incremental-candidate
SH_SRCS+= /work/session46-restored-baseline/return-rom-r01/candidate/wf_rom_handoff.c /work/session46-restored-baseline/return-rom-r01/candidate/wf_rom_handoff_saturn.c
SH_CFLAGS:= -I/work/session27-incremental-candidate/derived -I/work/session27-incremental-candidate $(SH_CFLAGS)
SH_CFLAGS+= -DWF_TAG_PREFETCH=1
SH_SRCS+= /work/session27-incremental-candidate/wf_tag_prefetch.c /work/session27-incremental-candidate/wf_tag_prefetch_ticket.c /work/session27-incremental-candidate/wf_tag_prefetch_saturn.c
SH_LDFLAGS+= -fstack-usage -save-temps=obj
SH_SRCS+= /work/session46-restored-baseline/worldshift-r01/after.c
include $(YAUL_INSTALL_ROOT)/share/build.post.bin.mk

SH_CFLAGS:= -I/work/session46-restored-baseline/return-rom-r01/candidate $(SH_CFLAGS)
