#!/usr/bin/env bash
set -euo pipefail
wp01_source_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
: "${YAUL_INSTALL_ROOT:?Set YAUL_INSTALL_ROOT to your supplied sh2eb-elf Yaul toolchain}"
export PATH="$YAUL_INSTALL_ROOT/bin:$PATH"
export YAUL_ARCH_SH_PREFIX=sh2eb-elf
export YAUL_PROG_SH_PREFIX=sh2eb-elf
export YAUL_ARCH_M68K_PREFIX=m68keb-elf
export YAUL_OPTION_MALLOC_IMPL=tlsf
: "${MAKE_ISO_XORRISOFS:?Set MAKE_ISO_XORRISOFS to xorrisofs}"
make -C "$wp01_source_dir" "$@"
