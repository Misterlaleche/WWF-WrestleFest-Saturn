#!/bin/sh
# SDK boot/IP -> ISO -> WP33 exact raw MODE1 BIN/CUE. No audio tracks.
set -eu
export PYTHONDONTWRITEBYTECODE=1
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
SDK=$(CDPATH= cd -- "${1:?Pass extracted sh2eb-elf SDK directory}" && pwd)
DOCKER=${DOCKER:-docker}
PYTHON=${PYTHON:-python3}
IMAGE=ijacquez/yaul@sha256:b6d272204767cd0a734139dd3a2ab298ef5912fb7b7224b78e6d9fac1a7825a6
RESULTS="$ROOT/validation/results/runtime18"
case "${R18_REPLAY_TEST:-}" in
  noinput) RESULTS="$RESULTS/replay-noinput" ;;
  '') ;;
  *) RESULTS="$RESULTS/replay" ;;
esac
R18_ENTRY=src/wf_saturn_entry.c
R18_PRESENT=src/wf_r18_present.c
if [ -n "${R18_CAPTURE_STOP:-}" ]; then
  case "$R18_CAPTURE_STOP" in *[!0-9]*) echo 'Capture stop must be a positive frame' >&2; exit 1;; esac
  test "$R18_CAPTURE_STOP" -gt 0 || { echo 'Capture stop must be a positive frame' >&2; exit 1; }
  test "${R18_PROFILE_TEST:-}" != 1 || { echo 'Capture and profile are separate test entries' >&2; exit 1; }
  RESULTS="$ROOT/validation/results/runtime18/capture-noinput-$R18_CAPTURE_STOP"
  R18_ENTRY=tests/r18/hud_capture_entry.c
  R18_REPLAY_TEST=noinput
  R18_PROFILE_STOP="$R18_CAPTURE_STOP"
fi
if [ "${R18_PROFILE_TEST:-}" = 1 ]; then
  RESULTS="$ROOT/validation/results/runtime18/profile-controlled"
  if [ -n "${R18_PROFILE_START:-}" ] && [ -n "${R18_PROFILE_STOP:-}" ]; then
    RESULTS="$ROOT/validation/results/runtime18/profile-${R18_PROFILE_START}-${R18_PROFILE_STOP}"
  fi
  R18_ENTRY=tests/r18/profile_entry.c
  R18_REPLAY_TEST=1
fi
if [ "${R18_PROFILE_DETAIL:-}" = 1 ]; then
  test "${R18_PROFILE_TEST:-}" = 1 || { echo 'Detailed profiling requires R18_PROFILE_TEST=1' >&2; exit 1; }
  R18_PRESENT=tests/r18/profile_present.c
fi
if [ "${R18_CAPTURE_RESULT:-}" = 1 ]; then
  test -z "${R18_CAPTURE_STOP:-}" && test "${R18_PROFILE_TEST:-}" != 1 || { echo 'Result capture cannot be combined with a frame stop or profiler' >&2; exit 1; }
  R18_REPLAY_TEST=${R18_REPLAY_TEST:-noinput}
  RESULTS="$ROOT/validation/results/runtime18/capture-result-$R18_REPLAY_TEST"
  R18_ENTRY=tests/r18/result_capture_entry.c
fi
mkdir -p "$RESULTS"
"$PYTHON" - "$ROOT/src/runtime/audit/runtime18-cd/FONT8.BIN" <<'PYFONT'
import hashlib,sys
from pathlib import Path
p=Path(sys.argv[1])
if not p.exists() or hashlib.sha256(p.read_bytes()).hexdigest()!='a3045c6193d23c334c6af592cc1dfee029dc12eec810d3e2ce353df69580391a':
    raise SystemExit('Missing/invalid canonical FONT8.BIN: generate with src/runtime/tools/build_r18_font.py wwfwfest.zip OUTPUT')
