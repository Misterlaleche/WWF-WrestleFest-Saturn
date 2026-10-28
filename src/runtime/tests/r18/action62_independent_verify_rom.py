"""Independent raw62 review on frozen source; intact ROM, no combat run."""
from pathlib import Path
import argparse,hashlib,json,shutil,subprocess,sys,zipfile
ROOT=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser();p.add_argument('output',type=Path);a=p.parse_args()
out=a.output.resolve();out.mkdir(parents=True,exist_ok=True);frozen=out/'frozen/src/runtime';assert not frozen.exists();frozen.mkdir(parents=True)
for name in('src','include','generated'):shutil.copytree(ROOT/name,frozen/name)
names=['Makefile','audit/runtime18-cd/ROM68K.BIN','tests/r18/action62_verify_rom.py','tests/r18/action62_oracle_bridge.c','tests/r18/action62_manifest.json']
names += ['tests/r18/action62_snapshot_host_policy'+s+'.bin'for s in('000','002','005')]
for name in names:
 dest=frozen/name;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(ROOT/name,dest)
owner_hash=hashlib.sha256((frozen/'src/wf_r18_action62.inc').read_bytes()).hexdigest()
assert owner_hash=='e7f4cbfb22895a290fc267dde5472b66e459d581bb9873170665957ad13ac344'
script=frozen/'tests/r18/action62_verify_rom.py';body=script.read_text()
# Keep native production assertions enabled in both private compilations.
body=body.replace("'-O2','-Wall'","'-O2','-UNDEBUG','-Wall'")
script.write_text(body)
at=body.index('core=')
extra='''author_fixtures=fixtures
def fixtures():
 yield from itertools.islice(author_fixtures(),3)
 # Isolate each 21732 flag BYTE, including same actor serving as A0/A1.
 for field,byte in itertools.product(('own33','partner32','partner33','partner74','partner34'),range(256)):
  slot=byte%9;m,a,t,pa=seed(slot);pa=a if byte%4==0 else actors[(slot+3)%9]
  put(m,a+0x86,pa|((byte^0x6c)<<24),4);put(m,a+0x26,t|(byte<<24),4)
  put(m,a+0x70,byte%3);put(m,pa+0x70,(byte+1)%3)
  put(m,a+0x60,((byte^0x93)<<8)|0x62);put(m,a+0x1c,((byte&0x7f)<<8)|5)
  put(m,pa+0x7a,0xdeadbeef,4)
  if field=='own33':put(m,a+0x33,byte,1)
  else:put(m,pa+{'partner32':0x32,'partner33':0x33,'partner74':0x74,'partner34':0x34}[field],byte,1)
  yield 'independent-init-allBYTE-'+field+'-tagged-peer-self',m,slot,None
 # All mode bytes and full LONG excluded-target tags in the Royal scan.
 for mode,credit in itertools.product(range(256),(0,0x8001)):
  slot=(mode*5)%9;m,a,t,pa=seed(slot);put(m,RAM+0x161,mode,1);put(m,RAM+0x7c,credit)
  put(m,a+0x26,actors[4]|(mode<<24),4);put(m,a+0x70,mode%3)
  put(m,a+6,(0,0xffff,0x7fff,0x8000)[mode%4]);put(m,a+10,(mode*257)&65535)
  for i,c in enumerate(actors[4:]):
   put(m,c+6,((mode+i)*257)&65535);put(m,c+10,((mode-i)*129)&65535)
   put(m,c+0xb5,(mode+i)&0x88,1);put(m,c+0x32,(mode+i)&0x10,1);put(m,c+0x56,(mode+i)&0x20,1)
  yield 'independent-allMODE-BYTE-Royal-LONGexclude-WORDdistance',m,slot,None
 # 11432 must retain flags' BYTE in D0, clear both aliases in sequence,
 # and10D04 must use bit6/7 and fresh BYTE rather than signed/WORD tests.
 for flags,role in itertools.product(range(256),(0,0x40,0x7f,0x80,0xc0)):
  slot=flags%9;m,a,t,pa=seed(slot);pa=a if flags&1 else actors[(slot+3)%9]
  q=pa if flags&2 else actors[(slot+5)%9]
  put(m,a+0x86,pa|(flags<<24),4);put(m,pa+0x7a,q|((flags^0xb5)<<24),4)
  put(m,q+0x34,flags^0xad,1);put(m,pa+0x34,flags,1)
  put(m,a+0x1c,((0x80|(flags&0x7f))<<8)|5);put(m,a+0x56,role,1)
  put(m,a+0xaa,(0,1,2,0x3fff,0x4000,0x4001,0x8000,0xffff)[flags%8])
  put(m,a+0xa3,(0,1,0x80,0xff)[flags%4],1);put(m,a+0x66,0)
  yield 'independent-latched-allFLAGS34-role-gates-raw-alias',m,slot,None
 for byte in range(256):
  slot=byte%9;m,a,t,pa=seed(slot);put(m,a+0x1c,0x8005);put(m,a+0x56,byte,1)
  put(m,RAM+0x161,(byte^0x59)&255,1);put(m,a+0xaa,1);put(m,a+0xa3,byte,1)
  yield 'independent-allROLE-and-FRESH-BYTE-combinations',m,slot,None
 # Missing addresses are legal until the original branch consumes them.
 for slot,bad,gate in itertools.product(range(9),(0,0xff000000,0xdeadbeef,0x1c05b1),range(4)):
  m,a,t,pa=seed(slot);put(m,a+0x86,bad,4);put(m,a+0x70,slot%3)
  if gate==0:put(m,RAM+0x7c,0)
  elif gate==1:put(m,a+0x33,0,1)
  elif gate==2:put(m,a+0x1c,0x8005);put(m,RAM+0x161,0xff,1)
  else:put(m,a+0x1c,0x8005);put(m,a+0x56,0x80,1)
  yield 'independent-lazy-partner-all9-slots',m,slot,None
'''
derived=body[:at]+extra+body[at:]
derived=derived.replace('random.Random(0x195c2)','random.Random(0x62a411432)')
start=derived.index(' guards=0');end=derived.index(' assert bytes(mem.r_block',start)
guard_extra=''' guards=0
 if not args.natural_only:
  for slot,bad in itertools.product(range(9),(3,0x7f,0x100,0x7fff,0x8000,0xffff)):
   m,a,t,pa=seed(slot);put(m,a+0x70,bad);compare('independent10C60-prefix-all9',m,slot,[rng.getrandbits(32)for _ in range(8)],-0x10ca8,0x10ca8);guards+=1
   m,a,t,pa=seed(slot);put(m,pa+0x32,1,1);put(m,pa+0x70,bad);compare('independent217C8-prefix-all9',m,slot,[rng.getrandbits(32)for _ in range(8)],-0x217c8,0x217c8);guards+=1
  for slot,bad,route in itertools.product(range(9),(0,0x40000000,0xff000000,0xdeadbeef,0x1c05b1,0xff1c0f1c),('init','latched','resume')):
   m,a,t,pa=seed(slot)
   if route!='init':put(m,a+0x1c,0xff05)
   if route=='resume':put(m,pa+0x34,0xff,1);put(m,pa+0x7a,bad,4);pc=0x11462
   else:put(m,a+0x86,bad,4);pc=0x21756 if route=='init'else 0x11448
   compare('independent-'+route+'-prefix-all9',m,slot,[rng.getrandbits(32)for _ in range(8)],-pc,pc);guards+=1
  m,a,t,pa=seed();put(m,a+0x60,0x35);b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)();nd=ctypes.c_uint32();assert native.action62_oracle(b,d,0,ctypes.byref(nd))==-0x12614;guards+=1
'''
derived=derived[:start]+guard_extra+derived[end:]
# Use the actual frozen C exporter to independently recheck all captured
# actor/register bytes, producer archives and layout before exercising fixtures.
needle=' machine=mus.Machine'
capture_check=''' native.action62_capture.argtypes=[ctypes.c_void_p,ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_size_t)]
 manifest=meta;origin=Path(manifest['origin']);build=Path(manifest['build'])
 for filename,key in(('frozen-sources.zip','archive_sha256'),('source-manifest.json','source_manifest_sha256')):
  assert hashlib.sha256((build/filename).read_bytes()).hexdigest()==manifest[key]
 producer_sources=json.loads((build/'source-manifest.json').read_text())['sources_sha256']
 for filename in('include/wf_runtime.h','include/wf_actor.h'):
  assert hashlib.sha256((ROOT/filename).read_bytes()).hexdigest()==producer_sources[filename.replace('/','\\\\')]
 for cap in meta['captures']:
  directory=origin/f'policy-{cap["policy"]:03}';dump=(directory/'blocked-runtime.bin').read_bytes()
  assert hashlib.sha256((directory/'report.json').read_bytes()).hexdigest()==cap['report_sha256']
  for filename,h in cap['dumps_sha256'].items():assert hashlib.sha256((directory/filename).read_bytes()).hexdigest()==h
  output=ctypes.create_string_buffer(0x4000);dout=(ctypes.c_uint32*8)();layout=(ctypes.c_size_t*3)()
  assert native.action62_capture(ctypes.create_string_buffer(dump),output,dout,layout)==0
  snap=(HERE/cap['file']).read_bytes();aa=(directory/'blocked-actors.bin').read_bytes();dd=(directory/'blocked-dregs.bin').read_bytes()
  assert list(layout)==[7664,384,7340]and len(dump)==7664
  assert dump[:384]==(directory/'blocked-globals.bin').read_bytes()
  assert dump[384:384+9*0x10c]==aa==output.raw[0x5b0:0xf1c]
  assert dump[7340:7340+32]==dd==bytes(dout)
  assert snap==bytes(dout)+output.raw
 print('PASS independent capture all3 x9actors/D8/layout/provenance',flush=True)
'''
derived=derived.replace(needle,capture_check+needle,1)
(frozen/'tests/r18/action62_independent_extra_derived.py').write_text(derived)
hashes={str(x.relative_to(frozen)):hashlib.sha256(x.read_bytes()).hexdigest()for x in frozen.rglob('*')if x.is_file()}
with zipfile.ZipFile(out/'frozen-sources.zip','w',zipfile.ZIP_DEFLATED)as z:
 for name in hashes:z.write(frozen/name,name)
