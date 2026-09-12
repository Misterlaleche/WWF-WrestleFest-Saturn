#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
CC=${CC:-cc}
$CC -std=c11 -Wall -Wextra -Werror -pedantic \
  -I"$ROOT/src/runtime" -I"$ROOT/src/renderer" \
  "$ROOT/src/runtime/wp12_romexact.c" \
  "$ROOT/src/renderer/wp12_renderer_contract.c" \
  "$ROOT/tests/test_wp12_host.c" -o /tmp/wp12_r02_host
/tmp/wp12_r02_host
python3 "$ROOT/tests/audit_rom.py" "${WWFWFEST_ZIP:-/mnt/data/wwfwfest.zip}"
python3 "$ROOT/tests/audit_source_tables.py" "$ROOT" "${WWFWFEST_ZIP:-/mnt/data/wwfwfest.zip}"
python3 "$ROOT/tests/ownership_audit.py" "$ROOT"
