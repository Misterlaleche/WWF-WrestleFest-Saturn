CC ?= cc
PYTHON ?= python3
CFLAGS := -std=c11 -O2 -Wall -Wextra -Werror
INCLUDES := -Isrc -I'../../Entrada Ring/source' -I'../../Escena Presentadora/source/scene/src'
SCENES := '../../Entrada Ring/source/wp01_scene.c' '../../Escena Presentadora/source/scene/src/wp02_scene.c'
CORE := src/wf_arcade_clock.c src/wf_prematch.c
.PHONY: test sanitize verify-sources sh2

verify-sources:
	$(PYTHON) tools/check_sources.py

test: verify-sources test-platform
	@mkdir -p build
	$(CC) $(CFLAGS) $(INCLUDES) tests/test_clock.c src/wf_arcade_clock.c -o build/test_clock
	./build/test_clock
	$(CC) $(CFLAGS) $(INCLUDES) tests/test_prematch.c src/wf_prematch.c $(SCENES) -o build/test_prematch
	./build/test_prematch
	PYTHONDONTWRITEBYTECODE=1 $(PYTHON) -m unittest discover -s tests -p 'test_*.py' -v

sanitize: verify-sources sanitize-platform
	@mkdir -p build
	$(CC) $(CFLAGS) -fsanitize=address,undefined $(INCLUDES) tests/test_clock.c src/wf_arcade_clock.c -o build/test_clock_sanitized
	./build/test_clock_sanitized
	$(CC) $(CFLAGS) -fsanitize=address,undefined $(INCLUDES) tests/test_prematch.c src/wf_prematch.c $(SCENES) -o build/test_prematch_sanitized
	./build/test_prematch_sanitized

sh2: verify-sources
	bash tools/build_sh2.sh

.PHONY: test-platform sanitize-platform
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
