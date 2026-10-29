#!/usr/bin/env python3
"""Rebuild only this independent WP02 directory; never writes an old source/repository."""
from pathlib import Path
import os,subprocess,shutil,sys
root=Path(__file__).resolve().parent
wp=root.parent
sdk=Path(os.environ.get('YAUL_INSTALL_ROOT',str(wp/'toolchain/sh2eb-elf'))).resolve()
ref='--reference' in sys.argv
name='WP02-CHECK-R02' if ref else 'WP02-TAG-R02'
out=root/('build-reference' if ref else 'build')
out.mkdir(exist_ok=True)
# Refresh only inside the working audit tree; packaged sources are self-contained.
for folder,names in [(wp/'research_rom',['wp02_scene.c','wp02_scene.h','wp02_rom_tables.h']),
 (wp/'research_render/generated',['wp02_assets.c','wp02_assets.h']),
 (wp/'research_audio/integration',['wp02_audio.c','wp02_audio.h','wp02_audio_assets.h','wp02_pcm16.c','wp02_pcm16.h','wp02_pcm16_gain.h','wp02_bgm.c','wp02_bgm.h','wp02_bgm_codec.c','wp02_bgm_codec.h','ponesound_min.c','ponesound_min.h','wf_audio_sfx.c','wf_audio_sfx.h'])]:
 if folder.is_dir():
  for n in names:shutil.copy2(folder/n,root/'src'/n)
if (wp/'research_render/generated').is_dir():
 for n in ['TEX.BIN','CRAM.BIN','RING.BIN']:shutil.copy2(wp/'research_render/generated'/n,root/'cd'/n)
if (wp/'research_audio/integration').is_dir():
 shutil.copy2(wp/'research_audio/integration/cd/WP02PCM.PAK',root/'cd/WP02PCM.PAK')
 shutil.copy2(wp/'research_audio/integration/cd/WP02BGM.PAK',root/'cd/WP02BGM.PAK')
 shutil.copy2(wp/'research_audio/integration/assets/SDRV.BIN',root/'cd/SDRV.BIN')
cc=str(sdk/'bin/sh2eb-elf-gcc')
flags=['-std=c11','-O2','-Wall','-Wextra','-Werror=implicit-function-declaration','-specs=yaul.specs','-specs=yaul-main.specs','-I'+str(root/'src'),'-I'+str(sdk/'sh2eb-elf/include/yaul')]
if ref:flags+=['-DWP02_REFERENCE=1']
objs=[]
for p in sorted((root/'src').glob('*.c')):
 obj=out/(p.stem+'.o');subprocess.run([cc,*flags,'-c',str(p),'-o',str(obj)],check=True);objs.append(str(obj))
elf=out/(name+'.elf');binary=out/(name+'.bin')
subprocess.run([cc,'-O2','-specs=yaul.specs','-specs=yaul-main.specs',*objs,'-Wl,--gc-sections','-Wl,-Map,'+str(out/(name+'.map')),'-Wl,--defsym=___master_stack=0x06004000','-Wl,--defsym=___slave_stack=0x06002000','-o',str(elf)],check=True)
subprocess.run([str(sdk/'bin/sh2eb-elf-objcopy'),'-O','binary',str(elf),str(binary)],check=True)
subprocess.run([str(sdk/'bin/sh2eb-elf-size'),str(elf)],check=True)
env=os.environ.copy();env['YAUL_INSTALL_ROOT']=str(sdk);env['YAUL_ARCH_SH_PREFIX']='sh2eb-elf';env['PATH']=str(sdk/'bin')+os.pathsep+env['PATH']
subprocess.run([str(sdk/'bin/make-ip'),str(binary),'V1.002','20261029','JTUBKAEL','J','WWF WP02 TAG INTRO R02','0x06004000','0x06002000','0x06004000','0'],env=env,check=True)
shutil.copy2(binary,root/'cd/A.BIN')
for n in ['ABS.TXT','BIB.TXT','CPY.TXT']:(root/'cd'/n).write_text('Independent WP02 native scene test. Original art and sounds from user-supplied ROM.\n')
iso=out/(name+'.iso')
subprocess.run([os.environ.get('WP02_MKISOFS',str(wp/'deps/sysroot/usr/bin/genisoimage') if (wp/'deps/sysroot/usr/bin/genisoimage').is_file() else 'genisoimage'),'-quiet','-sysid','SEGA SEGASATURN','-V','WP02','-publisher','WP02 INDEPENDENT','-preparer','WP02','-iso-level','1','-input-charset','iso8859-1','-G',str(out/'IP.BIN'),'-o',str(iso),str(root/'cd')],check=True)
print('ISO',iso)
