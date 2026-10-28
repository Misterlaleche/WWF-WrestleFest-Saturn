#!/usr/bin/env python3
"""Bootstrap654 and its native clear/DIP/coinage/ranking helpers versus ROM."""
import argparse
import ctypes as ct
import hashlib
import itertools
import json
import os
from pathlib import Path
import tempfile
from frontend_irq_verify_rom import ROOT,RAM,SIZE,SHA1,PERSIST,State,Context,Effect,Harness,build

READS={0x1e1e:0x140020,0x1e30:0x140022,0x1e40:0x140024,0x1e52:0x140026}

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path);args=p.parse_args()
    counts={'clear':0,'DIP':0,'coinage':0,'rankings':0,'sequences':0,'boundaries':0,'guards':0}
    with tempfile.TemporaryDirectory(prefix='wf-reset-',ignore_cleanup_errors=os.name=='nt') as temp:
        native=build(temp);h=Harness(native,0x654)
        for name in ('clear_1f1e','dips_1e1e','coinage_0400','rankings_6d82'):
            getattr(native,'wf_r18_frontend_reset_'+name).argtypes=[ct.POINTER(Context)]
        def standalone(name,pc,ram=None,ports=(0xffff,)*4,sr=0x271f):
            h.begin(pc,sr,ram,ports);h.cpu.w_sp(h.cpu.r_sp()-4);h.mem.w32(h.cpu.r_sp(),0x70000)
            h.oracle_to({0x70000},reads=READS if pc==0x1e1e else {})
            result=getattr(native,'wf_r18_frontend_reset_'+name)(ct.byref(h.ctx))
            h.check(State(0x70000,0),result);return h.buf.raw
        for sr in (0x2000,0x2110,0x261f,0x270f):
            before=h.rng.randbytes(SIZE);after=standalone('clear_1f1e',0x1f1e,before,sr=sr)
            assert after[:0x60]==before[:0x60] and after[0x60:PERSIST]==bytes(PERSIST-0x60)
            assert after[PERSIST:]==before[PERSIST:];counts['clear']+=1
        raw_cases=[(0,)*4,(0xffff,)*4]
        raw_cases += [tuple(h.rng.getrandbits(16) for _ in range(4)) for _ in range(512)]
        raw_cases += [tuple(1<<bit if port==slot else 0 for port in range(4)) for slot in range(4) for bit in range(16)]
        for ports in raw_cases:
            after=standalone('dips_1e1e',0x1e1e,ports=ports)
            packed=~(((ports[0]&0x3000)<<2)|(ports[1]&0x3f00)|((ports[2]&0x3f00)>>8)|((ports[3]&0x300)>>2))&0xffff
            assert int.from_bytes(after[0x66:0x68],'big')==packed;counts['DIP']+=1
        for sr,config in itertools.product((0x2000,0x2110,0x261f,0x270f),range(256)):
            ram=bytearray(h.rng.randbytes(SIZE));ram[0x67]=config
            after=standalone('coinage_0400',0x400,ram,sr=sr)
            assert (after[0x4b],after[0x4d])==((1,1),(1,2),(2,1),(3,1))[config&3]
            assert after[0x4e:0x50]==b'\0\0';counts['coinage']+=1
        for sr in (0x2000,0x2110,0x261f,0x270f):
            after=standalone('rankings_6d82',0x6d82,sr=sr)
            assert after[0xc2:0x146]==h.rom[0x6db0:0x6e34];counts['rankings']+=1
        targets={0x654:0x69e,0x69e:0x6a6,0x6a6:0x6be,0x6be:0x6c2,0x6c2:0x6ca,0x6ca:0x6d2,
                 0x6d2:0x6d6,0x6d6:0x6da,0x6da:0x6de,0x6de:0x6e2,0x6e2:0x6e6,
                 0x6e6:0x6ee,0x6ee:0x6f6,0x6f6:0x6fc}
        writes={0x69e:(0x184000,0),0x6c2:(0x14000c,0x3100),0x6e6:(0x140002,0),0x6ee:(0x140000,0)}
        for sr,ports in itertools.product((0x2000,0x2110,0x261f,0x270f),raw_cases[:4]):
            ram=bytearray(h.rng.randbytes(SIZE));seed=bytes(ram[0x5c:0x60]);lower=bytes(ram[:0x4b])
            h.begin(0x654,sr,ram,ports);state=State();native.wf_r18_frontend_reset_init(ct.byref(state))
            while state.pc!=0x6fc:
                h.oracle_to({targets[state.pc]},calls=(0x1fc0,0x2006),writes=writes,reads=READS)
                result=native.wf_r18_frontend_reset_step(ct.byref(state),ct.byref(h.ctx));h.check(state,result)
                counts['boundaries']+=1
            assert result==2 and h.trace==[0x1fc0,0x2006,0x1fc0]
            assert h.buf.raw[0x5c:0x60]==seed and h.buf.raw[:0x4b]==lower
            assert h.ctx.usp==0x1c3fd4 and h.ctx.ssp==0x1d0000 and h.ctx.sr&0x2000==0
            assert h.buf.raw[0xc2:0x146]==h.rom[0x6db0:0x6e34];counts['sequences']+=1
        h.begin(0x654,0x100);state=State();native.wf_r18_frontend_reset_init(ct.byref(state));before=h.native_snapshot()
        assert native.wf_r18_frontend_reset_step(ct.byref(state),ct.byref(h.ctx))==-2
        assert h.native_snapshot()==before;counts['guards']+=1
        @Effect
        def reject(*unused):return -1
        h.begin(0x6be,0x2700);h.ctx.effect=reject;state=State(0x6be,0);before=h.native_snapshot()
        assert native.wf_r18_frontend_reset_step(ct.byref(state),ct.byref(h.ctx))==-1
        assert state.pc==0x6be and state.blocked==0x1fc0 and h.native_snapshot()==before;counts['guards']+=1
        for fail_at in range(4):
            calls=[]
            @Effect
            def fail_read(user,event,context):
                calls.append(event.contents.owner_rom)
                if len(calls)-1==fail_at:return -1
                event.contents.value=0xa5a5;return 0
            h.begin(0x1e1e,0x271f);h.ctx.effect=fail_read;before=h.native_snapshot()
            assert native.wf_r18_frontend_reset_dips_1e1e(ct.byref(h.ctx))==-1
            assert h.native_snapshot()==before and len(calls)==fail_at+1;counts['guards']+=1
        h.finish()
        report={'cases':counts,'rom_sha1':SHA1,'source_sha256':hashlib.sha256((ROOT/'src/wf_r18_frontend_reset.c').read_bytes()).hexdigest(),
                'rom_modifications':[],'helper_intercepts':[],
                'limitations':'Native654/1F1E/1E1E/400/6D82. 1FC0 and2006 original-ROM callbacks; native device implementation outside. Full persistent RAM below3AD4, D/A/SR/quiescent SSP/USP compared; temporary stack scratch above3AD4 excluded. Physical DIP policy and RNG seed supplied by platform.'}
        if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
        print(f'PASS {sum(counts.values())} frontend reset helper/sequence/boundary/guard checks')

if __name__=='__main__':main()
