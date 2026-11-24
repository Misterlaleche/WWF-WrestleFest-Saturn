#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
wf_test_cc=${WF_TEST_CC:-cc}
wf_test_dir=${WF_TEST_DIR:-build/video-tests}
mkdir -p "$wf_test_dir"
"$wf_test_cc" -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
  -Icore/include -Iport tests/test_jo_video.c port/wf_jo_video.c \
  -o "$wf_test_dir/pixels"
ASAN_OPTIONS=detect_leaks=0 "$wf_test_dir/pixels"
"$wf_test_cc" -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
  -ffunction-sections -fdata-sections -DWF_JO_ENGINE -Icore/include -Iport \
  tests/test_jo_backend_batch.c port/wf_jo_backend.c port/wf_jo_video.c \
  core/src/wf_saturn_memory.c -Wl,--gc-sections -o "$wf_test_dir/backend-batch"
ASAN_OPTIONS=detect_leaks=0 "$wf_test_dir/backend-batch"
"$wf_test_cc" -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
  -DWF_JO_ENGINE -Icore/include tests/test_jo_metasprite_alignment.c \
  core/src/wf_r18_metasprite.c -o "$wf_test_dir/metasprite-alignment"
ASAN_OPTIONS=detect_leaks=0 "$wf_test_dir/metasprite-alignment" cd/ROM68K.BIN
"$wf_test_cc" -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
  -Iport tests/test_jo_tile_slots.c -o "$wf_test_dir/tile-slots"
ASAN_OPTIONS=detect_leaks=0 "$wf_test_dir/tile-slots"
"$wf_test_cc" -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-int-to-pointer-cast \
  -fsanitize=address,undefined -DWF_PLATFORM_SATURN=1 -DWF_JO_ENGINE=1 -DWF_JO_VIDEO_TEST=1 \
  -Icore/include -Iport -Itests tests/test_jo_pipeline.c port/wf_jo_video.c \
  core/src/wf_saturn_memory.c -o "$wf_test_dir/pipeline"
ASAN_OPTIONS=detect_leaks=0 "$wf_test_dir/pipeline"
"$wf_test_cc" -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-misleading-indentation \
  -fsanitize=address,undefined -Icore/include tests/test_jo_ring.c \
  core/src/wf_r18_ring.c -o "$wf_test_dir/ring"
ASAN_OPTIONS=detect_leaks=0 "$wf_test_dir/ring"
"$wf_test_cc" -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
  -Icore/include tests/test_demolition_logo.c core/src/wf_r18_match_support.c \
  core/src/wf_r18_video.c core/src/wf_r18_hud.c core/src/wf_r17_m68k_shadow.c \
  -o "$wf_test_dir/demolition-logo"
ASAN_OPTIONS=detect_leaks=0 "$wf_test_dir/demolition-logo" cd/ROM68K.BIN cd/FONT8.BIN
