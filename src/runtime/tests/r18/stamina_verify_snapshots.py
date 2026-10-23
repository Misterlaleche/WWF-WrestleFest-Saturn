#!/usr/bin/env python3
"""Replay observed HOST damage-owner inputs through unmodified original ROM.

Binary snapshots come from stamina_phase_trace.c; all nine actor records and
D0-D7 are captured before and after each real $24E58 invocation. This proves
equivalence of that owner on the observed route, not full arcade-match replay.
"""
import argparse
import hashlib
import json
import pathlib
import struct
import machine68k as mus

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('rom',type=pathlib.Path)
parser.add_argument('snapshots',type=pathlib.Path)
parser.add_argument('--report',type=pathlib.Path)
args=parser.parse_args()
rom=args.rom.read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
source=args.snapshots.read_bytes();position=0;count=0;mismatches=[];damage_events=[]
machine=mus.Machine(mus.CPUType.M68000,2048);cpu=machine.cpu;bus=machine.mem;bus.w_block(0,rom)
def word(actors,slot,offset):return int.from_bytes(actors[slot*0x10c+offset:slot*0x10c+offset+2],'big')
while position<len(source):
    magic,frame,status,length=struct.unpack_from('=4I',source,position);position+=16
    assert magic==0x5354414d and status==0 and length==9*0x10c,(magic,frame,status,length)
    before_regs=struct.unpack_from('=8I',source,position);position+=32
    before=source[position:position+length];position+=length
    after_regs=struct.unpack_from('=8I',source,position);position+=32
    after=source[position:position+length];position+=length
    bus.w_block(0x1c0000,bytes(0x2000));bus.w_block(0x1c05b0,before)
    for i,value in enumerate(before_regs):cpu.w_reg(i,value)
    for i in range(7):cpu.w_reg(8+i,0)
    cpu.w_sp(0x1d0000);bus.w32(0x1d0000,0x70000);cpu.w_pc(0x24e58)
    for budget in range(100000):
        if cpu.r_pc()==0x70000:break
        cpu.execute(1)
    assert cpu.r_pc()==0x70000,(frame,hex(cpu.r_pc()))
    rom_actors=bytes(bus.r_block(0x1c05b0,length));rom_regs=tuple(cpu.r_reg(i) for i in range(8))
    if rom_actors!=after or rom_regs!=after_regs:
        mismatches.append({'frame':frame,'actor_differences':[(hex(i),n,r) for i,(n,r) in enumerate(zip(after,rom_actors)) if n!=r][:16],'native_D':after_regs,'ROM_D':rom_regs})
    for slot in range(9):
        if word(before,slot,0x66)!=word(after,slot,0x66):
            damage_events.append({'frame':frame,'slot':slot,'live_before':word(before,slot,0x66),'pending':word(before,slot,0x68),'live_after':word(after,slot,0x66),'queue':word(after,slot,0x6a),'tier':word(after,slot,0x70)})
    count+=1
machine.cleanup()
report={'snapshots':count,'source_sha256':hashlib.sha256(source).hexdigest(),'rom_sha1':hashlib.sha1(rom).hexdigest(),'damage_events':damage_events,'mismatches':mismatches}
if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps({'snapshots':count,'damage_events':len(damage_events),'mismatches':mismatches},indent=2))
assert not mismatches,'observed native damage owner differs from original ROM'
