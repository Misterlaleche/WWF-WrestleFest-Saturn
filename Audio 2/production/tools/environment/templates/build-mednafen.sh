#!/usr/bin/env bash
set -euo pipefail
WF_BUILD_ENV_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$WF_BUILD_ENV_ROOT/env.sh"
mkdir -p "$WF_BUILD_ENV_ROOT/build/mednafen" "$WF_BUILD_ENV_ROOT/mednafen"
cd "$WF_BUILD_ENV_ROOT/build/mednafen"
"$WF_BUILD_ENV_ROOT/src/upstream/mednafen/configure" \
  --prefix="$WF_BUILD_ENV_ROOT/mednafen" \
  --disable-nls --disable-dependency-tracking \
  --disable-apple2 --disable-gb --disable-gba --disable-lynx \
  --disable-md --disable-nes --disable-ngp --disable-pce --disable-pce-fast \
  --disable-pcfx --disable-psx --disable-sasplay --disable-sms \
  --disable-snes --disable-snes-faust --disable-ssfplay --disable-vb --disable-wswan \
  --enable-ss --enable-debugger \
  --disable-alsa --disable-jack \
  --with-sdl-prefix="$WF_BUILD_ENV_ROOT/host/usr" \
  CPPFLAGS="-I$WF_BUILD_ENV_ROOT/host/usr/include -I$WF_BUILD_ENV_ROOT/host/usr/include/x86_64-linux-gnu" \
  LDFLAGS="-L$WF_BUILD_ENV_ROOT/host/usr/lib/x86_64-linux-gnu" \
  CFLAGS="-O2 -g" CXXFLAGS="-O2 -g" \
  > "$WF_BUILD_ENV_ROOT/logs/mednafen-configure.log" 2>&1
make -j"${WF_BUILD_JOBS:-8}" > "$WF_BUILD_ENV_ROOT/logs/mednafen-build.log" 2>&1
make install > "$WF_BUILD_ENV_ROOT/logs/mednafen-install.log" 2>&1
# Upstream Mednafen 1.32.1 returns 255 after successfully printing help.
python3 - "$WF_BUILD_ENV_ROOT" <<'PYVERSION'
from pathlib import Path
import os, subprocess, sys
root = Path(sys.argv[1])
(root / 'mednafen-home').mkdir(exist_ok=True)
env = dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy',
           MEDNAFEN_HOME=str(root / 'mednafen-home'))
result = subprocess.run([str(root / 'mednafen/bin/mednafen'), '-help'],
                        env=env, text=True, capture_output=True)
(root / 'logs/mednafen-help.log').write_text(result.stdout + result.stderr)
if result.returncode not in (0, 255) or not result.stdout.startswith('Starting Mednafen 1.32.1\n') or 'Usage:' not in result.stdout or '-qtrecord' not in result.stdout:
    raise SystemExit('Native Mednafen 1.32.1 version/help verification failed')
print('Verified standalone Mednafen 1.32.1 build.')
PYVERSION
