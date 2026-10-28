"""Differential $68 ring exit branches against the unmodified $1C03E ROM."""
from pathlib import Path
import ctypes,hashlib,itertools,json,os,random,re,shutil,struct,subprocess,tempfile
import machine68k as mus

ROOT=Path(__file__).resolve().parents[2]
HERE=Path(__file__).resolve().parent
RAM=0x1c0000
ROM=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes()
assert hashlib.sha1(ROM).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
assert int.from_bytes(ROM[0x12614+0x68*4:0x12618+0x68*4],'big')==0x1981e
meta=json.loads((HERE/'action68_manifest.json').read_bytes())
capture=(HERE/'action68_snapshot_noinput_803.bin').read_bytes()
assert hashlib.sha256(capture).hexdigest()==meta['snapshot_sha256']
assert len(capture)==meta['snapshot_bytes']==16472
assert list(struct.unpack_from('=6I',capture))==meta['header']==[803,4,1,0x1c03e,302,0]
assert hashlib.sha256((ROOT.parent.parent/meta['source']).read_bytes()).hexdigest()==meta['snapshot_sha256']
trace=(ROOT.parent.parent/Path(meta['source']).parent/'trace.csv').read_bytes()
assert hashlib.sha256(trace).hexdigest()==meta['source_trace_sha256']
assert trace.splitlines()[meta['source_trace_line']-1].startswith(b'803,4,dispatch,1c03e,')
assert b',0068,' in trace.splitlines()[meta['source_trace_line']-1]
assert trace.splitlines()[meta['source_trace_line']-1].endswith(b'snapshot-00302.bin')
natural=[]
for item in meta['captures']:
 data=(HERE/f'action68_snapshot_noinput_{item["frame"]}.bin').read_bytes()
 assert len(data)==16472 and hashlib.sha256(data).hexdigest()==item['sha256']
 original=(ROOT.parent.parent/Path(meta['source']).parent/item['source_snapshot']).read_bytes()
 assert data==original
 assert list(struct.unpack_from('=6I',data))[:4]==[item['frame'],4,1,0x1c03e]
 line=trace.splitlines()[item['trace_line']-1]
 assert line.startswith(f'{item["frame"]},4,dispatch,1c03e,'.encode())
 assert b',0068,' in line and line.endswith(item['source_snapshot'].encode())
 actor=0x5b0+4*0x10c
 assert int.from_bytes(data[56+actor+0x44:56+actor+0x46],'big')==item['phase']
 natural.append((item,data))
rng=random.Random(0x19832)
actors=[RAM+0x5b0+i*0x10c for i in range(9)]
coverage=set();groups={};descriptors={};successors={};count=0

def put(m,at,value,n=2):
 m[at:at+n]=(value&((1<<(8*n))-1)).to_bytes(n,'big')

def seed(ident,phase,mode,latch):
 m=bytearray(0x200000);m[:len(ROM)]=ROM;m[RAM:RAM+0x4000]=rng.randbytes(0x4000)
 slot=ident%9;a=actors[slot]
 put(m,RAM+0x161,mode,1)
 put(m,a+2,ident);put(m,a+0x1c,latch);put(m,a+0x20,0x8005)
 put(m,a+0x60,0x68);put(m,a+0x44,phase);put(m,a+0x22,1);put(m,a+0x24,0)
 put(m,a+0x32,0,1);put(m,a+0x33,0,1);put(m,a+0x56,0,1)
 put(m,a+0x74,0);put(m,a+0x75,0,1);put(m,a+0x37,0,1)
 return m,a,slot

def fixtures():
 for item,data in natural:
  m=bytearray(0x200000);m[:len(ROM)]=ROM;m[RAM:RAM+0x2000]=data[56:8248]
  yield f'natural-noinput-TAG{item["frame"]}',m,4,struct.unpack_from('=8I',data,24)
 for ident,mode,latch,frame,timer,signal,legal in itertools.product(
     range(12),(0,1),(5,0x8005),(0,1,0xfe),(0,1),(0,0x10),(0,1)):
  m,a,slot=seed(ident,0,mode,latch)
  put(m,a+0x24,frame);put(m,a+0x22,timer)
  put(m,a+0x37,signal,1);put(m,a+0x33,legal,1)
  yield 'phase0',m,slot,None
 for ident,phase,mode,latch,speed,flags,role,rel,alternate,arrived in itertools.product(
     range(12),(1,2,3),(0,1),(5,0x8005),(0,8),(0,0x20),
     (0,0x80),(0,0x8000),(0,1),(False,True)):
  if role and (ident&1)!=(phase&1):continue
  m,a,slot=seed(ident,phase,mode,latch)
  put(m,a+0x32,speed,1);put(m,a+0x33,flags,1);put(m,a+0x56,role,1)
  put(m,a+0x74,rel);put(m,a+0x75,alternate,1)
  if phase==1:
   tx=0x3b0 if ident&1 else 0x142
   put(m,a+6,tx if arrived else tx+0x20);put(m,a+10,0xe0 if arrived else 0x100)
  elif phase==2:
   put(m,a+6,0x279 if arrived else 0x2b9)
   put(m,a+10,0xe0 if arrived else 0x100)
  else:
   put(m,a+6,0x279);put(m,a+10,0x100 if arrived else 0xe0)
  put(m,a+0xbe,tx if phase==1 else 0x279)
  put(m,a+0xc0,0xe0)
  yield f'phase{phase}-'+('arrived' if arrived else 'moving'),m,slot,None

