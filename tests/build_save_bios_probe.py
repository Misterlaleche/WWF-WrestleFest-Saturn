#!/usr/bin/env python3
"""Build a small independent Jo disc using the exact production save adapter."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess

ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--out',type=Path,required=True)
p.add_argument('--scenario',type=int,choices=(0,1,2),default=0)
a=p.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False)
(out/'sdk/Compiler').mkdir(parents=True)
for name in ('COMMON','LINUX','WINDOWS'):
    (out/'sdk/Compiler'/name).symlink_to(ROOT/'sdk/Compiler'/name,target_is_directory=True)
shutil.copytree(ROOT/'sdk/jo_engine',out/'sdk/jo_engine',
    ignore=shutil.ignore_patterns('*.o','*.d','jo_options.txt'))
inputs={}
for source,target in [('tests/test_save_bios.c','main.c'),('port/wf_jo_save.c','wf_jo_save.c'),
    ('port/wf_jo_save.h','wf_jo_save.h'),('core/src/wf_r18_save.c','wf_r18_save.c'),
    ('core/include/wf_r18_save.h','wf_r18_save.h')]:
    data=(ROOT/source).read_bytes();(out/target).write_bytes(data)
    inputs[source]=hashlib.sha256(data).hexdigest()
rom=(ROOT/'source_assets/ROM68K.BIN').read_bytes()
assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
factory=rom[0x6db0:0x6db0+132];expected=bytearray(factory);expected[1]=0x51
for i in range(11):expected[66+i*6]=0
(out/'save_fixture.h').write_text('\n'.join(
    'static const unsigned char '+name+'[132]={'+','.join(map(str,data))+'};'
    for name,data in [('save_factory',factory),('save_expected',expected)])+'\n')
(out/'expected-rankings.bin').write_bytes(expected)
(out/'cd').mkdir()
for name in ('ABS.TXT','BIB.TXT','CPY.TXT'):(out/'cd'/name).write_text('Private BIOS persistence test\n')
(out/'makefile').write_text('''JO_MINIMAL = 1
JO_DEBUG = 1
JO_NTSC = 1
JO_SCREEN_WIDTH = 320
JO_SCREEN_HEIGHT = 240
JO_FRAMERATE = 1
JO_COMPILE_WITH_PRINTF_MODULE = 1
JO_GLOBAL_MEMORY_SIZE_FOR_MALLOC = 32768
JO_CD_SECTOR_SIZE = 2352
JO_GAME_TITLE = WF PRIVATE SAVE BIOS TEST
JO_GAME_VERSION = V1.000
JO_GAME_RELEASE_DATE = 20261113
JO_GAME_MAKER_ID = WF TEST
JO_GAME_PRODUCT_NUMBER = T-1245G
JO_ENGINE_SRC_DIR = sdk/jo_engine
COMPILER_DIR = sdk/Compiler
SRCS = main.c wf_jo_save.c wf_r18_save.c
CCFLAGS = -fno-peephole2 -I. -DSAVE_SCENARIO='''+str(a.scenario)+'''
include sdk/Compiler/COMMON/jo_engine_makefile
LDFLAGS += -fno-peephole2
''')
with (out/'build.log').open('w') as log:
    subprocess.run(['make','-j4','MKISOFS=genisoimage'],cwd=out,stdout=log,stderr=subprocess.STDOUT,check=True)
report={'scenario':a.scenario,'inputs':inputs,'factory_rom_sha1':hashlib.sha1(rom).hexdigest(),
    'expected_rankings_sha256':hashlib.sha256(expected).hexdigest(),
    'artifacts':{name:hashlib.sha256((out/name).read_bytes()).hexdigest()
        for name in ('game.elf','game.raw','game.cue','game.bin')}}
(out/'build.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
