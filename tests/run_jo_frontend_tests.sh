#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
wf_front_build=${WF_FRONT_TEST_BUILD:-build/frontend-transport-tests}
python tools/build_runtime_tests.py --build-dir "$wf_front_build"
${CC:-cc} -std=c11 -O2 -g -Wall -Wextra -Werror \
  -DWF_JO_ENGINE -DWF_JO_ASSETS_HOST -DWF_JO_FRONTEND_TEST -DZ_SOLO -DNO_GZIP \
  -Icore/include -Iport -Iport/zlib -Itests -Itests/native \
  tests/test_jo_frontend.c tests/native/assets_fixture.c \
  port/wf_jo_frontend.c port/wf_jo_clock.c "$wf_front_build"/*.o \
  -o "$wf_front_build/jo_frontend_probe"
wf_front_probe="$(pwd)/$wf_front_build/jo_frontend_probe"
cd cd
WF_JO_TEST_CACHE_SLOTS=71 "$wf_front_probe" 0 1
WF_JO_TEST_CACHE_SLOTS=69 "$wf_front_probe" 1 3
WF_JO_TEST_DIRTY_FRAMES=1 WF_JO_TEST_CACHE_SLOTS=69 "$wf_front_probe" 1 3
WF_JO_TEST_CACHE_SLOTS=65 "$wf_front_probe" 0 1 0x1b4e
WF_JO_TEST_CACHE_SLOTS=65 "$wf_front_probe" 1 3 0x1b4e
WF_JO_TEST_CACHE_SLOTS=65 "$wf_front_probe" 1 1 0x1afc
WF_JO_TEST_CACHE_SLOTS=65 "$wf_front_probe" 1 1 0x1afc 1
WF_JO_TEST_CACHE_SLOTS=65 "$wf_front_probe" 0 1 0x434
WF_JO_TEST_CACHE_SLOTS=65 "$wf_front_probe" 0 1 0x978
