"""Run the emitted C DSP loader in Beetle; catch signed-ADRS ring aliasing.

The expected neighbors and interpolation come from a sample ramp and the
requested unsigned Q12 cursor, independently of the DSP assembler. A stopped
phase increment isolates each exact cursor while all four voice gates run.
No disc image is created or changed. This is an emulator test, not hardware QA.
"""
import argparse
import ctypes as C
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import struct
import subprocess
import sys

REPO = Path(__file__).resolve().parents[3]


def field(data, name, size, start=0):
    key = bytes([len(name)]) + name.encode()
    at = start
    while (at := data.find(key, at)) >= 0:
        length = int.from_bytes(data[at + len(key):at + len(key) + 4], 'little')
        if length == size:
            return at + len(key) + 4
        at += 1
    raise ValueError((name, size))


def signed24(value):
    value &= 0xffffff
    return value - 0x1000000 if value & 0x800000 else value


def sample(voice, cursor):
    return voice * 4000 + (cursor & 4095) * 2 - 12000


def main():
    p = argparse.ArgumentParser(description=__doc__)
    for name in ('core', 'cue', 'bios', 'out'):
        p.add_argument('--' + name, type=Path, required=True)
    p.add_argument('--vcvars', type=Path, default=Path('C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/Auxiliary/Build/vcvars64.bat'))
    p.add_argument('--assembler', type=Path, default=REPO / 'tools/audio/native_probe/build_dsp_mixer.py')
    p.add_argument('--loader-source', type=Path, default=REPO / 'src/audio/native/wf_scsp_dsp.c')
    p.add_argument('--baseline', type=Path, default=REPO / 'validation/local-20261028/audio-music-rootcause/nofl-constants-test/baseline.state')
    a = p.parse_args()
    a.out.mkdir(parents=True, exist_ok=True)
    spec = importlib.util.spec_from_file_location('assembler', a.assembler)
    assembler = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(assembler)
    assembler.generate(a.out)
    dll_path = a.out / 'dsp-loader.dll'
    command = ['cl', '/nologo', '/std:c11', '/O2', '/LD', '/I' + str(REPO / 'src/audio/native'),
               '/I' + str(a.out.resolve()), str(a.loader_source.resolve()), '/Fe' + str(dll_path.resolve()),
               '/Fo' + str((a.out / 'dsp-loader.obj').resolve()), '/link', '/EXPORT:wf_scsp_dsp_load']
    script = a.out / 'compile.cmd'
    script.write_text('@echo off\ncall "' + str(a.vcvars) + '" >nul\nif errorlevel 1 exit /b 1\n' +
                      ' '.join('"' + x + '"' for x in command) + '\nexit /b %errorlevel%\n')
    (a.out / 'tmp').mkdir(exist_ok=True)
    compile_env = dict(os.environ, TEMP=str((a.out / 'tmp').resolve()), TMP=str((a.out / 'tmp').resolve()))
    compiled = subprocess.run(['cmd', '/d', '/c', str(script.resolve())], capture_output=True, text=True,
                              env=compile_env, cwd=a.out.resolve())
    (a.out / 'compile.json').write_text(json.dumps(dict(command=command, returncode=compiled.returncode,
                                                       stdout=compiled.stdout, stderr=compiled.stderr), indent=2) + '\n')
    if compiled.returncode:
        raise RuntimeError(compiled.stderr)
    writes = []
    writer_type = C.CFUNCTYPE(C.c_bool, C.c_uint32, C.c_uint16, C.c_void_p)

    @writer_type
    def writer(address, value, user):
        writes.append((address, value))
        return True

    dll = C.CDLL(str(dll_path.resolve()))
    dll.wf_scsp_dsp_load.argtypes = [writer_type, C.c_void_p]
    dll.wf_scsp_dsp_load.restype = C.c_bool
    assert dll.wf_scsp_dsp_load(writer, None)
    (a.out / 'loader-writes.json').write_text(json.dumps(writes, indent=2) + '\n')

    # Established callbacks and core initialization, without the old experiment.
    helper = REPO / 'validation/local-20261028/audio-music-rootcause/test_nofl_snapshot.py'
    saved_argv = sys.argv
    sys.argv = [str(helper), '--core', str(a.core), '--cue', str(a.cue), '--bios', str(a.bios), '--out', str(a.out)]
    env = {'__name__': 'core_harness'}
    exec(compile(helper.read_text().split('for _ in range(1800):')[0], str(helper), 'exec'), env)
    sys.argv = saved_argv
    core, load_state, save_state = [env[n] for n in ('core', 'load_state', 'save_state')]
    original = a.baseline.read_bytes()
    mp = field(original, 'DSP.MPROG', 1024)
    ram = field(original, 'RAM', 524288, mp)
    temp = field(original, 'DSP.TEMP', 512)
    coef = field(original, 'DSP.COEF', 128)
    madrs = field(original, 'DSP.MADRS', 64)
    mems = field(original, 'DSP.MEMS', 128)
    efreg = field(original, 'DSP.EFREG', 32)
    dec = field(original, 'DSP.MDEC_CT', 2)
    prepared = bytearray(original)
    for address, value in writes:
        if address < 0x80000:
            struct.pack_into('<H', prepared, ram + address, value)
        elif 0x100700 <= address < 0x100780:
            struct.pack_into('<H', prepared, coef + address - 0x100700, value >> 3)
        elif 0x100780 <= address < 0x1007c0:
            struct.pack_into('<H', prepared, madrs + address - 0x100780, value)
        elif 0x100800 <= address < 0x100c00:
            index, word = divmod((address - 0x100800) // 2, 4)
            old = struct.unpack_from('<Q', prepared, mp + index * 8)[0]
            shift = (3 - word) * 16
            struct.pack_into('<Q', prepared, mp + index * 8, (old & ~(65535 << shift)) | (value << shift))
    for voice in range(4):
        base = 0x3000 + voice * 8194
        for cursor in range(4097):
            struct.pack_into('<h', prepared, ram + base + cursor * 2, sample(voice, cursor))
        struct.pack_into('<H', prepared, coef + (voice + 1) * 2, 4096)
        # Signed13 -unity cancels the interpolator's deliberately inverted
        # TEMP result, making the externally audible effect equal the ramp.
        struct.pack_into('<H', prepared, coef + (voice + 5) * 2, 4096)
    # Constant at b014 is the phase velocity. Zero it only for exact cursor
    # observations; all loader constants and program are still exercised above.
    struct.pack_into('<H', prepared, ram + 0xb014, 0x6000)  # SCSP packed float zero
    rows = []
    moving_rows = []
    try:
        for cursor in (0, 1, 1000, 2047, 2048, 2049, 3000, 4094, 4095):
            for fraction in (0, 1024, 2048, 4095):
                patched = bytearray(prepared)
                phase = signed24((cursor << 12) | fraction)
                for index in range(128):
                    struct.pack_into('<i', patched, temp + index * 4, phase)
                load_state(bytes(patched))
                core.retro_run()
                result = save_state()
                if cursor == 0 and fraction == 0:
                    (a.out / 'first-result.state').write_bytes(result)
                current_dec = struct.unpack_from('<H', result, dec)[0]
                for voice in range(4):
                    observed = [struct.unpack_from('<i', result, mems + (2 + voice * 2 + n) * 4)[0] >> 8 for n in (0, 1)]
                    expected = [sample(voice, cursor + n) for n in (0, 1)]
                    interp_index = (12 + voice * 16 + current_dec + 1) & 127
                    interpolated = struct.unpack_from('<i', result, temp + interp_index * 4)[0]
                    # FRCL in SHIFT3 latches the lower 12 phase bits. The DSP
                    # multiplies signed24 samples by that unsigned Q12 value.
                    expected_interpolated = (expected[0] << 8) - ((expected[0] << 8) * fraction >> 12) + ((expected[1] << 8) * fraction >> 12)
                    output = struct.unpack_from('<h', result, efreg + voice * 2)[0]
                    row = dict(cursor=cursor, fraction=fraction, voice=voice, observed=observed,
                               expected=expected, interpolated=interpolated, expected_interpolated=expected_interpolated,
                               output=output, expected_output=expected_interpolated >> 8,
                               neighbors_ok=observed == expected,
                               interpolation_ok=interpolated == -expected_interpolated and output == expected_interpolated >> 8)
                    rows.append(row)
                print(f'cursor={cursor} fraction={fraction}: neighbors={all(r["neighbors_ok"] for r in rows[-4:])} interpolation={all(r["interpolation_ok"] for r in rows[-4:])}', flush=True)
        for cursor in (0, 2047, 4095):
            patched = bytearray(prepared)
            initial_phase = (cursor << 12) | 4000
            for index in range(128):
                struct.pack_into('<i', patched, temp + index * 4, signed24(initial_phase))
            # Actual emitted phase velocity and clock gate; no test substitute.
            struct.pack_into('<H', patched, ram + 0xb014, 0xe519)
            struct.pack_into('<H', patched, coef + 9 * 2, 4096)
            initial_dec = struct.unpack_from('<H', patched, dec)[0]
            struct.pack_into('<i', patched, temp + ((initial_dec + 1) & 127) * 4, 0)
            load_state(bytes(patched))
            core.retro_run()
            result = save_state()
            current_dec = struct.unpack_from('<H', result, dec)[0]
            elapsed_samples = (initial_dec - current_dec) & 8191
            expected_phase = (initial_phase + elapsed_samples * 743) & 0xffffff
            final_cursor, fraction = expected_phase >> 12, expected_phase & 4095
            clock = struct.unpack_from('<H', result, ram + 0xb010)[0]
            for voice in range(4):
                observed = [struct.unpack_from('<i', result, mems + (2 + voice * 2 + n) * 4)[0] >> 8 for n in (0, 1)]
                expected = [sample(voice, final_cursor + n) for n in (0, 1)]
                phase_index = (8 + voice * 16 + current_dec + 1) & 127
                final_phase = struct.unpack_from('<i', result, temp + phase_index * 4)[0]
                expected_interpolated = (expected[0] << 8) - ((expected[0] << 8) * fraction >> 12) + ((expected[1] << 8) * fraction >> 12)
                output = struct.unpack_from('<h', result, efreg + voice * 2)[0]
                moving_rows.append(dict(start_cursor=cursor, voice=voice, elapsed_samples=elapsed_samples,
                                        clock=clock, phase=final_phase, expected_phase=signed24(expected_phase),
                                        observed=observed, expected=expected, output=output,
                                        expected_output=expected_interpolated >> 8,
                                        ok=clock == elapsed_samples and final_phase == signed24(expected_phase) and
                                        observed == expected and output == expected_interpolated >> 8))
    finally:
        core.retro_unload_game()
        core.retro_deinit()
    report = dict(core_sha256=hashlib.sha256(a.core.read_bytes()).hexdigest(),
                  cue_sha256=hashlib.sha256(a.cue.read_bytes()).hexdigest(),
                  baseline_sha256=hashlib.sha256(original).hexdigest(),
                  loader_sha256=hashlib.sha256(a.loader_source.read_bytes()).hexdigest(),
                  assembler_sha256=hashlib.sha256(a.assembler.read_bytes()).hexdigest(),
                  disc_images_created=False, controlled_state_edit=True, physical_validation=False,
                  cases=len(rows), rows=rows, neighbors_failures=sum(not r['neighbors_ok'] for r in rows),
                  interpolation_failures=sum(not r['interpolation_ok'] for r in rows), moving_rows=moving_rows,
                  moving_failures=sum(not r['ok'] for r in moving_rows))
    (a.out / 'pcm-report.json').write_text(json.dumps(report, indent=2) + '\n')
    failures = [r for r in rows if not r['neighbors_ok'] or not r['interpolation_ok']]
    assert not failures, f'{len(failures)} PCM failures; first={failures[0]}'
    assert all(r['ok'] for r in moving_rows), moving_rows
    print('Emitted C loader and Beetle PCM neighbors/interpolation PASS', flush=True)


if __name__ == '__main__':
    main()
