#!/usr/bin/env bash
# Source this file to use the isolated, locally extracted toolchain.
WF_TOOL_ENV_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
export YAUL_INSTALL_ROOT="$WF_TOOL_ENV_ROOT/sh2eb-elf"
export YAUL_ARCH_SH_PREFIX=sh2eb-elf
export YAUL_PROG_SH_PREFIX=sh2eb-elf
export YAUL_ARCH_M68K_PREFIX=m68k-linux-gnu
export YAUL_OPTION_MALLOC_IMPL=tlsf
export MAKE_ISO_XORRISOFS="$WF_TOOL_ENV_ROOT/host/usr/bin/xorrisofs"
export PATH="$WF_TOOL_ENV_ROOT/bin:$YAUL_INSTALL_ROOT/bin:$WF_TOOL_ENV_ROOT/host/usr/bin:$PATH"
export LD_LIBRARY_PATH="$WF_TOOL_ENV_ROOT/host/usr/lib/x86_64-linux-gnu:$WF_TOOL_ENV_ROOT/host/lib/x86_64-linux-gnu${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export PKG_CONFIG="$WF_TOOL_ENV_ROOT/host/usr/bin/pkgconf"
export PKG_CONFIG_LIBDIR="$WF_TOOL_ENV_ROOT/host/usr/lib/x86_64-linux-gnu/pkgconfig:$WF_TOOL_ENV_ROOT/host/usr/share/pkgconfig"
export PKG_CONFIG_SYSROOT_DIR="$WF_TOOL_ENV_ROOT/host"
export MEDNAFEN_HOME="${MEDNAFEN_HOME:-$WF_TOOL_ENV_ROOT/mednafen-home}"
export SILENT=1
export NOCOLOR=1
