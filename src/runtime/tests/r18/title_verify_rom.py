#!/usr/bin/env python3
"""Title controller versus original ROM; external helpers are explicit contracts.

ROM helpers execute without patches/intercepts. Their entry state is compared
to each native callback, then their independently executed return state feeds
the dependency contract. This proves the controller, not native helper code.
"""
import argparse
import ctypes as ct
import hashlib
import json
import os
from pathlib import Path
import random
import subprocess
import tempfile
import machine68k as mus

ROOT=Path(__file__).resolve().parents[2]
RAM=0x1c0000
SIZE=0x4000
SHA1='a726c46f350723ea73158bc78055d17d418b14e2'

class Registers(ct.Structure):
    _fields_=[('d',ct.c_uint32*8),('valid',ct.c_uint8)]
class Event(ct.Structure):
    _fields_=[('kind',ct.c_uint32),('owner_rom',ct.c_uint32),('address',ct.c_uint32),('value',ct.c_uint32)]
Effect=ct.CFUNCTYPE(ct.c_int,ct.c_void_p,ct.POINTER(Event),ct.c_void_p,ct.POINTER(Registers),ct.POINTER(ct.c_uint32))
class Context(ct.Structure):
    _fields_=[('ram',ct.c_void_p),('size',ct.c_size_t),('regs',ct.POINTER(Registers)),('a',ct.POINTER(ct.c_uint32)),('effect',Effect),('user',ct.c_void_p)]
