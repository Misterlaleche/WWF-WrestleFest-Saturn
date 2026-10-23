#!/usr/bin/env python3
"""Stamina state, pacing and HUD stores versus unchanged original ROM.

Test dependency: machine68k==0.4.1 (Musashi); no emulation in production.
Builds a temporary native shared library from the current CORE sources.
Compares complete actor/global RAM, FG0/palette bytes, D0-D7 and ordered
FG0 stores after every owner. Participant helpers also compare A0-A6.
The fixtures set equivalent inputs; they do not claim complete match coverage.
"""
import argparse
import ctypes
import hashlib
import json
import pathlib
import random
import re
import subprocess
import tempfile

import machine68k as mus

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('rom',type=pathlib.Path)
parser.add_argument('--report',type=pathlib.Path)
args=parser.parse_args()
runtime=pathlib.Path(__file__).resolve().parents[2]
rom=args.rom.read_bytes()
assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
entries=(0x24e58,0x7548,0x8ecc,0x7506,0x75ee,0x782c)
fields=(0x66,0x68,0x6a,0x6c,0x6e,0x70,0x72,0x106)
cases=[]

def fixture(name,owners=(0,1),values=None,active=(0,),reserve=()):
    memory=bytearray(0x200000);memory[:len(rom)]=rom
    memory[0xc0000:0xc4000]=b'\xa5'*0x4000
    def w(at,value,n=2):memory[at:at+n]=(value&((1<<(8*n))-1)).to_bytes(n,'big')
    w(0x1c007c,1);w(0x1c0161,1,1)
    for slot in active:
        a=0x1c05b0+slot*0x10c
        w(a,128,1);w(a+0x56,128 if slot in reserve else 0,1)
        w(a+0x66,110);w(a+0x6e,110);w(a+0x72,110)
        w(a+0x8a,0x140020+2*(slot%4),4)
    for at,value in (values or {}).items():w(at,value[0],value[1]) if isinstance(value,tuple) else w(at,value)
    cases.append((name,memory,list(owners)))

a=0x1c05b0
fixture('initial-full-stamina',(3,0,1))
for pending in (1,7,23,24,109,110,111):
    fixture('damage-%d'%pending,(0,1,1,1,1),{a+0x68:pending})
fixture('damage-saturates-display',(0,)+tuple([1]*56),{a+0x68:111})
fixture('lockout-defers-damage',(0,1),{a+0x68:23,a+0xfe:1,a+0x6a:1})
fixture('no-pending-preserves-tier',(0,),{a+0x66:25,a+0x70:2})
fixture('no-pending-max-zero-preserves',(0,),{a+0x72:0,a+0x70:0x1234})
for value in (23,24,25,72,73,74):
    fixture('tier-boundary-%d'%value,(0,),{a+0x66:value+1,a+0x68:1})
fixture('maintenance-countdown',(0,),{a+0x54:2,a+0x52:0x1234,a+0x32:(8,1),a+0xd0:2})
fixture('maintenance-expiration',(0,),{a+0x54:0,a+0x52:0x1234,a+0x32:(8,1),a+0xd0:1})
fixture('lockout-still-maintains-timers',(0,),{a+0xfe:1,a+0x68:7,a+0x54:1,a+0x32:(8,1),a+0xd0:1})
fixture('nine-active-actors',(0,),{a+8*0x10c+0x68:7},active=tuple(range(9)))
fixture('royal-128th-tick',(2,0,1),{a+0x66:24,a+0x6e:24,a+0x70:2,a+0x106:127})
fixture('royal-full-pauses-counter',(2,),{a+0x106:127})
fixture('royal-excluded-pauses-counter',(2,),{a+0x66:24,a+0x1d:(0x0b,1),a+0x106:127})
fixture('royal-before-threshold',(2,1),{a+0x66:24,a+0x6e:24,a+0x106:126})
fixture('royal-128-eligible-owners',tuple([2]*128)+tuple([1]),{a+0x66:24,a+0x6e:24,a+0x70:2})
fixture('positive-queue-replaced-by-damage',(0,1),{a+0x68:7,a+0x6a:1})
for visible in (0,1,2,15,16,17,23,24,25,95,96,97,110):
    fixture('glyph-%d'%visible,(4,),{a+0x6e:visible})
for delta in (-3,-2,-1,1,2,3):
    fixture('signed-display-delta-%d'%delta,(1,1),{a+0x66:55,a+0x6e:55,a+0x6a:delta})
fixture('display-negative-clamp',(1,),{a+0x6e:0,a+0x6a:-3})
fixture('display-max-clamp',(1,),{a+0x6e:110,a+0x6a:3})
fixture('hud-byte-gate-high-byte-clear',(1,),{0x1c16c4:0x0100,a+0x6a:1})
fixture('hud-byte-gate-low-byte-set',(1,),{0x1c16c4:1,a+0x6a:1})
fixture('hud-uncredited',(1,),{0x1c007c:0,a+0x6a:1})
fixture('hud-four-actors',(3,1),active=(0,1,2,3))
fixture('hud-reserve-not-updated',(1,),{a+0x6a:-7},reserve=(0,))
for slot in range(4):
    fixture('init-all-slots-%d'%slot,(3,),active=(slot,))

