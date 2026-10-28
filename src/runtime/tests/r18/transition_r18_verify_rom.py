#!/usr/bin/env python3
"""RINGSIDE reset/map/palette transport against the unmodified 68000 ROM.

Only the board's vblank input bit is supplied by the oracle device. All game,
map and palette routines execute from the original, SHA1-checked bytes.
"""
import argparse, ctypes, hashlib, json, os, random, re, struct, subprocess, tempfile
from pathlib import Path
import machine68k as mus
p=argparse.ArgumentParser();p.add_argument('--report',type=Path);p.add_argument('--quick',action='store_true');args=p.parse_args()
here=Path(__file__).resolve().parent;root=here.parents[1];repo=root.parents[1];assets=root/'audit/runtime18-cd'
rom=(assets/'ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
snapshot=(here/'f98c_transition_reference_f88acad.bin').read_bytes();assert len(snapshot)==16472
original=bytearray(0x200000);original[:len(rom)]=rom;original[0x1c0000:0x1c2000]=snapshot[56:8248]
rng=random.Random(0x26e66)
def w(mem,at,value,n=2):mem[at:at+n]=(value&((1<<(n*8))-1)).to_bytes(n,'big')
cases=[('natural-noinput-frame1343-f98c',0,bytearray(original),struct.unpack_from('=8I',snapshot,24))]
cases.append(('natural-derived-scene0-to2-to0',3,bytearray(original),struct.unpack_from('=8I',snapshot,24)))
rescue_report_path=repo/'validation/local-20261026/action68-interleaved-natural-803-rescue-character02.json'
rescue_report=json.loads(rescue_report_path.read_bytes())
rescue_state=(repo/'validation/local-20261026'/rescue_report['terminal_state_name']).read_bytes()
assert rescue_report['status']=='PASS' and hashlib.sha256(rescue_state).hexdigest()==rescue_report['terminal_state_sha256']
assert rescue_report['verifier_sha256']==hashlib.sha256((here/'action68_sequence_verify_rom.py').read_bytes()).hexdigest()
assert rescue_report['bridge_sha256']==hashlib.sha256((here/'action_reversal_family_oracle_bridge.c').read_bytes()).hexdigest()
assert rescue_state[:4]==b'R68T' and len(rescue_state)==4+32+0x4000*2
rescue_mem=bytearray(original);rescue_mem[0x1c0000:0x1c4000]=rescue_state[36:36+0x4000]
rescue_mem[0xc0000:0xc4000]=rescue_state[36+0x4000:]
assert rescue_mem[0x1c0161]==0 and rescue_mem[0x1c1678]&0x80
assert int.from_bytes(rescue_mem[0x1c05b0+4*0x10c+0x60:0x1c05b0+4*0x10c+0x62],'big')==0x6a
cases.append(('derived-action68-rescue-to-real-scene',0,rescue_mem,struct.unpack_from('=8I',rescue_state,4)))
for royal in (0,1):
 for facing in (0,128):
  for ident in (0,4,8):
   mem=bytearray(original);w(mem,0x1c0162,0);w(mem,0x1c1678,0xc0 if royal else 0x80,1);w(mem,0x1c1679,facing,1);w(mem,0x1c15fc,1)
   a=0x1c05b0+ident*0x10c;w(mem,a,0x8000);w(mem,a+0x33,5,1);w(mem,a+0x60,0x6a);w(mem,a+0x56,8)
   cases.append((f'derived-stage0-royal{royal}-facing{facing}-slot{ident}',0,mem,tuple(rng.getrandbits(32) for _ in range(8))))
for mode in (0,2):
 for x,y in [(0x190,0x200),(0x350,0x200),(0x1e0,0x230),(0x320,0x280)]:
  mem=bytearray(original);w(mem,0x1c007e,mode);w(mem,0x1c1804,x);w(mem,0x1c1806,y);w(mem,0x1c15fc,1);w(mem,0x1c16d2,0x3123);w(mem,0x1c17fe,0x21);w(mem,0x1c006f,0,1)
  cases.append((f'derived-map{mode}-x{x}-y{y}',2,mem,tuple(rng.getrandbits(32) for _ in range(8))))
for config in (0,2,3):
 mem=bytearray(original);w(mem,0x1c007e,2);w(mem,0x1c15fc,config);w(mem,0x1c1804,0x350);w(mem,0x1c1806,0x200)
 cases.append((f'derived-text-palette-config{config}',2,mem,tuple(rng.getrandbits(32) for _ in range(8))))
mem=bytearray(original);w(mem,0x1c007e,0);w(mem,0x1c006f,16,1);w(mem,0x1c1804,0x350);w(mem,0x1c1806,0x200)
for i,v in enumerate((0x123,0x456,0x789,0xabc)):w(mem,0x100000+i*2,v)
cases.append(('derived-display-freeze-bit4',2,mem,tuple(rng.getrandbits(32) for _ in range(8))))
for i in range(3):
 mem=bytearray(original);mem[0x1c16d2:0x1c19ae]=rng.randbytes(0x2dc);mem[0x80000:0x84000]=rng.randbytes(0x4000);mem[0xc2000:0xc3000]=rng.randbytes(0x1000);w(mem,0x140008,0xa55a)
 cases.append((f'derived-reset{i}',1,mem,tuple(rng.getrandbits(32) for _ in range(8))))
if args.quick:cases=cases[:1]
# Every compared region is the board-visible effect of the actual routine.
global_ranges=[(int(a,16),int(n)) for a,n in re.findall(r' G\(0x([0-9a-f]+),[^,]+,(\d+)\);',(here/'transition_r18_oracle_bridge.c').read_text().split('#undef G')[0])]
ranges=[(0x1c0000+a,n) for a,n in global_ranges]+[(0x1c05b0,9*0x10c),(0x1c0f1c,0x218),(0x1c1134,0x120),(0x1c16d6,0x100),(0x1c1808,16),(0x1c182a,0x184),(0x80000,0x4000),(0xc0000,0x4000),(0x180000,0x4000),(0x186000,0x800),(0x188000,0x800),(0x140008,2)]
core=re.search(r'^CORE=(.*)$',(root/'Makefile').read_text(),re.M)[1].split()
r18=['src/wf_r18_transition.c','src/wf_r18_world.c','src/wf_r18_video.c','src/wf_r18_ring.c','src/wf_r18_display.c','src/wf_r18_hud.c','src/wf_r18_referee.c']
results=[];guard_results=[];coverage=set();postscene_state_sha256=None
with tempfile.TemporaryDirectory() as temp:
 lib=Path(temp)/'transition.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,*r18,'tests/r18/transition_r18_oracle_bridge.c','-o',str(lib)],cwd=root,check=True)
 native=ctypes.CDLL(str(lib));native.transition_r18_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint,ctypes.c_char_p]
 for label,op,mem,regs in cases:
  nb=ctypes.create_string_buffer(bytes(mem));nr=(ctypes.c_uint32*8)(*regs);status=native.transition_r18_oracle(nb,nr,op,str(assets).encode())
  machine=mus.Machine(mus.CPUType.M68000,2048);u=machine.cpu;m=machine.mem;m.w_block(0,bytes(mem));u.w_sr(0x2700)
  for i,v in enumerate(regs):u.w_reg(i,v)
  def enter(pc):u.w_sp(0x1d0000);m.w32(0x1d0000,0x70000);u.w_pc(pc)
  enter(0x1f6c if op==1 else 0x26e66 if op==2 else 0xf98c);phase=0
  for steps in range(300000):
   at=u.r_pc();coverage.add(at)
   # Physical board vblank polling: asserted while waiting for the edge,
   # deasserted after palette transport. No game owner is bypassed.
   if at in (0xfb5a,0xfc80,0x2a0a):m.w8(0x140026,4)
   if at in (0xfb8a,0xfcb0,0x2a88):m.w8(0x140026,0)
   if at==0x70000:
    if op==1 and phase==0:phase=1;enter(0x1fde);continue
    if op==3 and phase==0:phase=1;m.w8(0x1c1678,0xc0);enter(0xf98c);continue
    if op==2 and phase==0:
     phase=1;m.w16(0x1c15f4,m.r16(0x1c007e));m.w16(0x1c15f8,m.r16(0x1c007e));enter(0x2a06);continue
    if op==2 and phase==1:phase=2;enter(0xc1ea);continue
    break
   u.execute(1)
  assert u.r_pc()==0x70000,(label,'ROM runaway',hex(u.r_pc()))
  diffs=[]
  for at,n in ranges+([] if op==1 else [(0x140010,2),(0x100000,8)]):
   diffs.extend({'address':hex(at+i),'native':g,'rom':e} for i,(g,e) in enumerate(zip(nb.raw[at:at+n],m.r_block(at,n))) if g!=e)
  regdiff=[{'register':'D'+str(i),'native':hex(nr[i]),'rom':hex(u.r_reg(i))} for i in range(8) if nr[i]!=u.r_reg(i)]
  broad_diff=None
  if label=='derived-action68-rescue-to-real-scene':
   broad_diff={name:{'count':sum(a!=b for a,b in zip(nb.raw[at:at+n],m.r_block(at,n))),
                     'first':[(hex(at+i),a,b) for i,(a,b) in enumerate(zip(nb.raw[at:at+n],m.r_block(at,n))) if a!=b][:8]}
               for name,at,n in [('RAM16KB',0x1c0000,0x4000),('FG16KB',0xc0000,0x4000)]}
   assert not status and not regdiff and all(not value['count'] for value in broad_diff.values())
   postscene_state=b'R68S'+struct.pack('=8I',*nr)+nb.raw[0x1c0000:0x1c4000]+nb.raw[0xc0000:0xc4000]
   postscene_state_sha256=hashlib.sha256(postscene_state).hexdigest()
   (repo/'validation/local-20261026/action68-rescue-postscene-character02.state').write_bytes(postscene_state)
  results.append({'label':label,'native_status':status,'memory_differences':diffs,
                  'register_differences':regdiff,'broad_diff':broad_diff});machine.cleanup()
 # Missing/unusable scene data must stop the real transport; none of these
 # cases substitutes a fallback map or palette.
 guard_root=Path(temp)/'guards';guard_root.mkdir()
 pack=(assets/'R18RING.BIN').read_bytes()
 for label,mode,payload in [('unsupported-scene',1,pack),('missing-file',0,None),('truncated-file',0,pack[:100]),('wrong-mode',2,pack),('over-capacity',0,pack+bytes(0x1f000))]:
  folder=guard_root/label;folder.mkdir()
  if payload is not None:(folder/('R18MAP2.BIN' if mode==2 else 'R18RING.BIN')).write_bytes(payload)
  mem=bytearray(original);w(mem,0x1c007e,mode);nb=ctypes.create_string_buffer(bytes(mem));nr=(ctypes.c_uint32*8)(*cases[0][3])
  status=native.transition_r18_oracle(nb,nr,2,str(folder).encode());guard_results.append({'label':label,'native_status':status,'passed':status==-0x26e66})
 if os.name=='nt':assert ctypes.windll.kernel32.FreeLibrary(ctypes.c_void_p(native._handle))
