"""Freeze an ABI4 master experiment. Private sources stay on this computer."""
from pathlib import Path
import hashlib,json,shutil

here=Path(__file__).resolve().parent
original=here.parent/'windows-master-probe/partition18'
root=here.parents[5]
repo=here/'runtime/repo'
assert not repo.exists(),'Preserve the previous source snapshot.'
shutil.copytree(original/'repo',repo)
for name in ('wf_music_elf.c','wf_music_elf.h','wf_music_module.h'):
    shutil.copyfile(here/'src'/name,repo/'src/audio/native'/name)
entry=repo/'src/runtime/src/wf_r18_scsp.c'
source=entry.read_text()
for old,new in [
    ('wf_music_elf_image(scratch,bytes,&image)',
     'wf_music_elf_image(scratch,bytes,workspace,workspace_bytes,&image)'),
    ('wf_music_elf_copy(&image,music_write,NULL)',
     'wf_music_elf_copy(&image,workspace,workspace_bytes,music_write,NULL)')]:
    assert source.count(old)==1,old
    source=source.replace(old,new)
entry.write_text(source,newline='\n')
builder=(original/'build_master.py').read_text()
old='HERE=Path(__file__).resolve().parent;PROBE=HERE.parent;CONTROL=PROBE.parent;ROOT=PROBE.parents[5];RUNTIME=ROOT/\'runtime18\''
assert builder.count(old)==1
builder=builder.replace(old,'HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[6];RUNTIME=ROOT/\'runtime18\'')
(here/'runtime/build_master.py').write_text(builder,newline='\n')

# Reuse the nine boot scenarios against the exact changed body, with the new
# trusted range arguments verified by the existing parser/copy boundary stubs.
test=(original/'test_boot.c').read_text()
test=test.replace('wf_music_elf_image(void *p,uint32_t b,wf_music_image_t *im)',
                  'wf_music_elf_image(void *p,uint32_t b,uint32_t base,uint32_t size,wf_music_image_t *im)')
test=test.replace('record(3);im->prepare=',
                  'assert(base==0x06080000u&&size==18432u);record(3);im->prepare=')
test=test.replace('wf_music_elf_copy(const wf_music_image_t *im,void (*write)',
                  'wf_music_elf_copy(const wf_music_image_t *im,uint32_t base,uint32_t size,void (*write)')
test=test.replace('(void)im;assert(write==music_write&&!u);',
                  '(void)im;assert(base==0x06080000u&&size==18432u);assert(write==music_write&&!u);')
(here/'runtime/test_boot.c').write_text(test,newline='\n')
start=source.index('static bool music_boot(void)')
end=source.index('static uint16_t last_frt;',start)
(here/'runtime/extracted-music-boot.inc').write_text(source[start:end],newline='\n')
vc='C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/Auxiliary/Build/vcvars64.bat'
(here/'runtime/test_boot.cmd').write_text('@echo off\ncall "'+vc+'" >nul\nif errorlevel 1 exit /b 1\ncl /nologo /O2 /W4 /WX /std:c11 test_boot.c /Fetest_boot.exe\nif errorlevel 1 exit /b 1\ntest_boot.exe\nexit /b %errorlevel%\n')
manifest={str(p.relative_to(repo)).replace('\\','/'):hashlib.sha256(p.read_bytes()).hexdigest()
          for p in sorted(repo.rglob('*'))if p.is_file()}
(here/'runtime/PREPARED.json').write_text(json.dumps(dict(original=str(original),
    original_master_sha256='e56b027b8a3e89f92ecbb0a419363c11eb2996334b8b819e87ab9137152d0221',
    files=manifest,canonical_changed=False,media_generated=False),indent=2)+'\n')
print('Frozen ABI4 master source; nine exact-body boot scenarios prepared. No compilation or media.')
