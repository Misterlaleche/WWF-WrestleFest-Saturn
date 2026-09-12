CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -O2 -Iinclude
CORE=src/wf_actor.c src/wf_payment.c src/wf_spatial.c src/wf_target_relation.c src/wf_pair_relation.c src/wf_actor_prepass.c src/wf_state5_exec.c src/arcade_cpu_final_bridge.c src/wf_cpu_runtime.c src/wf_gameplay_bindings.c src/wf_damage.c src/wf_scheduler.c src/wf_core_phases.c src/wf_input.c generated/wf_action_matrix.c generated/wf_animation_tables.c generated/wf_physics_tables.c generated/wf_state5_tables.c generated/wf_collision_tables.c src/wf_collision.c src/wf_collision_runtime.c src/wf_collision_dispatch.c src/wf_victim65.c src/wf_ground.c src/wf_pin.c src/wf_ringout.c src/wf_match_rules.c src/wf_royal.c src/wf_doomsday.c src/wf_animation.c src/wf_physics.c src/wf_camera.c src/wf_visual.c src/wf_graphics.c src/wf_vdp1_plan.c src/wf_combat_frame.c src/wf_saturn_render_bridge.c src/wf_saturn_memory.c src/wf_saturn_yaul_backend.c src/wf_saturn_combat_present.c
.PHONY: test clean
test: build/test_core build/test_input build/test_animation_physics build/test_state5 build/test_collision_victim build/test_visual_camera build/test_graphics_pack build/test_runtime06 build/test_runtime07 test-runtime08 test-runtime09 test-runtime09-assets test-saturn-render-bridge test-runtime10 test-runtime11 test-runtime12 test-runtime13 test-runtime14 test-runtime15 test-runtime16
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