PYFONT
"$PYTHON" - "$ROOT/src/runtime/audit/runtime18-cd/R18MAP2.BIN" <<'PYRINGSIDE'
import hashlib,sys
from pathlib import Path
p=Path(sys.argv[1])
if not p.exists() or hashlib.sha256(p.read_bytes()).hexdigest()!='979b622c418307f90b1ce98524db7bade230adf399bd13e6fc3fe4adc699acf0':
    raise SystemExit('Missing/invalid R18MAP2.BIN: generate build_r18_graphics.py --mode 2 --palette-mode 2; copy only generated R18RING.BIN as R18MAP2.BIN')
PYRINGSIDE
"$PYTHON" "$ROOT/src/runtime/tools/build_r18_hot_sprites.py" \
  "$ROOT/src/runtime/validation/runtime18-hot-tiles.txt" \
  "$ROOT/src/runtime/audit/runtime18-cd/SPRITES.PAK" \
  "$ROOT/src/runtime/audit/runtime18-cd/HOTSPR.BIN"
"$DOCKER" run --rm --entrypoint /bin/bash -v "$ROOT:/workspace" -v "$SDK:/sdk:ro" -w /workspace/src/runtime -e YAUL_INSTALL_ROOT=/sdk -e YAUL_ARCH_SH_PREFIX=sh2eb-elf -e YAUL_ARCH_M68K_PREFIX=m68keb-elf -e YAUL_OPTION_MALLOC_IMPL=tlsf -e R18_REPLAY_TEST="${R18_REPLAY_TEST:-}" -e R18_ENTRY="$R18_ENTRY" -e R18_PRESENT="$R18_PRESENT" -e R18_PROFILE_START="${R18_PROFILE_START:-}" -e R18_PROFILE_STOP="${R18_PROFILE_STOP:-}" -e SILENT=1 -e NOCOLOR=1 -e MAKE_ISO_XORRISOFS=/usr/bin/xorrisofs "$IMAGE" -c '
set -euo pipefail
export PATH=/sdk/bin:$PATH
/sdk/bin/sh2eb-elf-gcc --version
make -f Makefile.saturn-disc clean
make -f Makefile.saturn-disc -j4
elf=audit/runtime18-build/wrestlefest-runtime18.elf
/sdk/bin/sh2eb-elf-nm -u "$elf" > audit/runtime18-build/unresolved.txt
test ! -s audit/runtime18-build/unresolved.txt
/sdk/bin/sh2eb-elf-size "$elf"
sha256sum "$elf" audit/runtime18-build/IP.BIN
' > "$RESULTS/build.log" 2>&1 || { cat "$RESULTS/build.log"; exit 1; }
cp "$ROOT/src/runtime/audit/runtime18-build/unresolved.txt" "$RESULTS/unresolved.txt"
cp "$ROOT/src/runtime/audit/runtime18-cd/A.BIN" "$RESULTS/A.BIN"
cp "$ROOT/src/runtime/audit/runtime18-build/wrestlefest-runtime18.elf" "$RESULTS/wrestlefest-runtime18.elf"
cp "$ROOT/src/runtime/audit/runtime18-build/wrestlefest-runtime18.map" "$RESULTS/wrestlefest-runtime18.map"
cat "$RESULTS/build.log"
ISO="$ROOT/src/runtime/audit/runtime18-disc/wrestlefest-runtime18.iso"
# Replace only the generated pair, retaining other validation evidence.
rm -f "$RESULTS/wrestlefest-runtime18.bin" "$RESULTS/wrestlefest-runtime18.cue"
"$PYTHON" "$ROOT/validation/iso_to_bin_exact.py" "$ISO" "$RESULTS" > "$RESULTS/conversion.json"
"$PYTHON" "$ROOT/validation/verify_disc_exact.py" --root "$ROOT/src/runtime" --iso "$ISO" --bin "$RESULTS/wrestlefest-runtime18.bin" --cue "$RESULTS/wrestlefest-runtime18.cue" --json "$RESULTS/disc-validation.json"
