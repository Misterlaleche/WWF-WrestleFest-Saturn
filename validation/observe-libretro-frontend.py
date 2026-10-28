#!/usr/bin/env python3
"""Run a BIN/CUE in the installed Beetle Saturn core and sample early video."""
import argparse
import ctypes as C
import hashlib
import json
from pathlib import Path

from PIL import Image

p = argparse.ArgumentParser()
p.add_argument("--core", type=Path, required=True)
p.add_argument("--cue", type=Path, required=True)
p.add_argument("--bios-dir", type=Path, required=True)
p.add_argument("--out", type=Path, required=True)
p.add_argument("--frames", type=int, default=2400)
a = p.parse_args()
a.out.mkdir(parents=True, exist_ok=True)


class Variable(C.Structure):
    _fields_ = [("key", C.c_char_p), ("value", C.c_char_p)]


class Game(C.Structure):
    _fields_ = [("path", C.c_char_p), ("data", C.c_void_p), ("size", C.c_size_t), ("meta", C.c_char_p)]


ENV = C.CFUNCTYPE(C.c_bool, C.c_uint, C.c_void_p)
VIDEO = C.CFUNCTYPE(None, C.c_void_p, C.c_uint, C.c_uint, C.c_size_t)
AUDIO = C.CFUNCTYPE(None, C.c_int16, C.c_int16)
BATCH = C.CFUNCTYPE(C.c_size_t, C.c_void_p, C.c_size_t)
POLL = C.CFUNCTYPE(None)
INPUT = C.CFUNCTYPE(C.c_int16, C.c_uint, C.c_uint, C.c_uint, C.c_uint)
system = str(a.bios_dir.resolve()).encode()
save = str(a.out.resolve()).encode()
pixel = 1
latest = None
options = {b"beetle_saturn_region": b"North America", b"beetle_saturn_cart": b"None"}


@ENV
def environment(cmd, data):
    global pixel
    if cmd in (9, 31):
        C.cast(data, C.POINTER(C.c_char_p))[0] = system if cmd == 9 else save
        return True
    if cmd == 3:
        C.cast(data, C.POINTER(C.c_bool))[0] = True
        return True
    if cmd == 10:
        pixel = C.cast(data, C.POINTER(C.c_uint))[0]
        return pixel in (1, 2)
    if cmd == 15:
        var = C.cast(data, C.POINTER(Variable)).contents
        if var.key in options:
            var.value = options[var.key]
            return True
        return False
    if cmd == 17:
        C.cast(data, C.POINTER(C.c_bool))[0] = False
        return True
    if cmd == 52:
        C.cast(data, C.POINTER(C.c_uint))[0] = 0
        return True
    return False


@VIDEO
def video(data, width, height, pitch):
    global latest
    if data and data != C.c_void_p(-1).value:
        latest = (C.string_at(data, pitch * height), width, height, pitch, pixel)


@AUDIO
def audio(left, right):
    pass


@BATCH
def batch(data, frames):
    return frames


@POLL
def poll():
    pass


@INPUT
def input_state(port, device, index, button):
    return 0


core = C.CDLL(str(a.core.resolve()))
for name, typ, callback in (
    ("environment", ENV, environment), ("video_refresh", VIDEO, video),
    ("audio_sample", AUDIO, audio), ("audio_sample_batch", BATCH, batch),
    ("input_poll", POLL, poll), ("input_state", INPUT, input_state),
):
    fn = getattr(core, "retro_set_" + name)
    fn.argtypes = [typ]
    fn(callback)
core.retro_init()
core.retro_set_controller_port_device.argtypes = [C.c_uint, C.c_uint]
core.retro_set_controller_port_device(0, 1)
core.retro_load_game.argtypes = [C.POINTER(Game)]
core.retro_load_game.restype = C.c_bool
game = Game(str(a.cue.resolve()).encode(), None, 0, None)
assert core.retro_load_game(C.byref(game)), "Beetle Saturn rejected CUE"
checkpoints = set(range(200, a.frames + 1, 400)) | {400, 800, 1200, 1600, 2000, 2400}
observed = []
try:
    for frame in range(1, a.frames + 1):
        core.retro_run()
        if frame in checkpoints and latest:
            raw, width, height, pitch, fmt = latest
            if fmt == 1:
                picture = Image.frombytes("RGB", (width, height), raw, "raw", "BGRX", pitch)
            else:
                picture = Image.frombytes("RGB", (width, height), raw, "raw", "BGR;16", pitch)
            output = a.out / f"frame-{frame:04}.png"
            picture.save(output)
            observed.append({"frame": frame, "sha256": hashlib.sha256(picture.tobytes()).hexdigest(), "size": [width, height]})
            print(json.dumps(observed[-1]), flush=True)
finally:
    core.retro_unload_game()
    core.retro_deinit()
    (a.out / "report.json").write_text(json.dumps({"cue": str(a.cue), "frames": a.frames, "observed": observed, "physical_test": False}, indent=2) + "\n")