class State(ct.Structure):
    _fields_=[('pc',ct.c_uint32),('blocked',ct.c_uint32)]

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--smoke',action='store_true')
    parser.add_argument('--report',type=Path)
    args=parser.parse_args()
    rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes()
    assert hashlib.sha1(rom).hexdigest()==SHA1
    rng=random.Random(0x81ba)
    dependencies={0x2a06,0x26e66,0x1f9e,0x2aea,0x27b8,0x2836,0x2503c,0x9aca,0xb312,0x2052}
    coverage=set();calls={};sync_counts={0x823a:0,0x8302:0,0x83a4:0};total=0;branch_cases=0;text_groups=set()
    with tempfile.TemporaryDirectory(prefix='wf-title-',ignore_cleanup_errors=os.name=='nt') as tmp:
        lib=Path(tmp)/'title.so'
        subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',
                        'src/wf_r18_title.c','src/wf_r17_m68k_shadow.c','-o',str(lib)],cwd=ROOT,check=True)
        native=ct.CDLL(str(lib));native.wf_r18_title_step.argtypes=[ct.POINTER(State),ct.POINTER(Context)]
        native.wf_r18_title_init.argtypes=[ct.POINTER(State)]
        mem=bytearray(0x200000);mem[:len(rom)]=rom
        initial=[rng.getrandbits(32) for _ in range(8)]
        regs=Registers((ct.c_uint32*8)(*initial),1);a=(ct.c_uint32*7)(*[0x70000000+i*0x100 for i in range(7)])
        buffer=ct.create_string_buffer(bytes(mem),len(mem));state=State();native.wf_r18_title_init(ct.byref(state))
        machine=mus.Machine(mus.CPUType.M68000,2048);cpu,memory=machine.cpu,machine.mem
        memory.w_block(0,bytes(mem))
        for i in range(8):cpu.w_reg(i,initial[i])
        for i in range(7):cpu.w_reg(8+i,a[i])
        cpu.w_sp(0x1d0000);memory.w32(0x1d0000,0x962);cpu.w_pc(0x81ba)

        def snapshot():
            return bytes(memory.r_block(RAM,SIZE)),[cpu.r_reg(i) for i in range(8)],[cpu.r_reg(8+i) for i in range(7)]

        expected=[];callback_errors=[]
        @Effect
        def effect(user,event,ram,registers,addresses):
            try:
                want,before,after=expected.pop(0)
                got=(event.contents.kind,event.contents.owner_rom,event.contents.address,event.contents.value)
                assert got==want,('event',got,want)
                if got[:2]==(0,0x2503c):text_groups.add(registers.contents.d[0]&0xffff)
                assert ct.string_at(ram,SIZE)==before[0],('callback RAM',want)
                assert list(registers.contents.d)==before[1],('callback D0-D7',want,list(registers.contents.d),before[1])
                assert list(addresses[:7])==before[2],('callback A0-A6',want,list(addresses[:7]),before[2])
                ct.memmove(ram,after[0],SIZE)
                for i,v in enumerate(after[1]):registers.contents.d[i]=v
                for i,v in enumerate(after[2]):addresses[i]=v
                return 0
            except Exception as error:
                callback_errors.append(repr(error));return -1
        ctx=Context(ct.addressof(buffer)+RAM,SIZE,ct.pointer(regs),a,effect,None)

        # First RED is independent of the oracle and catches a missing entry.
        result=native.wf_r18_title_step(ct.byref(state),ct.byref(ctx))
        assert state.pc==0x81ca,('entry did not reach original2A06 boundary',hex(state.pc))
        assert result==0
        # Reinitialize both sides so this first operation also gets ROM proof.
        ct.memmove(ct.addressof(buffer),bytes(mem),len(mem));regs.d[:]=initial
        for i in range(7):a[i]=0x70000000+i*0x100
        native.wf_r18_title_init(ct.byref(state))

        def boundary():
            nonlocal total
            start=state.pc
            # Determine the next native boundary using a recording oracle run
            # up to the corresponding basic-block end. No native helper result
            # is used to calculate the ROM controller's behavior.
            targets={0x81ba:0x81ca,0x81ca:0x81d0,0x81d0:0x81ee,0x81ee:0x821a,
                0x821a:0x823a,0x8272:0x8296,0x8296:0x8302,0x8378:0x837e,
                0x837e:0x83a4,0x83a4:0x8482,0x848e:0x962,0x962:0x974,0x974:0xac0}
            allowed={targets[start]} if start in targets else ({0x821a,0x8272} if start==0x823a else {0x8296,0x8272,0x8378} if start==0x8302 else {0x837e,0x848e})
            expected.clear();steps=0
            while True:
                pc=cpu.r_pc()
                if steps and pc in allowed:break
                coverage.add(pc)
                if pc in dependencies:
                    before=snapshot();return_pc=memory.r32(cpu.r_sp())
                    for helper_steps in range(300000):
                        if cpu.r_pc()==return_pc:break
                        memory.w16(0x140026,0x0400 if helper_steps%128<64 else 0)
                        cpu.execute(1)
                    assert cpu.r_pc()==return_pc,('helper runaway',hex(pc),hex(cpu.r_pc()))
                    expected.append(((0,pc,0,0),before,snapshot()));calls[pc]=calls.get(pc,0)+1
                    steps+=1;continue
                if pc in sync_counts and memory.r8(RAM+0x6f)&2==0:
                    before=snapshot();memory.w8(RAM+0x6f,memory.r8(RAM+0x6f)|2)
                    expected.append(((2,pc,RAM+0x6f,2),before,snapshot()));sync_counts[pc]+=1
                scrolls={0x8244:(0x100000,0x1cd4),0x824e:(0x100004,0x1cd6),
                         0x830c:(0x100000,0x1cd4),0x8316:(0x100004,0x1cd6),
                         0x83ae:(0x100000,0x1cd4),0x83b8:(0x100004,0x1cd6)}
                if pc in scrolls:
                    destination,source=scrolls[pc];before=snapshot();value=memory.r16(RAM+source)
                    cpu.execute(1);expected.append(((1,pc,destination,value),before,snapshot()));steps+=1;continue
                cpu.execute(1);steps+=1
                assert steps<200000,('controller runaway',hex(start),hex(cpu.r_pc()))
            result=native.wf_r18_title_step(ct.byref(state),ct.byref(ctx));total+=1
            assert not callback_errors,(hex(start),callback_errors)
            assert not expected,(hex(start),'missing dependency',len(expected))
            assert result>=0,(hex(start),'native blocked',hex(state.blocked))
            assert state.pc==cpu.r_pc(),('PC',hex(start),hex(state.pc),hex(cpu.r_pc()))
            actual=snapshot()
            assert buffer.raw[RAM:RAM+SIZE]==actual[0],('RAM boundary',hex(start),hex(state.pc))
            assert list(regs.d)==actual[1],('D boundary',hex(start),list(regs.d),actual[1])
            assert list(a)==actual[2],('A boundary',hex(start),list(a),actual[2])
            return result
        while state.pc!=0xac0:
            result=boundary()
            if args.smoke and total==2:break
        if not args.smoke:
            assert sync_counts=={0x823a:128,0x8302:504,0x83a4:1480},sync_counts
            assert state.pc==0xac0 and result==2
            assert memory.r16(RAM+0x1cd4)==4224 and memory.r16(RAM+0x1cd6)==2112
            assert bytes(memory.r_block(0,len(rom)))==rom
            natural_sync=dict(sync_counts)
            for slot,(set_id,x,y) in enumerate(((32,120,96),(33,184,96),(36,56,96),(37,248,96),
                    (35,120,160),(34,184,16),(38,184,160),(41,120,16),(40,248,160),
                    (42,56,16),(39,56,160),(43,248,16))):
                at=RAM+0x1258+slot*0x2a
                assert [memory.r16(at+n) for n in (0,2,4,0x14,0x16,0x22,0x24)]==[0x8000,set_id,4,x,y,0,0]

            def set_field(offset,value,width=2):
                data=value.to_bytes(width,'big');memory.w_block(RAM+offset,data)
                ct.memmove(ct.addressof(buffer)+RAM+offset,data,width)

            # Original ACK may remain absent across arbitrary polls. No time,
            # scroll or local timer may advance until an actual ACK arrives.
            @Effect
            def no_ack(user,event,ram,registers,addresses):
                return 0 if event.contents.kind==2 else -1
            state.pc=0x823a;set_field(0x6f,0,1);ctx.effect=no_ack
            before=(ct.string_at(ctx.ram,SIZE),list(regs.d),list(a))
            for _ in range(3):
                assert native.wf_r18_title_step(ct.byref(state),ct.byref(ctx))==1
                assert state.pc==0x823a
                assert (ct.string_at(ctx.ram,SIZE),list(regs.d),list(a))==before
            ctx.effect=effect
            # Independent branch fixtures exercise both credit-byte domains,
            # cabinet bit3 in the HIGH BYTE, blink transitions and WORD wrap.
            import itertools
            for counter,credits,config,old_flag,blink in itertools.product(
                    (0,0x3f,0x40,0x7f,0xfffe,0xffff),
                    (0,0x100,0x200,0x300,1,2),(0,3,0x800,0x803),(0,0x80),(0,1)):
                state.pc=0x83a4;cpu.w_pc(0x83a4);cpu.w_sp(0x1d0000)
                set_field(0x6f,0,1);set_field(0x1cd8,counter);set_field(0x4e,credits)
                set_field(0x66,config);set_field(0x1278,old_flag,1);set_field(0x1279,blink,1)
                boundary();branch_cases+=1
            assert text_groups=={1,2,3,0x53,0x54,0x55,0x56,0x8054,0x8056},text_groups
            required=(0x8332,0x833e,0x8352,0x8368,0x83f4,0x8408,0x842a,0x8438,
                      0x844c,0x8452,0x8460,0x8470,0xa44e,0xa48c,0x962,0x974)
            assert all(pc in coverage for pc in required),('uncovered branches',[hex(pc) for pc in required if pc not in coverage])
        else:natural_sync=dict(sync_counts)
        report={'passed_boundaries':total,'rom_sha1':SHA1,'natural_sync_counts':{hex(k):v for k,v in natural_sync.items()},
            'text_branch_cases':branch_cases,'no_ack_wait_polls':0 if args.smoke else 3,'text_groups':[hex(v) for v in sorted(text_groups)],
            'dependency_calls':{hex(k):v for k,v in calls.items()},'covered_controller_pcs':[hex(v) for v in sorted(coverage)],
            'source_sha256':hashlib.sha256((ROOT/'src/wf_r18_title.c').read_bytes()).hexdigest(),
            'limitations':'Controller differential with original-ROM dependency pre/post contracts; does not validate native graphics, palette/B312, text, sound2052 or VBlank/credit callbacks. A7/CCR excluded. Credited START IRQ outside controller.',
            'rom_modifications':[],'helper_intercepts':[]}
        if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
        machine.cleanup();print(f'PASS {total} title boundaries; RAM16KiB/D0-D7/A0-A6/dependency contracts')

if __name__=='__main__':main()
