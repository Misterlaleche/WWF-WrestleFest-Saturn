CC ?= cc
PYTHON ?= python3
CFLAGS := -std=c11 -O2 -Wall -Wextra -Werror
INCLUDES := -Isrc -I'../../Entrada Ring/source' -I'../../Escena Presentadora/source/scene/src'
SCENES := '../../Entrada Ring/source/wp01_scene.c' '../../Escena Presentadora/source/scene/src/wp02_scene.c'
CORE := src/wf_arcade_clock.c src/wf_prematch.c
.PHONY: test sanitize verify-sources sh2

verify-sources:
	$(PYTHON) tools/check_sources.py

test: verify-sources
	@mkdir -p build
	$(CC) $(CFLAGS) $(INCLUDES) tests/test_clock.c src/wf_arcade_clock.c -o build/test_clock
	./build/test_clock
	$(CC) $(CFLAGS) $(INCLUDES) tests/test_prematch.c src/wf_prematch.c $(SCENES) -o build/test_prematch
	./build/test_prematch
	PYTHONDONTWRITEBYTECODE=1 $(PYTHON) -m unittest discover -s tests -p 'test_*.py' -v

sanitize: verify-sources
	@mkdir -p build
	$(CC) $(CFLAGS) -fsanitize=address,undefined $(INCLUDES) tests/test_clock.c src/wf_arcade_clock.c -o build/test_clock_sanitized
	./build/test_clock_sanitized
	$(CC) $(CFLAGS) -fsanitize=address,undefined $(INCLUDES) tests/test_prematch.c src/wf_prematch.c $(SCENES) -o build/test_prematch_sanitized
	./build/test_prematch_sanitized

sh2: verify-sources
	bash tools/build_sh2.sh
