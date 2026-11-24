#!/usr/bin/env python3
"""Actual $F18A phase vs original ROM. All core objects in the recorded build
must come from the given source tree. A --sanitized-core build additionally
requires the supplied sanitizer wrapper; no Saturn devices are exercised.
"""
import argparse,hashlib,itertools,json,os,struct,subprocess
from pathlib import Path
from test_human_p0_04 import ROOT,ref,w,sha,RAM,OUTSIZE,ROM_SHA1,ptr

def cases_for_phase():
    cases=[]
    def fixture(name):
        path=ROOT/'tests/fixtures'/name;mf=json.loads((path/'manifest.json').read_text())
        for f,d in mf['files'].items():assert sha((path/f).read_bytes())==d
        return (path/'arcade-ram.bin').read_bytes(),json.loads((path/'state.json').read_text())['d']
    active,d=fixture('royal_human_active');a=0x5b0
    def add(name,b,regs=d,group='raw15-16',guard=0):cases.append(dict(name=name,raw=bytes(b),regs=list(regs),group=group,guard=guard))
    add('active-observed',active,group='observed')
    for sel,fresh,target,link,tier in itertools.product((0x15,0x16),(1,2,3),(0,0x80000000,0xff000000),(0,0x80000000,ptr(4),ptr(4)|0xff000000),range(3)):
        b=bytearray(active);w(b,a+0x60,sel);w(b,a+0xa3,fresh,1);w(b,a+0x7a,target,4);w(b,a+0x26,link,4)
        if link&0xffffff:w(b,ptr(4)-RAM+0x70,tier)
        w(b,a+0x44,0x1234);w(b,a+0x46,0x5678)
        add(f'phase-{sel:x}-f{fresh}-t{target:x}-l{link:x}-tier{tier}',b)
    for sel,ch,remap,face in itertools.product((0x15,0x16),range(12),range(15),(0,0x80)):
        b=bytearray(active);w(b,a+0x60,sel);w(b,a+2,ch);w(b,a+0x30,remap);w(b,a+0x2e,face,1)
        w(b,a+0xa3,(ch+remap)%3+1,1);w(b,a+0x62,(ch*17)^remap,1)
        add(f'phase-remap-{sel:x}-c{ch}-m{remap}-f{face}',b,group='remap')
    idle,idle_d=fixture('royal_human_target');add('idle-observed',idle,idle_d,group='observed')
    for st,ch,half,fresh in itertools.product((0x8000,0x8001),range(12),(0,0x80),(1,2,3)):
        b=bytearray(idle);w(b,a+0x20,st);w(b,a+2,ch);w(b,a+0x62,(b[a+0x62]&0x7f)|half,1);w(b,a+0xa3,fresh,1)
        add(f'idle-st{st:x}-c{ch}-h{half}-f{fresh}',b,idle_d,group='idle-regression')
    for sel in (0x15,0x16):
        for field,value,dep in ((0x7a,2,0xea42),(0x7a,0x1c05b2,0xea42),(0x26,2,0xe1d4),(0x26,0x801c05b2,0xe1d4)):
            b=bytearray(active);w(b,a+0x60,sel);w(b,a+field,value,4)
            add(f'guard-{sel:x}-{field:x}-{value:x}',b,group='guard',guard=dep)
    for field,mask in ((0x34,0x20),(0x74,0x80)):
        b=bytearray(idle);b[a+field]|=mask;add(f'closed-idle-branch-{field:x}',b,idle_d,group='idle-regression')
    return cases

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--build-dir',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--source',type=Path,default=ROOT/'core/src/wf_r17_human_raw23.c');p.add_argument('--sanitized-core',action='store_true');args=p.parse_args()
    out=args.output.resolve();out.mkdir(parents=True,exist_ok=True);build=args.build_dir.resolve();mf=build/'runtime-build.json';m=json.loads(mf.read_text());source=args.source.resolve()
    assert not m['sources_changed_during_build']
    for name,digest in m['sources'].items():
        if name!='core/src/wf_r17_human_raw23.c':assert sha((ROOT/name).read_bytes())==digest,('stale source',name)
    if args.sanitized_core:assert Path(m['configuration']['compiler']).name=='cc_sanitize.sh','use sanitizer wrapper for ALL objects'
    objects=[]
    for name,digest in sorted(m['objects'].items()):
        assert sha((build/name).read_bytes())==digest,('stale object',name)
        if name!='wf_r17_human_raw23.o':objects.append(str(build/name))
    source_hash=sha(source.read_bytes());cmd=[os.environ.get('CC','cc'),*m['configuration']['flags'],'-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-pie','-no-pie',str(ROOT/'tests/p0_04/phase_probe.c'),str(source),str(ROOT/'tests/native/assets_fixture.c'),*objects,'-o',str(out/'phase')]
    cp=subprocess.run(cmd,capture_output=True,text=True);(out/'compile.log').write_text(cp.stdout+cp.stderr);cp.check_returncode()
    rom=(ROOT/'source_assets/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()==ROM_SHA1
    cases=cases_for_phase();inputs=b''.join(struct.pack('>II8I',3,0,*c['regs'])+c['raw'] for c in cases);(out/'inputs.bin').write_bytes(inputs)
    cp=subprocess.run([str(out/'phase'),str(out/'inputs.bin'),str(out/'outputs.bin')],capture_output=True,text=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1'))
    (out/'native.log').write_text(cp.stdout+cp.stderr);cp.check_returncode();data=(out/'outputs.bin').read_bytes();assert len(data)==OUTSIZE*len(cases)
    rows=[];failures=[];exceptions=[];pcs=set();expectedhash=hashlib.sha256()
    for i,c in enumerate(cases):
        record=data[i*OUTSIZE:(i+1)*OUTSIZE];status,owner,dep,*d=struct.unpack('>11I',record[:44]);ram=record[44:]
        er=c['raw'];ed=list(c['regs']);n=0
        if c['guard']:
            ed[0]=(ed[0]&0xffff0000)|1
            good=status==1 and owner==0xf18a and dep==c['guard'] and ram==er and d==ed
        else:
            cpu=ref.CPU(rom,c['raw'],c['regs']);cpu.a=[0]*7+[0x1d07fc]
            try:er,ed=cpu.execute_to(0xf18a,limit=10000)
            except Exception as exc:exceptions.append(dict(name=c['name'],pc=hex(cpu.pc),exception=str(exc)));continue
            assert cpu.rom==rom;pcs.update(cpu.trace);n=len(cpu.trace);expectedhash.update(er+struct.pack('>8I',*ed))
            good=status==0 and owner==0 and dep==0 and ram==er and d==ed
        row=dict(name=c['name'],group=c['group'],passed=good,status=status,owner=hex(owner),dependency=hex(dep),instructions=n,output_sha256=sha(record));rows.append(row)
        if not good:failures.append(dict(**row,ram_differences=[dict(address=hex(RAM+j),native=x,rom=y) for j,(x,y) in enumerate(zip(ram,er)) if x!=y][:12],register_differences=[dict(register=j,native=hex(x),rom=hex(y)) for j,(x,y) in enumerate(zip(d,ed)) if x!=y]))
    assert sha(source.read_bytes())==source_hash
    groups={g:dict(total=sum(c['group']==g for c in cases),passed=sum(r['group']==g and r['passed'] for r in rows)) for g in sorted({c['group'] for c in cases})}
    report=dict(status='FAIL' if failures or exceptions else 'PASS_PHASE',total=len(cases),evaluated=len(rows),passed=sum(r['passed'] for r in rows),failed=len(failures),groups=groups,source_sha256=source_hash,command=cmd,build_manifest_sha256=sha(mf.read_bytes()),all_core_sanitized=args.sanitized_core,oracle='limited in-tree opcode runner',rom_sha1=ROM_SHA1,rom_sha256=sha(rom),inputs_sha256=sha(inputs),outputs_sha256=sha(data),oracle_outputs_sha256=expectedhash.hexdigest(),original_pcs_executed=[hex(x) for x in sorted(pcs)],failures=failures,exceptions=exceptions,cases=rows)
    (out/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k] for k in ('status','total','evaluated','passed','failed','groups')}))
    if failures:print(json.dumps(failures[:2]))
    if exceptions:print('ORACLE ERRORS',json.dumps(exceptions[:3]))
    return int(report['status']=='FAIL')
if __name__=='__main__':raise SystemExit(main())
