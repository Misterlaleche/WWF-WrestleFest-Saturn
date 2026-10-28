#!/bin/bash
set -euo pipefail
BASE=/run/royalbuild
SDK=/home/builder/sdk/sh2eb-elf
export TMPDIR="$BASE" PYTHONDONTWRITEBYTECODE=1
export YAUL_INSTALL_ROOT="$SDK" YAUL_ARCH_SH_PREFIX=sh2eb-elf YAUL_ARCH_M68K_PREFIX=m68keb-elf
export YAUL_OPTION_MALLOC_IMPL=tlsf SILENT=1 NOCOLOR=1 PATH="$SDK/bin:$PATH"
cd "$BASE"
tar -xzf r05-sources.tar.gz
tar -xzf royal-update.tar.gz
cd "$BASE/runtime18/src/runtime"
make -f Makefile.saturn-disc -j2 "$PWD/audit/runtime18-build/wrestlefest-runtime18.elf"
ELF=audit/runtime18-build/wrestlefest-runtime18.elf
sh2eb-elf-nm -u "$ELF" > "$BASE/unresolved.txt"
test ! -s "$BASE/unresolved.txt"
sh2eb-elf-size "$ELF"
python3 "$BASE/runtime18/validation/verify_saturn_stack_layout.py" "$ELF"
python3 "$BASE/runtime18/validation/verify-hwram-budget.py" audit/runtime18-build/wrestlefest-runtime18.map --json "$BASE/hwram-budget.json"
sha256sum "$ELF"
echo CONTROL_COLLISION_SH2_LINK_PASS
