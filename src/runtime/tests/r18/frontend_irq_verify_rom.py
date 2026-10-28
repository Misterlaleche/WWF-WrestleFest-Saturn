#!/usr/bin/env python3
"""Accepted-START IRQ caller versus intact ROM, including SR/SSP/USP."""
import argparse
import ctypes as ct
import hashlib
import itertools
import json
import os
from pathlib import Path
import random
import subprocess
import tempfile
import machine68k as mus
from title_verify_rom import Registers, Event

ROOT=Path(__file__).resolve().parents[2]
RAM=0x1c0000;SIZE=0x4000;PERSIST=0x3ad4
SHA1='a726c46f350723ea73158bc78055d17d418b14e2'
class Context(ct.Structure):pass
Effect=ct.CFUNCTYPE(ct.c_int,ct.c_void_p,ct.POINTER(Event),ct.POINTER(Context))
Context._fields_=[('ram',ct.c_void_p),('size',ct.c_size_t),('regs',ct.POINTER(Registers)),
                 ('a',ct.POINTER(ct.c_uint32)),('ssp',ct.c_uint32),('usp',ct.c_uint32),
                 ('sr',ct.c_uint16),('effect',Effect),('user',ct.c_void_p)]
class State(ct.Structure):
    _fields_=[('pc',ct.c_uint32),('blocked',ct.c_uint32),('saved_d',ct.c_uint32*8),
              ('saved_a',ct.c_uint32*7),('return_pc',ct.c_uint32),('return_sr',ct.c_uint16),
              ('frame_valid',ct.c_uint8),('registers_saved',ct.c_uint8)]

class Harness:
    def __init__(self, native, seed):
        self.native=native;self.rng=random.Random(seed)
        self.rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes()
        assert hashlib.sha1(self.rom).hexdigest()==SHA1
        self.machine=mus.Machine(mus.CPUType.M68000,2048)
        self.cpu=self.machine.cpu;self.mem=self.machine.mem;self.mem.w_block(0,self.rom)
        self.pending=[];self.errors=[];self.effect=Effect(self.callback)
        self.trace=[];self.boundaries=0
    def begin(self, pc, sr, ram=None, ports=(0xffff,)*4):
        rng=self.rng;ram=bytes(ram if ram is not None else rng.randbytes(SIZE))
        self.buf=ct.create_string_buffer(ram,SIZE)
        self.regs=Registers((ct.c_uint32*8)(*[rng.getrandbits(32) for _ in range(8)]),1)
        self.a=(ct.c_uint32*7)(*[rng.getrandbits(32) for _ in range(7)])
        self.ctx=Context(ct.addressof(self.buf),SIZE,ct.pointer(self.regs),self.a,0x1d0000,0x1d8000,sr,self.effect,None)
        self.mem.w_block(RAM,ram)
        for i in range(8):self.cpu.w_reg(i,self.regs.d[i])
        for i in range(7):self.cpu.w_reg(8+i,self.a[i])
        self.cpu.w_sr(sr);self.cpu.w_isp(self.ctx.ssp);self.cpu.w_usp(self.ctx.usp)
        self.cpu.w_pc(pc)
        for i,word in enumerate(ports):self.mem.w16(0x140020+i*2,word)
        self.pending.clear();self.errors.clear();self.trace.clear()
    def snapshot(self, call_entry=False):
        sr=self.cpu.r_sr();ssp=self.cpu.r_isp();usp=self.cpu.r_usp()
        if call_entry:
            if sr&0x2000:ssp+=4*int(call_entry)
            else:usp+=4*int(call_entry)
        return (bytes(self.mem.r_block(RAM,PERSIST)),[self.cpu.r_reg(i) for i in range(8)],
                [self.cpu.r_reg(8+i) for i in range(7)],sr,ssp,usp)
    def native_snapshot(self):
        c=self.ctx
        return (ct.string_at(c.ram,PERSIST),list(c.regs.contents.d),list(c.a[:7]),c.sr,c.ssp,c.usp)
    def callback(self,user,event,context):
        try:
            e=event.contents;got=(e.kind,e.owner_rom,e.address,e.value)
            want,before,after,read_value=self.pending.pop(0)
            assert got==want,('event',got,want)
            assert self.native_snapshot()==before,('callback pre-state',got,self.diff(self.native_snapshot(),before))
            if e.kind==0:
                ct.memmove(context.contents.ram,after[0],PERSIST)
                for i,v in enumerate(after[1]):context.contents.regs.contents.d[i]=v
                for i,v in enumerate(after[2]):context.contents.a[i]=v
                context.contents.sr=after[3];context.contents.ssp=after[4];context.contents.usp=after[5]
            if e.kind==2:e.value=read_value
            return 0
        except Exception as error:self.errors.append(repr(error));return -1
    @staticmethod
    def diff(actual,want):
        names=('RAM','D','A','SR','SSP','USP');out=[]
        for name,a,b in zip(names,actual,want):
            if a!=b:
                if name=='RAM':out.append((name,next((hex(i),a[i],b[i]) for i in range(PERSIST) if a[i]!=b[i])))
                else:out.append((name,a,b))
        return out
    def oracle_to(self, targets, calls=(), writes=None, reads=None):
        calls=set(calls);writes=writes or {};reads=reads or {};steps=0
        while True:
            pc=self.cpu.r_pc()
            if steps and pc in targets:return
            if pc in calls:
                before=self.snapshot(True);ret=self.mem.r32(self.cpu.r_sp());self.trace.append(pc)
                for _ in range(150000):
                    if self.cpu.r_pc()==ret:break
                    self.cpu.execute(1)
                assert self.cpu.r_pc()==ret,('helper runaway',hex(pc),hex(self.cpu.r_pc()))
                self.pending.append(((0,pc,0,0),before,self.snapshot(),None));steps+=1;continue
            if pc in writes:
                item=writes[pc];address,value=item[:2];depth=item[2] if len(item)>2 else 0
                before=self.snapshot(depth);self.cpu.execute(1)
                assert self.mem.r16(address)==value
                self.pending.append(((1,pc,address,value),before,self.snapshot(),None));steps+=1;continue
            if pc in reads:
                item=reads[pc];address,depth=item if isinstance(item,tuple) else (item,pc!=0x834)
                before=self.snapshot(depth);value=self.mem.r16(address)
                self.cpu.execute(1);self.pending.append(((2,pc,address,0),before,self.snapshot(),value));steps+=1;continue
            self.cpu.execute(1);steps+=1
            assert steps<250000,('caller runaway',hex(pc),hex(self.cpu.r_pc()))
    def check(self, state, result):
        assert not self.errors,self.errors
        assert not self.pending,('missing events',[event[0] for event in self.pending])
        assert result>=0,('native blocked',hex(state.pc),hex(state.blocked))
        assert state.pc==self.cpu.r_pc(),('PC',hex(state.pc),hex(self.cpu.r_pc()))
        assert self.native_snapshot()==self.snapshot(),self.diff(self.native_snapshot(),self.snapshot())
        assert bytes(self.mem.r_block(0,len(self.rom)))==self.rom
        self.boundaries+=1
    def finish(self):self.machine.cleanup()

