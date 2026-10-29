#!/usr/bin/env python3
"""Execute supplied WrestleFest ROM prematch callbacks; no game logic is translated.
Unicorn 2.1.4 is a research-only dependency; Saturn deliverable is native C.
Input vector explicitly defines cold RNG/register state, not claimed hardware power-on.
"""
from pathlib import Path
import sys, json, hashlib, argparse, zipfile
HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE/'pydeps'))
from unicorn import Uc, UC_ARCH_M68K, UC_MODE_BIG_ENDIAN, UC_HOOK_CODE, UC_HOOK_MEM_WRITE, UC_HOOK_MEM_INVALID
from unicorn.m68k_const import *
SHA1 = 'a726c46f350723ea73158bc78055d17d418b14e2'
PRES = 0x1c14ce
ACTORS = [0x1c05b0 + i*0x10c for i in range(9)]
DREGS = [UC_M68K_REG_D0, UC_M68K_REG_D1, UC_M68K_REG_D2, UC_M68K_REG_D3, UC_M68K_REG_D4, UC_M68K_REG_D5, UC_M68K_REG_D6, UC_M68K_REG_D7]

def load_rom(path):
    with zipfile.ZipFile(path) as z:
        e=z.read('31e14-0.ic18');o=z.read('31e13-0.ic19')
    p=bytearray(len(e)+len(o));p[::2]=e;p[1::2]=o
    assert hashlib.sha1(p).hexdigest()==SHA1
    return bytes(p)

