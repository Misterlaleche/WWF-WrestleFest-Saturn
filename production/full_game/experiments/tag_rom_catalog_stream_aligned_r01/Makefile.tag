SH_PROGRAM:= wf-combat09
SH_BUILD_DIR:= /work/session294-tag-rom-catalog-stream-aligned-r01/build
SH_OUTPUT_DIR:= /work/session294-tag-rom-catalog-stream-aligned-r01/build/unused-disc
include $(YAUL_INSTALL_ROOT)/share/build.pre.mk
SH_SRCS:= src/saturn/combat_main.c /work/session294-tag-rom-catalog-stream-aligned-r01/wf_combat_saturn_scene.c src/saturn/wf_workspace.c src/wf_combat_diagnostic.c src/saturn/wf_platform.c src/saturn/wf_failure_screen.c src/saturn/wf_cd.c src/wf_cart.c src/wf_arcade_clock.c src/wf_pacer.c src/wf_board.c src/wf_combat_handoff.c /work/session46-restored-baseline/small-profiles-r01/derived/cdfs-tag.c /work/session13-alpha/overlays-r18b/tag-batch2-cd/wp1979_cd_read_transition.c vendor/tag_r3/src/runtime/src/wp2015_cd_rate.c
SH_SRCS+= src/wf_assets.c /work/session294-tag-rom-catalog-stream-aligned-r01/wf_scene_resident.c
SH_SRCS+= vendor/tag_r3/src/runtime/src/wp2015_irq_mac.c vendor/tag_r3/src/runtime/src/wp2015_irq_mac_guard.sx
SH_SRCS+= src/saturn/wf_sdk_assert.c
SH_SRCS+= src/saturn/wf_combat_loader.c src/saturn/wf_combat_jump.sx src/wf_scene_checkpoint.c src/saturn/wf_alpha_allocator.c
SH_CFLAGS+= -DWF_TAG_ROM_CATALOG=1 -DWF_COMBAT_TAG_NONE_JOURNAL_LEASE=1 -DWF_COMBAT_TAG_BASE_SCENES=1 -DWF_COMBAT_TAG_SMALL_PROFILES=1 -include /work/session219-tag-layout-core-original-r01/include/wf_saturn_memory_override.h -include /work/session219-tag-layout-core-original-r01/include/wf_tag_none_memory_contract.h -I/work/session219-tag-layout-core-original-r01/include -I/work/session46-restored-baseline/none-shared-journal-tag-r02/derived -DWF_COMBAT_TAG_SMALL_PROFILES=1 -DWF_COMBAT_TAG_BASE_SCENES=1 -DWF_COMBAT_TAG_SCENE6=1 -DWF_COMBAT_TAG_SMALL_CART_SCENES=1 -I/work/session22-base-residency/small-profile-all127-r01/derived -DWF_ALPHA_RETURN=1 -DWF_COMBAT_IMAGE_RAM_END=0x060fffc0u -DWF_COMBAT_TAG_HOT_STARTUP_STREAM=1 -DWF_COMBAT_TAG_STARTUP_STREAM=1 -DWF_COMBAT_TAG_EXCEPTION_SHIM=1 -DWF_COMBAT_TAG_BATCH7=1 -O2 -DWF_COMBAT_MODULE=1 -DWF_NATIVE_PROFILE=0 -DWF_COMBAT_TAG_TRANSPORT=1 -DWF_COMBAT_BANKED_TRANSPORT=1 -DWF_PLATFORM_SATURN=1 -DWF_R18_ROM_EXTRA_CAPACITY=23u -I/work/session30-base-referee/core-r02/include -Isrc -Isrc/saturn -Ibuild/frontend -Ibuild/session-include -Ivendor/tag_r3/src/runtime/include -Wall -Wextra -Werror -Wno-misleading-indentation
SH_SPECS:= yaul.specs yaul-main.specs /work/session13-alpha/overlays-r18b/preliminary-lto.specs
SH_LDFLAGS+= /work/session13-alpha/overlays-r18b/sdk_mm_guard.o /work/session13-alpha/overlays-r18b/sdk_cpu_exceptions.o -flto -O2 /work/session260-stage-overlap-fix-r01/libwf_session.a
SH_LDFLAGS+= -Wl,-T,/work/session46-restored-baseline/sh2-tag-journal-tag-r02/no-journal.ld
SH_DEFSYMS+= -Wl,--defsym=___master_stack=0x06004000 -Wl,--defsym=___slave_stack=0x06001E00
include $(YAUL_INSTALL_ROOT)/share/build.post.bin.mk

