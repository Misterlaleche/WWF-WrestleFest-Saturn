"""Independent structural review; HOST success is a separate required gate."""
from pathlib import Path
import hashlib,importlib.util,json
here=Path(__file__).resolve().parent;fix=here.parent
base=fix/'integrated-native18k-admission/music-hot'
candidate=fix/'scalar-inline/music-hot'
old=(base/'z80-native.c').read_text();new=(candidate/'z80-native.c').read_text()
sha=lambda text:hashlib.sha256(text.encode()).hexdigest()
start=new.index('#if defined(__GNUC__)\n#define WF_SCALAR_ALWAYS_INLINE ')
end=new.index('#undef WF_SCALAR_ALWAYS_INLINE\n\n',start)+len('#undef WF_SCALAR_ALWAYS_INLINE\n\n')
private=new[start:end]
def body(text,name):
    first=text.index('{',text.index('unsigned '+name+'('));last=first+1;depth=1
    while depth:
        depth+=(text[last]=='{')-(text[last]=='}');last+=1
    return text[first:last]
assert body(old,'wf_z80_scalar_step')==body(new,'wf_z80_scalar_step')==body(private,'wf_z80_scalar_inline')
reverted=(new[:start]+new[end:]).replace('WF_SMALL unsigned wf_z80_scalar_step(',
    'WF_SMALL __attribute__((section(".wf_hotcode"))) unsigned wf_z80_scalar_step(',1)
reverted=reverted.replace('  if(!n)n=wf_z80_scalar_inline(z,budget,limit);',
    '  if(!n)n=wf_z80_scalar_step(z,budget,limit);',1)
assert reverted==old,'Unexpected change beyond the private inline body, public section and batch call'
assert (candidate/'z80-cold.c').read_text()==(base/'z80-cold.c').read_text()
spec=importlib.util.spec_from_file_location('elf',fix.parent/'emulator/cold_overlay.py')
c=importlib.util.module_from_spec(spec);spec.loader.exec_module(c)
elf=c.Elf(candidate/'MUSIC.DEBUG.ELF')
assert not any(name.startswith(('_wf_z80_scalar_step','_wf_z80_scalar_inline'))for name in elf.symbols)
hot_start=elf.symbols['_wf_hotcode_start'][0];hot_end=elf.symbols['_wf_hotcode_end'][0]
assert hot_start==0x060889d0 and hot_end+3220<=0x0608d1d0
build=json.loads((fix/'scalar-inline/SH2-RESULT.json').read_text())
assert build['module_abi']==3 and build['lookahead_fs']==4096 and build['state_bytes']==3220
assert build['master_sha256']=='e56b027b8a3e89f92ecbb0a419363c11eb2996334b8b819e87ab9137152d0221'
result=dict(structural_review_passed=True,baseline_hot_LF_sha256=sha(old),candidate_hot_LF_sha256=sha(new),
    body_byte_exact=True,cold_LF_byte_exact=True,one_variable=True,
    public_body_preserved=True,private_body_inlined_and_public_GC_removed=True,
    master_abi_budget_lookahead_state_unchanged=True,hot_bytes=hot_end-hot_start,
    state_bytes=3220,owned_window_bytes=18432,spare_bytes=0x0608d1d0-hot_end-3220,
    HOST_required_before_emulation=True,emulation_run=False,media_created=False)
(here/'REVIEW.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2))
