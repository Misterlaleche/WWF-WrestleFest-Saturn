CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -O2 -Iinclude
CORE=src/wf_r17_postack_2017a.c src/wf_actor.c src/wf_payment.c src/wf_spatial.c src/wf_target_relation.c src/wf_pair_relation.c src/wf_actor_prepass.c src/wf_state5_exec.c src/arcade_cpu_final_bridge.c src/wf_cpu_runtime.c src/wf_gameplay_bindings.c src/wf_damage.c src/wf_scheduler.c src/wf_core_phases.c src/wf_input.c generated/wf_action_matrix.c generated/wf_animation_tables.c generated/wf_physics_tables.c generated/wf_state5_tables.c generated/wf_collision_tables.c src/wf_collision.c src/wf_collision_runtime.c src/wf_collision_dispatch.c src/wf_victim65.c src/wf_ground.c src/wf_pin.c src/wf_ringout.c src/wf_match_rules.c src/wf_royal.c src/wf_doomsday.c src/wf_animation.c src/wf_physics.c src/wf_camera.c src/wf_visual.c src/wf_graphics.c src/wf_vdp1_plan.c src/wf_combat_frame.c src/wf_saturn_render_bridge.c src/wf_saturn_memory.c src/wf_saturn_yaul_backend.c src/wf_saturn_combat_present.c src/wf_r17_m68k_shadow.c src/wf_r17_bootstrap.c src/wf_r17_rng1129e.c src/wf_r17_human_ddc8.c src/wf_r17_human_raw23.c src/wf_r17_raw23.c src/wf_r17_actions_r11.c src/wf_r17_dispatch_1c03e.c src/wf_r17_resolver_280dc.c src/wf_r17_collision_24062.c src/wf_r17_ringside_fdee.c src/wf_r17_fixed_four_10120.c src/wf_r17_transient_reset_10e6a.c src/wf_r17_effects_7a00.c src/wf_r17_render_contract.c src/wf_r17_referee.c src/wf_r17_scheduler_owners.c src/wf_r17_follow_10e86.c src/wf_r17_dependencies.c src/wf_r17_integration.c src/wf_r17_post7a00.c src/wf_r17_participant_helpers.c src/wf_r17_participant_native.c src/wf_r17_royal_9132.c src/wf_r17_tag_6e34.c
.PHONY: test clean
test: build/test_core build/test_input build/test_animation_physics build/test_state5 build/test_collision_victim build/test_visual_camera build/test_graphics_pack build/test_runtime06 build/test_runtime07 test-runtime08 test-runtime09 test-runtime09-assets test-saturn-render-bridge test-runtime10 test-runtime11 test-runtime12 test-runtime13 test-runtime14 test-runtime15 test-runtime16 test-runtime17 test-runtime17-referee test-runtime17-f4c2 test-runtime17-collision test-runtime17-fdee test-wp35-wp36 test-runtime17-10120 test-runtime17-10e6a test-runtime17-7a00
	./build/test_core
	./build/test_input
	./build/test_animation_physics
	./build/test_state5
	./build/test_collision_victim
	./build/test_visual_camera
	./build/test_graphics_pack
	./build/test_runtime06
	./build/test_runtime07
build/test_core: $(CORE) tests/test_core.c
	mkdir -p build
	$(CC) $(CFLAGS) $(CORE) tests/test_core.c -o $@
build/test_input: $(CORE) tests/test_input.c
	mkdir -p build
	$(CC) $(CFLAGS) $(CORE) tests/test_input.c -o $@
build/test_animation_physics: $(CORE) tests/test_animation_physics.c
	mkdir -p build
	$(CC) $(CFLAGS) $(CORE) tests/test_animation_physics.c -o $@
build/test_state5: $(CORE) tests/test_state5.c
	mkdir -p build
	$(CC) $(CFLAGS) $(CORE) tests/test_state5.c -o $@
build/test_collision_victim: $(CORE) tests/test_collision_victim.c
	mkdir -p build
	$(CC) $(CFLAGS) $(CORE) tests/test_collision_victim.c -o $@
build/test_visual_camera: $(CORE) tests/test_visual_camera.c
	mkdir -p build
	$(CC) $(CFLAGS) $(CORE) tests/test_visual_camera.c -o $@
build/test_graphics_pack: $(CORE) tests/test_graphics_pack.c
	mkdir -p build
	$(CC) $(CFLAGS) $(CORE) tests/test_graphics_pack.c -o $@
build/test_runtime06: $(CORE) tests/test_runtime06.c
	mkdir -p build
	$(CC) $(CFLAGS) $(CORE) tests/test_runtime06.c -o $@
clean:
	rm -rf build

build/test_runtime07: $(CORE) tests/test_runtime07.c
	mkdir -p build
	$(CC) $(CFLAGS) $(CORE) tests/test_runtime07.c -o $@