failed=[r for r in results if r['native_status'] or r['memory_differences'] or r['register_differences']]
report={'rom_sha1':hashlib.sha1(rom).hexdigest(),
        'snapshot_sha256':hashlib.sha256(snapshot).hexdigest(),
        'rescue_terminal_state_sha256':hashlib.sha256(rescue_state).hexdigest(),
        'rescue_sequence_report_sha256':hashlib.sha256(rescue_report_path.read_bytes()).hexdigest(),
        'postscene_state_sha256':postscene_state_sha256,
        'verifier_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        'sources_sha256':{name:hashlib.sha256((root/name).read_bytes()).hexdigest()
                          for name in ['src/wf_r17_dependencies.c',*r18,
                                       'tests/r18/transition_r18_oracle_bridge.c']},
        'count':len(results),'passed':len(results)-len(failed),'natural_count':1,
        'guards':guard_results,'device_model':'only polled vblank input $140026 bit2',
        'rom_pc_coverage':[hex(x) for x in sorted(coverage)],'results':results}
if args.report:args.report.parent.mkdir(exist_ok=True,parents=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
print('R18 transition ROM:',report['passed'],'/',report['count'],'; guards',sum(r['passed'] for r in guard_results),'/',len(guard_results))
for r in failed[:5]:print(r['label'],'status',r['native_status'],'memory',r['memory_differences'][:8],'regs',r['register_differences'])
raise SystemExit(bool(failed) or any(not r['passed'] for r in guard_results))
