#!/usr/bin/env python3
"""P0-04 production $DCDE vs intact ROM: synthetic linked raw15/raw16 boundaries.

No outcomes or helpers are patched. All 16 KiB RAM and D0-D7 are compared.
Reference is the limited in-tree interpreter, NOT independent CPU certification.
Guard cases deliberately do not execute invalid ROM addresses.
"""
import argparse, hashlib, importlib.util, itertools, json, os, random, struct, subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('p004_cpu',ROOT/'tests/p0_04/rom_subset.py')
ref=importlib.util.module_from_spec(spec);spec.loader.exec_module(ref)
RAM=0x1c0000; SIZE=0x4000; OUTSIZE=SIZE+44; ROM_SHA1='a726c46f350723ea73158bc78055d17d418b14e2'
REGS=[0xabcde000+i*0x101 for i in range(8)]
def sha(b):return hashlib.sha256(b).hexdigest()
def w(b,o,v,n=2):b[o:o+n]=(v&((1<<(n*8))-1)).to_bytes(n,'big')
def ptr(s):return RAM+0x5b0+s*0x10c

def state(slot=0,selector=0x15,target=0,link=None,tier=0,ch=0,fresh=1,old=0,face=0,remap=0):
    b=bytearray(SIZE);a=ptr(slot)-RAM
    # Independent literal fixture, including intentionally nonzero timer and D regs.
    w(b,0x161,1,1);w(b,0x5c,0x8b5637a6,4);w(b,0x80,7217,4)
    w(b,a,0x8000);w(b,a+2,ch);w(b,a+6,0x280);w(b,a+0xa,0x160);w(b,a+0xe,0x140)
    w(b,a+0x20,0x8005);w(b,a+0x60,selector);w(b,a+0xa3,fresh,1);w(b,a+0x30,remap)
    w(b,a+0x44,0x1234);w(b,a+0x46,0xabcd);w(b,a+0x2e,face,1)
    w(b,a+0x62,old,1);w(b,a+0x7a,target,4)
    if link is None:link=ptr((slot+2)%9)
    w(b,a+0x26,link,4)
    for address in (target & 0xffffff,link & 0xffffff):
        if address in [ptr(s) for s in range(9)] and address != ptr(slot):
            off=address-RAM;w(b,off+0x70,tier);w(b,off+6,0x280);w(b,off+0xa,0x160)
            w(b,off+0x34,7);w(b,off+0x20,0x8005);w(b,off+0x60,6)
    return b

