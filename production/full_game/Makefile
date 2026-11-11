CC ?= cc
PYTHON ?= python3
CFLAGS := -std=c11 -O2 -Wall -Wextra -Werror
INCLUDES := -Isrc -I'../../Entrada Ring/source' -I'../../Escena Presentadora/source/scene/src'
SCENES := '../../Entrada Ring/source/wp01_scene.c' '../../Escena Presentadora/source/scene/src/wp02_scene.c'
CORE := src/wf_arcade_clock.c src/wf_prematch.c
.PHONY: test sanitize verify-sources sh2

verify-sources:
	$(PYTHON) tools/check_sources.py

test: verify-sources test-platform test-assets test-identity test-live test-session test-combat-transfer test-cold-index test-cd-seek test-failure-screen test-crop-copy test-cropped-atlas
	@mkdir -p build
	$(CC) $(CFLAGS) $(INCLUDES) tests/test_clock.c src/wf_arcade_clock.c -o build/test_clock
	./build/test_clock
	$(CC) $(CFLAGS) -Isrc tests/test_pacer.c src/wf_pacer.c src/wf_arcade_clock.c -o build/test_pacer
	./build/test_pacer
	$(CC) $(CFLAGS) $(INCLUDES) tests/test_prematch.c src/wf_prematch.c $(SCENES) -o build/test_prematch
	./build/test_prematch
	PYTHONDONTWRITEBYTECODE=1 $(PYTHON) -m unittest discover -s tests -p 'test_*.py' -v

sanitize: verify-sources sanitize-platform sanitize-assets sanitize-identity sanitize-live sanitize-session sanitize-cold-index sanitize-cd-seek sanitize-failure-screen sanitize-crop-copy sanitize-cropped-atlas
	@mkdir -p build
	$(CC) $(CFLAGS) -fsanitize=address,undefined $(INCLUDES) tests/test_clock.c src/wf_arcade_clock.c -o build/test_clock_sanitized
	./build/test_clock_sanitized
	$(CC) $(CFLAGS) -fsanitize=address,undefined -Isrc tests/test_pacer.c src/wf_pacer.c src/wf_arcade_clock.c -o build/test_pacer_san
	UBSAN_OPTIONS=halt_on_error=1 ./build/test_pacer_san
	$(CC) $(CFLAGS) -fsanitize=address,undefined $(INCLUDES) tests/test_prematch.c src/wf_prematch.c $(SCENES) -o build/test_prematch_sanitized
	./build/test_prematch_sanitized

sh2: verify-sources
	bash tools/build_sh2.sh

.PHONY: test-cold-index sanitize-cold-index
test-cold-index:
	PYTHONDONTWRITEBYTECODE=1 $(PYTHON) tools/run_cold_index_checks.py
sanitize-cold-index:
	PYTHONDONTWRITEBYTECODE=1 $(PYTHON) tools/run_cold_index_checks.py --sanitize

.PHONY: test-cd-seek sanitize-cd-seek
test-cd-seek:
	PYTHONDONTWRITEBYTECODE=1 $(PYTHON) tools/run_cd_seek_checks.py
sanitize-cd-seek:
	PYTHONDONTWRITEBYTECODE=1 $(PYTHON) tools/run_cd_seek_checks.py --sanitize

.PHONY: test-rans-copy sanitize-rans-copy
test-rans-copy:
	PYTHONDONTWRITEBYTECODE=1 $(PYTHON) tools/run_rans_copy_checks.py
sanitize-rans-copy:
	PYTHONDONTWRITEBYTECODE=1 $(PYTHON) tools/run_rans_copy_checks.py --sanitize

.PHONY: test-crop-copy sanitize-crop-copy
test-crop-copy:
	@mkdir -p build
	$(CC) $(CFLAGS) -Isrc tests/crop_copy_probe.c -o build/crop_copy_probe
	./build/crop_copy_probe
