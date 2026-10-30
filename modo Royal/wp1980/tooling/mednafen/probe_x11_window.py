#!/usr/bin/env python3
"""Three-second owned Mednafen/Xvfb window probe; no memory writes or movie."""
import argparse
import ctypes as C
import json
import os
from pathlib import Path
import select
import subprocess
import sys
import time

HERE = Path(__file__).resolve().parent
PROJECT = HERE.parents[1]
HOST = Path('/workspace/scratch/1e822aeaeac7/wp1979/tooling/host')
RUNTIME = Path('/workspace/scratch/8e35fd0cc989/wp02_r02/deps/sysroot')


class Attributes(C.Structure):
    _fields_ = [('x', C.c_int), ('y', C.c_int), ('width', C.c_int), ('height', C.c_int),
                ('border_width', C.c_int), ('depth', C.c_int), ('visual', C.c_void_p),
                ('root', C.c_ulong), ('class_', C.c_int), ('bit_gravity', C.c_int),
                ('win_gravity', C.c_int), ('backing_store', C.c_int),
                ('backing_planes', C.c_ulong), ('backing_pixel', C.c_ulong),
                ('save_under', C.c_int), ('colormap', C.c_ulong), ('map_installed', C.c_int),
                ('map_state', C.c_int), ('all_event_masks', C.c_long), ('your_event_mask', C.c_long),
                ('do_not_propagate_mask', C.c_long), ('override_redirect', C.c_int), ('screen', C.c_void_p)]


class ClassHint(C.Structure):
    _fields_ = [('res_name', C.c_void_p), ('res_class', C.c_void_p)]


def x_tree(display):
    x = C.CDLL('libX11.so.6')
    x.XOpenDisplay.argtypes = [C.c_char_p];x.XOpenDisplay.restype = C.c_void_p
    x.XCloseDisplay.argtypes = [C.c_void_p]
    x.XDefaultRootWindow.argtypes = [C.c_void_p];x.XDefaultRootWindow.restype = C.c_ulong
    x.XQueryTree.argtypes = [C.c_void_p, C.c_ulong, C.POINTER(C.c_ulong), C.POINTER(C.c_ulong), C.POINTER(C.POINTER(C.c_ulong)), C.POINTER(C.c_uint)]
    x.XGetWindowAttributes.argtypes = [C.c_void_p, C.c_ulong, C.POINTER(Attributes)]
    x.XFetchName.argtypes = [C.c_void_p, C.c_ulong, C.POINTER(C.c_void_p)]
    x.XGetClassHint.argtypes = [C.c_void_p, C.c_ulong, C.POINTER(ClassHint)]
    x.XFree.argtypes = [C.c_void_p]
    d = x.XOpenDisplay(display.encode())
    if not d:
        raise RuntimeError('XOpenDisplay failed for owned display')
    result = []
    def visit(window, depth):
        if len(result) >= 64 or depth > 5:
            return
        attrs = Attributes();name = C.c_void_p();hint = ClassHint()
        if not x.XGetWindowAttributes(d, window, C.byref(attrs)):
            return
        item = dict(window=window, depth=depth, x=attrs.x, y=attrs.y, width=attrs.width,
                    height=attrs.height, map_state=attrs.map_state, override_redirect=attrs.override_redirect)
        if x.XFetchName(d, window, C.byref(name)) and name.value:
            item['name'] = C.string_at(name).decode(errors='replace');x.XFree(name)
        if x.XGetClassHint(d, window, C.byref(hint)):
            for key, pointer in [('res_name', hint.res_name), ('res_class', hint.res_class)]:
                if pointer:
                    item[key] = C.string_at(pointer).decode(errors='replace');x.XFree(pointer)
        result.append(item)
        root = C.c_ulong();parent = C.c_ulong();children = C.POINTER(C.c_ulong)();count = C.c_uint()
        if x.XQueryTree(d, window, C.byref(root), C.byref(parent), C.byref(children), C.byref(count)):
            for i in range(count.value):visit(children[i], depth + 1)
            if children:x.XFree(children)
    try:visit(x.XDefaultRootWindow(d), 0)
    finally:x.XCloseDisplay(d)
    return result


def environment(prefix):
    env = os.environ.copy()
    env['LD_LIBRARY_PATH'] = str(prefix / 'usr/lib/x86_64-linux-gnu') + ':' + str(prefix / 'lib/x86_64-linux-gnu')
    return env


