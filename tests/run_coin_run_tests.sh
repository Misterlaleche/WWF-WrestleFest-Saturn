#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
wf_coin_build=${WF_COIN_TEST_BUILD:-build/coin-run-tests}
mkdir -p "$wf_coin_build"
${CC:-cc} -std=c11 -O2 -g -Wall -Wextra -Werror -fno-builtin \
  ${WF_COIN_TEST_CFLAGS:-} -Icore/include \
  tests/test_coin_run.c core/src/wf_r18_frontend_coin.c \
  core/src/wf_r17_m68k_shadow.c -Wl,--wrap=memcpy -o "$wf_coin_build/coin_run"
"$wf_coin_build/coin_run"