build/test_runtime08: $(CORE) tests/test_runtime08.c
	mkdir -p build
	$(CC) $(CFLAGS) -Iinclude $(CORE) tests/test_runtime08.c -o $@

test-runtime08: build/test_runtime08
	./build/test_runtime08

build/test_runtime09: $(CORE) tests/test_runtime09.c
	mkdir -p build
	$(CC) $(CFLAGS) $(CORE) tests/test_runtime09.c -o $@

test-runtime09: build/test_runtime09
	./build/test_runtime09

build/test_runtime09_assets: $(CORE) tests/test_runtime09_assets.c
	mkdir -p build
	$(CC) $(CFLAGS) $(CORE) tests/test_runtime09_assets.c -o $@

test-runtime09-assets: build/test_runtime09_assets
	./build/test_runtime09_assets

build/test_saturn_render_bridge: $(CORE) tests/test_saturn_render_bridge.c
	mkdir -p build
	$(CC) $(CFLAGS) $(CORE) tests/test_saturn_render_bridge.c -o $@

test-saturn-render-bridge: build/test_saturn_render_bridge
	./build/test_saturn_render_bridge

build/test_runtime10: $(CORE) tests/test_runtime10.c
	mkdir -p build
	$(CC) $(CFLAGS) $(CORE) tests/test_runtime10.c -o $@

test-runtime10: build/test_runtime10
	./build/test_runtime10


build/test_runtime11: $(CORE) tests/test_runtime11.c
	mkdir -p build
	$(CC) $(CFLAGS) $(CORE) tests/test_runtime11.c -o $@

test-runtime11: build/test_runtime11
	./build/test_runtime11


build/test_runtime12: $(CORE) tests/test_runtime12.c
	mkdir -p build
	$(CC) $(CFLAGS) $(CORE) tests/test_runtime12.c -o $@

test-runtime12: build/test_runtime12
	./build/test_runtime12


build/test_runtime13: $(CORE) tests/test_runtime13.c
	mkdir -p build
	$(CC) $(CFLAGS) $(CORE) tests/test_runtime13.c -o $@

test-runtime13: build/test_runtime13
	./build/test_runtime13

build/test_runtime14: $(CORE) tests/test_runtime14.c
	mkdir -p build
	$(CC) $(CFLAGS) $(CORE) tests/test_runtime14.c -o $@

test-runtime14: build/test_runtime14
	./build/test_runtime14


build/test_runtime15: $(CORE) tests/test_runtime15.c
	mkdir -p build
	$(CC) $(CFLAGS) $(CORE) tests/test_runtime15.c -o $@

test-runtime15: build/test_runtime15
	./build/test_runtime15

build/test_runtime16: $(CORE) tests/test_runtime16.c
	mkdir -p build
	$(CC) $(CFLAGS) $(CORE) tests/test_runtime16.c -o $@

test-runtime16: build/test_runtime16
	./build/test_runtime16

build/test_runtime17: $(CORE) tests/test_runtime17.c
	mkdir -p build
	$(CC) $(CFLAGS) $(CORE) tests/test_runtime17.c -o $@
test-runtime17: build/test_runtime17
	./build/test_runtime17

build/test_runtime17_referee: $(CORE) tests/test_runtime17_referee.c
	mkdir -p build
	$(CC) $(CFLAGS) $(CORE) tests/test_runtime17_referee.c -o $@
test-runtime17-referee: build/test_runtime17_referee
	./build/test_runtime17_referee


build/test_wp35_wp36: $(CORE) tests/test_wp35_wp36.c
	mkdir -p build
	$(CC) $(CFLAGS) $(CORE) tests/test_wp35_wp36.c -o $@

test-wp35-wp36: build/test_wp35_wp36
	./build/test_wp35_wp36

build/test_runtime17_f4c2: $(CORE) tests/test_runtime17_f4c2.c
	mkdir -p build
	$(CC) $(CFLAGS) $(CORE) tests/test_runtime17_f4c2.c -o $@

test-runtime17-f4c2: build/test_runtime17_f4c2
	./build/test_runtime17_f4c2

build/test_runtime17_collision: $(CORE) tests/test_runtime17_collision.c
	mkdir -p build
	$(CC) $(CFLAGS) $(CORE) tests/test_runtime17_collision.c -o $@

test-runtime17-collision: build/test_runtime17_collision
	./build/test_runtime17_collision


build/test_runtime17_fdee: $(CORE) tests/test_runtime17_fdee.c
	mkdir -p build
	$(CC) $(CFLAGS) $(CORE) tests/test_runtime17_fdee.c -o $@

test-runtime17-fdee: build/test_runtime17_fdee
	./build/test_runtime17_fdee

build/test_runtime17_10120: $(CORE) tests/test_runtime17_10120.c
	@mkdir -p build
	$(CC) $(CFLAGS) $(CORE) tests/test_runtime17_10120.c -o $@

test-runtime17-10120: build/test_runtime17_10120
	./build/test_runtime17_10120


