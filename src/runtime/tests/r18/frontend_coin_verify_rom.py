#!/usr/bin/env python3
"""Original coin434 IRQ and48C/4BC/4F2 helpers against immutable ROM."""
import argparse
import ctypes as ct
import hashlib
import itertools
import json
import os
from pathlib import Path
import tempfile
from frontend_irq_verify_rom import ROOT,RAM,SIZE,SHA1,Context,Effect,Harness,build

class State(ct.Structure):
    _fields_=[('pc',ct.c_uint32),('blocked',ct.c_uint32),('saved_d',ct.c_uint32*3),
              ('return_pc',ct.c_uint32),('return_sr',ct.c_uint16),
              ('frame_valid',ct.c_uint8),('registers_saved',ct.c_uint8)]

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path);args=p.parse_args()
    counts={'credit':0,'service':0,'pulse':0,'IRQ':0,'boundaries':0,'guards':0}
    with tempfile.TemporaryDirectory(prefix='wf-coin-',ignore_cleanup_errors=os.name=='nt') as temp:
        native=build(temp);h=Harness(native,0x434)
        for name in ('pulse_048c','service_04bc','credit_04f2'):
            getattr(native,'wf_r18_frontend_coin_'+name).argtypes=[ct.POINTER(Context)]
        native.wf_r18_frontend_coin_enter.argtypes=[ct.POINTER(State),ct.c_uint16,ct.c_uint32]
        native.wf_r18_frontend_coin_step.argtypes=[ct.POINTER(State),ct.POINTER(Context)]
        def standalone(name,pc,ram,ports=(0xffff,)*4,d=None,sr=0x271f):
            h.begin(pc,sr,ram,ports)
            if d is not None:
                for i,value in enumerate(d):
                    h.regs.d[i]=value;h.cpu.w_reg(i,value)
            h.cpu.w_sp(h.cpu.r_sp()-4);h.mem.w32(h.cpu.r_sp(),0x70000)
            h.oracle_to({0x70000},reads={0x48c:(0x140020,1),0x4bc:(0x140020,1)},
                        writes={0x500:(0x14000c,0x312a,1 if pc==0x4f2 else 2)})
            result=getattr(native,'wf_r18_frontend_coin_'+name)(ct.byref(h.ctx));h.check(State(0x70000,0),result)
            return h.buf.raw
        for ratio,units,pending,credits,total in itertools.product((0,1,2,3,255),(0,1,2,255),(0,1,2,254,255),(0,98,99,255),(0,0xffff)):
            ram=bytearray(h.rng.randbytes(SIZE));ram[0x4b]=ratio;ram[0x4d]=units;ram[0x4e]=pending;ram[0x4f]=credits
            ram[0x50:0x52]=total.to_bytes(2,'big');ram[0x52:0x54]=total.to_bytes(2,'big')
            standalone('credit_04f2',0x4f2,ram);counts['credit']+=1
        for count,active,sr in itertools.product(range(256),(False,True),(0x2700,0x271f)):
            ram=bytearray(h.rng.randbytes(SIZE));ram[0x56]=count
            standalone('service_04bc',0x4bc,ram,ports=(0xfbff if active else 0xffff,)*4,sr=sr);counts['service']+=1
        for duration,release,active,port in itertools.product((0,1,0x35f,0x360,0xffff),(0,3,4,9,10,0x7f,0xff),(False,True),(0,1,2,7)):
            ram=bytearray(h.rng.randbytes(SIZE));ram[0x4b]=1;ram[0x4d]=1;ram[0x4e]=0;ram[0x4f]=2
            d=[h.rng.getrandbits(32) for _ in range(8)]
            d[0]=(d[0]&0xffff0000)|duration;d[1]=(d[1]&0xffffff00)|port;d[2]=(d[2]&0xffffff00)|release
            word=0xffff&~(0x100<<port) if active else 0xffff
            standalone('pulse_048c',0x48c,ram,ports=(word,)*4,d=d);counts['pulse']+=1
        targets={0x434:0x438,0x438:0x43e,0x43e:0x440,0x440:0x450,0x450:0x454,
                 0x454:0x460,0x460:0x470,0x470:0x474,0x474:0x480,0x480:0x486,0x486:0x48a,0x48a:0x70000}
        ram=bytearray(SIZE);ram[0x4b]=2;ram[0x4d]=1
        # Sustained original polling: qualify both ports, coin1, then coin2;
        # service>30 polls credits once on release. No wall-time conversion.
        sequence=[0xffff]*4+[0xfeff]*3+[0xffff]*4+[0xfdff]*2+[0xffff]*4+[0xfbff]*31+[0xffff]
        for word in sequence:
            h.begin(0x434,0x231f,ram,(word,)*4);h.ctx.ssp=0x1c3bce;h.cpu.w_isp(h.ctx.ssp)
            return_sr=0x111;h.mem.w16(h.ctx.ssp,return_sr);h.mem.w32(h.ctx.ssp+2,0x70000)
            before_d=list(h.regs.d);before_a=list(h.a);state=State()
            native.wf_r18_frontend_coin_enter(ct.byref(state),return_sr,0x70000)
            while state.pc!=0x70000:
                h.oracle_to({targets[state.pc]},reads={0x48c:(0x140020,1),0x4bc:(0x140020,1)},
                            writes={0x438:(0x140016,0),0x480:(0x140002,0),0x500:(0x14000c,0x312a,2)})
                result=native.wf_r18_frontend_coin_step(ct.byref(state),ct.byref(h.ctx));h.check(state,result);counts['boundaries']+=1
            assert result==3 and h.ctx.sr==return_sr and h.ctx.ssp==0x1c3bd4
            assert list(h.regs.d)==before_d and list(h.a)==before_a
            ram=bytearray(h.buf.raw);counts['IRQ']+=1
        assert ram[0x4f]==2 and ram[0x4e]==0 and int.from_bytes(ram[0x52:0x54],'big')==2
        @Effect
        def reject(*unused):return -1
        for name in ('pulse_048c','service_04bc','credit_04f2'):
            h.begin(0x4f2,0x2700);h.ctx.effect=reject;before=h.native_snapshot()
            assert getattr(native,'wf_r18_frontend_coin_'+name)(ct.byref(h.ctx))==-1
            assert h.native_snapshot()==before;counts['guards']+=1
        @Effect
        def reject_sound(user,event,context):
            if event.contents.kind==2:event.contents.value=0xffff;return 0
            return -1
        h.begin(0x48c,0x271f);h.regs.d[0]=1;h.regs.d[1]=0;h.regs.d[2]=4
        h.ctx.effect=reject_sound;before=h.native_snapshot()
        assert native.wf_r18_frontend_coin_pulse_048c(ct.byref(h.ctx))==-1
        assert h.native_snapshot()==before;counts['guards']+=1
        h.finish()
        report={'cases':counts,'rom_sha1':SHA1,'source_sha256':hashlib.sha256((ROOT/'src/wf_r18_frontend_coin.c').read_bytes()).hexdigest(),
                'rom_modifications':[],'helper_intercepts':[],
                'limitations':'Native434..558 coin/service IRQ and helpers; no external helper callbacks. Raw physical ports and exception frame supplied. All persistent RAM/D/A/SR/quiescent stacks compared; temporary stack scratch omitted.'}
        if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
        print(f'PASS {sum(counts.values())} frontend coin checks')

if __name__=='__main__':main()
