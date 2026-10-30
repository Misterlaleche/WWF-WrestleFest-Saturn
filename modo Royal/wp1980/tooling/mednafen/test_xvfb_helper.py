#!/usr/bin/env python3
"""Start and stop one isolated Xvfb to verify the supplied keyboard helper.

No emulator is launched. The preload is scoped to the Xvfb process environment;
the supplied X11 query clients receive a separate environment without it.
"""

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import select
import subprocess
import time


HOST = Path('/workspace/scratch/1e822aeaeac7/wp1979/tooling/host')
HERE = Path(__file__).resolve().parent


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    if os.environ.get('LD_PRELOAD'):
        raise RuntimeError('Refusing to replace an existing global LD_PRELOAD')
    helper = (HERE / 'libxvfb_xkb_path.so').resolve(strict=True)
    xvfb = HOST / 'usr/bin/Xvfb'
    env = os.environ.copy()
    env['LD_LIBRARY_PATH'] = str(HOST / 'usr/lib/x86_64-linux-gnu') + ':' + str(HOST / 'lib/x86_64-linux-gnu')
    env['LD_PRELOAD'] = str(helper)
    read_fd, write_fd = os.pipe()
    command = [str(xvfb), '-displayfd', str(write_fd), '-screen', '0', '320x240x24',
               '-nolisten', 'tcp', '-ac']
    report = dict(status='STARTING', scope='Owned Xvfb host-tool smoke only; no emulator',
                  command=command,
                  xvfb_environment={name: env[name] for name in ('LD_LIBRARY_PATH', 'LD_PRELOAD')},
                  files={str(path): sha(path) for path in (xvfb, HOST / 'usr/bin/xkbcomp', helper, HERE / 'xvfb_xkb_path.c')})
    process = None
    socket = None
    started = time.monotonic()
    try:
        with (output / 'xvfb.log').open('wb') as log:
            process = subprocess.Popen(command, env=env, pass_fds=(write_fd,), stdout=log, stderr=subprocess.STDOUT)
            os.close(write_fd);write_fd = -1
            if not select.select([read_fd], [], [], 10)[0]:
                raise RuntimeError('Xvfb did not publish its own display within10 seconds')
            number = os.read(read_fd, 64).decode().strip()
            if not number.isdecimal() or process.poll() is not None:
                raise RuntimeError('Xvfb failed before activating its display/keyboard')
            display = ':' + number
            socket = Path('/tmp/.X11-unix/X' + number)
            report.update(display=display, socket_path=str(socket), socket_exists_running=socket.exists())
            clients = dict(env)
            del clients['LD_PRELOAD']
            clients['DISPLAY'] = display
            report['client_environment'] = {name: clients.get(name) for name in ('DISPLAY', 'LD_LIBRARY_PATH', 'LD_PRELOAD')}
            queries = [
                ('geometry', [str(HOST / 'usr/bin/xdotool'), 'getdisplaygeometry']),
                ('keyboard_names', [str(HOST / 'usr/bin/setxkbmap'), '-query']),
                ('keyboard_map', [str(HOST / 'usr/bin/xkbcomp'), '-xkb', display, '-']),
            ]
            report['queries'] = []
            for name, query in queries:
                result = subprocess.run(query, env=clients, capture_output=True, text=True, timeout=10)
                (output / (name + '.stdout.txt')).write_text(result.stdout)
                (output / (name + '.stderr.txt')).write_text(result.stderr)
                report['queries'].append(dict(name=name, command=query, returncode=result.returncode,
                                               stdout_bytes=len(result.stdout.encode()), stderr_bytes=len(result.stderr.encode())))
                if result.returncode:
                    raise RuntimeError(name + ' query failed')
                if name == 'geometry' and result.stdout.strip() != '320 240':
                    raise RuntimeError('Unexpected isolated display geometry')
                if name == 'keyboard_map':
                    keycodes = re.findall(r'^\s*<[^>]+>\s*=\s*\d+;', result.stdout, re.M)
                    report['keyboard_keycode_definitions'] = len(keycodes)
                    report['keyboard_map_has_symbols'] = 'xkb_symbols' in result.stdout
                    if len(keycodes) < 80 or not report['keyboard_map_has_symbols']:
                        raise RuntimeError('Compiled keyboard map is incomplete')
            report['status'] = 'PASS'
    except BaseException as exc:
        report.update(status='FAIL', error=repr(exc))
        raise
    finally:
        if write_fd >= 0:
            os.close(write_fd)
        os.close(read_fd)
        if process is not None:
            if process.poll() is None:
                process.terminate()
                try:
                    process.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    process.kill();process.wait(timeout=5)
            report['exit_code_after_owned_cleanup'] = process.returncode
        report['socket_exists_after_cleanup'] = socket.exists() if socket else None
        report['wall_seconds'] = time.monotonic() - started
        (output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
