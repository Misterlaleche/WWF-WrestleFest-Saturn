#!/usr/bin/env python3
"""Freeze current sources and the hardware-tested SFX-only firmware for R05."""
import hashlib
import io
import json
import tarfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT/'validation/local-20261028/eval-r05-build'
ARCHIVE = OUT/'r05-sources.tar.gz'
FIRMWARE = ROOT/'validation/local-20261026/audio-r02-firmware-underrun/legacy-profile/SCSP.ELF'
EXPECTED_FIRMWARE = '42b33b2e31a1c558f7d2d8ebba09fa7359cbe8197244389b994438733cb54e33'

def digest(data):
    return hashlib.sha256(data).hexdigest()

OUT.mkdir(parents=True,exist_ok=True)
if ARCHIVE.exists():
    raise FileExistsError(ARCHIVE)
files=[]
for directory in ('src/runtime/src','src/runtime/include','src/runtime/generated',
                  'src/runtime/validation','src/audio/native','src/runtime/audit/runtime18-cd'):
    files.extend(path for path in (ROOT/directory).rglob('*') if path.is_file()
                 and '__pycache__' not in path.parts and path.suffix.lower() not in ('.pyc','.o','.pdb','.exe'))
files.extend(ROOT/path for path in ('src/runtime/Makefile.saturn-disc',
    'src/runtime/tools/build_r18_hot_sprites.py', 'validation/iso_to_bin_exact.py',
    'validation/verify_disc_exact.py', 'validation/verify-hwram-budget.py',
    'validation/verify_saturn_stack_layout.py'))
files=sorted(set(files))
firmware=FIRMWARE.read_bytes()
assert digest(firmware)==EXPECTED_FIRMWARE
manifest={}
with tarfile.open(ARCHIVE,'x:gz',compresslevel=6) as archive:
    for path in files:
        name=str(path.relative_to(ROOT)).replace('\\','/')
        original=path.read_bytes();data=original
        if name=='src/runtime/audit/runtime18-cd/SCSP.ELF':
            assert len(firmware)<=len(original)==54416
            data=firmware+bytes(len(original)-len(firmware))
        elif path.suffix.lower() in ('.c','.h','.inc','.s','.ld','.mk','.py','.txt') or path.name.startswith('Makefile'):
            data=data.replace(b'\r\n',b'\n')
        member=tarfile.TarInfo('runtime18/'+name)
        member.size=len(data);member.mode=0o644;member.mtime=0
        archive.addfile(member,io.BytesIO(data))
        manifest[name]={'source_sha256':digest(original),'build_sha256':digest(data),'bytes':len(data)}
assert 'src/runtime/audit/runtime18-cd/SCSP.ELF' in manifest
record={'archive_sha256':digest(ARCHIVE.read_bytes()),'files':manifest,
        'sfx_firmware_sha256':EXPECTED_FIRMWARE,
        'sfx_padded_sha256':manifest['src/runtime/audit/runtime18-cd/SCSP.ELF']['build_sha256'],
        'note':'Music firmware is replaced only inside the frozen build; workspace firmware remains unchanged.'}
(OUT/'source-manifest.json').write_text(json.dumps(record,indent=2)+'\n')
print(json.dumps({'archive':str(ARCHIVE),'files':len(files),'archive_sha256':record['archive_sha256'],
                  'sfx_padded_sha256':record['sfx_padded_sha256']}))
