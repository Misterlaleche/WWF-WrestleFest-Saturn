#!/usr/bin/env python3
"""P0-06: execute unchanged ROM for EBC4/EBF6 and E496, compare real C.

Checks all 16 KiB RAM and D0-D7, plus hand-derived MASH outcomes. The inherited
opcode runner is not an independent CPU emulator or Saturn timing validation.
Invalid-input cases assert documented native guards, not undefined ROM behavior.
"""
import argparse
import hashlib
import itertools
import json
import os
from pathlib import Path
import struct
import subprocess
from test_human_p0_04 import ROOT, RAM, SIZE, OUTSIZE, ROM_SHA1, REGS, w, ptr, sha, compile_probe
from test_human_p0_05 import state, ref, compile_phase

# Literal transitions from ROM EBE6, EC88..EDA0. Not generated from C.
MASH = {0x4a:0x4b, 0x51:0x4b, 0x5d:0x67, 0x5e:0x56, 0x5f:0x5c,
        0x60:0x5b, 0x61:0x59, 0x62:0x5a, 0x63:0x55, 0x52:0x58,
        0x53:0x57, 0x64:0x4b}
TAGS = (0, 0x80000000, 0xff000000)


def cases_for(phase=False):
    cases=[]
    def add(name,b,slot=0,group='mash',pcs=(),guard=0,literal=None,prefix=()):
        if phase:
            # Real prepass dispatch, with all other slots inactive. No callbacks
            # replaced. Human flag is required to enter DCDE; bit2 clear in FLAGS34.
            a=ptr(slot)-RAM
            w(b,a+0x33,b[a+0x33]|2,1)
            w(b,a+0x34,b[a+0x34]&~4,1)
        cases.append(dict(name=name,raw=bytes(b),slot=slot,group=group,pcs=list(pcs),
                          guard=guard,literal=literal,prefix=list(prefix),regs=list(REGS)))
    for sel,ch,fresh,tag,hi in itertools.product(MASH,range(12),(1,2,3),TAGS,(0,0x9000)):
        slot=0 if phase else (sel+ch+fresh)%9;a=ptr(slot)-RAM
        b=state(slot,st=0x8005,ch=ch,fresh=fresh,target=tag)
        w(b,a+0x60,hi|sel);w(b,a+0xaa,0x4000)
        add(f'mash-{sel:x}-c{ch}-f{fresh}-t{tag:x}-h{hi:x}',b,slot,
            pcs=(0xebc4,) if sel in (0x4a,0x51) else (0xebf6,0xec7a),
            literal=(5,MASH[sel],0))
    # Neighbor counter values must NOT trigger completion, even if the low
    # selector byte is a special. No-input cases preserve selector and MASH.
    for sel,mash,fresh,tag in itertools.product((0x4a,0x5d,0x64),(0,0x3fff,0x4000,0x4001,0xffff),(0,1),TAGS):
        b=state(st=0x8005,fresh=fresh,target=tag);w(b,0x610,sel);w(b,0x65a,mash)
        add(f'threshold-{sel:x}-m{mash:x}-f{fresh}-t{tag:x}',b,group='threshold',
            pcs=(0xebc4,) if fresh else (),literal=(5,MASH[sel],0) if fresh and mash==0x4000 else None)
    # FF bit4 at physical zero prevents both state4 completion branches.
    # Valid target bit4 distinguishes the existing $38/$78 paths.
    for sub,target,flags,tag in itertools.product((8,9),(0,ptr(1)),(0,0x10),TAGS):
        b=state(st=0x8004,target=target|tag);w(b,0x615,sub,1);w(b,0x65a,0x4000)
        if target:w(b,target-RAM+0x34,flags,1)
        add(f'state4-u{sub}-t{target:x}-b{flags:x}-tag{tag:x}',b,group='state4',pcs=(0xebf6,),
            literal=(5,0x38 if sub==8 else 0x78,0) if target and not flags else None)
    # Carry-clear continuation from MASH into idle classifiers/directions.
    for st,flags,rel,fresh,tag in itertools.product((0x8000,0x8001),(0,0x20,0x22),(0,0x80),(1,3),TAGS):
        b=state(st=st,flags=flags,rel=rel,fresh=fresh,target=tag);w(b,0x65a,0x4000)
        add(f'mash-fallthrough-s{st:x}-b{flags:x}-r{rel:x}-f{fresh}-t{tag:x}',b,group='fallthrough',pcs=(0xebf6,))
    # Every character/input at zero bus, with both facings and high-byte tags.
    for ch,fresh,tag,face in itertools.product(range(12),(1,2,3),TAGS,(0,0x80)):
        b=state(st=0x8001,ch=ch,fresh=fresh,target=tag,face=face,remap=0xffff)
        w(b,0x5e2,3,1);w(b,0x65f,1,1)
        add(f'alt-c{ch}-f{fresh}-t{tag:x}-d{face:x}',b,group='alternate',pcs=(0xe496,0xf178))
    # Real targets, reciprocal empty/nonempty links, tags, all actor slots.
    # Self-target must be handled in original write/test order.
    for slot,selflink,tag,reciprocal,fresh in itertools.product(range(9),(False,True),TAGS,(0,0x12345678),(1,3)):
        target=ptr(slot if selflink else (slot+1)%9);a=ptr(slot)-RAM
        b=state(slot,st=0x8001,ch=slot%12,fresh=fresh,target=target|tag)
        w(b,a+0x32,3,1);w(b,a+0xaf,1,1)
        w(b,target-RAM+0x26,reciprocal,4)
        add(f'actor-s{slot}-self{int(selflink)}-t{tag:x}-l{reciprocal:x}-f{fresh}',b,slot,group='actor-target',pcs=(0xe496,0xf178))
    # E496 only accepts state1/AF1. State or AF neighbor values must not link.
    for st,af,fresh,held in itertools.product((0x8000,0x8001,0x8005),(0,1,2,0xff),(0,1,3),(0,8)):
        b=state(st=st,fresh=fresh,held=held);w(b,0x5e2,3,1);w(b,0x65f,af,1)
        add(f'alt-prefix-s{st:x}-a{af}-f{fresh}-h{held}',b,group='prefix',pcs=(0xe496,) if fresh else (),
            guard=0xf25a if phase and st==0x8001 and af==0xff else 0)
        if cases[-1]['guard']==0xf25a:
            # Deliberately invalid state1 subindex 63: production guard, not
            # a legal call through the original 14-entry jump table.
            cases[-1]['guard_d1']=(REGS[1]&0xffff0000)|252
    # MASH terminal must preempt E496 and the later action-selection tables.
    for sel,tag,fresh in itertools.product(MASH,TAGS,(1,2,3)):
        b=state(st=0x8005,target=tag,fresh=fresh);w(b,0x5e2,3,1);w(b,0x65f,1,1)
        w(b,0x610,sel);w(b,0x65a,0x4000)
        add(f'priority-{sel:x}-t{tag:x}-f{fresh}',b,group='priority',pcs=(0xebc4,),literal=(5,MASH[sel],0))
    # Never use the ROM-zero exception for a nonzero invalid bus address.
    for address,tag in itertools.product((1,2,0x100,ptr(0)+2,0x1c2000),TAGS):
        b=state(st=0x8005,target=address|tag);w(b,0x610,0x5d);w(b,0x65a,0x4000)
        add(f'guard-mash-{address:x}-t{tag:x}',b,group='guard',guard=0xebf6)
        b=state(st=0x8001,target=address|tag);w(b,0x5e2,3,1);w(b,0x65f,1,1)
        add(f'guard-alt-{address:x}-t{tag:x}',b,group='guard',guard=0xe496)
    # The ROM indexes by a WORD; guard must reject the complete word BEFORE
    # array access. F178 already stored own link; that prefix must remain.
    for ch,target in itertools.product((12,255,256,0x1000,0xffff),TAGS):
        b=state(st=0x8001,ch=ch,target=target);w(b,0x5e2,3,1);w(b,0x65f,1,1)
        add(f'guard-char-{ch}-t{target:x}',b,group='char-guard',guard=0xe4fe,prefix=((0x5d6,target,4),))
    return cases