def state(memory):
    return {hex(offset):int.from_bytes(memory[a+offset:a+offset+2],'big') for offset in fields}

report={'rom_sha1':hashlib.sha1(rom).hexdigest(),'fixtures':[],'mismatches':[]}
with tempfile.TemporaryDirectory(prefix='stamina-rom-') as temp:
    library=pathlib.Path(temp)/'stamina.so'
    core=re.search(r'^CORE=(.*)$',(runtime/'Makefile').read_text(),re.M).group(1).split()
    subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,'tests/r18/stamina_oracle_bridge.c','-o',str(library)],cwd=runtime,check=True)
    native=ctypes.CDLL(str(library));native.wf_stamina_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint,ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint)]
    for index,(name,memory,owners) in enumerate(cases):
        rng=random.Random(0x754800+index);d=[rng.getrandbits(32) for _ in range(8)];areg=[0x1c05b0]+[0x12345678+i for i in range(1,7)]
        if owners[0] in (4,5):d[0]=0
        nd=(ctypes.c_uint32*8)(*d);na=(ctypes.c_uint32*7)(*areg);nb=ctypes.create_string_buffer(bytes(memory))
        machine=mus.Machine(mus.CPUType.M68000,2048);cpu=machine.cpu;bus=machine.mem;bus.w_block(0,bytes(memory))
        for i,v in enumerate(d):cpu.w_reg(i,v)
        for i,v in enumerate(areg):cpu.w_reg(8+i,v)
        steps=[];fixture_failed=False
        for step,owner in enumerate(owners):
            writes=[]
            def trace(op,size,address,value):
                if op=='W' and (0xc0000<=address<0xc4000 or 0x180000<=address<0x181000):writes.append((address,value,1<<size))
            bus.set_trace_func(trace);bus.set_trace_mode(True)
            cpu.w_sp(0x1d0000);bus.w32(0x1d0000,0x70000);cpu.w_pc(entries[owner])
            for budget in range(100000):
                if cpu.r_pc()==0x70000:break
                cpu.execute(1)
            assert cpu.r_pc()==0x70000,(name,owner,hex(cpu.r_pc()))
            ntrace=(ctypes.c_uint32*(4096*3))();ncount=ctypes.c_uint()
            status=native.wf_stamina_oracle(nb,nd,na,owner,ntrace,ctypes.byref(ncount));assert status==0,(name,owner,status)
            native_writes=[tuple(ntrace[i*3:i*3+3]) for i in range(ncount.value)]
            actual=nb.raw
            expected_ram=bytes(bus.r_block(0x1c0000,0x2000));expected_fg=bytes(bus.r_block(0xc0000,0x4000));expected_palette=bytes(bus.r_block(0x180000,0x1000))
            failures=[]
            for label,address,want in [('RAM',0x1c0000,expected_ram),('FG0',0xc0000,expected_fg),('palette',0x180000,expected_palette)]:
                got=actual[address:address+len(want)]
                if got!=want:failures.append({'part':label,'differences':[(hex(address+i),g,e) for i,(g,e) in enumerate(zip(got,want)) if g!=e][:20]})
            want_d=[cpu.r_reg(i) for i in range(8)]
            if list(nd)!=want_d:failures.append({'part':'D0-D7','native':list(nd),'ROM':want_d})
            if owner>=3 and list(na)!=[cpu.r_reg(8+i) for i in range(7)]:failures.append({'part':'A0-A6'})
            if native_writes!=writes:failures.append({'part':'ordered stores','native_count':len(native_writes),'ROM_count':len(writes),'first_difference':next(((i,g,e) for i,(g,e) in enumerate(zip(native_writes,writes)) if g!=e),None)})
            rom_state=bytearray(0x200000);rom_state[0x1c0000:0x1c2000]=expected_ram
            steps.append({'owner':hex(entries[owner]),'native':state(actual),'ROM':state(rom_state),'stores':len(writes)})
            if failures:
                report['mismatches'].append({'fixture':name,'step':step,'owner':hex(entries[owner]),'failures':failures});fixture_failed=True;break
        report['fixtures'].append({'name':name,'initial':state(memory),'steps':steps,'pass':not fixture_failed})
        machine.cleanup()
report['passed']=sum(item['pass'] for item in report['fixtures']);report['total']=len(cases)
if args.report:
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps({'passed':report['passed'],'total':report['total'],'mismatches':report['mismatches']},indent=2))
assert not report['mismatches'],'native/ROM stamina divergence; see report'
