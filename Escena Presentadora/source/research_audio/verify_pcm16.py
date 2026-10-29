#!/usr/bin/env python3
"""Cross-check every exported PCM16 sample against the supplied C decoder.

The Test255 C decoder's internal 12-bit signal is exported before its final
PCM8 rounding. That independent C path is compared with every big-endian
sample produced by prepare_assets.py, using original ROM bytes as input.
"""
import argparse
import ctypes
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import zipfile

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument('--sources',type=Path,default=Path('project_sources'))
    ap.add_argument('--integration',type=Path,default=Path('wp02/research_audio/integration'))
    args=ap.parse_args()
    base=args.integration.resolve()
    out=base.parent/'verification';out.mkdir(exist_ok=True)
    with zipfile.ZipFile(next(args.sources.glob('13-*.zip'))) as z:
        original=z.read('wf_oki_decode.c')
    source=original.decode().replace('#include "wf_oki_decode.h"',
                                     '#include <stddef.h>\n#include <stdint.h>')
    source=source.replace('wf_oki_decode_pcm8','reference_decode_signal12')
    source=source.replace('int8_t *dst','int16_t *dst')
    start=source.index('   int q=signal/16;')
    end=source.index('   dst[out++]=(int8_t)q;')+len('   dst[out++]=(int8_t)q;')
    source=source[:start]+'   dst[out++]=(int16_t)signal;'+source[end:]
    cfile=out/'reference_signal12.c';cfile.write_text(source)
    so=out/'reference_signal12.so'
    subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-O2',
                    '-shared','-fPIC',str(cfile),'-o',str(so)],check=True)
    lib=ctypes.CDLL(str(so));decode=lib.reference_decode_signal12
    decode.argtypes=[ctypes.POINTER(ctypes.c_uint8),ctypes.c_size_t,
                     ctypes.POINTER(ctypes.c_int16),ctypes.c_size_t]
    decode.restype=ctypes.c_size_t
    with zipfile.ZipFile(next(args.sources.glob('01-*.zip'))) as z:
        rom=z.read('31j10.ic73')
    meta=json.loads((base/'audio_assets.json').read_text())
    pack=(base/'cd/WP02PCM.PAK').read_bytes()
    assert hashlib.sha256(rom).hexdigest()==meta['rom_sha256']
    assert hashlib.sha256(pack).hexdigest()==meta['pcm_pack_sha256']
    samples=0
    for row in meta['samples']:
        off=row['rom_offset'];n=row['adpcm_bytes']
        adpcm=rom[off:off+n]
        csrc=(ctypes.c_uint8*n).from_buffer_copy(adpcm)
        cdst=(ctypes.c_int16*(n*2))()
        assert decode(csrc,n,cdst,n*2)==n*2
        data=pack[row['sector']*2048:row['sector']*2048+row['pcm16_bytes']]
        actual=[s[0] for s in struct.iter_unpack('>h',data)]
        assert actual==[s*16 for s in cdst],(row['bank'],row['sample_id'])
        assert all((s&15)==0 for s in actual)
        samples+=len(actual)
    for name,want in meta['original_file_sha256'].items():
        assert hashlib.sha256((base/name).read_bytes()).hexdigest()==want,name
    report=dict(status='PASS',pcm16_samples_verified=samples,
                assets_verified=len(meta['samples']),
                original_decoder_source_sha256=hashlib.sha256(original).hexdigest(),
                source='Test255 wf_oki_decode.c before its final PCM8 quantization',
                unchanged_source_files=len(meta['original_file_sha256']),
                no_generated_or_substitute_audio=True)
    (out/'PCM16-VERIFICATION.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))

if __name__=='__main__':main()