core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split()
with tempfile.TemporaryDirectory(prefix='wf-action68-',ignore_cleanup_errors=os.name=='nt') as temp:
 frozen=Path(temp)
 for part in ('src','include','generated'):shutil.copytree(ROOT/part,frozen/part)
 bridge='tests/r18/action68_oracle_bridge.c';(frozen/'tests/r18').mkdir(parents=True)
 shutil.copy2(HERE/'action68_oracle_bridge.c',frozen/bridge)
 source_hash=hashlib.sha256((frozen/'src/wf_r17_actions_r11.c').read_bytes()).hexdigest()
 lib=frozen/'action.so'
 subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Werror','-Iinclude',*core,bridge,'-o',str(lib)],cwd=frozen,check=True)
 native=ctypes.CDLL(str(lib));native.action68_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint,ctypes.POINTER(ctypes.c_uint32)]
 machine=mus.Machine(mus.CPUType.M68000,2048);cpu,mem=machine.cpu,machine.mem
 for name,m,slot,initial_regs in fixtures():
  regs=list(initial_regs) if initial_regs is not None else [rng.getrandbits(32) for _ in range(8)]
  m[0xc0000:0xc4000]=rng.randbytes(0x4000)
  mem.w_block(0,bytes(m));cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_pc(0x1c03e)
  for i in range(15):cpu.w_reg(i,regs[i] if i<8 else 0)
  cpu.w_reg(8,actors[slot]);descriptor=0
  for _ in range(20000):
   pc=cpu.r_pc();coverage.add(pc)
   if pc==0x1c0c8:descriptor=cpu.r_reg(9)
   if pc==0x70000:break
   cpu.execute(1)
  assert cpu.r_pc()==0x70000,(name,count,'ROM runaway',hex(cpu.r_pc()))
  b=ctypes.create_string_buffer(bytes(m));d=(ctypes.c_uint32*8)(*regs);nd=ctypes.c_uint32()
  status=native.action68_oracle(b,d,slot,ctypes.byref(nd))
  assert status==0,(name,count,'native blocked',hex(-status))
  assert nd.value==descriptor,(name,count,'descriptor',hex(nd.value),hex(descriptor))
  for at,n in ((RAM,0x4000),(0xc0000,0x4000)):
   expected=bytes(mem.r_block(at,n));got=b.raw[at:at+n]
   assert got==expected,(name,count,'RAM/FG',[(hex(at+j),x,y) for j,(x,y) in enumerate(zip(got,expected)) if x!=y][:12])
  assert list(d)==[cpu.r_reg(i) for i in range(8)],(name,count,'D0-D7')
  descriptors[f'{descriptor:06X}']=descriptors.get(f'{descriptor:06X}',0)+1
  next_action=int.from_bytes(b.raw[actors[slot]+0x60:actors[slot]+0x62],'big')&0xff
  successors[f'{next_action:02X}']=successors.get(f'{next_action:02X}',0)+1
  count+=1;groups[name]=groups.get(name,0)+1
 assert set(descriptors)=={'01981E','0115F2','01160A','011622','01163A'},descriptors
 assert {'68','69','6A','7A'}<=set(successors),successors
 assert bytes(mem.r_block(0,len(ROM)))==ROM
 machine.cleanup()
 if os.name=='nt':assert ctypes.windll.kernel32.FreeLibrary(ctypes.c_void_p(native._handle))
report={'status':'PASS','cases':count,'groups':groups,'descriptors':descriptors,
        'successor_selectors':successors,'rom_sha1':hashlib.sha1(ROM).hexdigest(),
        'natural_captures':[{'frame':item['frame'],'phase':item['phase'],'sha256':item['sha256']}
                            for item,_ in natural],
        'source_sha256':source_hash,'verifier_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        'bridge_sha256':hashlib.sha256((HERE/'action68_oracle_bridge.c').read_bytes()).hexdigest(),
        'coverage':[hex(pc) for pc in sorted(coverage)],
        'compared':['RAM16KB','FG16KB','D0-D7','tick descriptor'],
        'limits':['Synthetic actor states, not a natural ring-exit trajectory with physics.',
                  'Original stack/A/CCR and audio IO outside typed combat ABI.']}
out=ROOT.parent.parent/'validation/local-20261026/action68-ring-exit-matrix.json'
out.write_text(json.dumps(report,indent=2)+'\n')
print('PASS',count,'raw68 ROM comparisons; successors',successors)
