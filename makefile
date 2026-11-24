JO_MINIMAL = 1
JO_DEBUG = 1
JO_NTSC = 1
JO_SCREEN_WIDTH = 320
JO_SCREEN_HEIGHT = 240
JO_FRAMERATE = 1
JO_FRAMERATE_DYNAMIC = 1
JO_COMPILE_WITH_FS_MODULE = 1
JO_COMPILE_WITH_AUDIO_MODULE = 1
JO_COMPILE_WITH_AUDIO_DRIVER_MODULE = 0
JO_COMPILE_WITH_AUDIO_DSP_MODULE = 0
JO_COMPILE_WITH_DUAL_CPU_MODULE = 1
JO_COMPILE_WITH_RAM_CARD_MODULE = 0
JO_COMPILE_WITH_PRINTF_MODULE = 1
ifeq ($(WF_PROFILE),1)
JO_COMPILE_WITH_PROFILER_MODULE = 1
endif
JO_GLOBAL_MEMORY_SIZE_FOR_MALLOC = 32768
ifeq ($(WF_FRONTEND),1)
# A single free region after JoWork must hold the64KiB music stream.
# The engine already gives both unused HWRAM regions to its allocator.
JO_GLOBAL_MEMORY_SIZE_FOR_MALLOC = 0
endif
JO_VDP1_MAX_COMMANDS = 520
JO_MAX_SPRITE = 16
JO_CD_SECTOR_SIZE = 2352
JO_GAME_TITLE = WWF WRESTLEFEST JO ENGINE J01
JO_GAME_VERSION = V1.005
JO_GAME_RELEASE_DATE = 20261113
JO_GAME_MAKER_ID = WF JO ENGINE
# Dungeons & Dragons Collection: Shadow over Mystara (4 MB) compatibility ID.
# Keep Jo's own bootstrap, WWF title and the existing region code.
JO_GAME_PRODUCT_NUMBER = T-1245G
JO_GAME_PERIPHERALS = JW
JO_ENGINE_SRC_DIR = sdk/jo_engine
COMPILER_DIR = sdk/Compiler
include core_sources.mk
SRCS = $(WF_CORE_SRCS) port/main.c port/wf_jo_assets.c port/wf_jo_backend.c \
 port/wf_jo_video.c port/wf_jo_clock.c port/wf_jo_audio.c port/wf_jo_frontend.c port/wf_jo_scene_prepare.c port/wf_jo_save.c \
 core/src/wp1980_royal_init.c core/src/wp1980_input.c \
 port/zlib/inflate.c port/zlib/inffast.c port/zlib/inftrees.c \
 port/zlib/adler32.c port/zlib/zutil.c
# The bundled SH-2 GCC 8.2 peephole2 pass can corrupt a uint16_t bit-test/store.
# Apply the documented workaround to every translation unit and the LTO link.
CCFLAGS = -fno-peephole2 -MMD -MP -Icore/include -Iport -Iport/zlib -DWF_JO_ENGINE=1 \
 -DWF_PLATFORM_JO=1 -DWF_PLATFORM_SATURN=1 -DWF_JO_DUAL_COMMAND_BUFFERS=1 -DZ_SOLO -DNO_GZIP
ifneq ($(WF_AUTORUN),)
CCFLAGS += -DWF_JO_AUTORUN=1 -DWF_JO_AUTORUN_MODE=$(WF_AUTORUN)
endif
ifeq ($(WF_FRONTEND),1)
CCFLAGS += -DWF_JO_FRONTEND=1
endif
ifeq ($(WF_CAPTURE),1)
# Observe only the terminal snapshot; the detailed profiler changes timing.
CCFLAGS += -DWF_JO_CAPTURE=1
endif
# The delivered WF packs are native raw files, verified before ISO creation.
# Set to 0 for the generic JoLZ-compatible asset loader.
WF_RAW_DISC_ASSETS ?= 1
ifeq ($(WF_RAW_DISC_ASSETS),1)
CCFLAGS += -DWF_JO_RAW_DISC_ASSETS=1
endif
include sdk/Compiler/COMMON/jo_engine_makefile

# BEGIN WF RAW DISC PACKAGING GATE
WF_ASSET_LOCK ?= asset-lock.json
ifeq ($(WF_RAW_DISC_ASSETS),1)
.PHONY: wf-raw-disc-assets
wf-raw-disc-assets:
	python3 tools/check_raw_disc_assets.py --cd $(ASSETS_DIR) --manifest $(WF_ASSET_LOCK)