build/test_runtime17_10e6a: $(CORE) tests/test_runtime17_10e6a.c
	@mkdir -p build
	$(CC) $(CFLAGS) $(CORE) tests/test_runtime17_10e6a.c -o $@

test-runtime17-10e6a: build/test_runtime17_10e6a
	./build/test_runtime17_10e6a


build/test_runtime17_7a00: $(CORE) tests/test_runtime17_7a00.c
	@mkdir -p build
	$(CC) $(CFLAGS) $(CORE) tests/test_runtime17_7a00.c -o $@

test-runtime17-7a00: build/test_runtime17_7a00
	./build/test_runtime17_7a00

build/test_runtime17_8ecc: $(CORE) tests/test_runtime17_8ecc.c
	@mkdir -p build
	$(CC) $(CFLAGS) $(CORE) tests/test_runtime17_8ecc.c -o $@

test-runtime17-8ecc: build/test_runtime17_8ecc
	./build/test_runtime17_8ecc

build/test_runtime17_9052: $(CORE) tests/test_runtime17_9052.c
	@mkdir -p build
	$(CC) $(CFLAGS) $(CORE) tests/test_runtime17_9052.c -o $@

test-runtime17-9052: build/test_runtime17_9052
	./build/test_runtime17_9052

build/test_runtime17_899c: $(CORE) tests/test_runtime17_899c.c
	@mkdir -p build
	$(CC) $(CFLAGS) $(CORE) tests/test_runtime17_899c.c -o $@

test-runtime17-899c: build/test_runtime17_899c
	./build/test_runtime17_899c

build/test_runtime17_262d2: $(CORE) tests/test_runtime17_262d2.c
	@mkdir -p build
	$(CC) $(CFLAGS) $(CORE) tests/test_runtime17_262d2.c -o $@

test-runtime17-262d2: build/test_runtime17_262d2
	./build/test_runtime17_262d2


build/test_runtime17_post7_new: $(CORE) tests/test_runtime17_post7_new.c
	@mkdir -p build
	$(CC) $(CFLAGS) $(CORE) tests/test_runtime17_post7_new.c -o $@

test-runtime17-post7-new: build/test_runtime17_post7_new
	./build/test_runtime17_post7_new

build/test_runtime17_branch_a_e2e: $(CORE) tests/test_runtime17_branch_a_e2e.c
	@mkdir -p build
	$(CC) $(CFLAGS) $(CORE) tests/test_runtime17_branch_a_e2e.c -o $@

test-runtime17-branch-a-e2e: build/test_runtime17_branch_a_e2e
	./build/test_runtime17_branch_a_e2e

build/test_runtime17_branch_b_pre: $(CORE) tests/test_runtime17_branch_b_pre.c
	mkdir -p build
	$(CC) $(CFLAGS) $(CORE) tests/test_runtime17_branch_b_pre.c -o $@
test-runtime17-branch-b-pre: build/test_runtime17_branch_b_pre
	./build/test_runtime17_branch_b_pre

build/test_runtime17_7548: $(CORE) tests/test_runtime17_7548.c
	mkdir -p build
	$(CC) $(CFLAGS) $(CORE) tests/test_runtime17_7548.c -o $@
test-runtime17-7548: build/test_runtime17_7548
	./build/test_runtime17_7548

build/test_runtime17_bcfe: $(CORE) tests/test_runtime17_bcfe.c
	mkdir -p build
	$(CC) $(CFLAGS) $(CORE) tests/test_runtime17_bcfe.c -o $@
test-runtime17-bcfe: build/test_runtime17_bcfe
	./build/test_runtime17_bcfe

build/test_runtime17_c04a: $(CORE) tests/test_runtime17_c04a.c
	mkdir -p build
	$(CC) $(CFLAGS) $(CORE) tests/test_runtime17_c04a.c -o $@
test-runtime17-c04a: build/test_runtime17_c04a
	./build/test_runtime17_c04a

build/test_runtime17_c710: $(CORE) tests/test_runtime17_c710.c
	mkdir -p build
	$(CC) $(CFLAGS) $(CORE) tests/test_runtime17_c710.c -o $@
test-runtime17-c710: build/test_runtime17_c710
	./build/test_runtime17_c710

build/test_runtime17_branch_b_e2e: $(CORE) tests/test_runtime17_branch_b_e2e.c
	mkdir -p build
	$(CC) $(CFLAGS) $(CORE) tests/test_runtime17_branch_b_e2e.c -o $@
test-runtime17-branch-b-e2e: build/test_runtime17_branch_b_e2e
	./build/test_runtime17_branch_b_e2e

build/test_runtime17_2017a: $(CORE) tests/test_runtime17_2017a.c
	mkdir -p build
	$(CC) $(CFLAGS) $(CORE) tests/test_runtime17_2017a.c -o $@
test-runtime17-2017a: build/test_runtime17_2017a
	./build/test_runtime17_2017a
