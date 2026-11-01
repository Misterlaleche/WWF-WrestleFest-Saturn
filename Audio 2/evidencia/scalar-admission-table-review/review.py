"""Independent exact-delta review of the scalar admission representation."""
from pathlib import Path
import hashlib,importlib.util,json,re
here=Path(__file__).resolve().parent;fix=here.parent
base=fix/'native18k-standalone';candidate=fix/'scalar-admission-table/package'
old=(base/'templates/z80-native.c').read_text();new=(candidate/'templates/z80-native.c').read_text()
first=old.index('  switch(op){case ',old.index('unsigned wf_z80_scalar_step('))
last=old.index('\n',first)
admission=old[first:last]
opcodes={int(value,16)for value in re.findall(r'case (0x[0-9a-fA-F]+):',admission)}
assert len(opcodes)==40 and admission.endswith('break;default:return 0;}')
a=new.index('static const uint8_t wf_scalar_admission[256] ')
b=new.index('};\n\n',a)+len('};\n\n')
declaration=new[a:b];initial=declaration[declaration.index('{')+1:declaration.rindex('}')]
table=[int(value)for value in re.findall(r'\d+',initial)]
assert len(table)==256 and table==[int(i in opcodes)for i in range(256)]
reverted=(new[:a]+new[b:]).replace('  if(!wf_scalar_admission[op])return 0;',admission,1)
assert reverted==old,'Unexpected change beyond the admission table and check'
assert (candidate/'templates/z80-cold.c').read_bytes()==(base/'templates/z80-cold.c').read_bytes()
spec=importlib.util.spec_from_file_location('elf',fix.parent/'emulator/cold_overlay.py')
c=importlib.util.module_from_spec(spec);spec.loader.exec_module(c)
elf=c.Elf(candidate/'build/MUSIC.DEBUG.ELF')
address,size=elf.symbols['_wf_scalar_admission']
assert size==256 and 0x060889d0<=address<address+size<=elf.symbols['_wf_hotcode_end'][0]
segment=next(p for p in elf.ph if p[0]==1 and p[2]<=address and address+size<=p[2]+p[4])
actual=elf.data[segment[1]+address-segment[2]:segment[1]+address-segment[2]+size]
assert actual==bytes(table)
hot_bytes=elf.symbols['_wf_hotcode_end'][0]-0x060889d0
assert hot_bytes+3220<=18432
result=dict(structural_review_passed=True,one_variable=True,all_256_admissions_exact=True,
    source_reverts_to_baseline_LF=True,cold_bytes_unchanged=True,linked_table_bytes_exact=True,
    table_in_owned_HWRAM=True,hot_code_and_table_bytes=hot_bytes,state_bytes=3220,
    owned_window_bytes=18432,spare_bytes=18432-hot_bytes-3220,
    candidate_hot_LF_sha256=hashlib.sha256(new.encode()).hexdigest(),
    HOST_required_before_emulation=True,emulator_run=False,media_created=False)
(here/'REVIEW.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2))
