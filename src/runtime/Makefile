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