test: test-runtime17-2017a

build/test_runtime17_royal_postack_e2e: $(CORE) tests/test_runtime17_royal_postack_e2e.c
	mkdir -p build
	$(CC) $(CFLAGS) $(CORE) tests/test_runtime17_royal_postack_e2e.c -o $@
test-runtime17-royal-postack-e2e: build/test_runtime17_royal_postack_e2e
	./build/test_runtime17_royal_postack_e2e
test: test-runtime17-royal-postack-e2e

build/test_tag_6e34: $(CORE) tests/test_tag_6e34.c
	mkdir -p build
	$(CC) $(CFLAGS) $(CORE) tests/test_tag_6e34.c -o $@

build/test_royal_9132: $(CORE) tests/test_royal_9132.c
	mkdir -p build
	$(CC) $(CFLAGS) $(CORE) tests/test_royal_9132.c -o $@

build/test_runtime17_participant_bindings: $(CORE) tests/test_runtime17_participant_bindings.c
	mkdir -p build
	$(CC) $(CFLAGS) $(CORE) tests/test_runtime17_participant_bindings.c -o $@

.PHONY: test-participant-owners
test: test-participant-owners
test-participant-owners: build/test_tag_6e34 build/test_royal_9132 build/test_runtime17_participant_bindings
	./build/test_tag_6e34
	./build/test_royal_9132
	./build/test_runtime17_participant_bindings


build/test_runtime17_render_shadow: $(CORE) tests/test_runtime17_render_shadow.c | build
	$(CC) $(CFLAGS) $(CORE) tests/test_runtime17_render_shadow.c -o $@

test-render-shadow: build/test_runtime17_render_shadow
	./build/test_runtime17_render_shadow
test: test-render-shadow

build/test_runtime17_active_rom: $(CORE) tests/test_runtime17_active_rom.c | build
	$(CC) $(CFLAGS) $(CORE) tests/test_runtime17_active_rom.c -o $@
test-active-rom: build/test_runtime17_active_rom
	./build/test_runtime17_active_rom
test: test-active-rom

# Direct-boot integration probe; the ROM and graphics remain local audit assets.
R18_HOST=src/wf_r18_referee.c src/wf_r18_match_result.c src/wf_r18_bootstrap.c src/wf_r18_metasprite.c src/wf_r18_present.c src/wf_r18_video.c src/wf_r18_hud.c src/wf_r18_display.c src/wf_r18_world.c src/wf_r18_ring.c
build/direct_combat_probe: $(CORE) $(R18_HOST) tests/r17/direct_combat_probe.c include/wf_r18_rom_hot_pages.h
	mkdir -p build
	$(CC) $(CFLAGS) $(CORE) $(R18_HOST) tests/r17/direct_combat_probe.c -o $@
.PHONY: test-runtime18-direct test-runtime18-complete-match test-runtime18-complete-match-asan
test-runtime18-direct: build/direct_combat_probe audit/runtime18-cd/HOTSPR.BIN
	WF_PROBE_FRAMES=2000 ./build/direct_combat_probe audit/runtime18-cd/ROM68K.BIN audit/runtime18-cd/SPRITES.PAK audit/runtime18-cd/R18RING.BIN audit/runtime18-cd/HOTSPR.BIN
	WF_PROBE_FRAMES=490 WF_PROBE_ATTACK=1 ./build/direct_combat_probe audit/runtime18-cd/ROM68K.BIN audit/runtime18-cd/SPRITES.PAK audit/runtime18-cd/R18RING.BIN audit/runtime18-cd/HOTSPR.BIN

# Success requires the actual $11B6 result boundary, not merely a frame count.
test-runtime18-complete-match: build/direct_combat_probe audit/runtime18-cd/HOTSPR.BIN
	WF_PROBE_FRAMES=20000 WF_PROBE_ATTACK=1 WF_PROBE_REQUIRE_COMPLETE=1 WF_PROBE_REQUIRE_RESIDENT=1 WF_PROBE_SPRITE_TRACE=build/runtime18-match-cold-reads.csv ./build/direct_combat_probe audit/runtime18-cd/ROM68K.BIN audit/runtime18-cd/SPRITES.PAK audit/runtime18-cd/R18RING.BIN audit/runtime18-cd/HOTSPR.BIN
	test "$$(wc -l < build/runtime18-match-cold-reads.csv)" -eq 1
	@echo 'PASS complete replay: zero cold sprite reads'

build/direct_combat_probe_asan: $(CORE) $(R18_HOST) tests/r17/direct_combat_probe.c include/wf_r18_rom_hot_pages.h
	mkdir -p build
	$(CC) -std=c11 -Wall -Wextra -Werror -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -Iinclude $(CORE) $(R18_HOST) tests/r17/direct_combat_probe.c -o $@
