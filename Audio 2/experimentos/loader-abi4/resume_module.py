"""Resume the frozen ABI4 experiment after a sandbox DLL-launch failure."""
from pathlib import Path
import hashlib,importlib.util,json,os,sys
here=Path(__file__).resolve().parent
root=here.parents[5];repo=here/'module/repo';out=here/'module/build'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
manifest=json.loads((here/'module/SOURCE-MANIFEST.json').read_text())
assert all(sha(repo/p)==h for p,h in manifest.items())
master=here/'runtime/master-build/wrestlefest-runtime18-scsp.elf'
assert sha(master)=='87a5f061a1b73c7d46392d15c6b60ce951b244dc5eae755f03b8f0a31aca12cb'
assert not (here/'module/RESULT.json').exists(),'Preserve the completed build.'
tool=Path('C:/Users/VersusPC/Documents/Codex/2026-10-25/https-github-com-misterlaleche-wwf-wrestlefest/work/sh2-toolchain')
os.environ['PATH']=str(tool/'bin')+os.pathsep+os.environ['PATH']
sys.path.insert(0,str(repo/'tools/audio/native_probe'))
spec=importlib.util.spec_from_file_location('builder',repo/'tools/audio/native_probe/build_music_module.py')
b=importlib.util.module_from_spec(spec);spec.loader.exec_module(b)
report=b.build(repo,tool,root/'toolchains/sh2eb-elf',root/'local-rom/31a11-2.ic42',out)
spec=importlib.util.spec_from_file_location('cold',here.parents[1]/'emulator/cold_overlay.py')
c=importlib.util.module_from_spec(spec);spec.loader.exec_module(c)
m=c.Elf(master);debug=c.Elf(out/'MUSIC.DEBUG.ELF');packed=c.Elf(out/'MUSIC.ELF')
start=m.symbols['___wf_music_hot_start'][0];end=m.symbols['___wf_music_hot_end'][0]
assert debug.symbols['_wf_hotcode_start'][0]==start
assert debug.symbols['_wf_hotcode_end'][0]+3220<=end
assert len(packed.ph)==4 and all(h[2]==h[3] for h in packed.ph)
assert packed.ph[3][2]==start and packed.ph[3][5]==debug.symbols['_wf_hotcode_end'][0]-start+3220
report.update(master_sha256=sha(master),hot_start=hex(start),hot_end=hex(end),
    state_bytes=3220,hot_code_bytes=debug.symbols['_wf_hotcode_end'][0]-start,
    direct_four_loads=True,canonical_changed=False,media_created=False,
    scope='Loader and ownership experiment; V25 still has known FM drops.')
(here/'module/RESULT.json').write_text(json.dumps(report,indent=2)+'\n')
assert all(sha(repo/p)==h for p,h in manifest.items())
print(json.dumps(report,indent=2))
