"""Royal GAME OVER1AFC..1B1D against intact ROM; no intercepted instructions."""
from pathlib import Path
import ctypes as ct,hashlib,json,os,random,subprocess,tempfile
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2]
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes()
assert hashlib.sha256(rom).hexdigest()=='265ac89deeb7e708ac6fda724356bfd6af1c9012928a4e551e6e2faa424c2306'
rng=random.Random(0x1afc);cases=0
with tempfile.TemporaryDirectory(prefix='wf-royal-result-',ignore_cleanup_errors=os.name=='nt') as t:
 lib=Path(t)/'result.so'
 subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Werror','-Iinclude','src/wf_r18_result_screen.c','src/wf_r18_video.c','src/wf_r18_frontend_palette.c','src/wf_r17_m68k_shadow.c','tests/r18/royal_result_oracle_bridge.c','-o',str(lib)],cwd=ROOT,check=True)
 fn=ct.CDLL(str(lib)).wf_royal_result_oracle
 fn.argtypes=[ct.c_void_p,ct.c_void_p,ct.POINTER(ct.c_uint32),ct.c_void_p,ct.c_void_p,ct.POINTER(ct.c_uint32),ct.c_void_p,ct.POINTER(ct.c_uint)]
 for world in range(7):
  for repeat in range(4):
   raw=bytearray(rng.randbytes(0x4000));raw[0x15f4:0x15f6]=world.to_bytes(2,'big')
   fg=rng.randbytes(0x4000);pal=rng.randbytes(0x9000);regs=[rng.getrandbits(32)for _ in range(8)]
   m=mus.Machine(mus.CPUType.M68000,2048);c,mem=m.cpu,m.mem
   mem.w_block(0,rom);mem.w_block(0x1c0000,bytes(raw));mem.w_block(0xc0000,fg);mem.w_block(0x180000,pal)
   for i,x in enumerate(regs):c.w_reg(i,x)
   c.w_sp(0x1d0000);c.w_pc(0x1afc);events=[];snaps=[];edges=0
   def observe(k,v):
    events.append([k,v,*[c.r_reg(i)for i in range(8)]])
    snaps.append(bytes(mem.r_block(0xc0000,0x4000))+bytes(mem.r_block(0x180000,0x4000))+bytes(mem.r_block(0x188000,0x800))+bytes(mem.r_block(0x186000,0x800)))
   for _ in range(60000):
    pc=c.r_pc()
    if pc==0x1b1e:break
    if pc==0x2a0a:observe(0,1);mem.w8(0x140026,4)
    if pc==0x2a88:observe(1,0);observe(0,0);mem.w8(0x140026,0)
    if pc==0x1b14:observe(1,0)
    if pc==0x21e6:observe(2,(c.r_reg(0)&0xffff)+1)
    if pc==0x21ea:mem.w8(0x140026,4);edges+=1
    if pc==0x21f4:mem.w8(0x140026,0)
    c.execute(1)
   assert c.r_pc()==0x1b1e,hex(c.r_pc())
   rb=ct.create_string_buffer(rom);nb=ct.create_string_buffer(bytes(raw));nr=(ct.c_uint32*8)(*regs)
   nf=ct.create_string_buffer(fg);np=ct.create_string_buffer(pal);ev=(ct.c_uint32*50)();ss=ct.create_string_buffer(0xd000*5);count=ct.c_uint()
   assert fn(rb,nb,nr,nf,np,ev,ss,ct.byref(count))==0
   assert count.value==len(events)==5 and edges==97
   for i,e in enumerate(events):
    assert list(ev[i*10:i*10+2])==e[:2],('event',world,i)
    # $2A06 preserves caller D0 via MOVEM. Its temporary loop registers
    # at the physical polls are not the translated payload's public shadow.
    if i>=3:assert list(ev[i*10+2:i*10+10])==e[2:],('Dregs after2A06',world,i)
    assert ss.raw[i*0xd000:i*0xd000+0x9000]==snaps[i],('snapshot',world,i)
   assert nb.raw[:0x4000]==bytes(mem.r_block(0x1c0000,0x4000))
   assert nf.raw[:0x4000]==bytes(mem.r_block(0xc0000,0x4000))
   assert np.raw[:0x9000]==bytes(mem.r_block(0x180000,0x9000))
   assert list(nr)==[c.r_reg(i)for i in range(8)]
   m.cleanup();cases+=1
 print(json.dumps({'status':'PASS','cases':cases,'intercepts':[],'range':'1AFC..1B1D','wait_edges':97,'compared':['raw RAM16KB','FG16KB','palette RAM36KB','D0-D7','level/publish/wait boundaries'],'excluded':'1B1E records,65C0,BOOT restart; Royal winner1B4E'}))