def stop(process):
    if process and process.poll() is None:
        process.terminate()
        try:process.wait(timeout=5)
        except subprocess.TimeoutExpired:process.kill();process.wait(timeout=5)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    output = args.output.resolve();output.mkdir(parents=True, exist_ok=False)
    (output / 'home').mkdir();(output / 'xdg').mkdir(mode=0o700)
    donor = PROJECT / 'research/build/mednafen/controller-smoke-02/command.json'
    original = json.loads(donor.read_text())['emulator']
    command = [];i = 0
    while i < len(original) - 1:
        if original[i] == '-qtrecord':i += 2;continue
        command.append(original[i]);i += 1
    command.append(str(PROJECT / 'research/build/mednafen/final-cart-full/disc/wp1980_royal_4mb.cue'))
    xserver = emulator = None
    rf, wf = os.pipe()
    report = dict(scope='Owned three-second X11 diagnostic, not final game validation', emulator_command=command)
    with (output / 'xvfb.log').open('wb') as xlog, (output / 'emulator.log').open('wb') as elog:
        try:
            xenv = environment(HOST)
            if xenv.get('LD_PRELOAD'):raise RuntimeError('Global preload unexpectedly present')
            xenv['LD_PRELOAD'] = str(HERE / 'libxvfb_xkb_path.so')
            xcommand = [str(HOST / 'usr/bin/Xvfb'), '-displayfd', str(wf), '-screen', '0', '1280x720x24', '-nolisten', 'tcp', '-ac']
            xserver = subprocess.Popen(xcommand, env=xenv, pass_fds=(wf,), stdout=xlog, stderr=subprocess.STDOUT)
            os.close(wf);wf = -1
            if not select.select([rf], [], [], 10)[0]:raise RuntimeError('No Xvfb display')
            number = os.read(rf, 64).decode().strip()
            if not number.isdecimal():raise RuntimeError('Invalid Xvfb display')
            display = ':' + number
            env = environment(RUNTIME)
            if env.get('LD_PRELOAD'):raise RuntimeError('Mednafen must not receive a preload')
            env.update(DISPLAY=display, MEDNAFEN_HOME=str(output / 'home'), XDG_RUNTIME_DIR=str(output / 'xdg'),
                       SDL_VIDEODRIVER='x11', SDL_AUDIODRIVER='dummy')
            identity_path = output / 'launched-process.json'
            emulator = subprocess.Popen([sys.executable, str(HERE / 'launch_identity.py'), str(identity_path), *command],
                                        env=env, stdout=elog, stderr=subprocess.STDOUT)
            report['xvfb_command'] = xcommand
            report['environment'] = {name: env.get(name) for name in ('DISPLAY', 'SDL_VIDEODRIVER', 'SDL_AUDIODRIVER', 'LD_LIBRARY_PATH', 'LD_PRELOAD')}
            deadline = time.monotonic() + 3
            while time.monotonic() < deadline:
                if emulator.poll() is not None:raise RuntimeError('Mednafen exited before window probe')
                time.sleep(0.1)
            identity = json.loads(identity_path.read_text())
            proc_pid = identity['proc_pid']
            actual_env = {}
            for entry in Path(f'/proc/{proc_pid}/environ').read_bytes().split(b'\0'):
                key, _, value = entry.partition(b'=')
                if key.decode(errors='ignore') in report['environment']:
                    actual_env[key.decode()] = value.decode(errors='replace')
            report['actual_child_environment'] = actual_env
            maps = Path(f'/proc/{proc_pid}/maps').read_text()
            (output / 'mednafen-maps.txt').write_text(maps)
            report['sdl_and_x11_libraries'] = sorted({line.split()[-1] for line in maps.splitlines()
                if any(token in line for token in ('libSDL', 'libX11', 'libxcb', 'libXext', 'libxvfb_xkb'))})
            client_env = environment(HOST);client_env['DISPLAY'] = display
            queries = [['search', '--onlyvisible', '--name', '.'], ['search', '--name', '.'],
                       ['search', '--onlyvisible', '--class', '.'], ['search', '--class', '.'],
                       ['search', '--onlyvisible', '--name', ''], ['getdisplaygeometry']]
            report['queries'] = []
            for query in queries:
                result = subprocess.run([str(HOST / 'usr/bin/xdotool'), *query], env=client_env,
                                        capture_output=True, text=True, timeout=5)
                report['queries'].append(dict(args=query, returncode=result.returncode,
                                               stdout=result.stdout, stderr=result.stderr))
            report['window_tree'] = x_tree(display)
            candidates = [item for item in report['window_tree'] if item['depth'] == 1 and
                          item['map_state'] == 2 and item['width'] > 100 and item['height'] > 100]
            report['candidate_windows'] = candidates
            if len(candidates) == 1:
                window = str(candidates[0]['window'])
                events = [['windowfocus', '--sync', window], ['key', 'F12']]
                report['keyboard_exit'] = []
                for query in events:
                    result = subprocess.run([str(HOST / 'usr/bin/xdotool'), *query], env=client_env,
                                            capture_output=True, text=True, timeout=5)
                    report['keyboard_exit'].append(dict(args=query, returncode=result.returncode,
                                                        stdout=result.stdout, stderr=result.stderr))
                try:emulator.wait(timeout=5)
                except subprocess.TimeoutExpired:pass
                report['exited_after_ordinary_F12'] = emulator.poll() == 0
            else:
                report['exited_after_ordinary_F12'] = False
            report['status'] = 'PROBE_COMPLETED'
        except BaseException as exc:
            report.update(status='FAIL', error=repr(exc));raise
        finally:
            stop(emulator);stop(xserver)
            if wf >= 0:os.close(wf)
            os.close(rf)
            report['emulator_exit_code'] = emulator.returncode if emulator else None
            report['xvfb_exit_code'] = xserver.returncode if xserver else None
            (output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':main()