test-runtime18-complete-match-asan: build/direct_combat_probe_asan audit/runtime18-cd/HOTSPR.BIN
	ASAN_OPTIONS=halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 WF_PROBE_FRAMES=20000 WF_PROBE_ATTACK=1 WF_PROBE_REQUIRE_COMPLETE=1 WF_PROBE_REQUIRE_RESIDENT=1 ./build/direct_combat_probe_asan audit/runtime18-cd/ROM68K.BIN audit/runtime18-cd/SPRITES.PAK audit/runtime18-cd/R18RING.BIN audit/runtime18-cd/HOTSPR.BIN

build/test_runtime18_cdfs_window: src/wf_r18_cdfs.c tests/r18/test_cdfs_window.c tests/r18/cd-block.h | build
	$(CC) $(CFLAGS) -Itests/r18 src/wf_r18_cdfs.c tests/r18/test_cdfs_window.c -o $@
.PHONY: test-runtime18-cdfs
test-runtime18-cdfs: build/test_runtime18_cdfs_window
	./build/test_runtime18_cdfs_window audit/runtime18-disc/wrestlefest-runtime18.iso audit/runtime18-cd/SPRITES.PAK

build/test_runtime18_rom_cache: $(CORE) $(R18_HOST) tests/r18/test_rom_cache.c | build
	$(CC) $(CFLAGS) $(CORE) $(R18_HOST) tests/r18/test_rom_cache.c -o $@
.PHONY: test-runtime18-rom-cache
test-runtime18-rom-cache: build/test_runtime18_rom_cache
	./build/test_runtime18_rom_cache

audit/runtime18-cd/HOTSPR.BIN: tools/build_r18_hot_sprites.py validation/runtime18-hot-tiles.txt audit/runtime18-cd/SPRITES.PAK
	python3 tools/build_r18_hot_sprites.py validation/runtime18-hot-tiles.txt audit/runtime18-cd/SPRITES.PAK $@

build/test_runtime18_hot_sprites: $(CORE) $(R18_HOST) tests/r18/test_hot_sprites.c | build
	$(CC) $(CFLAGS) $(CORE) $(R18_HOST) tests/r18/test_hot_sprites.c -o $@
.PHONY: test-runtime18-hot-sprites
test-runtime18-hot-sprites: build/test_runtime18_hot_sprites audit/runtime18-cd/HOTSPR.BIN
	./build/test_runtime18_hot_sprites audit/runtime18-cd/SPRITES.PAK audit/runtime18-cd/HOTSPR.BIN

build/test_runtime18_ring_scanline: src/wf_r18_ring.c tests/r18/test_ring_scanline.c | build
	$(CC) $(CFLAGS) src/wf_r18_ring.c tests/r18/test_ring_scanline.c -o $@
.PHONY: test-runtime18-ring-scanline
test-runtime18-ring-scanline: build/test_runtime18_ring_scanline
	./build/test_runtime18_ring_scanline audit/runtime18-cd/R18RING.BIN

build/test_runtime18_ring_scroll_cache: src/wf_r18_ring.c tests/r18/test_ring_scroll_cache.c | build
	$(CC) $(CFLAGS) src/wf_r18_ring.c tests/r18/test_ring_scroll_cache.c -o $@
.PHONY: test-runtime18-ring-scroll-cache
test-runtime18-ring-scroll-cache: build/test_runtime18_ring_scroll_cache
	./build/test_runtime18_ring_scroll_cache audit/runtime18-cd/R18RING.BIN

build/test_r18_yaul_dma: tests/r18/test_yaul_dma.c tests/r18/mock-yaul/yaul.h src/wf_saturn_yaul_backend.c src/wf_saturn_memory.c src/wf_saturn_render_bridge.c src/wf_vdp1_plan.c
	mkdir -p build
	$(CC) $(CFLAGS) -DWF_PLATFORM_SATURN -Itests/r18/mock-yaul src/wf_saturn_yaul_backend.c src/wf_saturn_memory.c src/wf_saturn_render_bridge.c src/wf_vdp1_plan.c tests/r18/test_yaul_dma.c -o $@
test-runtime18-yaul-dma: build/test_r18_yaul_dma
	./build/test_r18_yaul_dma

ROM_PYTHON ?= python3
.PHONY: test-runtime18-cpu-rom test-runtime18-yaul-dma
test-runtime18-cpu-rom:
	$(ROM_PYTHON) tools/generate_cpu_decision_data.py audit/runtime18-cd/ROM68K.BIN --check
	$(ROM_PYTHON) tools/verify_cpu_decisions_rom.py audit/runtime18-cd/ROM68K.BIN

.PHONY: test-runtime18-victim-rom test-runtime18-referee-rom test-runtime18-result-rom test-runtime18-knockdown-rom
test-runtime18-victim-rom:
	$(ROM_PYTHON) tools/verify_victim_rom.py audit/runtime18-cd/ROM68K.BIN