class Oracle:
    def __init__(self, rom, identities=(0,1,2,3), seed=0x12345678):
        self.rom=rom
        self.u=Uc(UC_ARCH_M68K, UC_MODE_BIG_ENDIAN)
        self.u.ctl_set_cpu_model(UC_CPU_M68K_M68000)
        self.u.mem_map(0, 0x200000)
        self.u.mem_write(0, rom)
        self.u.reg_write(UC_M68K_REG_SR,0x2700)
        self.u.reg_write(UC_M68K_REG_A7,0x1c3bd0)
        self.w32(0x1c3bd0,0x1ff000)
        self.frames=[];self.raw_events=[];self.hw_events=[];self.rng_calls=[]
        self.u.hook_add(UC_HOOK_MEM_INVALID,self.alias24)
        self.w32(0x1c005c,seed)
        self.w16(0x1c007c,0x8000) # enable actual sound submission, paid path context
        self.w16(0x1c17e6,0x1e0);self.w16(0x1c17ee,0x230)
        for a in (0x140020,0x140022,0x140024,0x140026): self.w16(a,0xffff)
        for i,(a,ident,(x,y)) in enumerate(zip(ACTORS,identities,[(0x240,0x150),(0x220,0x160),(0x2c0,0x150),(0x2e0,0x160)])):
            self.w16(a,0x8000);self.w16(a+2,ident);self.w16(a+0x56,ident)
            self.w16(a+6,x);self.w16(a+0xa,y);self.w16(a+0xe,0x140)
            self.w8(a+0x33,0x80 if i>=2 else 0)
        a=0x1c11f4
        self.w16(a,0x8000);self.w16(a+2,0xc);self.w16(a+6,0x280);self.w16(a+0xa,0x198);self.w16(a+0xe,0x140)
        self.u.hook_add(UC_HOOK_CODE,self.sync,begin=0xa728,end=0xa728)
        self.u.hook_add(UC_HOOK_CODE,self.frame,begin=0xa732,end=0xa732)
        self.u.hook_add(UC_HOOK_CODE,self.sound,begin=0x2052,end=0x2052)
        self.u.hook_add(UC_HOOK_CODE,self.rng,begin=0x21b4,end=0x21b4)
        self.u.hook_add(UC_HOOK_MEM_WRITE,self.hw,begin=0x14000c,end=0x14000d)
        # Stop only AFTER the ROM has actually cleared the presenter.
        self.u.hook_add(UC_HOOK_CODE,self.clear,begin=0xa794,end=0xa794)
    def alias24(self,u,access,a,size,value,data):
        # MC68000 has a 24-bit external address bus. Unicorn leaves higher
        # internal address bits intact; mirror read-only ROM pages on demand.
        low = a & 0xffffff
        if low >= len(self.rom) or a == low: return False
        page = a & ~4095
        u.mem_map(page,4096)
        u.mem_write(page,self.rom[(low & ~4095):(low & ~4095)+4096])
        return True
    def w8(self,a,v): self.u.mem_write(a,bytes([v&255]))
    def w16(self,a,v): self.u.mem_write(a,(v&65535).to_bytes(2,'big'))
    def w32(self,a,v): self.u.mem_write(a,(v&0xffffffff).to_bytes(4,'big'))
    def r8(self,a): return self.u.mem_read(a,1)[0]
    def r16(self,a): return int.from_bytes(self.u.mem_read(a,2),'big')
    def r32(self,a): return int.from_bytes(self.u.mem_read(a,4),'big')
    def sync(self,u,addr,size,data): self.w8(0x1c006f,self.r8(0x1c006f)|2)
    def actor(self,a):
        return dict(active=self.r16(a),type=self.r16(a+2),selector=self.r16(a+4),x=self.r32(a+6),y=self.r32(a+0xa),z=self.r32(a+0xe),state=self.r16(a+0x1c),counter=self.r16(a+0x22),phase=self.r16(a+0x24),counter1e=self.r16(a+0x1e),flags33=self.r8(a+0x33))
    def frame(self,u,addr,size,data):
        n=len(self.frames)
        self.frames.append(dict(tick=n+1,presenter=self.actor(PRES),wrestlers=[self.actor(a) for a in ACTORS[:4]],referee=self.actor(0x1c11f4),names_visible=int(any(self.u.mem_read(0xc0100,0x1d00))),rng=self.r32(0x1c005c),group=self.r16(0x1c0168),raw_sounds=[e[1] for e in self.raw_events if e[0]==n],hardware_sounds=[e[1] for e in self.hw_events if e[0]==n]))
    def sound(self,u,addr,size,data): self.raw_events.append([len(self.frames),u.reg_read(UC_M68K_REG_D0)&0xffff])
    def hw(self,u,access,addr,size,value,data): self.hw_events.append([len(self.frames),value])
    def rng(self,u,addr,size,data):
        self.rng_calls.append(dict(tick=len(self.frames)+1,seed=self.r32(0x1c005c),mix=[u.reg_read(r) for r in DREGS[1:]],global80=self.r32(0x1c0080)))
    def clear(self,u,addr,size,data):
        self.cleared_presenter=self.r16(PRES)
        u.emu_stop()
    def run(self):
        try: self.u.emu_start(0xa654,0x1ff000,count=20000000)
        except Exception:
            print('PC',hex(self.u.reg_read(UC_M68K_REG_PC)), 'frames',len(self.frames),file=sys.stderr)
            raise
        assert getattr(self,'cleared_presenter',None)==0, ('missing clear',hex(self.u.reg_read(UC_M68K_REG_PC)),len(self.frames))
        return self.frames

def main():
    ap=argparse.ArgumentParser();ap.add_argument('romzip');ap.add_argument('--ids',default='0,1,2,3');ap.add_argument('--seed',default='0x12345678');ap.add_argument('--out',default=str(HERE/'oracle_trace.json'));args=ap.parse_args()
    o=Oracle(load_rom(args.romzip),list(map(int,args.ids.split(','))),int(args.seed,0));f=o.run()
    out=dict(sha1=SHA1,seed=int(args.seed,0),ids=list(map(int,args.ids.split(','))),frames=f,rng_calls=o.rng_calls,cleared_presenter=o.cleared_presenter)
    Path(args.out).write_text(json.dumps(out,separators=(',',':'))+'\n')
    print('frames',len(f),'raw_sound_events',o.raw_events,'rng_calls',len(o.rng_calls),'clear',o.cleared_presenter)
if __name__=='__main__':main()