sanitize-crop-copy:
	@mkdir -p build
	$(CC) $(CFLAGS) -fsanitize=address,undefined -Isrc tests/crop_copy_probe.c -o build/crop_copy_probe_san
	UBSAN_OPTIONS=halt_on_error=1 ./build/crop_copy_probe_san

.PHONY: test-platform sanitize-platform
.PHONY: test-alpha-handoff
test-alpha-handoff:
	@mkdir -p build
	$(CC) $(CFLAGS) -fsanitize=address,undefined -Isrc -I'../../modo Royal/wp1980/work/hwfix01/native/include' tests/combat_return_probe.c src/wf_combat_handoff.c src/wf_pacer.c src/wf_arcade_clock.c src/wf_board.c '../../modo Royal/wp1980/work/hwfix01/native/src/wf_r18_match_result.c' '../../modo Royal/wp1980/work/hwfix01/native/src/wf_r17_m68k_shadow.c' -o build/test_combat_return
	UBSAN_OPTIONS=halt_on_error=1 ./build/test_combat_return
	$(CC) $(CFLAGS) -fsanitize=address,undefined -Isrc -I'../../modo Royal/wp1980/work/hwfix01/native/include' tests/scene_checkpoint_probe.c src/wf_scene_checkpoint.c '../../modo Royal/wp1980/work/hwfix01/native/src/wf_r18_runtime_handoff.c' '../../modo Royal/wp1980/work/hwfix01/native/src/wf_r17_bootstrap.c' '../../modo Royal/wp1980/work/hwfix01/native/src/wf_r17_m68k_shadow.c' -o build/test_scene_checkpoint
	UBSAN_OPTIONS=halt_on_error=1 ./build/test_scene_checkpoint
.PHONY: test-cropped-atlas sanitize-cropped-atlas
.PHONY: test-shared-crop sanitize-shared-crop
.PHONY: test-grouped-crop sanitize-grouped-crop
.PHONY: test-retained-crop sanitize-retained-crop
test: test-retained-crop
sanitize: sanitize-retained-crop
test-retained-crop:
	PYTHONDONTWRITEBYTECODE=1 $(PYTHON) tests/test_retained_crop_pool.py
	PYTHONDONTWRITEBYTECODE=1 $(PYTHON) tools/run_retained_crop_checks.py

sanitize-retained-crop:
	PYTHONDONTWRITEBYTECODE=1 $(PYTHON) tools/run_retained_crop_checks.py --sanitize

test: test-grouped-crop
sanitize: sanitize-grouped-crop
test-grouped-crop:
	PYTHONDONTWRITEBYTECODE=1 $(PYTHON) tests/test_grouped_crop_pool.py
	PYTHONDONTWRITEBYTECODE=1 $(PYTHON) tools/run_grouped_crop_checks.py

sanitize-grouped-crop:
	PYTHONDONTWRITEBYTECODE=1 $(PYTHON) tools/run_grouped_crop_checks.py --sanitize

test: test-shared-crop
sanitize: sanitize-shared-crop
test-shared-crop:
	PYTHONDONTWRITEBYTECODE=1 $(PYTHON) tests/test_shared_crop_pool.py
	PYTHONDONTWRITEBYTECODE=1 $(PYTHON) tools/run_shared_crop_checks.py

sanitize-shared-crop:
	PYTHONDONTWRITEBYTECODE=1 $(PYTHON) tools/run_shared_crop_checks.py --sanitize

test-cropped-atlas:
	@mkdir -p build
	$(CC) $(CFLAGS) -Isrc tests/cropped_atlas_probe.c -o build/cropped_atlas_probe
	./build/cropped_atlas_probe
	PYTHONDONTWRITEBYTECODE=1 $(PYTHON) tools/run_cropped_backend_checks.py
	PYTHONDONTWRITEBYTECODE=1 $(PYTHON) -m unittest discover -s tests -p test_cropped_atlas.py