test-runtime18-referee-rom:
	$(ROM_PYTHON) tests/r18/verify_referee_palette_rom.py
	$(ROM_PYTHON) tests/r18/verify_referee_dependencies_rom.py
test-runtime18-result-rom:
	$(ROM_PYTHON) tools/verify_match_result_rom.py audit/runtime18-cd/ROM68K.BIN
test-runtime18-knockdown-rom:
	$(ROM_PYTHON) tools/verify_cpu_knockdown_rom.py audit/runtime18-cd/ROM68K.BIN

.PHONY: test-runtime18-pin-rom test-runtime18-count-rom
test-runtime18-pin-rom:
	$(ROM_PYTHON) tools/verify_pin_actions_rom.py audit/runtime18-cd/ROM68K.BIN
test-runtime18-count-rom:
	$(ROM_PYTHON) tests/r18/verify_referee_count_chain_rom.py

.PHONY: test-runtime18-movement-rom
test-runtime18-movement-rom:
	$(ROM_PYTHON) tools/verify_movement_rom.py audit/runtime18-cd/ROM68K.BIN

.PHONY: test-runtime18-pin-escape-rom
test-runtime18-pin-escape-rom:
	$(ROM_PYTHON) tests/r18/verify_pin_escape_rom.py

.PHONY: test-runtime18-tag-entry-rom test-runtime18-victim-boundaries-rom test-runtime18-throw-victim-rom
test-runtime18-tag-entry-rom:
	$(ROM_PYTHON) tools/verify_tag_entry_rom.py audit/runtime18-cd/ROM68K.BIN
test-runtime18-victim-boundaries-rom:
	$(ROM_PYTHON) tests/r18/verify_victim_boundaries_rom.py
test-runtime18-throw-victim-rom:
	$(ROM_PYTHON) tools/verify_throw_victim_rom.py audit/runtime18-cd/ROM68K.BIN

.PHONY: test-runtime18-cpu-state4-rom test-runtime18-hot-boundaries
test-runtime18-cpu-state4-rom:
	$(ROM_PYTHON) tools/verify_cpu_state4_rom.py audit/runtime18-cd/ROM68K.BIN
test-runtime18-hot-boundaries:
	$(ROM_PYTHON) tests/r18/verify_hot_sprite_boundaries.py

.PHONY: test-runtime18-ring-exit-rom test-runtime18-throw-actions-rom
test-runtime18-ring-exit-rom:
	$(ROM_PYTHON) tools/verify_ring_exit_rom.py audit/runtime18-cd/ROM68K.BIN
test-runtime18-throw-actions-rom:
	$(ROM_PYTHON) tools/verify_throw_actions_rom.py audit/runtime18-cd/ROM68K.BIN

.PHONY: test-runtime18-cpu-tag-rom test-runtime18-cpu-corner-rom test-runtime18-raw71-rom test-runtime18-raw4c-rom
test-runtime18-cpu-tag-rom:
	$(ROM_PYTHON) tools/verify_cpu_tag_rom.py audit/runtime18-cd/ROM68K.BIN
test-runtime18-cpu-corner-rom:
	$(ROM_PYTHON) tools/verify_cpu_corner_rom.py audit/runtime18-cd/ROM68K.BIN
test-runtime18-raw71-rom:
	$(ROM_PYTHON) tools/verify_raw71_rom.py audit/runtime18-cd/ROM68K.BIN
test-runtime18-raw4c-rom:
	$(ROM_PYTHON) tools/verify_raw4c_rom.py audit/runtime18-cd/ROM68K.BIN

.PHONY: test-runtime18-timeup-rom
test-runtime18-timeup-rom:
	$(ROM_PYTHON) tools/verify_timeup_rom.py audit/runtime18-cd/ROM68K.BIN

.PHONY: test-runtime18-tag-finish-rom
test-runtime18-tag-finish-rom:
	$(ROM_PYTHON) tools/verify_tag_finish_rom.py audit/runtime18-cd/ROM68K.BIN

.PHONY: test-runtime18-rom-validation
test-runtime18-rom-validation: test-runtime18-cpu-rom test-runtime18-victim-rom \
 test-runtime18-referee-rom test-runtime18-result-rom test-runtime18-knockdown-rom \
 test-runtime18-pin-rom test-runtime18-count-rom test-runtime18-movement-rom \
 test-runtime18-pin-escape-rom test-runtime18-tag-entry-rom \
 test-runtime18-victim-boundaries-rom test-runtime18-throw-victim-rom \
 test-runtime18-cpu-state4-rom test-runtime18-hot-boundaries \
 test-runtime18-ring-exit-rom test-runtime18-throw-actions-rom \
 test-runtime18-cpu-tag-rom test-runtime18-cpu-corner-rom test-runtime18-raw71-rom \
 test-runtime18-raw4c-rom test-runtime18-timeup-rom test-runtime18-tag-finish-rom \
 test-runtime18-cpu-defender-rom test-runtime18-cpu-assist-rom test-runtime18-raw0a-rom test-runtime18-raw10-rom

