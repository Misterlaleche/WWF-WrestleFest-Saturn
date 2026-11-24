#!/usr/bin/env python3
"""P0-05: idle physical-zero targets, relationship priority and flags.

Expectations execute the unmodified canonical ROM with the inherited limited
opcode runner. No production or ROM helpers are replaced. This is not an
independent 68000 implementation or a Saturn hardware acceptance test.
"""
import argparse, hashlib, importlib.util, itertools, json, os, struct, subprocess
from pathlib import Path
from test_human_p0_04 import (ROOT, RAM, SIZE, OUTSIZE, ROM_SHA1, REGS, w,
                              ptr, sha, state as linked_state, compile_probe)

_spec=importlib.util.spec_from_file_location('p005_subset',ROOT/'tests/p0_05/rom_subset.py')
ref=importlib.util.module_from_spec(_spec);_spec.loader.exec_module(ref)


def state(slot=0, st=0x8000, ch=0, fresh=1, flags=0x20, rel=0,
          target=0, mode=1, old=0x55, held=0, remap=0, face=0):
    b = linked_state(slot, selector=0x1234, target=target, link=ptr((slot+2)%9),
                     ch=ch, fresh=fresh, old=old, face=face, remap=remap)
    a = ptr(slot)-RAM
    w(b, a+0x20, st); w(b, a+0x34, flags, 1); w(b, a+0x74, rel, 1)
    w(b, a+0xa9, held, 1); w(b, a+0x32, 2, 1); w(b, 0x161, mode, 1)
    w(b, a+0x86, ptr((slot+3)%9), 4)
    w(b, a+0x82, ptr((slot+1)%9), 4)
    # These sentinels must not be cleared by category $11/$00 or no-action.
    w(b, a+0x44, 0x1234); w(b, a+0x46, 0xabcd)
    return b


