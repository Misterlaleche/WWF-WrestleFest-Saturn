#!/usr/bin/env python3
"""Verify the real WFS1 encoder/decoder against Python's independent CRC32."""
import ctypes as ct
import struct
import subprocess
import tempfile
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


class Data(ct.Structure):
    _fields_ = [('rankings', ct.c_uint8 * 132), ('options', ct.c_uint8 * 16)]


def main():
    with tempfile.TemporaryDirectory(prefix='wf-save-') as directory:
        path = Path(directory) / 'save.so'
        subprocess.run(['cc', '-std=c11', '-shared', '-fPIC', '-Wall', '-Wextra', '-Werror',
                        '-I' + str(ROOT / 'core/include'), str(ROOT / 'core/src/wf_r18_save.c'),
                        '-o', str(path)], check=True)
        lib = ct.CDLL(str(path))
        lib.wf_r18_save_encode.argtypes = [ct.POINTER(Data), ct.POINTER(ct.c_uint8)]
        lib.wf_r18_save_decode.argtypes = [ct.POINTER(ct.c_uint8), ct.POINTER(Data)]
        lib.wf_r18_save_defaults.argtypes = [ct.POINTER(Data), ct.POINTER(ct.c_uint8)]
        source = Data((ct.c_uint8 * 132)(*range(132)), (ct.c_uint8 * 16)(*range(240, 256)))
        image = (ct.c_uint8 * 160)()
        assert lib.wf_r18_save_encode(ct.byref(source), image) == 1
        payload = b'WFS1\0\1\0\xa0' + bytes(source)
        expected = payload + struct.pack('>I', zlib.crc32(payload))
        assert bytes(image) == expected
        output = Data()
        assert lib.wf_r18_save_decode(image, ct.byref(output)) == 1
        assert bytes(output) == bytes(source)
        # Every single-bit corruption is rejected without partially
        # overwriting the caller's live factory rankings/options.
        untouched = bytes(output)
        for byte in range(160):
            for bit in range(8):
                damaged = (ct.c_uint8 * 160).from_buffer_copy(expected)
                damaged[byte] ^= 1 << bit
                assert lib.wf_r18_save_decode(damaged, ct.byref(output)) == 0
                assert bytes(output) == untouched
        assert not lib.wf_r18_save_decode(None, ct.byref(output))
        assert not lib.wf_r18_save_decode(image, None)
        assert not lib.wf_r18_save_encode(None, image)
        assert not lib.wf_r18_save_encode(ct.byref(source), None)
        lib.wf_r18_save_defaults(ct.byref(output), source.rankings)
        assert bytes(output.rankings) == bytes(source.rankings)
        assert bytes(output.options) == bytes(16)
    print('PASS WFS1: independent wire image/CRC, round trip, 1280 corruptions, defaults/null safety')


if __name__ == '__main__':
    main()
