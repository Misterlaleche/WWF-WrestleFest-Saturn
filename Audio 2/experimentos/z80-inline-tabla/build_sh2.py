from pathlib import Path
import hashlib,importlib.util,json,os,sys
base=Path(__file__).resolve().parent;repo=base/'repo';fix=base.parent;runtime=fix.parents[3];workspace=runtime.parent
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest();master=fix/'saturn-real-v25/master/wrestlefest-runtime18-scsp.elf'
assert sha(master)=='e56b027b8a3e89f92ecbb0a419363c11eb2996334b8b819e87ab9137152d0221'
tool=Path('C:/Users/VersusPC/Documents/Codex/2026-10-25/https-github-com-misterlaleche-wwf-wrestlefest/work/sh2-toolchain');os.environ['PATH']=str(tool/'bin')+os.pathsep+os.environ['PATH'];sys.path.insert(0,str(repo/'tools/audio/native_probe'))
spec=importlib.util.spec_from_file_location('inline_builder',repo/'tools/audio/native_probe/build_music_module.py');b=importlib.util.module_from_spec(spec);spec.loader.exec_module(b)
out=base/'music-hot';report=b.build(repo,tool,workspace/'toolchains/sh2eb-elf',workspace/'local-rom/31a11-2.ic42',out)
spec=importlib.util.spec_from_file_location('cold',fix.parent/'emulator/cold_overlay.py');c=importlib.util.module_from_spec(spec);spec.loader.exec_module(c);elf=c.Elf(out/'MUSIC.DEBUG.ELF');s=elf.symbols
start=0x060889d0;end=0x0608d1d0;assert s['_wf_hotcode_start'][0]==start and s['_wf_hotcode_end'][0]+3220<=end
assert not any(name.startswith(('_wf_z80_scalar_step','_wf_z80_scalar_inline'))for name in s)
assert sha(out/'z80-cold.c')==sha(base/'baseline/z80-cold.c')
assert sha(out/'z80-native.c')==sha(base/'generated/z80-native.c')
report.update(strict_link_passed=True,effective_scalar_inline=True,public_scalar_GC_removed=True,private_scalar_no_out_of_line_copy=True,master_sha256=sha(master),hot_code_bytes=s['_wf_hotcode_end'][0]-start,state_bytes=3220,window_bytes=18432,workspace_spare=end-s['_wf_hotcode_end'][0]-3220,common_baseline_hot_code_bytes=13588,inline_single_hot_bytes=14084,table_single_hot_bytes=13500,hot_delta=s['_wf_hotcode_end'][0]-start-13588,physical_validation=False,media_created=False)
(base/'SH2-RESULT.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