def cases_for(phase=False):
    cases=[]
    def add(name,b,slot=0,group='table',guard=0,regs=REGS):
        cases.append(dict(name=name,raw=bytes(b),slot=slot,group=group,guard=guard,regs=list(regs)))
    # Cross product of every character, both idle states, branch-driving bits,
    # both old toggle values and each fresh action. Modes/tags are distributed.
    for st,ch,flags,rel,fresh,old in itertools.product((0x8000,0x8001),range(12),
              (0,2,0x20,0x22),(0,0x80),(1,2,3),(0x55,0xd5)):
        mode=(ch+fresh)%4;tag=(0,0x80000000,0xff000000)[ch%3]
        add(f'table-{st:x}-c{ch}-b{flags:x}-r{rel:x}-f{fresh}-o{old:x}',
            state(st=st,ch=ch,fresh=fresh,flags=flags,rel=rel,mode=mode,
                  old=old,target=tag,remap=(ch+fresh)%15))
    # Exhaustive single-byte FLAGS34 sweep; unrelated bits must not affect
    # the classifier selection. REL low bits/high bit combinations are separate.
    for flags,rel,st in itertools.product(range(256),(0,0x7f,0x80,0xff),(0x8000,0x8001)):
        k=flags+rel+st;slot=0 if phase else k%9
        add(f'bits-{flags:02x}-r{rel:02x}-s{st:x}',
            state(slot,st=st,ch=k%12,fresh=k%3+1,flags=flags,rel=rel,
                  mode=k%4,target=(0,0x80000000,0xff000000)[k%3],
                  old=(k*17)&255,held=k&15,face=k&0x80),slot,group='all-flags')
    # No D3 remap on categories $11/$00, including a remap index that would
    # be invalid if the dynamic category's helper were invoked by mistake.
    for ch,rel,remap,face,fresh in itertools.product(range(12),(0,0x80),
                                                  (0,1,7,14,0xffff),(0,0x80),(1,2,3)):
        add(f'no-remap-c{ch}-r{rel:x}-i{remap}-d{face:x}-f{fresh}',
            state(ch=ch,rel=rel,remap=remap,face=face,fresh=fresh),group='no-remap')
    # Filter geometry close to the canonical ROM-zero coordinates, including
    # unsigned wraparound and the literal X=0654,Y=FFFF. Old supported dynamic
    # path stays compared; table branches are mixed in the same set.
    for x,y,flags,rel in itertools.product((0,0x600,0x653,0x654,0x655,0x6b4,0xffff),
                                         (0,1,0x17,0xffc8,0xfffe,0xffff),(0,0x20),(0,0x80)):
        k=x+y+flags+rel;b=state(ch=k%12,flags=flags,rel=rel,fresh=k%3+1,face=k&0x80)
        w(b,0x5b6,x);w(b,0x5ba,y);w(b,0x5e3,(0,1,4,5)[k%4],1)
        add(f'geometry-x{x:x}-y{y:x}-b{flags:x}-r{rel:x}',b,group='geometry')
    # Prefix paths must still consume before the null-target classification.
    for mode,flags,rel,fresh,held,alt in itertools.product(range(4),(0x20,0x22),
                                                       (0,0x80),(0,1,2,3),(0,8),(0,1)):
        b=state(mode=mode,flags=flags,rel=rel,fresh=fresh,held=held)
        w(b,0x5e2,alt|2,1)
        add(f'prefix-m{mode}-b{flags:x}-r{rel:x}-f{fresh}-h{held}-a{alt}',b,group='prefix')
    for flags,rel,fresh,side in itertools.product((0x20,0x22),(0,0x80),(1,2,3),(0,0x80)):
        b=state(mode=0,flags=flags,rel=rel,fresh=fresh);a=0x5b0;p=ptr(3)-RAM
        w(b,p+0x20,1);w(b,p+0xae,0x8002)
        w(b,a+0x33,side,1);w(b,a+6,0x300 if side else 0x200);w(b,a+0xa,0x120 if side else 0x190)
        add(f'tag-priority-b{flags:x}-r{rel:x}-f{fresh}-side{side:x}',b,group='tag-priority')
    for flags,rel,fresh,object_id in itertools.product((0x20,0x22),(0,0x80),(1,2,3),(0,1)):
        b=state(mode=2,flags=flags,rel=rel,fresh=fresh);a=0x5b0;o=(0xf1c,0x1028)[object_id]
        w(b,a+0x33,4,1);w(b,o+0x1c,3);w(b,o+6,0x280);w(b,o+0xa,0x160)
        add(f'object-priority-b{flags:x}-r{rel:x}-f{fresh}-o{object_id}',b,group='object-priority')
    # Invalid nonzero target addresses do not acquire the zero-bus exception.
    for target,flags,rel in itertools.product((1,2,0x100,ptr(0)+2,0x1c2000,0x801c05b2),
                                            (0,0x20,0x22),(0,0x80)):
        add(f'guard-p{target:x}-b{flags:x}-r{rel:x}',
            state(target=target,flags=flags,rel=rel),group='guard',guard=0xea42)
    for ch,rel in itertools.product((12,256,65535),(0,0x80)):
        add(f'guard-char{ch}-r{rel:x}',state(ch=ch,rel=rel),group='guard',guard=0xe4fe)
    for lock,st in itertools.product((0,1,0x8000),(0,1,0x8000,0x8001)):
        b=state(st=st,flags=0x22,rel=0x80);w(b,0x5b0+0xfe,lock)
        add(f'inactive-s{st:x}-lock{lock:x}',b,group='activation')
    # Unmodified checkpoint snapshot is an observed regression, not new footage.
    p=ROOT/'tests/fixtures/royal_human_target';m=json.loads((p/'manifest.json').read_text())
    for name,digest in m['files'].items():assert sha((p/name).read_bytes())==digest
    add('inherited-observed',(p/'arcade-ram.bin').read_bytes(),group='observed',regs=json.loads((p/'state.json').read_text())['d'])
    if not phase:return cases
    # The public phase visits the active roster. Use the real snapshot for its
    # surrounding actors/global state; vary only slot0 fields audited above.
    baseline=(p/'arcade-ram.bin').read_bytes();d=json.loads((p/'state.json').read_text())['d']
    result=[]
    for c in cases:
        if c['group'] not in ('table','all-flags','no-remap','guard','observed'):continue
        b=bytearray(baseline);a=0x5b0
        if c['group']!='observed':
            b[a:a+0x10c]=c['raw'][a:a+0x10c]
            # The imported selector/player identity used by F18A is real.
            b[a+0x84:a+0x86]=baseline[a+0x84:a+0x86]
            # F18A only invokes human DCDE with FLAGS33.bit1 set and
            # FLAGS34.bit2 clear. Keep real human ownership; bit2 sweep
            # deliberately includes bypass controls as well.
            b[a+0x33] |= baseline[a+0x33] & 2
        result.append(dict(c,raw=bytes(b),slot=0,regs=list(d)))
    # F244 state1 dispatcher must leave (AE & 3f) << 2 in D1 before its
    # subroutine. Bypass human ownership deliberately to isolate that prefix.
    for index,high,faceflag,held in itertools.product(range(14),(0,0x40,0x8000,0xffc0),(0,1),(0,8)):
        b=bytearray(baseline);a=0x5b0;w(b,a+0x20,0x8001);w(b,a+0xae,high|index)
        w(b,a+0x34,0x26,1);w(b,a+0x33,2|faceflag,1);w(b,a+0xa8,held)
        regs=list(d);regs[1]=0x13570003
        result.append(dict(name=f'state1-i{index}-hi{high:x}-b{faceflag}-h{held}',raw=bytes(b),slot=0,
                           regs=regs,group='state1-dispatch',guard=0))
    for index,expected in ((14,56),(63,252)):
        for high in (0,0xffc0):
            b=bytearray(baseline);a=0x5b0;w(b,a+0x20,0x8001);w(b,a+0xae,high|index)
            w(b,a+0x34,0x26,1);w(b,a+0x33,2,1)
            regs=list(d);regs[1]=0x13570003
            result.append(dict(name=f'state1-guard-i{index}-hi{high:x}',raw=bytes(b),slot=0,regs=regs,
                               group='state1-guard',guard=0xf25a,guard_d1=0x13570000|expected))
    return result


