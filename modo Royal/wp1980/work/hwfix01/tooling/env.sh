#!/bin/bash
# Read-only compiler/SDK selection. For another machine, set
# WP1980_TOOLING_ROOT to a directory containing sh2eb-elf/ and optionally
# host/usr/bin/, or set YAUL_INSTALL_ROOT to an existing compatible Yaul SDK.
WP1980_LOCAL_TOOLING="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
if [[ -n "${WP1980_TOOLING_ROOT:-}" ]]; then
    export YAUL_INSTALL_ROOT="$WP1980_TOOLING_ROOT/sh2eb-elf"
elif [[ -n "${YAUL_INSTALL_ROOT:-}" && -f "$YAUL_INSTALL_ROOT/share/build.pre.mk" ]]; then
    WP1980_TOOLING_ROOT="$(dirname "$YAUL_INSTALL_ROOT")"
elif [[ -f "$WP1980_LOCAL_TOOLING/sh2eb-elf/share/build.pre.mk" ]]; then
    WP1980_TOOLING_ROOT="$WP1980_LOCAL_TOOLING"
    export YAUL_INSTALL_ROOT="$WP1980_TOOLING_ROOT/sh2eb-elf"
else
    # Session-local, read-only dependency already supplied with the project.
    WP1980_TOOLING_ROOT=/workspace/scratch/1e822aeaeac7/wp1979/tooling
    export YAUL_INSTALL_ROOT="$WP1980_TOOLING_ROOT/sh2eb-elf"
fi
if [[ ! -f "$YAUL_INSTALL_ROOT/share/build.pre.mk" ]]; then
    printf '%s\n' 'Set WP1980_TOOLING_ROOT or YAUL_INSTALL_ROOT to the supplied Yaul/SH-2 SDK.' >&2
    return 1 2>/dev/null || exit 1
fi
export YAUL_ARCH_SH_PREFIX=sh2eb-elf
export YAUL_PROG_SH_PREFIX=sh2eb-elf
export YAUL_ARCH_M68K_PREFIX=m68keb-elf
export YAUL_OPTION_MALLOC_IMPL=tlsf
export PATH="$YAUL_INSTALL_ROOT/bin:$WP1980_TOOLING_ROOT/host/usr/bin:$WP1980_TOOLING_ROOT/host/usr/games:$PATH"
if [[ -x "$WP1980_TOOLING_ROOT/host/usr/bin/xorrisofs" ]]; then
    export MAKE_ISO_XORRISOFS="$WP1980_TOOLING_ROOT/host/usr/bin/xorrisofs"
else
    export MAKE_ISO_XORRISOFS="$(command -v xorrisofs)"
fi
export LD_LIBRARY_PATH="$WP1980_TOOLING_ROOT/host/usr/lib/x86_64-linux-gnu:$WP1980_TOOLING_ROOT/host/lib/x86_64-linux-gnu${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export SILENT=1 NOCOLOR=1
