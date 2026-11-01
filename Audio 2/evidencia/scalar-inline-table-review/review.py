"""Independent factorial review: the table candidate plus exact scalar inlining."""
from pathlib import Path
import hashlib,importlib.util,json
here=Path(__file__).resolve().parent;fix=here.parent
base=fix/'scalar-admission-table/package/build'
candidate=fix/'scalar-inline-admission-table/music-hot'
old=(base/'z80-native.c').read_text();new=(candidate/'z80-native.c').read_text()
first=new.index('#if defined(__GNUC__)\n#define WF_SCALAR_ALWAYS_INLINE ')
last=new.index('#undef WF_SCALAR_ALWAYS_INLINE\n\n',first)+len('#undef WF_SCALAR_ALWAYS_INLINE\n\n')
private=new[first:last]
def body(text,name):
    start=text.index('{',text.index('unsigned '+name+'('));end=start+1;depth=1
    while depth:depth+=(text[end]=='{')-(text[end]=='}');end+=1
    return text[start:end]
assert body(old,'wf_z80_scalar_step')==body(new,'wf_z80_scalar_step')==body(private,'wf_z80_scalar_inline')
reverted=(new[:first]+new[last:]).replace('WF_SMALL unsigned wf_z80_scalar_step(',
    'WF_SMALL __attribute__((section(".wf_hotcode"))) unsigned wf_z80_scalar_step(',1)
reverted=reverted.replace('  if(!n)n=wf_z80_scalar_inline(z,budget,limit);',
    '  if(!n)n=wf_z80_scalar_step(z,budget,limit);',1)
assert reverted==old
assert (candidate/'z80-cold.c').read_text()==(base/'z80-cold.c').read_text()
spec=importlib.util.spec_from_file_location('elf',fix.parent/'emulator/cold_overlay.py')
c=importlib.util.module_from_spec(spec);spec.loader.exec_module(c)
elf=c.Elf(candidate/'MUSIC.DEBUG.ELF');before=c.Elf(base/'MUSIC.DEBUG.ELF')
assert not any(name.startswith(('_wf_z80_scalar_step','_wf_z80_scalar_inline'))for name in elf.symbols)
def symbol_bytes(image,name):
    address,size=image.symbols[name]
    p=next(p for p in image.ph if p[0]==1 and p[2]<=address and address+size<=p[2]+p[4])
    return image.data[p[1]+address-p[2]:p[1]+address-p[2]+size]
assert symbol_bytes(elf,'_wf_scalar_admission')==symbol_bytes(before,'_wf_scalar_admission')
assert len(symbol_bytes(elf,'_wf_scalar_admission'))==256
assert elf.symbols['_wf_hotcode_start'][0]==0x060889d0
hot_bytes=elf.symbols['_wf_hotcode_end'][0]-0x060889d0
assert hot_bytes+3220<=18432
release=candidate/'MUSIC.ELF';release_sha=hashlib.sha256(release.read_bytes()).hexdigest()
assert release_sha=='bee32079c629bdefed274bbcb90a9f7960872dddda0550d405119c9d6a9af0df'
result=dict(structural_review_passed=True,table_candidate_plus_exact_inline_only=True,
    public_private_bodies_exact=True,source_reverts_to_verified_table_baseline=True,
    cold_unchanged=True,linked_256_byte_table_identical=True,scalar_effectively_inlined=True,
    hot_bytes=hot_bytes,state_bytes=3220,owned_window_bytes=18432,spare_bytes=18432-hot_bytes-3220,
    elf_sha256=release_sha,candidate_hot_LF_sha256=hashlib.sha256(new.encode()).hexdigest(),
    HOST_required_before_emulation=True,emulator_run=False,media_created=False)
(here/'REVIEW.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2))
