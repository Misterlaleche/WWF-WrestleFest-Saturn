#!/usr/bin/env bash
set -euo pipefail
wp0_source_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
: "${YAUL_INSTALL_ROOT:?Set YAUL_INSTALL_ROOT to your sh2eb-elf Yaul toolchain.}"
export PATH="$YAUL_INSTALL_ROOT/bin:$PATH"
export YAUL_ARCH_SH_PREFIX=sh2eb-elf
export YAUL_PROG_SH_PREFIX=sh2eb-elf
export YAUL_ARCH_M68K_PREFIX=m68keb-elf
export YAUL_OPTION_MALLOC_IMPL=tlsf
if [[ -z "${MAKE_ISO_XORRISOFS:-}" ]]; then
    export MAKE_ISO_XORRISOFS="$(command -v xorrisofs)"
fi
: "${MAKE_ISO_XORRISOFS:?Install xorrisofs or set MAKE_ISO_XORRISOFS to its absolute path.}"
make -C "$wp0_source_dir"
wp0_output_dir="${1:-$wp0_source_dir/out/WF-WP0-R04}"
python3 "$wp0_source_dir/tools/iso_to_bin.py" "$wp0_source_dir/WF-WP0-R04.iso" "$wp0_output_dir"
