#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
PREFIX=${SH2_PREFIX:-sh2eb-elf}
$PREFIX-gcc -std=c11 -Wall -Wextra -Werror -ffreestanding -fno-builtin -m2 \
  -I"$ROOT/src/firstmatch" \
  -c "$ROOT/src/firstmatch/wp12_firstmatch_presentation.c" -o /tmp/wp12_firstmatch.sh2.o
$PREFIX-ld -r /tmp/wp12_firstmatch.sh2.o -o /tmp/wp12_r03_firstmatch.sh2.o
if [ -n "$($PREFIX-nm -u /tmp/wp12_r03_firstmatch.sh2.o)" ]; then
  $PREFIX-nm -u /tmp/wp12_r03_firstmatch.sh2.o
  exit 1
fi
echo 'WP12 R03 SH-2 first-match module: PASS / 0 unresolved'
