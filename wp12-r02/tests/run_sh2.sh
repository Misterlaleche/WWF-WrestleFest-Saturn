#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
PREFIX=${SH2_PREFIX:-sh2eb-elf}
$PREFIX-gcc -std=c11 -Wall -Wextra -Werror -ffreestanding -fno-builtin -m2 \
  -I"$ROOT/src/runtime" -I"$ROOT/src/renderer" \
  -c "$ROOT/src/runtime/wp12_romexact.c" -o /tmp/wp12_romexact.sh2.o
$PREFIX-gcc -std=c11 -Wall -Wextra -Werror -ffreestanding -fno-builtin -m2 \
  -I"$ROOT/src/runtime" -I"$ROOT/src/renderer" \
  -c "$ROOT/src/renderer/wp12_renderer_contract.c" -o /tmp/wp12_renderer_contract.sh2.o
$PREFIX-ld -r /tmp/wp12_romexact.sh2.o /tmp/wp12_renderer_contract.sh2.o -o /tmp/wp12_r02_module.sh2.o
if [ -n "$($PREFIX-nm -u /tmp/wp12_r02_module.sh2.o)" ]; then
  $PREFIX-nm -u /tmp/wp12_r02_module.sh2.o
  exit 1
fi
echo 'WP12 R02 SH-2 compile/link: PASS / 0 unresolved'
