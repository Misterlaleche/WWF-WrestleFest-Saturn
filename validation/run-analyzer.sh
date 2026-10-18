#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
CC=${CC:-gcc}
cd "$ROOT/src/runtime"
TMP_OBJ=$(mktemp "${TMPDIR:-/tmp}/wf-analyzer.XXXXXX")
trap 'rm -f "$TMP_OBJ"' EXIT HUP INT TERM
"$CC" --version
for source in src/*.c generated/*.c; do
  printf 'ANALYZE %s\n' "$source"
  "$CC" -std=c11 -Wall -Wextra -Werror -fanalyzer -O0 -Iinclude -c "$source" -o "$TMP_OBJ"
done
