"""Independent raw87 review: intact1C03E on frozen production, no combat run."""
from pathlib import Path
import argparse,hashlib,json,shutil,subprocess,sys,zipfile
ROOT=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser();p.add_argument('output',type=Path);a=p.parse_args()
out=a.output.resolve();out.mkdir(parents=True,exist_ok=True);frozen=out/'frozen/src/runtime';assert not frozen.exists();frozen.mkdir(parents=True)
for name in('src','include','generated'):shutil.copytree(ROOT/name,frozen/name)
for name in('Makefile','audit/runtime18-cd/ROM68K.BIN','tests/r18/action87_verify_rom.py','tests/r18/action87_oracle_bridge.c','tests/r18/action87_manifest.json','tests/r18/action87_snapshot_host7493.bin'):
 dest=frozen/name;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(ROOT/name,dest)
owner_hash=hashlib.sha256((frozen/'src/wf_r18_action87.inc').read_bytes()).hexdigest();assert owner_hash=='a7ca5213e512283dc1ea939d1fa11ea2ff493d3ad0a30c6dc2d813bc603b249a'
body=(frozen/'tests/r18/action87_verify_rom.py').read_text();start=body.index('def fixtures():');end=body.index('core=',start)
extra='''def fixtures():
 # All high facing bytes and selector aliases;10BD0 must preserve fractions.
 for byte in range(256):
  slot=byte%9;m,a,t=seed(slot)
  put(m,a+0x1c,((byte&0x7f)<<8)|5);put(m,a+0x60,((byte^0xaa)<<8)|0x87)
  put(m,a+0x2e,(byte<<8)|((byte*73)&255));put(m,a+6,(byte*257+0x7ff0)&0xffff)
  put(m,a+0x44,0xffff);put(m,a+0x26,0xdeadbeef if byte&1 else t,4)
  yield 'independent-init-all-facing-BYTE-selector-high-fractions',m,slot,None
 # Countdown remains WORD-sized and independent of an idle animation timer.
 for byte in range(256):
  slot=(byte*5)%9;m,a,t=seed(slot)
  put(m,a+0x1c,((0x80|(byte&0x7f))<<8)|5);put(m,a+0x24,(byte<<8)|0xfe);put(m,a+0x22,0xff00)
  put(m,a+0x44,(0,1,2,0xff,0x100,0x101,0x7fff,0x8000,0x8001,0xfffe,0xffff)[byte%11])
  put(m,a+0x26,(byte<<24)|actors[(slot+4)%9],4)
  yield 'independent-WORD-counter-high-phase-idle-timer',m,slot,None
 # Terminal write ordering: own/peer faces differ; self aliases overwrite in
 # original A0/A2 order, raw tagged back-link and Z-wrap must remain literal.
 for byte,selftarget in itertools.product(range(256),(0,1)):
  slot=byte%9;m,a,t=seed(slot);t=a if selftarget else actors[(slot+4)%9]
  put(m,a+0x26,(byte<<24)|t,4);put(m,a+0x44,1);put(m,a+0x2e,(byte<<8)|((byte*37)&255))
  if not selftarget:put(m,t+0x2e,((byte^0xa5)<<8)|((byte*19)&255))
  put(m,a+14,(0,0xffb7,0xffb8,0xffb9,0xffff,0x7fff,0x8000)[byte%7])
  put(m,a+0x60,((byte&255)<<8)|0x87)
  yield 'independent-terminal-facing-self-peer-rawtags-Z-boundary',m,slot,None
'''
extra_body=(body[:start]+extra+body[end:]).replace('random.Random(0x1aa9a)','random.Random(0x877493aa9a)')
extra_body=extra_body.replace('itertools.product((0,1,8),(0,0x80000000,0xdeadbeef','itertools.product(range(9),(0,0x80000000,0xdeadbeef')
(frozen/'tests/r18/action87_independent_extra_derived.py').write_text(extra_body)
hashes={str(x.relative_to(frozen)):hashlib.sha256(x.read_bytes()).hexdigest()for x in frozen.rglob('*')if x.is_file()}
with zipfile.ZipFile(out/'frozen-sources.zip','w',zipfile.ZIP_DEFLATED)as z:
 for name in hashes:z.write(frozen/name,name)
reports={}
for name,script in(('author-rerun','action87_verify_rom.py'),('independent-extra','action87_independent_extra_derived.py')):
 with(out/(name+'.log')).open('w')as log:r=subprocess.run([sys.executable,str(frozen/'tests/r18'/script),'--report',str(out/(name+'.json'))],cwd=frozen,stdout=log,stderr=subprocess.STDOUT)
 assert r.returncode==0,(name,r.returncode,(out/(name+'.log')).read_text()[-2800:])
 q=json.loads((out/(name+'.json')).read_text());assert q['status']=='PASS'
 key='src\\wf_r18_action87.inc';assert q['sources_sha256'].get(key,q['sources_sha256'].get(key.replace('\\','/')))==owner_hash
 reports[name]={'cases':q['cases'],'guards':q['retained_guards']};print('PASS',name,reports[name],flush=True)
assert reports=={'author-rerun':{'cases':2306,'guards':31},'independent-extra':{'cases':1024,'guards':91}}
result={'status':'PASS','owner_sha256':owner_hash,'reports':reports,'sources_sha256':hashes,'archive_sha256':hashlib.sha256((out/'frozen-sources.zip').read_bytes()).hexdigest(),'private_test_delta':['Additional fixtures only in a derived private script','Independent guards exercise all9 slots; original10PC coverage requirement unchanged'],'limits':['Whole intact1C03E/10BD0 execution; no ROM function intercepts and no global combat run','rawRAM16KB/FG16KB/D0-D7/represented globals/RNG/tickdescriptor compared; no A-register/CCR ABI','Original HOST7493 captured model is projected represented state; not full arcadeRAM or native trajectory']}
(out/'report.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items()if k!='sources_sha256'},indent=2))
