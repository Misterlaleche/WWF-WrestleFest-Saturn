"""Whole original514E input-source oracle; no emulation of a new combat run.

Existing native7b actors are a declared unit fixture. All other cases are
isolated RAM/MMIO/register fixtures. No engine, seed, result or input strategy
is executed. The native adapter's actual input_word body is extracted verbatim.
"""
from pathlib import Path
import argparse,ctypes as ct,hashlib,itertools,json,os,random,re,shutil,subprocess,tempfile,zipfile
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser();p.add_argument('output',type=Path);p.add_argument('--adapter-only',action='store_true');p.add_argument('--smoke',action='store_true');a=p.parse_args()
out=a.output.resolve();out.mkdir(parents=True,exist_ok=True);assert not(out/'report.json').exists()
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
rng=random.Random(0x514e8a);cases=guards=sequences=adapter_checks=coherence_checks=0;coverage=set();call_addresses=[]
def put(b,at,v,n=2):b[at:at+n]=(v&((1<<(n*8))-1)).to_bytes(n,'big')
def value(b,at,n=2):return int.from_bytes(b[at:at+n],'big')
def seed():
 b=bytearray(rng.randbytes(0x4000))
 for i in range(9):b[0x5b0+i*0x10c]=0
 return b
def actor(b,i,role,source):
 at=0x5b0+i*0x10c;b[at]=0x80;b[at+0x56]=role;put(b,at+0x8a,source,4)
def function_body(source,name):
 start=source.index('static wf_status_t '+name+'(');brace=source.index('{',start);end=brace+1;depth=1
 while depth:depth+=(source[end]=='{')-(source[end]=='}');end+=1
 return source[start:end]
