#!/usr/bin/env python3
"""P0-02: ROM-instruction comparison of $1F9F8 and $202BA, plus explicit guards.

This uses a narrow in-tree instruction interpreter, not a certified full CPU.
Optional --machine68k verifies it against an independent Musashi installation.
All input boundary cases are synthetic. No exact hardware state is claimed.
"""
import argparse
import hashlib
import itertools
import json
import os
from pathlib import Path
import struct
import subprocess
import sys
import zipfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'tests/p0_02'))
from rom_subset import CPU, REF, RAM, STOP
A = 0x5b0
R = REF - RAM
ACTORS = [0x1c05b0 + i * 0x10c for i in range(9)]
SIZE = 0x4000
OUTSIZE = 40 + SIZE
REGS = [0xabcde000 + i * 0x101 for i in range(8)]
ROM_SHA1 = 'a726c46f350723ea73158bc78055d17d418b14e2'


def sha(data):
    return hashlib.sha256(data).hexdigest()


def w(raw, offset, value, size=4):
    raw[offset:offset+size] = value.to_bytes(size, 'big')


def state():
    raw = bytearray(SIZE)
    w(raw, R+0x20, 2, 2)
    w(raw, R+0x56, ACTORS[0])
    w(raw, 0x1684, ACTORS[0]); w(raw, 0x1688, ACTORS[1])
    w(raw, R+0x06, 0x280, 2)
    w(raw, R+0x23, 10, 1)
    for i, pointer in enumerate(ACTORS):
        base = pointer - RAM
        w(raw, base+0x06, 0x180+i*0x30, 2)
        w(raw, base+0x32, 1, 1)
        w(raw, base+0x34, 0xfd, 1)
    return raw


def make_cases():
    cases = []
    def add(name, op, raw, guard=None, regs=None):
        cases.append(dict(name=name, operation=op, raw=bytes(raw), regs=list(REGS if regs is None else regs), guard=guard))
    # Target selection: all actor slots, raw pointer tags, no target, both conditions.
    targets = [0, 0x80000000, 0xff000000, ACTORS[0], ACTORS[1], 0x801c05b0, 0xff1c05b0]
    for slot, tag, target, via_substate in itertools.product(range(9), (0, 0x80000000, 0xff000000), targets, (False, True)):
        raw = state(); old = ACTORS[slot] | tag
        w(raw, R+0x56, old); w(raw, 0x1688, target)
        if via_substate:
            w(raw, ACTORS[slot]-RAM+0x32, 0, 1)
            w(raw, ACTORS[slot]-RAM+0x21, 1, 1); w(raw, ACTORS[slot]-RAM+0xaf, 4, 1)
        add(f'primary-s{slot}-tag{tag:08x}-to{target:08x}-sub{int(via_substate)}', 0, raw)
    for old in (0, 0x80000000, 0xff000000):
        raw = state(); w(raw, R+0x56, old); add(f'primary-old-null-{old:08x}', 0, raw)
    for lock in (1, 0x4000, 0xffff):
        raw = state(); w(raw, A+0xfe, lock, 2); add(f'primary-lock-{lock:x}', 0, raw)
    for st, sub in ((0, 4), (1, 3), (2, 4)):
        raw = state(); w(raw, A+0x32, 0, 1); w(raw, A+0x21, st, 1); w(raw, A+0xaf, sub, 1)
        add(f'primary-inactive-condition-{st}-{sub}', 0, raw)
    for slot, pin in itertools.product(range(9), (0, 1, 2, 3)):
        raw = state(); w(raw, 0x7c, 1, 2)
        w(raw, ACTORS[slot]-RAM, 0x8000, 2)
        w(raw, ACTORS[slot]-RAM+0x35, pin, 1); w(raw, ACTORS[slot]-RAM+0x33, 1, 1)
        add(f'acquire-slot{slot}-pin{pin}', 0, raw)
    # Count/instruction initialization: empty/aliased targets, both conditional bit writes.
    for link, p4, p8, flags in itertools.product((ACTORS[0], 0, 0x80000000),
              (ACTORS[0], 0, 0x80000000), (ACTORS[1], 0, 0xff000000), (0, 1)):
        raw = state(); w(raw, R+0x56, link); w(raw, 0x1684, p4); w(raw, 0x1688, p8)
        w(raw, A+0x32, flags, 1); w(raw, ACTORS[1]-RAM+0x32, flags, 1)
        add(f'action-null-{link:08x}-{p4:08x}-{p8:08x}-flags{flags}', 1, raw)
    for slot, tag, rx, facing in itertools.product(range(9), (0, 0x80000000), (0, 0x280, 0xffff), (0, 0x7f, 0x80, 0xff)):
        raw = state(); w(raw, R+0x56, ACTORS[slot]|tag); w(raw, R+6, rx, 2); w(raw, R+0x2e, facing, 1)
        add(f'action-s{slot}-tag{tag:08x}-x{rx:04x}-face{facing:02x}', 1, raw)
    for pose, timer in itertools.product((7, 8, 0), (0, 1, 2, 255)):
        raw = state(); w(raw, R+0x1c, 0x80, 1); w(raw, R+0x05, pose, 1); w(raw, R+0x23, timer, 1)
        w(raw, R+0x56, 0xdeadbeef); w(raw, 0x1684, 0xdeadbeef); w(raw, 0x1688, 0xdeadbeef)
        add(f'action-already-initialized-pose{pose}-timer{timer}', 1, raw)
    # A valid, byte-exact baseline control even before the known MOVE.W fix.
    raw = state(); regs = list(REGS); regs[0] = (regs[0] & 0xffff0000) | 0x180
    add('action-baseline-matching-register', 1, raw, regs=regs)
    # Public $1F914 dispatcher: primary9 is a literal RTS; action7 is selected
    # through the existing latch, not by directly calling the static leaf.
    for link, p4, p8, latch in itertools.product((ACTORS[0], 0, 0x80000000),
              (ACTORS[0], 0), (ACTORS[1], 0, 0xff000000), (0, 0x80)):
        raw = state(); w(raw, R+0x20, 9, 2); w(raw, R+0x1c, (latch<<8)|7, 2)
        w(raw, R+0x56, link); w(raw, 0x1684, p4); w(raw, 0x1688, p8)
        w(raw, R+0x2e, 0x7f, 1)
        add(f'public-count-{link:08x}-{p4:08x}-{p8:08x}-latch{latch:x}', 2, raw)
    # Guard policy is native safety, not an assertion of original ROM behavior.
    for invalid in (1, 0x1c0000, 0x1c0500, 0x1c05b2, 0x1fffffff):
        raw = state(); w(raw, 0x1688, invalid); add(f'guard-primary-{invalid:x}', 0, raw, 0x1c1688)
        for offset, dep in ((R+0x56, 0x202d6), (0x1684, 0x1c1684), (0x1688, 0x1c1688)):
            raw = state(); w(raw, offset, invalid); add(f'guard-action-{offset:x}-{invalid:x}', 1, raw, dep)
    for invalid in (1, 0x1c0000, 0x1c0500, 0x1c05b2, 0x1fffffff):
        for offset, dep in ((R+0x56, 0x202d6), (0x1684, 0x1c1684), (0x1688, 0x1c1688)):
            raw = state(); w(raw, R+0x20, 9, 2); w(raw, R+0x1c, 7, 2)
            w(raw, offset, invalid)
            add(f'guard-public-{offset:x}-{invalid:x}', 2, raw, dep)
    return cases


