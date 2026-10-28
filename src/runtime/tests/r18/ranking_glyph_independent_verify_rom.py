"""Independent6CFA/6D76 review: frozen owners, new seed and byte/WORD adversaries."""
from pathlib import Path
import argparse,hashlib,json,shutil,subprocess,sys,zipfile
from capstone import Cs,CS_ARCH_M68K,CS_MODE_BIG_ENDIAN,CS_MODE_M68K_000
ROOT=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser();p.add_argument('output',type=Path);a=p.parse_args()
out=a.output.resolve();out.mkdir(parents=True,exist_ok=True);frozen=out/'frozen/src/runtime';assert not frozen.exists();frozen.mkdir(parents=True)
shutil.copytree(ROOT/'include',frozen/'include')
for name in('src/wf_r18_ranking_glyph.c','src/wf_r17_m68k_shadow.c','tests/r18/ranking_glyph_verify_rom.py','tests/r18/ranking_glyph_oracle_bridge.c','audit/runtime18-cd/ROM68K.BIN'):
 dest=frozen/name;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(ROOT/name,dest)
owner=frozen/'src/wf_r18_ranking_glyph.c';owner_hash=hashlib.sha256(owner.read_bytes()).hexdigest()
assert owner_hash=='a9cc9233de579ea24e681c68bc91316427e33f54f07b388d66abd40ed213f3e9'
body=(frozen/'tests/r18/ranking_glyph_verify_rom.py').read_text()
extra=body.replace('def check(owner,mode,glyph,base,fail=-1):','def check(owner,mode,glyph,base,fail=-1,d3=None):')
extra=extra.replace('  mem.w_block(0,bytes(raw));','  if d3 is not None:dr[3]=d3\n  mem.w_block(0,bytes(raw));',1)
start=extra.index(' modes=(');end=extra.index(' machine.cleanup()',start)
cases=''' # All high address aliases, selector modes and non-ASCII glyph values.
 for tag,mode,glyph in itertools.product(range(256),(0,2,3,0xffff),(0x30,0x31,0x4b,0xffff)):
  check(0x6cfa,mode,glyph,(tag<<24)|0xc1f00)
 # Every high glyph BYTE with wrap boundaries;014B must not be substituted
 # for the exact WORD004B blank. No range clamp/default glyph is admitted.
 for high,low in itertools.product(range(256),(0,0x30,0x31,0x4a,0x4b,0x4c,0x7f,0xff)):
  glyph=(high<<8)|low;check(0x6cfa,high%3,glyph,0xff0c3cfc)
 # Direct6D76 consumes D3, not D2; odd bases are legal for these BYTE writes.
 for byte in range(256):
  value=((byte*257)<<16)|(byte*257)
  check(0x6d76,0xffff,0x4b,(byte<<24)|0xc0200|(byte&1),d3=value)
 for high,low in itertools.product((0,0x7fff0000,0x80000000,0xffff0000),(0,0xff,0x100,0x7fff,0x8000,0xffff)):
  check(0x6d76,0,0xffff,0x800c3ffc,d3=high|low)
 # Every write of each owner branch can fail; high D/A aliases survive
 # successful MOVEM only, and blocked writes retain their real prefix.
 for mode,glyph,amount in((0,0x30,8),(1,0x4b,4),(2,0x014b,8),(0xffff,0x4b,1)):
  for fail,tag in itertools.product(range(amount),(0,1,0x7f,0x80,0xff)):
   check(0x6cfa,mode,glyph,(tag<<24)|0xc0120,fail)
 for fail,tag,value in itertools.product(range(2),(0,1,0x7f,0x80,0xff),(0,0xffff,0xabcdef01)):
  check(0x6d76,0,0x31,(tag<<24)|0xc0131,fail,d3=value)
'''
extra=extra[:start]+cases+extra[end:]
(frozen/'tests/r18/ranking_glyph_independent_extra_derived.py').write_text(extra)
hashes={str(x.relative_to(frozen)):hashlib.sha256(x.read_bytes()).hexdigest()for x in frozen.rglob('*')if x.is_file()}
with zipfile.ZipFile(out/'frozen-sources.zip','w',zipfile.ZIP_DEFLATED)as z:
 for name in hashes:z.write(frozen/name,name)
rom=(frozen/'audit/runtime18-cd/ROM68K.BIN').read_bytes();decoder=Cs(CS_ARCH_M68K,CS_MODE_BIG_ENDIAN|CS_MODE_M68K_000)
expected={i.address for i in decoder.disasm(rom[0x6cfa:0x6d82],0x6cfa)};reports={}
for name,script,seed in(('author-new-seed','ranking_glyph_verify_rom.py','0x9b6cfa'),('independent-extra','ranking_glyph_independent_extra_derived.py','0x51d07663')):
 with(out/(name+'.log')).open('w')as log:r=subprocess.run([sys.executable,str(frozen/'tests/r18'/script),'--seed',seed,'--report',str(out/(name+'.json'))],cwd=frozen,stdout=log,stderr=subprocess.STDOUT)
 assert r.returncode==0,(name,r.returncode,(out/(name+'.log')).read_text()[-3000:])
 q=json.loads((out/(name+'.json')).read_text());assert q['status']=='PASS'and expected<=set(q['coverage']),(name,'full6CFA..6D80 PC coverage')
 assert q['sources_sha256']['src\\wf_r18_ranking_glyph.c']==owner_hash
 reports[name]={'owners':q['owners'],'prefixes':q['failed_bus_prefixes'],'bad_contexts':q['bad_context_guards'],'dependencies':q['missing_dependency_guards']};print('PASS',name,reports[name],flush=True)
result={'status':'PASS','owner_sha256':owner_hash,'reports':reports,'full_code_PC_count':len(expected),'full_code_PCs':sorted(expected),'sources_sha256':hashes,'archive_sha256':hashlib.sha256((out/'frozen-sources.zip').read_bytes()).hexdigest(),'limits':['RAM16KB/FG16KB/D0-D7/A0-A6+ordered width/address/value bus writes and failed prefixes; CCR/stack/timing excluded','No ROM function or bus intercepts; failed-prefix stop is before the corresponding original store','No ranking main/scene or global combat run; A2 transport must implement actual bus address/width contract']}
(out/'report.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items()if k!='sources_sha256'},indent=2))