sanitize-cropped-atlas:
	@mkdir -p build
	$(CC) $(CFLAGS) -fsanitize=address,undefined -Isrc tests/cropped_atlas_probe.c -o build/cropped_atlas_probe_san
	UBSAN_OPTIONS=halt_on_error=1 ./build/cropped_atlas_probe_san
	PYTHONDONTWRITEBYTECODE=1 $(PYTHON) tools/run_cropped_backend_checks.py --sanitize

test-platform:
	@mkdir -p build
	$(CC) $(CFLAGS) -Isrc tests/test_cart.c src/wf_cart.c -o build/test_cart
	./build/test_cart
	$(CC) $(CFLAGS) -Isrc tests/test_frontend.c src/wf_frontend.c -o build/test_frontend
	./build/test_frontend
sanitize-platform:
	@mkdir -p build
	$(CC) $(CFLAGS) -fsanitize=address,undefined -Isrc tests/test_cart.c src/wf_cart.c -o build/test_cart_san
	./build/test_cart_san
	$(CC) $(CFLAGS) -fsanitize=address,undefined -Isrc tests/test_frontend.c src/wf_frontend.c -o build/test_frontend_san
	./build/test_frontend_san

.PHONY: test-assets sanitize-assets
test-assets:
	@mkdir -p build
	$(CC) $(CFLAGS) -Isrc tests/test_assets.c src/wf_assets.c -o build/test_assets
	./build/test_assets
	$(CC) $(CFLAGS) -Isrc tests/test_crc_incremental.c src/wf_assets.c -o build/test_crc_incremental
	./build/test_crc_incremental
	$(CC) $(CFLAGS) -Isrc tests/test_load_window.c -o build/test_load_window
	./build/test_load_window
	$(CC) $(CFLAGS) -Isrc tests/test_rom_preload_pack.c src/wf_assets.c -o build/test_rom_preload_pack
	./build/test_rom_preload_pack
sanitize-assets:
	@mkdir -p build
	$(CC) $(CFLAGS) -fsanitize=address,undefined -Isrc tests/test_assets.c src/wf_assets.c -o build/test_assets_san
	./build/test_assets_san
	$(CC) $(CFLAGS) -fsanitize=address,undefined -Isrc tests/test_crc_incremental.c src/wf_assets.c -o build/test_crc_incremental_san
	UBSAN_OPTIONS=halt_on_error=1 ./build/test_crc_incremental_san
	$(CC) $(CFLAGS) -fsanitize=address,undefined -Isrc tests/test_load_window.c -o build/test_load_window_san
	UBSAN_OPTIONS=halt_on_error=1 ./build/test_load_window_san
	$(CC) $(CFLAGS) -fsanitize=address,undefined -Isrc tests/test_rom_preload_pack.c src/wf_assets.c -o build/test_rom_preload_pack_san
	UBSAN_OPTIONS=halt_on_error=1 ./build/test_rom_preload_pack_san

.PHONY: test-identity sanitize-identity
test-identity:
	@mkdir -p build
	$(CC) $(CFLAGS) -Isrc tests/test_roster_identity.c src/wf_roster_identity.c -o build/test_identity
	./build/test_identity
sanitize-identity:
	@mkdir -p build
	$(CC) $(CFLAGS) -fsanitize=address,undefined -Isrc tests/test_roster_identity.c src/wf_roster_identity.c -o build/test_identity_san
	./build/test_identity_san

.PHONY: test-session sanitize-session
test-session:
	$(PYTHON) tools/run_session_checks.py
sanitize-session:
	$(PYTHON) tools/run_session_checks.py --sanitize

.PHONY: test-live sanitize-live
test-live:
	$(CC) $(CFLAGS) -Isrc tests/test_damage.c src/wf_damage.c -o build/test_damage
	./build/test_damage
	$(CC) $(CFLAGS) -Isrc tests/test_board.c src/wf_board.c -o build/test_board
	./build/test_board
	$(CC) $(CFLAGS) -Isrc tests/test_video_plan.c src/wf_video_plan.c -o build/test_video_plan
	./build/test_video_plan