def compile_phase(out,source,build,prepass=None):
    mf=build/'runtime-build.json';m=json.loads(mf.read_text())
    assert not m['sources_changed_during_build']
    assert Path(m['configuration']['compiler']).name=='cc_sanitize.sh'
    for name,digest in m['sources'].items():
        if name!='core/src/wf_r17_human_raw23.c' and not (prepass and name=='core/src/wf_actor_prepass.c'):assert sha((ROOT/name).read_bytes())==digest,('stale source',name)
    objects=[]
    for name,digest in sorted(m['objects'].items()):
        assert sha((build/name).read_bytes())==digest,('stale object',name)
        if name!='wf_r17_human_raw23.o' and not (prepass and name=='wf_actor_prepass.o'):objects.append(str(build/name))
    sources=[source]+([prepass] if prepass else []);before={str(p):sha(p.read_bytes()) for p in sources}
    cmd=[os.environ.get('CC','cc'),*m['configuration']['flags'],'-fsanitize=address,undefined',
         '-fno-omit-frame-pointer','-fno-pie','-no-pie',str(ROOT/'tests/p0_04/phase_probe.c'),
         *map(str,sources),str(ROOT/'tests/native/assets_fixture.c'),*objects,'-o',str(out/'probe')]
    cp=subprocess.run(cmd,capture_output=True,text=True);(out/'compile.log').write_text(cp.stdout+cp.stderr);cp.check_returncode()
    return cmd,before