def run(out,source,group='all',no_trace=False,build=None,keep=False):
    out.mkdir(parents=True,exist_ok=True)
    rom=(ROOT/'source_assets/ROM68K.BIN').read_bytes()
    assert hashlib.sha1(rom).hexdigest()==ROM_SHA1
    assert rom[0x34]==0xff and rom[0x26:0x2a]==b'\xff'*4
    # Freeze the exact small blocks to prevent silently testing a different ROM.
    assert rom[0xebf6:0xec04].hex()=='2268007a08290004003466000078'
    assert rom[0xf178:0xf18a].hex()=='214900264aa900266606234800264e714e75'
    cases=[c for c in cases_for(bool(build)) if group=='all' or c['group']==group]
    assert cases
    cmd,before=compile_phase(out,source,build) if build else compile_probe(out,source,True)
    inputs=b''.join(struct.pack('>II8I',3 if build else 4 if no_trace else 2,c['slot'],*c['regs'])+c['raw'] for c in cases)
    (out/'inputs.bin').write_bytes(inputs)
    cp=subprocess.run([str(out/'probe'),str(out/'inputs.bin'),str(out/'outputs.bin')],capture_output=True,text=True,
        env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1'))
    (out/'native.log').write_text(cp.stdout+cp.stderr);cp.check_returncode()
    data=(out/'outputs.bin').read_bytes();assert len(data)==len(cases)*OUTSIZE
    rows=[];failures=[];exceptions=[];pcs=set();refhash=hashlib.sha256()
    for i,c in enumerate(cases):
        record=data[i*OUTSIZE:(i+1)*OUTSIZE];status,meta,third,*d=struct.unpack('>11I',record[:44]);ram=record[44:]
        dep=third if build else meta;owner=meta if build else 0
        er=bytearray(c['raw']);ed=list(c['regs']);trace=[]
        try:
            if c['guard']:
                for off,value,n in c['prefix']:w(er,off,value,n)
                if build:ed[0]=(ed[0]&0xffff0000)|(c['slot']+1)
                if 'guard_d1' in c:ed[1]=c['guard_d1']
                good=status==1 and dep==(0 if no_trace else c['guard']) and (not build or owner==0xf18a)
            else:
                cpu=ref.CPU(rom,c['raw'],c['regs'])
                if build:cpu.a=[0]*7+[0x1d07fc]
                else:cpu.a[0]=ptr(c['slot'])
                er,ed=cpu.execute_to(0xf18a if build else 0xdcde,limit=15000)
                trace=cpu.trace;pcs.update(trace)
                assert all(p in trace for p in c['pcs']),('required route skipped',c['name'],c['pcs'])
                assert cpu.rom==rom and not cpu.rom_writes,('unexpected ROM writes',c['name'],cpu.rom_writes)
                if c['literal'] and not build:
                    a=ptr(c['slot'])-RAM;st,sel,mash=c['literal']
                    actual=tuple(int.from_bytes(er[a+off:a+off+2],'big') for off in (0x20,0x60,0xaa))
                    assert actual==(st,sel,mash),('literal differs from opcode reference',c['name'],actual,c['literal'])
                good=status==0 and dep==0 and owner==0
            good=good and ram==er and d==ed
        except Exception as e:
            exceptions.append(dict(name=c['name'],error=repr(e)));continue
        refhash.update(er+struct.pack('>8I',*ed))
        row=dict(name=c['name'],group=c['group'],passed=good,status=status,dependency=hex(dep),
            instructions=len(trace),output_sha256=sha(record))
        rows.append(row)
        if not good:
            failures.append(dict(**row,ram_differences=[dict(at=hex(RAM+j),native=x,rom=y) for j,(x,y) in enumerate(zip(ram,er)) if x!=y][:12],
                registers=[dict(reg=j,native=hex(x),rom=hex(y)) for j,(x,y) in enumerate(zip(d,ed)) if x!=y]))
    drift=[s for s,h in before.items() if sha(Path(s).read_bytes())!=h]
    report=dict(status='FAIL' if failures or exceptions or drift else 'PASS_SCOPED_HOST',total=len(cases),evaluated=len(rows),
        passed=sum(r['passed'] for r in rows),failed=len(failures),exceptions=exceptions,failures=failures,
        groups={g:dict(total=sum(c['group']==g for c in cases),passed=sum(r['group']==g and r['passed'] for r in rows)) for g in sorted({c['group'] for c in cases})},
        phase=bool(build),no_trace=no_trace,source_hashes=before,source_drift=drift,command=cmd,
        rom_sha256=sha(rom),inputs_sha256=sha(inputs),outputs_sha256=sha(data),reference_outputs_sha256=refhash.hexdigest(),
        oracle='Inherited limited interpreter P0-05, plus literal MASH assertions. Not independent CPU or hardware validation.',
        original_pcs_executed=[hex(p) for p in sorted(pcs)],cases=rows)
    if build:report['build_manifest_sha256']=sha((build/'runtime-build.json').read_bytes())
    (out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:report[k] for k in ('status','total','evaluated','passed','failed','groups')}))
    if failures:print('FAILURES',json.dumps(failures[:3]))
    if exceptions:print('EXCEPTIONS',json.dumps(exceptions[:3]))
    if not keep:(out/'inputs.bin').unlink();(out/'outputs.bin').unlink()
    return int(report['status']=='FAIL')

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True)
    p.add_argument('--source',type=Path,default=ROOT/'core/src/wf_r17_human_raw23.c')
    p.add_argument('--group',default='all');p.add_argument('--no-trace',action='store_true');p.add_argument('--build-dir',type=Path)
    p.add_argument('--keep-vectors',action='store_true');a=p.parse_args()
    if a.no_trace and a.build_dir:p.error('phase probe does not support --no-trace')
    raise SystemExit(run(a.output.resolve(),a.source.resolve(),a.group,a.no_trace,a.build_dir.resolve() if a.build_dir else None,a.keep_vectors))