sanitize-live:
	$(CC) $(CFLAGS) -fsanitize=address,undefined -Isrc tests/test_damage.c src/wf_damage.c -o build/test_damage_san
	UBSAN_OPTIONS=halt_on_error=1 ./build/test_damage_san
	$(CC) $(CFLAGS) -fsanitize=address,undefined -Isrc tests/test_board.c src/wf_board.c -o build/test_board_san
	UBSAN_OPTIONS=halt_on_error=1 ./build/test_board_san
	$(CC) $(CFLAGS) -fsanitize=address,undefined -Isrc tests/test_video_plan.c src/wf_video_plan.c -o build/test_video_plan_san
	UBSAN_OPTIONS=halt_on_error=1 ./build/test_video_plan_san

.PHONY: test-combat-transfer
test-combat-transfer:
	@mkdir -p build
	$(CC) $(CFLAGS) -fsanitize=address,undefined -Isrc tests/combat_assets_probe.c -o build/test_combat_assets
	UBSAN_OPTIONS=halt_on_error=1 ./build/test_combat_assets
	$(CC) $(CFLAGS) -fsanitize=address,undefined -Isrc -I'../../modo Royal/wp1980/work/hwfix01/native/include' tests/combat_handoff_probe.c src/wf_combat_handoff.c src/wf_pacer.c src/wf_arcade_clock.c src/wf_board.c '../../modo Royal/wp1980/work/hwfix01/native/src/wf_r18_match_result.c' '../../modo Royal/wp1980/work/hwfix01/native/src/wf_r17_m68k_shadow.c' -o build/test_combat_handoff
	UBSAN_OPTIONS=halt_on_error=1 ./build/test_combat_handoff
	$(CC) $(CFLAGS) -fsanitize=address,undefined -Isrc tests/combat_image_probe.c -o build/test_combat_image
	UBSAN_OPTIONS=halt_on_error=1 ./build/test_combat_image
	PYTHONDONTWRITEBYTECODE=1 $(PYTHON) -c "import pathlib,sys; sys.path.insert(0,'tools'); from hw1_banked import project_cache; pathlib.Path('build/vram_reader_cache.c').write_text(project_cache(pathlib.Path('../../modo Royal/wp1980/work/hwfix01/native/src/wp1980_vram_cache.c').read_text()))"
	$(CC) $(CFLAGS) -fsanitize=address,undefined -DWP1980_VRAM_CACHE_SLOTS=8 -Isrc -I'../../modo Royal/wp1980/work/hwfix01/native/include' tests/vram_reader_probe.c src/wf_vram_reader.c build/vram_reader_cache.c -o build/test_vram_reader
	UBSAN_OPTIONS=halt_on_error=1 ./build/test_vram_reader
	$(CC) $(CFLAGS) -fsanitize=address,undefined -DWF_VDP2_RESIDENT_HOST_TEST -Isrc tests/vdp2_resident_probe.c src/wf_vdp2_resident.c -o build/test_vdp2_resident
	UBSAN_OPTIONS=halt_on_error=1 ./build/test_vdp2_resident
	$(CC) $(CFLAGS) -fsanitize=address,undefined -Isrc tests/tile_index_probe.c -o build/test_tile_index
	UBSAN_OPTIONS=halt_on_error=1 ./build/test_tile_index

.PHONY: test-failure-screen sanitize-failure-screen
.PHONY: test-startup-notice sanitize-startup-notice
test: test-startup-notice
sanitize: sanitize-startup-notice
test-startup-notice:
	@mkdir -p build
	$(CC) $(CFLAGS) -DWF_FAILURE_HOST_TEST -DWF_STARTUP_NOTICE_HOST_TEST -Isrc/saturn tests/startup_notice_probe.c src/saturn/wf_startup_notice.c src/saturn/wf_failure_screen.c -o build/test_startup_notice
	./build/test_startup_notice