.PHONY: test-runtime18-cpu-defender-rom test-runtime18-cpu-assist-rom
test-runtime18-cpu-defender-rom:
	$(ROM_PYTHON) tools/verify_cpu_defender_rom.py audit/runtime18-cd/ROM68K.BIN
test-runtime18-cpu-assist-rom:
	$(ROM_PYTHON) tools/verify_cpu_assist_rom.py audit/runtime18-cd/ROM68K.BIN

.PHONY: test-runtime18-raw0a-rom
test-runtime18-raw0a-rom:
	$(ROM_PYTHON) tools/verify_raw0a_rom.py audit/runtime18-cd/ROM68K.BIN

.PHONY: test-runtime18-raw10-rom
test-runtime18-raw10-rom:
	$(ROM_PYTHON) tools/verify_raw10_rom.py audit/runtime18-cd/ROM68K.BIN

.PHONY: test-runtime18-raw59-rom test-runtime18-cpu-tiers-rom test-runtime18-noinput-complete-match test-runtime18-noinput-complete-match-asan
test-runtime18-raw59-rom:
	$(ROM_PYTHON) tools/verify_raw59_links_rom.py audit/runtime18-cd/ROM68K.BIN
test-runtime18-cpu-tiers-rom:
	$(ROM_PYTHON) tools/verify_cpu_tiers_rom.py audit/runtime18-cd/ROM68K.BIN
test-runtime18-rom-validation: test-runtime18-raw59-rom test-runtime18-cpu-tiers-rom

test-runtime18-noinput-complete-match: build/direct_combat_probe audit/runtime18-cd/HOTSPR.BIN
	env -u WF_PROBE_ATTACK WF_PROBE_FRAMES=20000 WF_PROBE_REQUIRE_COMPLETE=1 WF_PROBE_REQUIRE_RESIDENT=1 WF_PROBE_SPRITE_TRACE=build/runtime18-noinput-cold-reads.csv ./build/direct_combat_probe audit/runtime18-cd/ROM68K.BIN audit/runtime18-cd/SPRITES.PAK audit/runtime18-cd/R18RING.BIN audit/runtime18-cd/HOTSPR.BIN

test-runtime18-noinput-complete-match-asan: build/direct_combat_probe_asan audit/runtime18-cd/HOTSPR.BIN
	env -u WF_PROBE_ATTACK ASAN_OPTIONS=halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 WF_PROBE_FRAMES=20000 WF_PROBE_REQUIRE_COMPLETE=1 WF_PROBE_REQUIRE_RESIDENT=1 ./build/direct_combat_probe_asan audit/runtime18-cd/ROM68K.BIN audit/runtime18-cd/SPRITES.PAK audit/runtime18-cd/R18RING.BIN audit/runtime18-cd/HOTSPR.BIN

.PHONY: test-runtime18-raw33-rom test-runtime18-raw0e-rom test-runtime18-raw51-rom test-runtime18-cpu-approach-rom
test-runtime18-raw33-rom:
	$(ROM_PYTHON) tools/verify_raw33_rom.py audit/runtime18-cd/ROM68K.BIN
test-runtime18-raw0e-rom:
	$(ROM_PYTHON) tools/verify_raw0e_rom.py audit/runtime18-cd/ROM68K.BIN
test-runtime18-raw51-rom:
	$(ROM_PYTHON) tools/verify_raw51_rom.py audit/runtime18-cd/ROM68K.BIN
test-runtime18-cpu-approach-rom:
	$(ROM_PYTHON) tools/verify_cpu_approach_rom.py audit/runtime18-cd/ROM68K.BIN
test-runtime18-rom-validation: test-runtime18-raw33-rom test-runtime18-raw0e-rom test-runtime18-raw51-rom test-runtime18-cpu-approach-rom

.PHONY: test-runtime18-cpu-aligned-rom
test-runtime18-cpu-aligned-rom:
	$(ROM_PYTHON) tools/verify_cpu_aligned_rom.py audit/runtime18-cd/ROM68K.BIN
test-runtime18-rom-validation: test-runtime18-cpu-aligned-rom

.PHONY: test-runtime18-camera-rom
test-runtime18-camera-rom:
	$(ROM_PYTHON) tools/verify_camera_sync_rom.py audit/runtime18-cd/ROM68K.BIN
test-runtime18-rom-validation: test-runtime18-camera-rom

build/test_runtime18_ring_priority: src/wf_r18_ring.c src/wf_saturn_memory.c tests/r18/test_ring_priority.c tests/r18/mock-yaul/yaul.h | build
	$(CC) $(CFLAGS) -D__sh__ -Itests/r18/mock-yaul src/wf_r18_ring.c src/wf_saturn_memory.c tests/r18/test_ring_priority.c -o $@
.PHONY: test-runtime18-ring-priority
test-runtime18-ring-priority: build/test_runtime18_ring_priority
	./build/test_runtime18_ring_priority audit/runtime18-cd/R18RING.BIN