def build(temp):
    lib=Path(temp)/'control.so'
    subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',
                    'src/wf_r18_frontend_irq.c','src/wf_r18_frontend_reset.c',
                    'src/wf_r18_frontend_coin.c',
                    'src/wf_r17_m68k_shadow.c','-o',str(lib)],cwd=ROOT,check=True)
    native=ct.CDLL(str(lib))
    for owner in ('irq','reset'):
        getattr(native,f'wf_r18_frontend_{owner}_init').argtypes=[ct.POINTER(State)]
        getattr(native,f'wf_r18_frontend_{owner}_step').argtypes=[ct.POINTER(State),ct.POINTER(Context)]
    return native

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path);args=p.parse_args()
    counts={'sequences':0,'boundaries':0,'guards':0}
    with tempfile.TemporaryDirectory(prefix='wf-irq-',ignore_cleanup_errors=os.name=='nt') as temp:
        native=build(temp);h=Harness(native,0x8ae)
        targets={0x8ae:0x8c4,0x8c4:0x8ca,0x8ca:0x8e6,0x8e6:0x8ea,0x8ea:0x8ee,0x8ee:0x8f2,0x8f2:0x52be}
        for sr,flags in itertools.product((0x2000,0x2110,0x261f,0x270f),range(8)):
            ram=bytearray(h.rng.randbytes(SIZE));ram[0x6f]=0x70|((flags&1)<<7)|((flags&2))|(flags>>2)
            h.begin(0x8ae,sr,ram);state=State();native.wf_r18_frontend_irq_init(ct.byref(state))
            while state.pc!=0x52be:
                h.oracle_to({targets[state.pc]},calls=(0x1f6c,0x1fde,0x1f9e),writes={0x8c4:(0x140000,0)})
                result=native.wf_r18_frontend_irq_step(ct.byref(state),ct.byref(h.ctx));h.check(state,result)
            assert result==2 and h.trace==[0x1f6c,0x1fde,0x1f9e]
            assert h.ctx.ssp==0x1c3bd4 and h.ctx.usp==0x1c3fd4 and h.ctx.sr&0x2000==0
            counts['sequences']+=1
        counts['boundaries']=h.boundaries
        h.begin(0x8ae,0x100);state=State();native.wf_r18_frontend_irq_init(ct.byref(state));before=h.native_snapshot()
        assert native.wf_r18_frontend_irq_step(ct.byref(state),ct.byref(h.ctx))==-2
        assert h.native_snapshot()==before;counts['guards']+=1
        h.begin(0x8e6,0x100);state=State(0x8e6,0)
        @Effect
        def reject(*unused):return -1
        h.ctx.effect=reject;before=h.native_snapshot()
        assert native.wf_r18_frontend_irq_step(ct.byref(state),ct.byref(h.ctx))==-1
        assert state.pc==0x8e6 and state.blocked==0x1f6c and h.native_snapshot()==before;counts['guards']+=1
        h.finish()
        report={'cases':counts,'rom_sha1':SHA1,'source_sha256':hashlib.sha256((ROOT/'src/wf_r18_frontend_irq.c').read_bytes()).hexdigest(),
                'rom_modifications':[],'helper_intercepts':[],
                'limitations':'8AE accepted caller only; external clears execute original ROM pre/post contracts. SR and quiescent SSP/USP compared. Temporary stack RAM3AD4..3FFF excluded; every persistent byte below3AD4 compared.'}
        if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
        print(f'PASS {sum(counts.values())} frontend IRQ sequence/boundary/guard checks')

if __name__=='__main__':main()
