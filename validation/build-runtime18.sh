#!/bin/sh
# SDK boot/IP -> ISO -> WP33 exact raw MODE1 BIN/CUE. No audio tracks.
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
SDK=$(CDPATH= cd -- "${1:?Pass extracted sh2eb-elf SDK directory}" && pwd)
DOCKER=${DOCKER:-docker}
PYTHON=${PYTHON:-python3}
IMAGE=ijacquez/yaul@sha256:b6d272204767cd0a734139dd3a2ab298ef5912fb7b7224b78e6d9fac1a7825a6
RESULTS="$ROOT/validation/results/runtime18"
mkdir -p "$RESULTS"
"$PYTHON" "$ROOT/src/runtime/tools/build_r18_hot_sprites.py" \
  "$ROOT/src/runtime/validation/runtime18-hot-tiles.txt" \
  "$ROOT/src/runtime/audit/runtime18-cd/SPRITES.PAK" \
  "$ROOT/src/runtime/audit/runtime18-cd/HOTSPR.BIN"
"$DOCKER" run --rm --entrypoint /bin/bash -v "$ROOT:/workspace" -v "$SDK:/sdk:ro" -w /workspace/src/runtime -e YAUL_INSTALL_ROOT=/sdk -e YAUL_ARCH_SH_PREFIX=sh2eb-elf -e YAUL_ARCH_M68K_PREFIX=m68keb-elf -e YAUL_OPTION_MALLOC_IMPL=tlsf -e SILENT=1 -e NOCOLOR=1 -e MAKE_ISO_XORRISOFS=/usr/bin/xorrisofs "$IMAGE" -c '
set -euo pipefail
export PATH=/sdk/bin:$PATH
/sdk/bin/sh2eb-elf-gcc --version
make -f Makefile.saturn-disc clean
make -f Makefile.saturn-disc -j4
elf=audit/runtime18-build/wrestlefest-runtime18.elf
/sdk/bin/sh2eb-elf-nm -u "$elf" > /workspace/validation/results/runtime18/unresolved.txt
test ! -s /workspace/validation/results/runtime18/unresolved.txt
/sdk/bin/sh2eb-elf-size "$elf"
sha256sum "$elf" audit/runtime18-build/IP.BIN
' > "$RESULTS/build.log" 2>&1 || { cat "$RESULTS/build.log"; exit 1; }
cat "$RESULTS/build.log"
ISO="$ROOT/src/runtime/audit/runtime18-disc/wrestlefest-runtime18.iso"
# Replace only the generated pair, retaining other validation evidence.
rm -f "$RESULTS/wrestlefest-runtime18.bin" "$RESULTS/wrestlefest-runtime18.cue"
"$PYTHON" "$ROOT/validation/iso_to_bin_exact.py" "$ISO" "$RESULTS" > "$RESULTS/conversion.json"
"$PYTHON" "$ROOT/validation/verify_disc_exact.py" --root "$ROOT/src/runtime" --iso "$ISO" --bin "$RESULTS/wrestlefest-runtime18.bin" --cue "$RESULTS/wrestlefest-runtime18.cue" --json "$RESULTS/disc-validation.json"
