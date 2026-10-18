#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
SDK=${1:?Pass the absolute path to the extracted sh2eb-elf directory}
DOCKER=${DOCKER:-docker}
IMAGE=ijacquez/yaul@sha256:b6d272204767cd0a734139dd3a2ab298ef5912fb7b7224b78e6d9fac1a7825a6
mkdir -p "$ROOT/validation/results/sh2"
"$DOCKER" run --rm --entrypoint /bin/bash -v "$ROOT:/workspace" -v "$SDK:/sdk:ro" -w /workspace/src/runtime -e YAUL_INSTALL_ROOT=/sdk -e SILENT=1 -e NOCOLOR=1 "$IMAGE" -c '
set -euo pipefail
/sdk/bin/sh2eb-elf-gcc --version
make -f Makefile.saturn-link clean
make -f Makefile.saturn-link -j4
elf=audit/saturn-link-build/gameplay-runtime-r17-link.elf
/sdk/bin/sh2eb-elf-nm -u "$elf" > /workspace/validation/results/sh2/unresolved.txt
test ! -s /workspace/validation/results/sh2/unresolved.txt
sha256sum "$elf"
' > "$ROOT/validation/results/sh2/build.log" 2>&1
cat "$ROOT/validation/results/sh2/build.log"
