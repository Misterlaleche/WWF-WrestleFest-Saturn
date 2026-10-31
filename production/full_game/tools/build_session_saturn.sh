#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
: "${YAUL_INSTALL_ROOT:?Supplied SDK required}"
python3 tools/check_sources.py
python3 tools/build_session.py --sh2 --entry wf_r18_frontend_driver_on_vblank --entry wf_r18_frontend_driver_on_raster --out build/session-core
python3 - <<'PY'
import shutil
from pathlib import Path
native=Path('../../modo Royal/wp1980/work/hwfix01/native')
shutil.copytree(native/'include','build/session-include',dirs_exist_ok=True)
Path('build/frontend').mkdir(exist_ok=True)
for name in ('wf_r18_cdfs','wp1979_cd_read'):
 for sub,ext in (('src','c'),('include','h')):shutil.copy2(native/sub/(name+'.'+ext),Path('build/frontend')/(name+'.'+ext))
PY
export YAUL_ARCH_SH_PREFIX=sh2eb-elf YAUL_PROG_SH_PREFIX=sh2eb-elf YAUL_ARCH_M68K_PREFIX=m68keb-elf YAUL_OPTION_MALLOC_IMPL=tlsf
export MAKE_ISO_XORRISOFS="${MAKE_ISO_XORRISOFS:-$(command -v xorrisofs)}" NOCOLOR=1 SILENT=1
make -f Makefile.session -B -j4
if [[ -n "${WF_DISC_OUTPUT:-}" ]]; then
 python3 vendor/frontend_test03/iso_to_bin.py build/session-disc/wrestlefest-session06.iso "$WF_DISC_OUTPUT"
fi
