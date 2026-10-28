"""Compare raw $6B ring-walk descriptor and exit branches with intact ROM."""
from pathlib import Path
import ctypes,hashlib,itertools,json,os,random,re,shutil,struct,subprocess,tempfile
import machine68k as mus

ROOT=Path(__file__).resolve().parents[2]
HERE=Path(__file__).resolve().parent
RAM=0x1c0000
ROM=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes()
assert hashlib.sha1(ROM).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
assert int.from_bytes(ROM[0x12614+0x6b*4:0x12618+0x6b*4],'big')==0x19bba
meta=json.loads((HERE/'action6b_manifest.json').read_bytes())
source_dir=ROOT.parent.parent/meta['source_directory']
trace=(source_dir/'trace.csv').read_bytes()
assert hashlib.sha256(trace).hexdigest()==meta['source_trace_sha256']
natural=[]
for item in meta['captures']:
 data=(HERE/f'action6b_snapshot_noinput_{item["frame"]}-slot{item["slot"]}.bin').read_bytes()
 assert len(data)==16472 and hashlib.sha256(data).hexdigest()==item['sha256']
 assert data==(source_dir/item['source_snapshot']).read_bytes()
 assert list(struct.unpack_from('=6I',data))[:4]==[item['frame'],item['slot'],1,0x1c03e]
 line=trace.splitlines()[item['trace_line']-1]
 assert line.startswith(f'{item["frame"]},{item["slot"]},dispatch,1c03e,'.encode())
 assert b',8005,0000,006b,' in line and line.endswith(item['source_snapshot'].encode())
 natural.append((item,data))
rng=random.Random(0x19bc0)
actors=[RAM+0x5b0+i*0x10c for i in range(9)]
coverage=set();groups={};descriptors={};exits=0;count=0

def put(m,at,value,n=2):
 m[at:at+n]=(value&((1<<(8*n))-1)).to_bytes(n,'big')

def fixture(slot,ident,flags32,flags33,flags56,flags74,flags75,latch,at_target):
 m=bytearray(0x200000);m[:len(ROM)]=ROM;m[RAM:RAM+0x4000]=rng.randbytes(0x4000)
 a=actors[slot]
 put(m,a+2,ident);put(m,a+0x1c,latch);put(m,a+0x20,0x8005)
 put(m,a+0x60,0x6b);put(m,a+0x22,1);put(m,a+0x24,0)
 put(m,a+0x32,flags32,1);put(m,a+0x33,flags33,1)
 put(m,a+0x56,flags56,1);put(m,a+0x74,flags74);put(m,a+0x75,flags75,1)
 tx=0x42c if flags33&0x80 else 0x210
 put(m,a+6,tx if at_target else (tx+0x40)&0xffff)
 put(m,a+10,0x120 if at_target else 0x170)
 put(m,a+0xbe,tx);put(m,a+0xc0,0x120)
 return m,a

def fixtures():
 for item,data in natural:
  m=bytearray(0x200000);m[:len(ROM)]=ROM;m[RAM:RAM+0x2000]=data[56:8248]
  yield 'natural-noinput',m,item['slot'],struct.unpack_from('=8I',data,24)
 for slot,ident,flags32,flags33,flags56,flags74,flags75,latch,arrived in itertools.product(
     (0,4,8),range(12),(0,8),(0,0x20,0x80,1,0x81),(0,0x80),
     (0,0x8000),(0,1),(5,0x8005),(False,True)):
  # Keep distinct branches while bounding the full Cartesian product.
  if slot!=ident%3*4 and (flags56 or flags75):continue
  m,a=fixture(slot,ident,flags32,flags33,flags56,flags74,flags75,latch,arrived)
  yield ('arrived' if arrived else 'moving'),m,slot,None

core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split()
with tempfile.TemporaryDirectory(prefix='wf-action6b-',ignore_cleanup_errors=os.name=='nt') as temp:
 frozen=Path(temp)
 for part in ('src','include','generated'):shutil.copytree(ROOT/part,frozen/part)
 bridge='tests/r18/action6b_oracle_bridge.c';(frozen/'tests/r18').mkdir(parents=True)
 shutil.copy2(HERE/'action6b_oracle_bridge.c',frozen/bridge)
 source_hash=hashlib.sha256((frozen/'src/wf_r17_actions_r11.c').read_bytes()).hexdigest()
 lib=frozen/'action.so'
 subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Werror','-Iinclude',*core,bridge,'-o',str(lib)],cwd=frozen,check=True)
 native=ctypes.CDLL(str(lib));native.action6b_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint,ctypes.POINTER(ctypes.c_uint32)]
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
  status=native.action6b_oracle(b,d,slot,ctypes.byref(nd))
  assert status==0,(name,count,'native blocked',hex(-status))
  assert nd.value==descriptor,(name,count,'descriptor',hex(nd.value),hex(descriptor))
  descriptors[f'{descriptor:06X}']=descriptors.get(f'{descriptor:06X}',0)+1
  for at,n in ((RAM,0x4000),(0xc0000,0x4000)):
   expected=bytes(mem.r_block(at,n));got=b.raw[at:at+n]
   assert got==expected,(name,count,'RAM/FG',[(hex(at+j),x,y) for j,(x,y) in enumerate(zip(got,expected)) if x!=y][:12])
  assert list(d)==[cpu.r_reg(i) for i in range(8)],(name,count,'D0-D7')
  exits+=(int.from_bytes(b.raw[actors[slot]+0x20:actors[slot]+0x22],'big')&0xff)==0
  count+=1;groups[name]=groups.get(name,0)+1
 assert set(descriptors)=={'0115F2','01160A','011622','01163A'},descriptors
 assert groups['moving']==2880 and groups['arrived']==2880 and groups['natural-noinput']==6
 assert exits==2880,(groups,exits)
 assert bytes(mem.r_block(0,len(ROM)))==ROM
 machine.cleanup()
 if os.name=='nt':assert ctypes.windll.kernel32.FreeLibrary(ctypes.c_void_p(native._handle))
report={'status':'PASS','cases':count,'groups':groups,'descriptors':descriptors,'exit_to_idle_cases':exits,
        'natural_captures':[{'frame':item['frame'],'slot':item['slot'],'sha256':item['sha256']}
                            for item,_ in natural],
        'rom_sha1':hashlib.sha1(ROM).hexdigest(),
        'source_sha256':source_hash,'verifier_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        'bridge_sha256':hashlib.sha256((HERE/'action6b_oracle_bridge.c').read_bytes()).hexdigest(),
        'coverage':[hex(pc) for pc in sorted(coverage)],
        'compared':['RAM16KB','FG16KB','D0-D7','tick descriptor'],
        'limits':['Synthetic actor states, not a natural match capture or full ring-exit trajectory.',
                  'Original stack/A/CCR and audio IO outside typed combat ABI.']}
out=ROOT.parent.parent/'validation/local-20261026/action6b-ring-walk-matrix.json'
out.write_text(json.dumps(report,indent=2)+'\n')
print('PASS',count,'raw6B ROM comparisons; moving',groups.get('moving',0),'arrived',groups.get('arrived',0))