with tempfile.TemporaryDirectory(prefix='wf-input-source-',ignore_cleanup_errors=os.name=='nt')as td:
 frozen=Path(td)
 for name in('src','include','generated'):shutil.copytree(ROOT/name,frozen/name)
 bridge=frozen/'tests/r18/input_source_oracle_bridge.c';bridge.parent.mkdir(parents=True);shutil.copy2(ROOT/'tests/r18/input_source_oracle_bridge.c',bridge)
 entry=(frozen/'src/wf_saturn_entry.c').read_text();body=function_body(entry,'input_word')
 host_body=function_body((ROOT/'tests/r18/full_flow_combat_probe.c').read_text(),'combat_input')
 bridge.write_text(bridge.read_text()+'\n'+body+'''\nint input_source_native_adapter(uint32_t at,uint16_t*out,unsigned pad){
 memset(&gameplay,0,sizeof gameplay);gameplay.input_hw_low[0]=(uint8_t)pad;
 for(unsigned i=1;i<9;i++)gameplay.input_hw_low[i]=(uint8_t)(0x80u+i);
 return input_word(&machine,at,out,NULL);
}\n''')
 bridge.write_text(bridge.read_text()+'\n#define combat_gc gameplay\n'+host_body+'''\n#undef combat_gc
int input_source_host_adapter(uint32_t at,uint16_t*out,unsigned pad){
 memset(&gameplay,0,sizeof gameplay);gameplay.input_hw_low[0]=(uint8_t)pad;
 return combat_input(&machine,at,out,NULL);
}\n''')
 make=(ROOT/'Makefile').read_text();sources=re.search(r'^CORE=(.*)$',make,re.M)[1].split()+re.search(r'^R18_HOST=(.*)$',make,re.M)[1].split()
 hashes={str(f.relative_to(frozen)):hashlib.sha256(f.read_bytes()).hexdigest()for f in frozen.rglob('*')if f.is_file()}
 with zipfile.ZipFile(out/'frozen-sources.zip','w',zipfile.ZIP_DEFLATED)as z:
  for name in hashes:z.write(frozen/name,name)
 lib=out/'input-source.dll';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Werror','-Iinclude',*sources,'tests/r18/input_source_oracle_bridge.c','-o',str(lib)],cwd=frozen,check=True)
 native=ct.CDLL(str(lib));native.input_source_oracle.argtypes=[ct.c_void_p,ct.POINTER(ct.c_uint32),ct.POINTER(ct.c_uint16),ct.c_uint,ct.c_uint32,ct.POINTER(ct.c_uint32),ct.POINTER(ct.c_uint32)];native.input_source_native_adapter.argtypes=[ct.c_uint32,ct.POINTER(ct.c_uint16),ct.c_uint]
 native.input_source_host_adapter.argtypes=native.input_source_native_adapter.argtypes
 def adapt():
  global adapter_checks,coherence_checks
  for high,port,pad in itertools.product((0,0x40000000,0x80000000,0xff000000),range(4),range(256)):
   at=high|0x140020|port*2;word=ct.c_uint16(0x1234);st=native.input_source_native_adapter(at,ct.byref(word),pad);want=0xff00|pad if port==0 else 0xffff
   assert st==0 and word.value==want,('native adapter',hex(at),hex(pad),st,hex(word.value),hex(want));adapter_checks+=1
   if high==0:
    hw=ct.c_uint16(0x1234);hs=native.input_source_host_adapter(at,ct.byref(hw),pad)
    assert hs==st and hw.value==word.value,('HOST/native WORD',hex(at),pad);coherence_checks+=1
  for at in(0,0x14001e,0x140021,0x140023,0x140025,0x140027,0x140028,0xdeadbeef):
   word=ct.c_uint16(0x1234);st=native.input_source_native_adapter(at,ct.byref(word),0);assert st!=0 and word.value==0x1234,('native adapter invalid',hex(at),st,word.value);adapter_checks+=1
   hw=ct.c_uint16(0x1234);hs=native.input_source_host_adapter(at,ct.byref(hw),0);assert hs==st and hw.value==word.value;coherence_checks+=1
  assert native.input_source_native_adapter(0x140020,None,0)!=0;adapter_checks+=1
  assert native.input_source_host_adapter(0x140020,None,0)!=0;coherence_checks+=1
 if not a.adapter_only:
  machine=mus.Machine(mus.CPUType.M68000,2048);machine.mem.w_block(0,rom)
  def original(raw,d,words,stop_slot=None):
   machine.mem.w_block(0x1c0000,bytes(raw))
   for port,w in enumerate(words):machine.mem.w16(0x140020+2*port,w)
   for i,v in enumerate(d):machine.cpu.w_reg(i,v)
   machine.cpu.w_sp(0x1d0000);machine.mem.w32(0x1d0000,0x70000);machine.cpu.w_pc(0x514e);addresses=[]
   for _ in range(10000):
    pc=machine.cpu.r_pc()
    if pc==0x70000:break
    coverage.add(pc)
    if pc==0x517c:
     if stop_slot is not None and machine.cpu.r_reg(8)==0x1c05b0+stop_slot*0x10c:break
     addresses.append(machine.cpu.r_reg(9)&0xffffff)
    machine.cpu.execute(1)
   assert machine.cpu.r_pc()==(0x517c if stop_slot is not None else 0x70000)
   return bytes(machine.mem.r_block(0x1c0000,0x4000)),[machine.cpu.r_reg(i)for i in range(8)],addresses
  def check(name,raw,d,words,flags=0,denied=0,stop_slot=None):
   global cases,guards
   want,wd,wa=original(raw,d,words,stop_slot);buf=ct.create_string_buffer(bytes(raw));nd=(ct.c_uint32*8)(*d);nw=(ct.c_uint16*4)(*words);status=(ct.c_uint32*3)();reads=(ct.c_uint32*16)()
   st=native.input_source_oracle(buf,nd,nw,flags,denied,status,reads)
   expected=1 if stop_slot is not None else 0
   assert st==expected,(name,'status',st,expected,list(status))
   got=buf.raw[:0x4000];assert got==want,(name,'RAM',[(hex(i),x,y)for i,(x,y)in enumerate(zip(got,want))if x!=y][:12])
   assert list(nd)==wd,(name,'D0-D7',list(nd),wd)
   called=list(reads)[:status[2]]
   if stop_slot is None:assert called==wa,(name,'physical requests',called,wa)
   else:
    assert list(status)[:2]==[0x514e,0x517c],(name,'guard PC',list(status));assert called[:len(wa)]==wa
    if flags&1:assert not called
    else:assert called[-1]==value(raw,0x5b0+stop_slot*0x10c+0x8a,4)&0xffffff
    guards+=1
   cases+=1;return bytearray(got),list(nd)
  report_path=ROOT.parents[1]/'validation/local-20261024/hardware-input/beetle-full-flow-stage7b-human-tag/report.json';captured=json.loads(report_path.read_text())
  raw=seed();raw[0x83]=1
  for i,b in enumerate(captured['actors_at_block']):raw[0x5b0+i*0x10c:0x5b0+(i+1)*0x10c]=bytes.fromhex(b)
  check('native7b TAG shared140020; declared padA unit fixture',raw,[rng.getrandbits(32)for _ in range(8)],[0xffef,0xffff,0xffff,0xffff])
  if not a.smoke:
   for port,pad,pipeline in itertools.product(range(4),range(256),range(2)):
    raw=seed();raw[0x83]=(0xa4|pipeline);slot=(pad+port)%9;actor(raw,slot,0x40 if pad&1 else 0,(0,0x40000000,0x80000000,0xff000000)[pad%4]|(0x140020+port*2))
    words=[rng.getrandbits(16)for _ in range(4)];words[port]=(words[port]&0xff00)|pad;check(f'port{port}/pad{pad}/pipe{pipeline}',raw,[rng.getrandbits(32)for _ in range(8)],words)
   for seq in range(20):
    raw=seed();d=[rng.getrandbits(32)for _ in range(8)]
    for i,role in enumerate((0,0x40,0x80,0xc0)):actor(raw,i,role,0x140020+(seq%4)*2)
    for frame in range(16):
     raw[0x83]=frame&1;words=[0xff00|rng.randrange(256)for _ in range(4)];raw,d=check(f'sequence{seq}/{frame}',raw,d,words);sequences+=1
  for source in(0,0x140021,0x140027,0xdeadbeef,0x40140023):
   raw=seed();raw[0x83]=1;actor(raw,0,0,0x140020);actor(raw,1,0x80,0xdeadbeef);actor(raw,2,0x40,source);actor(raw,3,0,0x140024)
   check(f'progressive bad source{source:x}',raw,[rng.getrandbits(32)for _ in range(8)],[0xffcf,0xffef,0xffdf,0xffbf],stop_slot=2)
  raw=seed();raw[0x83]=1;actor(raw,0,0,0x140020);actor(raw,2,0,0x140024)
  check('physical read failure after successful actor0',raw,[rng.getrandbits(32)for _ in range(8)],[0xffcf]*4,flags=2,denied=0x140024,stop_slot=2)
  check('missing callback at first consumed word',raw,[rng.getrandbits(32)for _ in range(8)],[0xffcf]*4,flags=1,stop_slot=0)
  raw=seed()
  for i in range(9):actor(raw,i,0x80 if i&1 else 0xc0,0xdeadbeef)
  check('CPU roles never consume missing hardware',raw,[rng.getrandbits(32)for _ in range(8)],[0]*4,flags=1)
  raw=seed();check('inactive slots never consume missing hardware',raw,[rng.getrandbits(32)for _ in range(8)],[0]*4,flags=1)
  raw=seed();raw[0x83]=1;actor(raw,0,0,0x140020);check('physical read cannot clobber CPU regs',raw,[rng.getrandbits(32)for _ in range(8)],[0xffcf]*4,flags=4)
  assert bytes(machine.mem.r_block(0,len(rom)))==rom;machine.cleanup()
 adapt()
 report={'status':'PASS','whole_ROM_cases':cases,'prefix_guards':guards,'sequence_quanta':sequences,'native_adapter_controls':adapter_checks,'host_native_adapter_controls':coherence_checks,'coverage_pcs':[hex(pc)for pc in sorted(coverage)],'sources_sha256':hashes,'adapter_body_sha256':hashlib.sha256(body.encode()).hexdigest(),'host_adapter_body_sha256':hashlib.sha256(host_body.encode()).hexdigest(),'native7b_fixture_sha256':hashlib.sha256(report_path.read_bytes()).hexdigest()if not a.adapter_only else None,'limits':['Isolated input owner and existing actor capture unit fixtures, no new match or strategy run','514E full rawRAM16KB +D0-D7 +physical request order; runtime ABI has no address-register/CCR fields','Original ROM executes intact, no intercepted functions; failure controls stop before the consumed517C read','Adapter uses sampled port0 only; high byteFF and other portsFFFF; no credits or coins generated']}
 (out/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items()if k not in('sources_sha256','coverage_pcs')},indent=2))
