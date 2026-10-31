#!/usr/bin/env bash
# Run inside supplied Linux build environment. Assets are never added to Git.
set -euo pipefail
cd "$(dirname "$0")/.."
: "${YAUL_INSTALL_ROOT:?Set supplied SDK root}"
: "${WF_FRONTEND_PACKAGE:?Set restored TEST03 package root}"
test "$("$YAUL_INSTALL_ROOT/bin/sh2eb-elf-gcc" -dumpfullversion)" = 13.2.0
python3 tools/check_sources.py
python3 - <<'PY'
import hashlib,json,os,shutil
from pathlib import Path
records=json.loads(Path('vendor/frontend_test03/SOURCE.json').read_text())['files']
out=Path('build/frontend-assets');out.mkdir(parents=True,exist_ok=True)
for r in records:
 if not r['path'].startswith('REFERENCE-SOURCE/assets/'):continue
 p=Path(os.environ['WF_FRONTEND_PACKAGE'])/r['path'];b=p.read_bytes()
 if len(b)!=r['bytes'] or hashlib.sha256(b).hexdigest()!=r['sha256']:raise SystemExit('Wrong asset: '+str(p))
 (out/p.name).write_bytes(b)
PY
python3 tools/prepare_frontend.py
export YAUL_ARCH_SH_PREFIX=sh2eb-elf YAUL_PROG_SH_PREFIX=sh2eb-elf YAUL_ARCH_M68K_PREFIX=m68keb-elf YAUL_OPTION_MALLOC_IMPL=tlsf
export MAKE_ISO_XORRISOFS="${MAKE_ISO_XORRISOFS:-$(command -v xorrisofs)}" NOCOLOR=1 SILENT=1
# Force source rebuild: SDK lacks full compiler flag dependency tracking.
make -f Makefile.saturn -B -j4
python3 - <<'PY'
import hashlib,json,subprocess,os
from pathlib import Path
paths=[Path('build/saturn/wrestlefest-full-game'+x) for x in ('.elf','.bin','.map')]+[Path('build/disc/wrestlefest-full-game.iso')]
report={'scope':'Partial platform+MODE/ROSTER, not full game','compiler':subprocess.check_output([os.environ['YAUL_INSTALL_ROOT']+'/bin/sh2eb-elf-gcc','--version'],text=True).splitlines()[0],
 'cart':'same executable: SDK detection +16-point bank validation','audio_backend':False,
 'files':{str(p):{'bytes':p.stat().st_size,'sha256':hashlib.sha256(p.read_bytes()).hexdigest()} for p in paths}}
Path('build/saturn/build.json').write_text(json.dumps(report,indent=2)+'\n')
PY
# Freeze/convert only to a new external directory; caller supplies destination.
if [[ -n "${WF_DISC_OUTPUT:-}" ]]; then
 python3 vendor/frontend_test03/iso_to_bin.py build/disc/wrestlefest-full-game.iso "$WF_DISC_OUTPUT"
fi