def read_rom(args):
    if args.rom_zip:
        with zipfile.ZipFile(args.rom_zip) as z:
            rom = bytearray(0x80000)
            rom[::2] = z.read('31e14-0.ic18'); rom[1::2] = z.read('31e13-0.ic19')
        rom = bytes(rom)
    else:
        rom = args.rom.read_bytes()
    if hashlib.sha1(rom).hexdigest() != ROM_SHA1:
        raise ValueError('Wrong arcade ROM identity: do not weaken this check')
    return rom


def musashi_check(rom, cases, expected):
    import machine68k as mus
    machine = mus.Machine(mus.CPUType.M68000, 2048)
    cpu, mem = machine.cpu, machine.mem
    def ignore_rom_write(at, value): pass
    mem.set_special_range_write_funcs(0, 8, w8=ignore_rom_write, w16=ignore_rom_write, w32=ignore_rom_write)
    try:
        for case, (ram, regs, _) in zip(cases, expected):
            if case['guard'] is not None: continue
            mem.w_block(0, rom); mem.w_block(RAM, case['raw'])
            cpu.w_sr(0x2700)
            for i, value in enumerate(case['regs']): cpu.w_reg(i, value)
            for i in range(8, 15): cpu.w_reg(i, REF if i == 8 else 0)
            cpu.w_sp(0x1d0800); mem.w32(0x1d0800, STOP)
            cpu.w_pc((0x1f9f8, 0x202ba, 0x1f914)[case['operation']])
            count = 0
            while cpu.r_pc() != (0x1f934 if case['operation'] == 2 else STOP):
                cpu.execute(1); count += 1
                if count > 2000: raise AssertionError(('external instruction limit', case['name']))
            assert mem.r_block(RAM, SIZE) == ram, ('external RAM mismatch', case['name'])
            assert [cpu.r_reg(i) for i in range(8)] == regs, ('external registers mismatch', case['name'])
            assert mem.r_block(0, len(rom)) == rom
    finally:
        machine.cleanup()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rom', type=Path, default=ROOT/'source_assets/ROM68K.BIN')
    parser.add_argument('--rom-zip', type=Path)
    parser.add_argument('--source', type=Path, default=ROOT/'core/src/wf_r17_referee.c')
    parser.add_argument('--output', type=Path, default=ROOT/'build/p0-02')
    parser.add_argument('--sanitize', action='store_true')
    parser.add_argument('--machine68k', action='store_true')
    args = parser.parse_args(); out = args.output.resolve(); out.mkdir(parents=True, exist_ok=True)
    source = args.source.resolve(); rom = read_rom(args); cases = make_cases()
    command = [os.environ.get('CC', 'cc'), '-std=c11', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
               '-ffunction-sections', '-fdata-sections', '-I'+str(ROOT/'core/include'),
               '-DWF_REF_SOURCE="'+str(source)+'"']
    if args.sanitize: command += ['-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-fno-pie', '-no-pie']
    command += [str(ROOT/'tests/p0_02/probe.c')]
    command += [str(ROOT/'core'/p) for p in ('src/wf_actor.c','src/wf_r17_m68k_shadow.c','src/wf_r18_runtime_handoff.c',
                                            'src/wf_physics.c','generated/wf_physics_tables.c','src/wf_r17_actions_r11.c')]
    binary = out/'probe'; command += ['-Wl,--gc-sections', '-o', str(binary)]
    build = subprocess.run(command, capture_output=True, text=True); (out/'compile.log').write_text(build.stdout+build.stderr)
    build.check_returncode()
    packed = b''.join(struct.pack('>I8I', c['operation'], *c['regs']) + c['raw'] for c in cases)
    (out/'inputs.bin').write_bytes(packed)
    run = subprocess.run([str(binary), str(out/'inputs.bin'), str(out/'outputs.bin')], capture_output=True, text=True,
                         env=dict(os.environ, ASAN_OPTIONS='detect_leaks=0', UBSAN_OPTIONS='halt_on_error=1'))
    (out/'native.log').write_text(run.stdout+run.stderr); run.check_returncode()
    data = (out/'outputs.bin').read_bytes(); assert len(data) == len(cases)*OUTSIZE
    failures=[]; results=[]; expected=[]; pcs=set()
    for i, case in enumerate(cases):
        rec = data[i*OUTSIZE:(i+1)*OUTSIZE]; status, dep, *d = struct.unpack('>10I', rec[:40]); actual = rec[40:]
        if case['guard'] is not None:
            good = status == 1 and dep == case['guard']
            why = dict(status=status, dependency=hex(dep), expected_dependency=hex(case['guard']))
            expected.append((None, None, None))
        else:
            oracle = CPU(rom, case['raw'], case['regs']); ram, regs = oracle.run((0x1f9f8, 0x202ba, 0x1f914)[case['operation']], stop=0x1f934 if case['operation'] == 2 else STOP)
            pcs.update(oracle.trace); expected.append((ram, regs, oracle))
            good = status == 0 and dep == 0 and actual == ram and d == regs
            why = dict(status=status, dependency=hex(dep),
                       ram_differences=[dict(address=hex(RAM+j), native=a, rom=b) for j,(a,b) in enumerate(zip(actual,ram)) if a!=b][:12],
                       register_differences=[dict(register='D'+str(j), native=hex(a), rom=hex(b)) for j,(a,b) in enumerate(zip(d,regs)) if a!=b])
        results.append(dict(name=case['name'], passed=good, guard=case['guard'], output_sha256=sha(rec)))
        if not good: failures.append(dict(name=case['name'], **why))
    external = 'NOT_RUN: use --machine68k with installed trusted dependency'
    if args.machine68k:
        musashi_check(rom, cases, expected); external = 'PASS'
    report = dict(status='FAIL' if failures else 'PASS_SCOPED_HOST', scope=__doc__,
                  source_sha256=sha(source.read_bytes()), rom_sha1=ROM_SHA1, rom_sha256=sha(rom),
                  total=len(cases), passed=len(cases)-len(failures), failed=len(failures),
                  differential_cases=sum(c['guard'] is None for c in cases), guards=sum(c['guard'] is not None for c in cases),
                  sanitized=args.sanitize, external_machine68k=external, command=command,
                  inputs_sha256=sha(packed), outputs_sha256=sha(data),
                  original_pcs_executed=[hex(pc) for pc in sorted(pcs)], failures=failures, cases=results)
    (out/'report.json').write_text(json.dumps(report, indent=2)+'\n')
    (out/'cases.json').write_text(json.dumps([{k:v for k,v in c.items() if k!='raw'} for c in cases], indent=2)+'\n')
    for name in ('primary-s0-tag00000000-to00000000-sub0','action-null-001c05b0-001c05b0-00000000-flags1'):
        for case, (ram, regs, oracle) in zip(cases, expected):
            if case['name']==name:
                (out/(name+'-trace.json')).write_text(json.dumps(dict(pcs=[hex(p) for p in oracle.trace], rom_writes=oracle.rom_writes),indent=2)+'\n')
    print(json.dumps({k:report[k] for k in ('status','total','passed','failed','differential_cases','guards','external_machine68k')}))
    if failures: print(json.dumps(failures[:4],indent=2))
    return bool(failures)


if __name__ == '__main__':
    raise SystemExit(main())