def cases_for():
    cases=[]
    def add(name,b,slot=0,group='linked',guard=0,regs=None):
        cases.append(dict(name=name,raw=bytes(b),slot=slot,group=group,guard=guard,regs=list(REGS if regs is None else regs)))
    # Entire dynamic table: all 12 ids, three tiers, old bit states and fresh inputs.
    for sel,ch,tier,old,fresh in itertools.product((0x15,0x16),range(12),range(3),(0,0xff),(1,2,3)):
        target=(0,0x80000000,0xff000000)[ch%3]
        link=ptr(2) | (0,0x80000000,0xff000000)[fresh-1]
        add(f'table-{sel:x}-c{ch}-t{tier}-o{old:x}-f{fresh}',state(selector=sel,target=target,link=link,tier=tier,ch=ch,fresh=fresh,old=old))
    # Null linked address is ROM, not an actor. Include all remap records/orientations.
    for sel,ch,remap,face,fresh in itertools.product((0x15,0x16),range(12),range(15),(0,0x80),(1,2,3)):
        tag=(0,0x80000000,0xff000000)[fresh-1]
        add(f'romlink-{sel:x}-c{ch}-m{remap}-d{face:x}-f{fresh}',state(selector=sel,target=tag,link=tag,ch=ch,fresh=fresh,old=ch*17,face=face,remap=remap),group='rom-link')
    # Existing supported target/linked pair must stay exact, including reciprocal/secondary fields.
    for slot,sel,tier,face,fresh in itertools.product(range(9),(0x15,0x16),range(3),(0,0x80),(1,2,3)):
        target=ptr((slot+1)%9)
        b=state(slot,sel,target=target,tier=tier,ch=(slot+fresh)%12,fresh=fresh,old=0x55 if face else 0xaa,face=face,remap=(slot+fresh)%15)
        # Allow actual $E958 same-primary arbitration for half the cases.
        if fresh==2:w(b,target-RAM+0x34,0)
        add(f'actorpair-s{slot}-{sel:x}-t{tier}-d{face:x}-f{fresh}',b,slot,group='existing-pair')
    # Valid primary actor + null linked bus: lookup tier must be ROM zero, not primary tier.
    for slot,sel,fresh,half in itertools.product(range(9),(0x15,0x16),(1,2,3),(0,0xff)):
        target=ptr((slot+1)%9);tag=(0,0x80000000,0xff000000)[fresh-1]
        b=state(slot,sel,target=target,link=tag,tier=slot%3,ch=(slot+fresh)%12,fresh=fresh,old=half,remap=slot)
        if fresh==2:w(b,target-RAM+0x34,0)
        add(f'primary-null-link-s{slot}-{sel:x}-f{fresh}-h{half}',b,slot,group='primary-null-link')
    # TAG partner special pre-action consumes the input before $E1D4.
    for sel,fresh,side in itertools.product((0x15,0x16),(1,2,3),(0,0x80)):
        b=state(selector=sel,fresh=fresh);a=0x5b0;partner=ptr(3)-RAM
        w(b,0x161,0,1);w(b,a+0x86,ptr(3),4);w(b,partner+0x20,1);w(b,partner+0xae,0x8002)
        w(b,a+0x33,side,1);w(b,a+6,0x300 if side else 0x200);w(b,a+0xa,0x120 if side else 0x190)
        add(f'tag-preaction-{sel:x}-f{fresh}-side{side:x}',b,group='priority')
    # Alias/self and relation flags; inputs differ but no target is invented or rewritten.
    for slot,sel,tag,fresh,flag in itertools.product(range(9),(0x15,0x16),(0,0x80000000,0xff000000),(1,3),(0,0x80)):
        b=state(slot,sel,target=tag,link=ptr(slot)|tag,ch=slot,fresh=fresh,old=0xa5,face=flag)
        w(b,ptr(slot)-RAM+0x70,slot%3);w(b,ptr(slot)-RAM+0x74,flag,1)
        add(f'selflink-s{slot}-{sel:x}-tag{tag:x}-f{fresh}-rel{flag:x}',b,slot,group='self-link')
    # $DD02 TAG pre-action and $DDC8/alternate prefixes are still earlier owners.
    for mode,sel,fresh,alt,lock in itertools.product((0,1,2,3),(0x15,0x16),(0,1,2,3),(0,1),(0,1)):
        b=state(selector=sel,fresh=fresh,link=ptr(2));a=0x5b0
        w(b,0x161,mode,1);w(b,a+0x86,ptr(3),4);w(b,a+0x32,alt|2,1);w(b,a+0x33,4,1);w(b,a+0xfe,lock)
        add(f'prefix-mode{mode}-sel{sel:x}-fresh{fresh}-alt{alt}-lock{lock}',b,group='prefix')
    # Invalid inputs must remain distinguishable, not fall back to a random wrestler.
    for sel in (0x15,0x16):
        for target in (1,2,0x1c05b2,0x1c2000):
            add(f'guard-target-{sel:x}-{target:x}',state(selector=sel,target=target),group='guard',guard=0xea42)
        for link in (1,2,0x100,0x1c05b2,0x1c2000,0x801c05b2):
            add(f'guard-link-{sel:x}-{link:x}',state(selector=sel,link=link),group='guard',guard=0xe1d4)
        for tier,ch in ((3,0),(65535,0),(0,12),(0,256),(0,65535)):
            add(f'guard-table-{sel:x}-t{tier}-c{ch}',state(selector=sel,tier=tier,ch=ch),group='guard',guard=0xe1d4)
    # Unmodified inherited pre-call RAM, not a reconstruction of the user's physical match.
    fixture=ROOT/'tests/fixtures/royal_human_active';fm=json.loads((fixture/'manifest.json').read_text())
    for name,digest in fm['files'].items():assert sha((fixture/name).read_bytes())==digest
    add('inherited-observed',(fixture/'arcade-ram.bin').read_bytes(),group='observed',regs=json.loads((fixture/'state.json').read_text())['d'])
    return cases

def compile_probe(out,source,sanitize):
    sources=[ROOT/'core'/s for s in ('src/wf_actor.c','src/wf_r18_runtime_handoff.c','src/wf_r17_m68k_shadow.c','src/wf_r17_resolver_280dc.c','src/wf_r17_rng1129e.c','src/wf_r17_human_ddc8.c')]+[source]
    before={str(s):sha(s.read_bytes()) for s in sources}
    cmd=[os.environ.get('CC','cc'),'-std=c11','-O1','-g','-Wall','-Wextra','-Werror','-I'+str(ROOT/'core/include')]
    if sanitize:cmd+=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-pie','-no-pie']
    cmd+=['-ffunction-sections','-fdata-sections',str(ROOT/'tests/p0_04/probe.c'),*map(str,sources),'-Wl,--gc-sections','-o',str(out/'probe')]
    cp=subprocess.run(cmd,capture_output=True,text=True);(out/'compile.log').write_text(cp.stdout+cp.stderr);cp.check_returncode()
    return cmd,before

