#!/bin/sh
set -eu
export PYTHONDONTWRITEBYTECODE=1
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
CC=${CC:-cc}
$CC -std=c11 -Wall -Wextra -Werror -pedantic \
  -I"$ROOT/src/firstmatch" \
  "$ROOT/src/firstmatch/wp12_firstmatch_presentation.c" \
  "$ROOT/tests/test_wp12_firstmatch.c" \
  -o /tmp/wp12_r03_host
/tmp/wp12_r03_host
python3 "$ROOT/tests/audit_r03_rom.py" "${WWFWFEST_ZIP:-/mnt/data/wwfwfest.zip}"
python3 "$ROOT/tests/audit_r03_assets.py" "$ROOT" "${WWFWFEST_ZIP:-/mnt/data/wwfwfest.zip}"
python3 "$ROOT/tests/ownership_audit.py" "$ROOT"
python3 "$ROOT/tests/audit_r03_artifacts.py" "$ROOT"
python3 "$ROOT/tests/audit_r03_repro.py" "$ROOT" "${WWFWFEST_ZIP:-/mnt/data/wwfwfest.zip}"
