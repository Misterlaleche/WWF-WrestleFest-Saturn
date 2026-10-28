#!/bin/sh
# Assertion-enabled shared raw frontend/paid composition under host sanitizers.
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
OUT=${1:?Pass an absolute output directory}
mkdir -p "$OUT"
cd "$ROOT/src/runtime"
CORE=$(sed -n 's/^CORE=//p' Makefile)
R18=$(sed -n 's/^R18_HOST=//p' Makefile)
# Source names come from the checked-in Makefile; they contain no whitespace.
cc -std=c11 -O1 -g -UNDEBUG -Wall -Wextra -Werror -Iinclude \
 -fsanitize=address,undefined -fno-omit-frame-pointer \
 $CORE $R18 tests/r18/match_driver_host.c -o "$OUT/match-driver-asan"
ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=halt_on_error=1 \
 "$OUT/match-driver-asan" audit/runtime18-cd/ROM68K.BIN
