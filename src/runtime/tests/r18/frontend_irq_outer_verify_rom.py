#!/usr/bin/env python3
"""Full original IRQ834..918 gates, ACK, credited START diversion and RTE."""
import argparse
import ctypes as ct
import hashlib
import itertools
import json
import os
from pathlib import Path
import tempfile
from frontend_irq_verify_rom import ROOT,RAM,SIZE,SHA1,State,Effect,Harness,build

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path);args=p.parse_args()
    counts={'sequences':0,'boundaries':0,'RTE':0,'accepted_START':0,'busy_wait':0,'guards':0}
    with tempfile.TemporaryDirectory(prefix='wf-irq-outer-',ignore_cleanup_errors=os.name=='nt') as temp:
        native=build(temp);h=Harness(native,0x834)
        native.wf_r18_frontend_irq_enter.argtypes=[ct.POINTER(State),ct.c_uint16,ct.c_uint32]
        deps=(0xb4b8,0xb53a,0x27012,0x27066,0x26e1a,0x2946a,0x26dac,0x264aa,0x978,0x1e92,0x1f6c,0x1fde,0x1f9e)
        targets={0x834:{0x834,0x83e},0x83e:{0x84e,0x90c},0x84e:{0x90c,0x85a},0x85a:{0x866,0x904},
                 0x866:{0x86e},0x86e:{0x874},0x874:{0x87a},0x87a:{0x880},0x880:{0x886},0x886:{0x88c},
                 0x88c:{0x892},0x892:{0x898},0x898:{0x89e},0x89e:{0x8a8,0x8f8},0x8a8:{0x8ac},
                 0x8ac:{0x8f8,0x8ae},0x8ae:{0x8c4},0x8c4:{0x8ca},0x8ca:{0x8e6},0x8e6:{0x8ea},
                 0x8ea:{0x8ee},0x8ee:{0x8f2},0x8f2:{0x52be},0x8f8:{0x900},0x900:{0x904},
                 0x904:{0x90c},0x90c:{0x910},0x910:{0x918},0x918:{0x70000}}
        writes={0x866:(0x140008,0),0x8c4:(0x140000,0),0x910:(0x140000,0)}
        configs=list(itertools.product((0,0x800),(0,1,2,3),(0,1,2,4,8,3)))
        configs += [(0,3,15)]*5
        for case,(config,credits,pressed) in enumerate(configs):
            ram=bytearray(SIZE);ram[:0x60]=h.rng.randbytes(0x60)
            ram[0x4b]=1;ram[0x4d]=1;ram[0x4e:0x50]=credits.to_bytes(2,'big')
            ram[0x66:0x68]=config.to_bytes(2,'big');ram[0x6f]=1;ram[0x76]=0x80
            if case==48:ram[0x76]=0
            if case==49:ram[0x6f]=0x81
            if case==50:ram[0x6f]=0
            if case==51:ram[0x7c]=0x80
            if case==52:ram[0x6f]=3
            ports=tuple(0xff7f if pressed&(1<<i) else 0xffff for i in range(4))
            h.begin(0x834,0x2300|(case&0x1f),ram,ports)
            h.ctx.ssp=0x1c3bce;h.cpu.w_isp(h.ctx.ssp);h.ctx.usp=0x1c3ed4;h.cpu.w_usp(h.ctx.usp)
            return_sr=(0x2100 if case&1 else 0x100)|(case&0x1f);h.mem.w16(h.ctx.ssp,return_sr);h.mem.w32(h.ctx.ssp+2,0x70000)
            original_d=list(h.regs.d);original_a=list(h.a)
            state=State();native.wf_r18_frontend_irq_enter(ct.byref(state),return_sr,0x70000)
            while state.pc not in (0x52be,0x70000):
                h.oracle_to(targets[state.pc],calls=deps,writes=writes,reads={0x834:0x140020})
                result=native.wf_r18_frontend_irq_step(ct.byref(state),ct.byref(h.ctx));h.check(state,result)
                counts['boundaries']+=1
            if state.pc==0x52be:
                assert result==2 and 0x978 in h.trace and 0x1e92 not in h.trace
                assert h.ctx.ssp==0x1c3bd4 and h.ctx.usp==0x1c3fd4
                counts['accepted_START']+=1
            else:
                assert result==3 and list(h.regs.d)==original_d and list(h.a)==original_a
                assert h.ctx.sr==return_sr and h.ctx.ssp==0x1c3bd4 and h.ctx.usp==0x1c3ed4
                counts['RTE']+=1
            if case in (48,49,50):assert not h.trace
            if case==51:assert 0x978 not in h.trace and 0x1e92 in h.trace
            counts['sequences']+=1
        h.begin(0x834,0x2300,bytearray(SIZE),(0xfdff,0xffff,0xffff,0xffff));state=State()
        native.wf_r18_frontend_irq_enter(ct.byref(state),0x100,0x70000)
        for _ in range(3):
            h.oracle_to({0x834},reads={0x834:0x140020})
            result=native.wf_r18_frontend_irq_step(ct.byref(state),ct.byref(h.ctx));h.check(state,result)
            assert result==1 and state.pc==0x834 and not state.registers_saved;counts['busy_wait']+=1
        @Effect
        def reject(*unused):return -1
        h.ctx.effect=reject;before=h.native_snapshot()
        assert native.wf_r18_frontend_irq_step(ct.byref(state),ct.byref(h.ctx))==-1
        assert h.native_snapshot()==before and state.blocked==0x834;counts['guards']+=1
        h.finish()
        report={'cases':counts,'rom_sha1':SHA1,'source_sha256':hashlib.sha256((ROOT/'src/wf_r18_frontend_irq.c').read_bytes()).hexdigest(),
                'rom_modifications':[],'helper_intercepts':[],
                'limitations':'IRQ control only. Mandatory effects/978/HUD execute intact original-ROM pre/post contracts. No native backend certification. RTE exception frame explicit; transient stack memory3AD4..3FFF omitted, persistent RAM/D/A/SR/SSP/USP exact.'}
        if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
        print(f'PASS {sum(counts.values())} frontend outer IRQ checks')

if __name__=='__main__':main()
