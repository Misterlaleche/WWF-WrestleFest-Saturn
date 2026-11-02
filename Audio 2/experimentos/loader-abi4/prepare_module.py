"""Build a direct-load ABI4 module paired with the isolated ABI4 master."""
from pathlib import Path
import hashlib,importlib.util,json,os,shutil,sys
here=Path(__file__).resolve().parent
root=here.parents[5]
repair=here.parents[1]
master=here/'runtime/master-build/wrestlefest-runtime18-scsp.elf'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
assert sha(master)=='87a5f061a1b73c7d46392d15c6b60ce951b244dc5eae755f03b8f0a31aca12cb'
spec=importlib.util.spec_from_file_location('cold',repair/'emulator/cold_overlay.py')
c=importlib.util.module_from_spec(spec);spec.loader.exec_module(c)
elf=c.Elf(master)
start=elf.symbols['___wf_music_hot_start'][0]
end=elf.symbols['___wf_music_hot_end'][0]
assert end-start==18432
repo=here/'module/repo'
assert not repo.exists(),'Keep each experiment immutable.'
shutil.copytree(here.parent/'saturn-real-v25/repo',repo)
native=repo/'src/audio/native'
shutil.copyfile(here/'src/wf_music_module.h',native/'wf_music_module.h')
shutil.copyfile(here/'src/compact_music_elf.py',repo/'tools/audio/native_probe/compact_music_elf.py')
path=native/'wf_music_module.c';source=path.read_text()
source=source.replace('wf_hotcode_start[],wf_hotcode_end[],wf_hotcode_load[]',
                      'wf_hotcode_start[],wf_hotcode_end[]')
source=source.replace('uintptr_t code=(uintptr_t)wf_hotcode_start,load=(uintptr_t)wf_hotcode_load;',
                      'uintptr_t code=(uintptr_t)wf_hotcode_start;')
first=source.index(' if(code!=load){')
last=source.index(' hot=(wf_music_hot_t *)',first)
source=source[:first]+''' if(code!=workspace){SHARED.error=14;SHARED.state=WF_MUSIC_FAULT;return;}
'''+source[last:]
path.write_text(source,newline='\n')
link='''OUTPUT_FORMAT("elf32-sh")
OUTPUT_ARCH(sh)
ENTRY(_wf_music_module_prepare)
PHDRS {program PT_LOAD FLAGS(5); state PT_LOAD FLAGS(6); shared PT_LOAD FLAGS(6); hot PT_LOAD FLAGS(7);}
SECTIONS {
 .header 0x05c5c000 : {KEEP(*(.wf_music_header))} :program
 .text : {*(.text .text.*)} :program
 .rodata : {*(.rodata .rodata.*) *(.wf_fm_tables) . = ALIGN(4);} :program
 ASSERT(. <= 0x05c7bf00,"program exceeds VDP1 scratch")
 .wf_hotcode HOT_START : {
  _wf_hotcode_start = .; *(.wf_hotcode .wf_hotcode.*) *(.wf_hotdata_const .wf_hotdata_const.*)
  . = ALIGN(4); _wf_hotcode_end = .;
 } :hot
 .wf_hotstate (NOLOAD) : {. += 3220;} :hot
 ASSERT(. <= HOT_END,"code/state exceed the owned 18KiB window")
 .bss 0x05c7c000 (NOLOAD) : {*(.bss .bss.* COMMON) . = ALIGN(4);} :state
 ASSERT(. <= 0x05c7f000,"private state exceeds12KiB")
 .shared 0x05c7f000 (NOLOAD) : {. += 4096;} :shared
 __music_image_end = 0x05c80000;
 /DISCARD/ : {*(.comment) *(.note*) *(.eh_frame*)}
}
'''.replace('HOT_START',hex(start)).replace('HOT_END',hex(end))
(native/'wf_music_module.ld').write_text(link,newline='\n')
path=repo/'tools/audio/native_probe/build_music_module.py';source=path.read_text()
old='0x25c7f000,0\n.section .rodata.music_rom'
assert source.count(old)==1
path.write_text(source.replace(old,'0x25c7f000,_wf_hotcode_start\n.section .rodata.music_rom'),newline='\n')
manifest={str(p.relative_to(repo)).replace('\\','/'):sha(p)
          for p in sorted(repo.rglob('*'))if p.is_file()}
(here/'module/SOURCE-MANIFEST.json').write_text(json.dumps(manifest,indent=2)+'\n')
tool=Path('C:/Users/VersusPC/Documents/Codex/2026-10-25/https-github-com-misterlaleche-wwf-wrestlefest/work/sh2-toolchain')
os.environ['PATH']=str(tool/'bin')+os.pathsep+os.environ['PATH']
sys.path.insert(0,str(repo/'tools/audio/native_probe'))
spec=importlib.util.spec_from_file_location('abi4_builder',repo/'tools/audio/native_probe/build_music_module.py')
b=importlib.util.module_from_spec(spec);spec.loader.exec_module(b)
out=here/'module/build'
report=b.build(repo,tool,root/'toolchains/sh2eb-elf',root/'local-rom/31a11-2.ic42',out)
debug=c.Elf(out/'MUSIC.DEBUG.ELF')
assert debug.symbols['_wf_hotcode_start'][0]==start
assert debug.symbols['_wf_hotcode_end'][0]+3220<=end
assert len(c.Elf(out/'MUSIC.ELF').ph)==4
report.update(master_sha256=sha(master),hot_start=hex(start),hot_end=hex(end),
    state_bytes=3220,hot_code_bytes=debug.symbols['_wf_hotcode_end'][0]-start,
    direct_four_loads=True,canonical_changed=False,media_created=False,
    scope='Loader and ownership experiment; V25 still has known FM drops.')
(here/'module/RESULT.json').write_text(json.dumps(report,indent=2)+'\n')
assert all(sha(repo/p)==h for p,h in manifest.items())
print(json.dumps(report,indent=2))
