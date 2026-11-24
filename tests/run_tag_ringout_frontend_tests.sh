#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
wf_ringout_build=${WF_TAG_RINGOUT_TEST_BUILD:-build/b12-tag-ringout-front}
python tools/build_runtime_tests.py --build-dir "$wf_ringout_build"
${CC:-cc} -std=c11 -O2 -g -Wall -Wextra -Werror \
  -DWF_JO_ENGINE -DWF_JO_ASSETS_HOST -DWF_JO_FRONTEND_TEST -DZ_SOLO -DNO_GZIP \
  -Icore/include -Iport -Iport/zlib -Itests -Itests/native \
  tests/test_tag_ringout_frontend.c tests/native/assets_fixture.c \
  port/wf_jo_frontend.c port/wf_jo_clock.c "$wf_ringout_build"/*.o \
  -o "$wf_ringout_build/tag_ringout_frontend"
wf_ringout_probe="$(pwd)/$wf_ringout_build/tag_ringout_frontend"
cd cd
WF_JO_TEST_CACHE_BYTES=565248 "$wf_ringout_probe"