def run(out,source,group='all',no_trace=False,build=None,keep_vectors=False,prepass=None):
    out.mkdir(parents=True,exist_ok=True)
    rom=(ROOT/'source_assets/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()==ROM_SHA1
    assert rom[0x21]==rom[0x33]==0xff and rom[0x34:0x36]==b'\xff\xff'
    assert rom[0x26:0x2a]==b'\xff'*4 and rom[0x70:0x72]==b'\0\0'
    cases=[c for c in cases_for(bool(build)) if group=='all' or c['group']==group];assert cases
    if build:cmd,before=compile_phase(out,source,build,prepass)
    else:cmd,before=compile_probe(out,source,True)
    inputs=b''.join(struct.pack('>II8I',3 if build else 4 if no_trace else 2,c['slot'],*c['regs'])+c['raw'] for c in cases)
    (out/'inputs.bin').write_bytes(inputs)
    cp=subprocess.run([str(out/'probe'),str(out/'inputs.bin'),str(out/'outputs.bin')],capture_output=True,text=True,
                      env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1'))
    (out/'native.log').write_text(cp.stdout+cp.stderr);cp.check_returncode()
    data=(out/'outputs.bin').read_bytes();assert len(data)==len(cases)*OUTSIZE
    rows=[];failures=[];exceptions=[];pcs=set();expectedhash=hashlib.sha256();classifiers={};rom_writes=[]
    for i,c in enumerate(cases):
        record=data[i*OUTSIZE:(i+1)*OUTSIZE];status,meta,dep_or_carry,*d=struct.unpack('>11I',record[:44]);ram=record[44:]
        dep=dep_or_carry if build else meta;owner=meta if build else 0
        er=c['raw'];ed=list(c['regs']);n=0;stages=[]
        if c['guard']:
            if build:ed[0]=(ed[0]&0xffff0000)|1
            if 'guard_d1' in c:ed[1]=c['guard_d1']
            good=status==1 and dep==(0 if no_trace else c['guard']) and (not build or owner==0xf18a) and ram==er and d==ed
        else:
            cpu=ref.CPU(rom,c['raw'],c['regs'])
            if build:cpu.a=[0]*7+[0x1d07fc]
            else:cpu.a[0]=ptr(c['slot'])
            try:er,ed=cpu.execute_to(0xf18a if build else 0xdcde,limit=10000)
            except Exception as exc:
                exceptions.append(dict(name=c['name'],pc=hex(cpu.pc),exception=str(exc)));continue
            assert cpu.rom==rom;pcs.update(cpu.trace);n=len(cpu.trace)
            expectedhash.update(er+struct.pack('>8I',*ed));good=status==0 and dep==0 and owner==0 and ram==er and d==ed
            stages=[hex(p) for p in cpu.trace if p in (0xdcde,0xe002,0xe2dc,0xe4ba,0xe830,0xddc8,0xdd02,0xf106)]
            if build:
                should_call=bool(c['raw'][0x5b0+0x33]&2) and not bool(c['raw'][0x5b0+0x34]&4)
                assert (0xdcde in cpu.trace)==should_call, ('phase fixture dispatch',c['name'])
            for p in stages:classifiers[p]=classifiers.get(p,0)+1
            if cpu.rom_writes:rom_writes.append(dict(name=c['name'],writes=cpu.rom_writes))
        row=dict(name=c['name'],group=c['group'],passed=good,status=status,owner=hex(owner),dependency=hex(dep),instructions=n,output_sha256=sha(record),stages=stages);rows.append(row)
        if not good:
            failures.append(dict(**row,ram_differences=[dict(address=hex(RAM+j),native=x,rom=y) for j,(x,y) in enumerate(zip(ram,er)) if x!=y][:12],register_differences=[dict(register=j,native=hex(x),rom=hex(y)) for j,(x,y) in enumerate(zip(d,ed)) if x!=y]))
    drift=[s for s,h in before.items() if sha(Path(s).read_bytes())!=h]
    counts={g:dict(total=sum(c['group']==g for c in cases),passed=sum(r['group']==g and r['passed'] for r in rows)) for g in sorted({c['group'] for c in cases})}
    report=dict(status='FAIL' if failures or exceptions or drift else 'PASS_SCOPED_HOST',total=len(cases),evaluated=len(rows),passed=sum(r['passed'] for r in rows),failed=len(failures),groups=counts,command=cmd,no_trace=no_trace,phase=bool(build),source_hashes=before,source_drift=drift,oracle='P0-04 limited interpreter plus literal-tested NOP; not independent CPU or Saturn validation',rom_sha1=ROM_SHA1,rom_sha256=sha(rom),inputs_sha256=sha(inputs),outputs_sha256=sha(data),oracle_outputs_sha256=expectedhash.hexdigest(),classifiers_executed=classifiers,original_pcs_executed=[hex(p) for p in sorted(pcs)],rom_writes=rom_writes,failures=failures,exceptions=exceptions,cases=rows)
    if build:report['build_manifest_sha256']=sha((build/'runtime-build.json').read_bytes())
    (out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:report[k] for k in ('status','total','evaluated','passed','failed','groups')}))
    if failures:print('DIFFERENCES',json.dumps(failures[:2]))
    if exceptions:print('ORACLE ERRORS',json.dumps(exceptions[:3]))
    if not keep_vectors:
        (out/'inputs.bin').unlink();(out/'outputs.bin').unlink()
    return int(report['status']=='FAIL')


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output',type=Path,required=True);p.add_argument('--source',type=Path,default=ROOT/'core/src/wf_r17_human_raw23.c')
    p.add_argument('--no-trace',action='store_true');p.add_argument('--group',default='all');p.add_argument('--build-dir',type=Path);p.add_argument('--keep-vectors',action='store_true')
    p.add_argument('--prepass-source',type=Path)
    a=p.parse_args();assert not (a.no_trace and a.build_dir);assert not a.prepass_source or a.build_dir
    return run(a.output.resolve(),a.source.resolve(),a.group,a.no_trace,a.build_dir.resolve() if a.build_dir else None,a.keep_vectors,a.prepass_source.resolve() if a.prepass_source else None)
if __name__=='__main__':raise SystemExit(main())
