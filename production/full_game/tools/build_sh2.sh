#!/usr/bin/env bash
# Compile the shared integration core. This is NOT a bootable game/disc.
set -euo pipefail
cd "$(dirname "$0")/.."
: "${YAUL_INSTALL_ROOT:?Set the supplied Yaul SDK root}"
compiler="$YAUL_INSTALL_ROOT/bin/sh2eb-elf-gcc"
archiver="$YAUL_INSTALL_ROOT/bin/sh2eb-elf-ar"
linker="$YAUL_INSTALL_ROOT/bin/sh2eb-elf-ld"
symbols="$YAUL_INSTALL_ROOT/bin/sh2eb-elf-nm"
test "$("$compiler" -dumpfullversion)" = 13.2.0
mkdir -p build/sh2
sources=(src/wf_arcade_clock.c src/wf_prematch.c '../../Entrada Ring/source/wp01_scene.c' '../../Escena Presentadora/source/scene/src/wp02_scene.c')
flags=(-std=c11 -O2 -Wall -Wextra -Werror -m2 -mb -ffreestanding -ffunction-sections -fdata-sections "-I$YAUL_INSTALL_ROOT/sh2eb-elf/include" -Isrc '-I../../Entrada Ring/source' '-I../../Escena Presentadora/source/scene/src')
objects=()
for source in "${sources[@]}"; do
    object="build/sh2/$(basename "${source%.c}").o"
    "$compiler" "${flags[@]}" -c "$source" -o "$object"
    objects+=("$object")
done
# Resolve actual SH-2 compiler helpers using the matching SDK multilib.
libgcc="$("$compiler" -m2 -mb -print-libgcc-file-name)"
test -f "$libgcc"
"$linker" -r "${objects[@]}" "$libgcc" -o build/sh2/wf_full_game_core.o
# Only this generated build artifact is replaced; checkpoints live elsewhere.
rm -f build/sh2/libwf_full_game_core.a
"$archiver" rcsD build/sh2/libwf_full_game_core.a build/sh2/wf_full_game_core.o
"$symbols" -u build/sh2/wf_full_game_core.o > build/sh2/unresolved.txt
python3 - "$compiler" <<'PY'
import hashlib,json,subprocess,sys
from pathlib import Path
root=Path('build/sh2')
undefined={line.split()[-1] for line in (root/'unresolved.txt').read_text().splitlines() if line.strip()}
unexpected=undefined-{'_memcpy','_memset','memcpy','memset'}
if unexpected: raise SystemExit('Unexpected unresolved symbols: '+repr(unexpected))
files={str(p):{'bytes':p.stat().st_size,'sha256':hashlib.sha256(p.read_bytes()).hexdigest()} for p in sorted(root.iterdir()) if p.suffix in ('.o','.a')}
report={'status':'SH2_CORE_COMPILED_AND_RELOCATABLY_LINKED','bootable_disc':False,
        'compiler':subprocess.check_output([sys.argv[1],'--version'],text=True).splitlines()[0],
        'flags':'-std=c11 -O2 -Wall -Wextra -Werror -m2 -mb -ffreestanding -ffunction-sections -fdata-sections; SDK include: sh2eb-elf/include',
        'audio_backend_linked':False,'unresolved_libc':sorted(undefined),'files':files}
(root/'build.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
PY
