"""Independent bounded EC34 review, frozen production and intact DCDE ROM.

Only private test copies receive extra fixtures and explicit -UNDEBUG. No
production, author test, input strategy or global combat probe is changed.
"""
from pathlib import Path
import argparse,hashlib,json,shutil,subprocess,sys,zipfile
ROOT=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser();p.add_argument('output',type=Path);a=p.parse_args()
out=a.output.resolve();out.mkdir(parents=True,exist_ok=True);frozen=out/'frozen/src/runtime';assert not frozen.exists();frozen.mkdir(parents=True)
for name in('src','include','generated'):shutil.copytree(ROOT/name,frozen/name)
for name in('Makefile','audit/runtime18-cd/ROM68K.BIN','tests/r18/human_ec34_verify_rom.py','tests/r18/human_ec34_oracle_bridge.c','tests/r18/human_ec34_manifest.json','tests/r18/human_ec34_snapshot_host6037.bin'):
 dest=frozen/name;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(ROOT/name,dest)
source_hash=hashlib.sha256((frozen/'src/wf_r17_human_raw23.c').read_bytes()).hexdigest();assert source_hash=='7ae4c4bf2286279b48c18e45f4ad257468408565eaf029ab1ad5f6b0ae79d066'
original=frozen/'tests/r18/human_ec34_verify_rom.py';body=original.read_text();body=body.replace("'-O2','-Wall'","'-O2','-UNDEBUG','-Wall'");original.write_text(body)
start=body.index('def fixtures():');end=body.index('core=',start)
extra='''def fixtures():
 for tag in range(256):
  slot=tag%9;m,a,t=seed(slot);put(m,a+0x26,tag<<24,4)
  yield 'independent-all256-physical-null-aliases',m,slot,None
 for flags in range(256):
  slot=flags%9;m,a,t=seed(slot);put(m,t+0x34,flags,1);put(m,a+0x26,(flags&255)<<24,4)
  yield 'independent-target-flags-allBYTE',m,slot,None
 for high in range(256):
  slot=high%9;m,a,t=seed(slot);put(m,a+0x20,(high<<8)|4);put(m,a+0x26,0xff000000,4)
  yield 'independent-own-active-gate-all-high-state-BYTE',m,slot,None
 for state in range(256):
  slot=state%9;m,a,t=seed(slot);other=actors[(slot+4)%9];put(m,a+0x26,0x80000000|other,4)
  put(m,other+0x20,0xa500|state);put(m,other+0x60,0x0035)
  yield 'independent-linked-state-all-lowBYTE',m,slot,None
 for selector,bit4 in itertools.product(range(256),(0,1)):
  slot=selector%9;m,a,t=seed(slot);other=actors[(slot+4)%9]
  put(m,a+0x26,0x40000000|other,4);put(m,other+0x20,0xff05)
  put(m,other+0x60,(((selector*73)&0xef)|(bit4<<4))<<8|selector)
  yield 'independent-all-selector-lowBYTE-highbit4',m,slot,None
'''
extra_body=(body[:start]+extra+body[end:]).replace('random.Random(0xec34)','random.Random(0x6037ec349)')
extra_body=extra_body.replace('itertools.product((0,1,8),(1,0x100','itertools.product(range(9),(1,0x100')
derived=frozen/'tests/r18/human_ec34_independent_extra_derived.py';derived.write_text(extra_body)
hashes={str(x.relative_to(frozen)):hashlib.sha256(x.read_bytes()).hexdigest()for x in frozen.rglob('*')if x.is_file()}
with zipfile.ZipFile(out/'frozen-sources.zip','w',zipfile.ZIP_DEFLATED)as z:
 for name in hashes:z.write(frozen/name,name)
reports={}
for name,script in(('author-rerun','human_ec34_verify_rom.py'),('independent-extra','human_ec34_independent_extra_derived.py')):
 with(out/(name+'.log')).open('w')as log:r=subprocess.run([sys.executable,str(frozen/'tests/r18'/script),'--report',str(out/(name+'.json'))],cwd=frozen,stdout=log,stderr=subprocess.STDOUT)
 assert r.returncode==0,(name,r.returncode,(out/(name+'.log')).read_text()[-3000:])
 q=json.loads((out/(name+'.json')).read_text());assert q['status']=='PASS'and q['source_sha256']==source_hash
 reports[name]={'cases':q['cases'],'prefix_guards':q['guards_and_ROM_prefixes'],'actual_ROM21_reads':q['real_ROM21_read_instructions']};print('PASS',name,reports[name],flush=True)
assert reports['author-rerun']['cases']==5325 and reports['author-rerun']['prefix_guards']==21
assert reports['independent-extra']['cases']==1536 and reports['independent-extra']['prefix_guards']==63
result={'status':'PASS','production_source_sha256':source_hash,'reports':reports,'sources_sha256':hashes,'archive_sha256':hashlib.sha256((out/'frozen-sources.zip').read_bytes()).hexdigest(),'private_test_delta':['-UNDEBUG for both native oracle builds','Extra fixture matrix only in derived private script','All9 slots exercised by independent guard loop; author fixture loop unchanged'],'limits':['Whole intact DCDE owner, not a new global combat or strategy run','Only LINK physicalnull is specialized to immutable canonicalROM21=FF; other nonactor sources remain guarded','RawRAM16KB/FG16KB/D0-D7/represented globals and RNG compared; no A-register/CCR ABI','Original author coverage requirements preserved independently in both runs']}
(out/'report.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items()if k!='sources_sha256'},indent=2))