sanitize-startup-notice:
	@mkdir -p build
	$(CC) $(CFLAGS) -fsanitize=address,undefined -DWF_FAILURE_HOST_TEST -DWF_STARTUP_NOTICE_HOST_TEST -Isrc/saturn tests/startup_notice_probe.c src/saturn/wf_startup_notice.c src/saturn/wf_failure_screen.c -o build/test_startup_notice_san
	UBSAN_OPTIONS=halt_on_error=1 ./build/test_startup_notice_san
FAILURE_TEST_INCLUDES := -Isrc -I'../../modo Royal/wp1980/work/hwfix01/native/include'
test-failure-screen:
	@mkdir -p build
	$(CC) $(CFLAGS) -DWF_FAILURE_HOST_TEST -Isrc/saturn tests/failure_screen_probe.c src/saturn/wf_failure_screen.c -o build/test_failure_screen
	./build/test_failure_screen
	$(CC) $(CFLAGS) -DWF_FAILURE_HOST_TEST -Isrc/saturn tests/combat_failure_bitmap_probe.c src/saturn/wf_failure_screen.c -o build/test_combat_failure_bitmap
	./build/test_combat_failure_bitmap
	$(CC) $(CFLAGS) -DWF_FAILURE_HOST_TEST -Isrc/saturn tests/failure_register_probe.c src/saturn/wf_failure_screen.c -o build/test_failure_register
	./build/test_failure_register
	$(CC) $(CFLAGS) -DWF_FAILURE_HOST_TEST -Isrc/saturn tests/failure_boundary_probe.c src/saturn/wf_failure_screen.c -o build/test_failure_boundary
	./build/test_failure_boundary
	$(CC) $(CFLAGS) $(FAILURE_TEST_INCLUDES) tests/combat_failure_capture_probe.c src/wf_combat_diagnostic.c -o build/test_combat_failure_capture
	./build/test_combat_failure_capture
sanitize-failure-screen:
	@mkdir -p build
	$(CC) $(CFLAGS) -fsanitize=address,undefined -DWF_FAILURE_HOST_TEST -Isrc/saturn tests/failure_screen_probe.c src/saturn/wf_failure_screen.c -o build/test_failure_screen_san
	UBSAN_OPTIONS=halt_on_error=1 ./build/test_failure_screen_san
	$(CC) $(CFLAGS) -fsanitize=address,undefined -DWF_FAILURE_HOST_TEST -Isrc/saturn tests/combat_failure_bitmap_probe.c src/saturn/wf_failure_screen.c -o build/test_combat_failure_bitmap_san
	UBSAN_OPTIONS=halt_on_error=1 ./build/test_combat_failure_bitmap_san
	$(CC) $(CFLAGS) -fsanitize=address,undefined -DWF_FAILURE_HOST_TEST -Isrc/saturn tests/failure_register_probe.c src/saturn/wf_failure_screen.c -o build/test_failure_register_san
	UBSAN_OPTIONS=halt_on_error=1 ./build/test_failure_register_san
	$(CC) $(CFLAGS) -fsanitize=address,undefined -DWF_FAILURE_HOST_TEST -Isrc/saturn tests/failure_boundary_probe.c src/saturn/wf_failure_screen.c -o build/test_failure_boundary_san
	UBSAN_OPTIONS=halt_on_error=1 ./build/test_failure_boundary_san
	$(CC) $(CFLAGS) -fsanitize=address,undefined $(FAILURE_TEST_INCLUDES) tests/combat_failure_capture_probe.c src/wf_combat_diagnostic.c -o build/test_combat_failure_capture_san
	UBSAN_OPTIONS=halt_on_error=1 ./build/test_combat_failure_capture_san