reports={}
for name,script in(('author-rerun','action62_verify_rom.py'),('independent-extra','action62_independent_extra_derived.py')):
 with(out/(name+'.log')).open('w')as log:r=subprocess.run([sys.executable,str(frozen/'tests/r18'/script),'--report',str(out/(name+'.json'))],cwd=frozen,stdout=log,stderr=subprocess.STDOUT)
 assert r.returncode==0,(name,r.returncode,(out/(name+'.log')).read_text()[-3800:])
 q=json.loads((out/(name+'.json')).read_text());assert q['status']=='PASS'and q['raw62_include_sha256']==owner_hash
 reports[name]={'cases':q['cases'],'guards':q['guards'],'quanta':q['sequence_quanta']};print('PASS',name,reports[name],flush=True)
assert reports['author-rerun']=={'cases':4101,'guards':23,'quanta':360}
result={'status':'PASS','owner_sha256':owner_hash,'reports':reports,'sources_sha256':hashes,'archive_sha256':hashlib.sha256((out/'frozen-sources.zip').read_bytes()).hexdigest(),'private_test_delta':['Native -UNDEBUG explicit in isolated copies','Extra fixtures and all9 guard prefixes only in derived private script; original19PC coverage requirement retained','Three captured HOST dumps independently re-exported using frozen C sizeof/offsetof and checked against all9actors/D8/producer hashes'],'limits':['Whole intact1C03E/21732/10C60/11432/10D04/2187E; no ROM function intercepts or global combat','rawRAM16KB/FG16KB/D0-D7/represented globals/RNG/descriptor compared; stack/A/CCR ABI outside scope','Captured typed globals projected over zero unrepresented arcade RAM; no native trajectory or full campaign claim']}
(out/'report.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items()if k!='sources_sha256'},indent=2))