$(TARGET2): wf-raw-disc-assets
endif
# END WF RAW DISC PACKAGING GATE

# BEGIN WF MUSIC HEADER VERIFICATION
# The compiled headers are valid only for these exact packaged WAV bytes.
.PHONY: wf-music-headers-verify
wf-music-headers-verify:
	python3 tools/generate_music_headers.py --verify --cd $(ASSETS_DIR)
port/wf_jo_audio.o $(TARGET) $(TARGET2): | wf-music-headers-verify
# END WF MUSIC HEADER VERIFICATION

# Repackage when the physical file order changes.
$(TARGET2): cd.sort

# Use four deterministic LTRANS workers. The executable remains below the
# 0x060DC000 boundary while avoiding impractical serial LTO rebuilds.
LDFLAGS += -fno-peephole2 -flto=4

-include $(SRCS:.c=.d) $(wildcard sdk/jo_engine/*.d)

.PHONY: host-clock
host-clock:
	mkdir -p build
	cc -std=c11 -O2 -Wall -Wextra -Werror -fsanitize=address,undefined -Iport tests/test_clock.c port/wf_jo_clock.c -o build/test_clock
	ASAN_OPTIONS=detect_leaks=0 build/test_clock

.PHONY: host-gfx1
host-gfx1:
	mkdir -p build
	cc -std=c11 -O2 -Wall -Wextra -Werror -Icore/include tests/test_gfx1_world_commit.c core/src/wf_r18_world.c -o build/test_gfx1_world_commit
	build/test_gfx1_world_commit

# P1-02: compact code that never runs in the active combat/render/audio loop.
# This recovers a safe static HWRAM margin without reducing runtime capacities.
WF_COLD_FRONTEND_OBJS := \
 core/src/wf_r17_bootstrap.o \
 core/src/wf_r18_match_flow.o \
 core/src/wf_r18_match_result.o \
 core/src/wf_r18_result_screen.o \
 core/src/wf_r18_bootstrap.o \
 core/src/wf_r18_transition.o \
 core/src/wf_r18_frontend_input.o \
 core/src/wf_r18_frontend_palette.o \
 core/src/wf_r18_frontend_fade.o \
 core/src/wf_r18_frontend_effect.o \
 core/src/wf_r18_mode_select.o \
 core/src/wf_r18_title_palette.o \
 core/src/wf_r18_frontend_boot.o \
 core/src/wf_r18_title.o \
 core/src/wf_r18_roster_select.o \
 core/src/wf_r18_prematch.o \
 core/src/wf_r18_frontend_io.o \
 core/src/wf_r18_frontend_map.o \
 core/src/wf_r18_frontend_render.o \
 core/src/wf_r18_frontend_leaf.o \
 core/src/wf_r18_royal_setup.o \
 core/src/wf_r18_frontend_driver.o \
 core/src/wf_r18_frontend_reset.o \
 core/src/wf_r18_frontend_coin.o \
 core/src/wf_r18_frontend_irq.o \
 core/src/wf_r18_frontend_mode_plate.o \
 core/src/wf_r18_frontend_board.o \
 core/src/wf_r18_match_setup.o \
 core/src/wf_r18_card_scene.o \
 core/src/wf_r18_runtime_handoff.o \
 core/src/wf_r18_frontend_world.o \
 core/src/wf_r18_attract_setup.o \
 core/src/wf_r18_post_tag.o \
 core/src/wf_r18_ranking.o \
 core/src/wf_r18_ranking_glyph.o \
 core/src/wf_r18_records_aux.o \
 core/src/wf_r18_save.o \
 core/src/wp1979_input.o \
 core/src/wp1980_royal_init.o \
 port/zlib/inflate.o \
 port/zlib/inffast.o \
 port/zlib/inftrees.o \
 port/zlib/zutil.o \
 port/wf_jo_assets.o \
 port/wf_jo_frontend.o \
 port/wf_jo_scene_prepare.o \
 port/wf_jo_save.o
$(WF_COLD_FRONTEND_OBJS): CCFLAGS += -Os
$(WF_COLD_FRONTEND_OBJS): makefile

WF_SH2_MIN_STATIC_MARGIN ?= 8192
.PHONY: sh2-memory-margin
sh2-memory-margin: $(TARGET)
	python3 tests/test_sh2_memory_margin.py game.map --minimum $(WF_SH2_MIN_STATIC_MARGIN)
