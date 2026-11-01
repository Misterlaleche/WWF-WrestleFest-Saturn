"""Effective linked inline+shared-table gate; no emulator or source mutation."""
from pathlib import Path
import argparse,importlib.util,json
b=Path(__file__).resolve().parent;fix=b.parent
p=argparse.ArgumentParser();p.add_argument('--elf',type=Path,required=True);p.add_argument('--label',required=True);a=p.parse_args()
spec=importlib.util.spec_from_file_location('cold',fix.parent/'emulator/cold_overlay.py');c=importlib.util.module_from_spec(spec);spec.loader.exec_module(c);e=c.Elf(a.elf)
def read(address,n):
 s=next(s for s in e.ph if s[0]==1 and s[2]<=address and address+n<=s[2]+s[4]);return e.data[s[1]+address-s[2]:s[1]+address-s[2]+n]
name='_wf_z80_control_batch';start,n=e.symbols[name];raw=read(start,n)
helpers={v[0]:k for k,v in e.symbols.items()if k.endswith('ashlsi3_r0')};sites=[];table_loads=[]
table=e.symbols.get('_wf_scalar_admission');public=[k for k in e.symbols if k.startswith(('_wf_z80_scalar_step','_wf_z80_scalar_inline'))]
for i in range(0,len(raw)-1,2):
 w=int.from_bytes(raw[i:i+2],'big');pc=start+i
 if w>>12!=13:continue
 literal=((pc+4)&~3)+(w&255)*4
 try:value=int.from_bytes(read(literal,4),'big')
 except StopIteration:continue
 if table and value==table[0]:table_loads.append(dict(pc=hex(pc),next_words=[raw[j:j+2].hex()for j in range(i,min(i+8,len(raw)),2)]))
 if value not in helpers:continue
 register=(w>>8)&15
 for step in range(1,7):
  j=i+step*2
  if j+2>len(raw):break
  if int.from_bytes(raw[j:j+2],'big')==(0x400b|(register<<8)):
   sites.append(dict(load_pc=hex(pc),call_pc=hex(start+j),symbol=helpers[value]));break
values=read(table[0],table[1])if table else b''
expected={0xcd,0x3a,0xc9,0xa7,0x08,0xc8,0xc3,0x20,0x32,0x00,0x3e,0xb8,0x09,0x21,0x10,0x06,0x4f,0x28,0x47,0xe6,0x87,0x5f,0x3d,0xf5,0xf1,0x30,0x7e,0xc0,0x16,0xca,0x23,0x77,0x11,0x05,0xaf,0x35,0x3c,0x57,0x5e,0x7a}
table_exact=len(values)==256 and values==bytes(i in expected for i in range(256))
hot=e.symbols['_wf_hotcode_end'][0]-e.symbols['_wf_hotcode_start'][0]
passed=not public and not sites and table_exact and len(table_loads)==1 and hot+3220<=18432
r=dict(passed=passed,public_or_private_scalar_symbols=public,batch_shift_helper_sites=sites,batch_admission_table_loads=table_loads,one_exact_shared_table256=table_exact,hot_bytes=hot,state_bytes=3220,spare=18432-hot-3220,scope='Actual linked SH2 words in batch; other functions may still use the shift helper')
(b/(a.label+'-FACTORIAL-GATE.json')).write_text(json.dumps(r,indent=2)+'\n');print(json.dumps(r,indent=2));raise SystemExit(not passed)