def run(out,source,sanitize=True,group='all',no_trace=False):
    out.mkdir(parents=True,exist_ok=True);rom=(ROOT/'source_assets/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()==ROM_SHA1
    # Literal properties used by the narrow const ROM view (not synthesized actor fields).
    assert rom[6:8]==bytes.fromhex('0654') and rom[10:12]==bytes.fromhex('ffff')
    assert rom[0x2e]==rom[0x33]==0xff and rom[0x70:0x72]==b'\0\0'
    cmd,before=compile_probe(out,source,sanitize)
    cases=[c for c in cases_for() if group=='all' or c['group']==group]
    assert cases
    inputs=b''.join(struct.pack('>II8I',4 if no_trace else 2,c['slot'],*c['regs'])+c['raw'] for c in cases);(out/'inputs.bin').write_bytes(inputs)
    cp=subprocess.run([str(out/'probe'),str(out/'inputs.bin'),str(out/'outputs.bin')],capture_output=True,text=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1'))
    (out/'native.log').write_text(cp.stdout+cp.stderr);cp.check_returncode()
    data=(out/'outputs.bin').read_bytes();assert len(data)==len(cases)*OUTSIZE
    rows=[];failures=[];exceptions=[];pcs=set();expectedhash=hashlib.sha256();selectors={}
    for i,c in enumerate(cases):
        record=data[i*OUTSIZE:(i+1)*OUTSIZE];status,dep,carry,*d=struct.unpack('>11I',record[:44]);ram=record[44:]
        if c['guard']:
            er=c['raw'];ed=c['regs'];good=status==1 and dep==(0 if no_trace else c['guard']) and ram==er and d==ed;n=0
        else:
            cpu=ref.CPU(rom,c['raw'],c['regs']);cpu.a[0]=ptr(c['slot'])
            try:er,ed=cpu.execute_to(0xdcde,limit=5000)
            except Exception as exc:
                exceptions.append(dict(name=c['name'],pc=hex(cpu.pc),exception=str(exc)));continue
            pcs.update(cpu.trace);n=len(cpu.trace);assert cpu.rom==rom
            good=status==0 and dep==0 and ram==er and d==ed
            expectedhash.update(er+struct.pack('>8I',*ed))
            a=ptr(c['slot'])-RAM;key=er[a+0x60:a+0x62].hex();selectors[key]=selectors.get(key,0)+1
        row=dict(name=c['name'],group=c['group'],passed=good,status=status,dependency=hex(dep),instructions=n,output_sha256=sha(record))
        rows.append(row)
        if not good:failures.append(dict(**row,ram_differences=[dict(address=hex(RAM+j),native=x,rom=y) for j,(x,y) in enumerate(zip(ram,er)) if x!=y][:10],register_differences=[dict(register=j,native=hex(x),rom=hex(y)) for j,(x,y) in enumerate(zip(d,ed)) if x!=y]))
    drift=[s for s,h in before.items() if sha(Path(s).read_bytes())!=h]
    counts={g:dict(total=sum(c['group']==g for c in cases),passed=sum(r['group']==g and r['passed'] for r in rows)) for g in sorted({c['group'] for c in cases})}
    report=dict(status='FAIL' if failures or exceptions or drift else 'PASS_SCOPED_HOST',total=len(cases),evaluated=len(rows),passed=sum(r['passed'] for r in rows),failed=len(failures),groups=counts,command=cmd,sanitize=sanitize,no_trace=no_trace,source_hashes=before,source_drift=drift,oracle='limited in-tree opcode interpreter; not independent CPU or Saturn validation',rom_sha1=ROM_SHA1,rom_sha256=sha(rom),inputs_sha256=sha(inputs),outputs_sha256=sha(data),oracle_outputs_sha256=expectedhash.hexdigest(),result_selectors=selectors,original_pcs_executed=[hex(p) for p in sorted(pcs)],failures=failures,exceptions=exceptions,cases=rows)
    (out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:report[k] for k in ('status','total','evaluated','passed','failed','groups')}))
    if exceptions:print('ORACLE ERRORS',json.dumps(exceptions[:3]))
    if failures:print('DIFFERENCES',json.dumps(failures[:2]))
    return int(report['status']=='FAIL')

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--source',type=Path,default=ROOT/'core/src/wf_r17_human_raw23.c');p.add_argument('--no-sanitize',action='store_true');p.add_argument('--group',default='all');p.add_argument('--no-trace',action='store_true')
    a=p.parse_args();return run(a.output.resolve(),a.source.resolve(),not a.no_sanitize,a.group,a.no_trace)
if __name__=='__main__':raise SystemExit(main())