build/test_runtime18_ring_ids: src/wf_r18_ring.c tests/r18/test_ring_ids.c | build
	$(CC) $(CFLAGS) src/wf_r18_ring.c tests/r18/test_ring_ids.c -o $@
.PHONY: test-runtime18-ring-ids
test-runtime18-ring-ids: build/test_runtime18_ring_ids
	./build/test_runtime18_ring_ids audit/runtime18-cd/R18RING.BIN

build/test_runtime18_ring_cells: src/wf_r18_ring.c tests/r18/test_ring_cells.c | build
	$(CC) $(CFLAGS) src/wf_r18_ring.c tests/r18/test_ring_cells.c -o $@
.PHONY: test-runtime18-ring-cells
test-runtime18-ring-cells: build/test_runtime18_ring_cells
	./build/test_runtime18_ring_cells audit/runtime18-cd/R18RING.BIN

.PHONY: test-runtime18-ring-native-asan
build:
	mkdir -p $@

test-runtime18-ring-native-asan: | build
	$(CC) $(CFLAGS) -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer src/wf_r18_ring.c tests/r18/test_ring_cells.c -o build/test_runtime18_ring_cells_asan
	ASAN_OPTIONS=halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 ./build/test_runtime18_ring_cells_asan audit/runtime18-cd/R18RING.BIN
	$(CC) $(CFLAGS) -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer src/wf_r18_ring.c tests/r18/test_ring_ids.c -o build/test_runtime18_ring_ids_asan
	ASAN_OPTIONS=halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 ./build/test_runtime18_ring_ids_asan audit/runtime18-cd/R18RING.BIN
	$(CC) $(CFLAGS) -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -D__sh__ -Itests/r18/mock-yaul src/wf_r18_ring.c src/wf_saturn_memory.c tests/r18/test_ring_priority.c -o build/test_runtime18_ring_priority_asan
	ASAN_OPTIONS=halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 ./build/test_runtime18_ring_priority_asan audit/runtime18-cd/R18RING.BIN

.PHONY: test-runtime18-display-rom
test-runtime18-display-rom:
	$(ROM_PYTHON) tests/r18/verify_display_rom.py

build/test_runtime18_hud: src/wf_r18_hud.c src/wf_r18_video.c src/wf_saturn_memory.c tests/r18/test_hud.c | build
	$(CC) $(CFLAGS) src/wf_r18_hud.c src/wf_r18_video.c src/wf_saturn_memory.c tests/r18/test_hud.c -o $@
test-runtime18-hud: build/test_runtime18_hud
	./build/test_runtime18_hud

build/test_runtime18_hud_yaul: src/wf_r18_hud.c src/wf_r18_video.c src/wf_saturn_memory.c tests/r18/test_hud_yaul.c tests/r18/mock-yaul/yaul.h | build
	$(CC) $(CFLAGS) -DWF_PLATFORM_SATURN -Itests/r18/mock-yaul src/wf_r18_hud.c src/wf_r18_video.c src/wf_saturn_memory.c tests/r18/test_hud_yaul.c -o $@
test-runtime18-hud-yaul: build/test_runtime18_hud_yaul
	./build/test_runtime18_hud_yaul audit/runtime18-cd/FONT8.BIN

.PHONY: test-runtime18-display-asan test-runtime18-hud test-runtime18-hud-yaul
test-runtime18-display-asan: | build
	$(CC) $(CFLAGS) -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer src/wf_r18_hud.c src/wf_r18_video.c src/wf_saturn_memory.c tests/r18/test_hud.c -o build/test_runtime18_hud_asan
	ASAN_OPTIONS=halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 ./build/test_runtime18_hud_asan
	$(CC) $(CFLAGS) -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -DWF_PLATFORM_SATURN -Itests/r18/mock-yaul src/wf_saturn_yaul_backend.c src/wf_saturn_memory.c src/wf_saturn_render_bridge.c src/wf_vdp1_plan.c tests/r18/test_yaul_dma.c -o build/test_runtime18_dma_asan
	ASAN_OPTIONS=halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 ./build/test_runtime18_dma_asan

build/test_runtime18_command_work: tests/r18/test_command_work.c src/wf_saturn_yaul_backend.c src/wf_saturn_memory.c | build
	$(CC) $(CFLAGS) -DWF_PLATFORM_SATURN -Itests/r18/mock-yaul tests/r18/test_command_work.c src/wf_saturn_memory.c src/wf_saturn_render_bridge.c src/wf_vdp1_plan.c -o $@
test-runtime18-command-work: build/test_runtime18_command_work
	./build/test_runtime18_command_work

.PHONY: test-runtime18-command-work test-runtime18-plan-equivalence
test-runtime18-plan-equivalence:
	$(ROM_PYTHON) tests/r18/verify_plan_equivalence.py
