#!/bin/bash
# Source this file before building Yaul programs or running locally unpacked tools.
WP1979_TOOLING_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
export YAUL_INSTALL_ROOT="$WP1979_TOOLING_ROOT/sh2eb-elf"
export YAUL_ARCH_SH_PREFIX=sh2eb-elf
export YAUL_PROG_SH_PREFIX=sh2eb-elf
export YAUL_ARCH_M68K_PREFIX=m68keb-elf
export YAUL_OPTION_MALLOC_IMPL=tlsf
export MAKE_ISO_XORRISOFS="$WP1979_TOOLING_ROOT/host/usr/bin/xorrisofs"
export PATH="$YAUL_INSTALL_ROOT/bin:$WP1979_TOOLING_ROOT/host/usr/bin:$WP1979_TOOLING_ROOT/host/usr/games:$PATH"
export LD_LIBRARY_PATH="$WP1979_TOOLING_ROOT/host/usr/lib/x86_64-linux-gnu:$WP1979_TOOLING_ROOT/host/lib/x86_64-linux-gnu${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export SILENT=1
export NOCOLOR=1
