#!/usr/bin/env python3
"""Generate private full-Z80 SFX fixtures, then validate native PCM starts.

The historical A15 physical model is retained as a reported comparison,
never silently treated as the acceptance oracle when it disagrees with ROM.
"""
import argparse
from concurrent.futures import ThreadPoolExecutor,as_completed
import csv
import json
import math
from pathlib import Path
import struct
import subprocess
import sys

from reproduce_full_board_references import HERE,PACKAGE,prepare_and_build,run_case,sha
from prepare_reference_inputs import resources

def flag_audit(sound):
    rows=resources.commands(sound);count=skips=0;failures=[]
    for index,row in enumerate(rows):
        pos=struct.unpack_from('<H',sound,0x21a3+index*2)[0];suppressed=False;original=[]
        for _ in range(128):
            op=sound[pos];pos+=1
            if op==0xf0:break
            if op==0xef:pos+=1
            elif op==0xf1:suppressed=True
            elif op==0xf2:suppressed=False
            elif 1<=op<=73:original.append(not suppressed);pos+=1
            else:raise ValueError('Unexpected original SFX stream opcode')
        else:raise ValueError('Unterminated original stream')
        encoded=[(event[3]&3)!=1 for event in row['events']]
        if encoded!=original:failures.append(row['command'])
        count+=len(original);skips+=original.count(False)
    return {'commands':len(rows),'events':count,'suppressed_continuations':skips,'mismatched_commands':failures,
            'passed':not failures,'predicate':'An encoded flags1 event is an F1 continuation; flags0/3 is a new emission in every original stream'}

def parse_starts(path):
    rows=list(csv.DictReader(Path(path).open()));origin=next(int(r['cycle']) for r in rows if r['kind']=='ORIGIN')
    starts=[dict(cycle=int(r['cycle'])-origin,bank=int(r['a'])>>8,voice=(int(r['a'])>>4)&15,
                 volume=int(r['a'])&15,sample=int(r['b'])) for r in rows if r['kind']=='PLAY' and int(r['cycle'])>=origin]
    skips=[dict(cycle=int(r['cycle'])-origin,logical_voice=int(r['a'])>>8,latch=int(r['a'])&255,sample=int(r['b']))
           for r in rows if r['kind']=='F1SKIP']
    return starts,skips

def fixture_text(cases):
    starts=[];indices=[]
    for case in sorted(cases,key=lambda c:c['command']):
        indices.append((len(starts),len(case['original_starts']),case['seconds']))
        starts.extend((s['bank'],s['sample'],s['volume']) for s in case['original_starts'])
    return ('typedef struct {unsigned bank,sample,volume;} original_start_t;\n'
      'typedef struct {unsigned offset,count,seconds;} original_start_case_t;\n'
      'static const original_start_t original_starts[]={\n'+''.join('{%d,%d,%d},\n'%s for s in starts)+'};\n'
      'static const original_start_case_t original_start_cases[98]={\n'+''.join('{%d,%d,%d},\n'%c for c in indices)+'};\n')

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--rom-zip',type=Path,required=True);p.add_argument('--out',type=Path,required=True)
    p.add_argument('--opm-dir',type=Path);p.add_argument('--cc',default='cc');p.add_argument('--jobs',type=int,default=2)
    a=p.parse_args();a.out=a.out.resolve();a.rom_zip=a.rom_zip.resolve()
    if not 1<=a.jobs<=4:p.error('Need1..4 workers')
    if a.out==PACKAGE or PACKAGE in a.out.parents:p.error('Private fixtures must stay outside source package')
    baseline=prepare_and_build(a.out,a.rom_zip,a.opm_dir.resolve() if a.opm_dir else None,a.cc)
    inputs=a.out/'inputs';model=a.out/'historical-a15-model'
    command=[sys.executable,str(HERE/'render_sfx_reference.py'),'--golden',str(inputs/'PCM16BE.PAK'),
        '--fixtures',str(inputs),'--reference',str(inputs/'a15_model'),'--out',str(model),'--cc',a.cc]
    subprocess.run(command,check=True,capture_output=True,text=True)
    old=json.loads((model/'reference-sfx-report.json').read_text())
    report={**baseline,'kind':'FULL_ORIGINAL_Z80_SFX_START_GATE_WITH_LEGACY_COMPARISON',
            'flag_encoding_audit':flag_audit((inputs/'SOUND.ROM').read_bytes()),'commands':[],'complete':False}
    paths=[PACKAGE/'src/wf_scsp_sfx.c',PACKAGE/'src/wf_scsp_sfx.h',PACKAGE/'tests/test_sfx_original_starts.c',Path(__file__).resolve()]
    native_before={str(p):sha(p) for p in paths}
    def job(previous):
        c=previous['command'];seconds=math.ceil(previous['seconds'])+2
        item=run_case(a.out,f'sfx-{c:02X}',[{'sample':0,'word':c}],seconds)
        starts,skips=parse_starts(item['trace'])
        a15=list(csv.DictReader(Path(previous['events']).open()))
        expected=[{key:int(row[key]) for key in ('bank','sample','volume')} for row in a15 if row['action']=='start']
        actual=[{key:row[key] for key in ('bank','sample','volume')} for row in starts]
        return {'command':c,'seconds':seconds,'capture':item,'original_starts':starts,'observed_F1_branches':skips,
                'historical_model_starts':expected,'historical_model_start_values_equal':actual==expected}
    with ThreadPoolExecutor(max_workers=a.jobs) as pool:
        for task in as_completed([pool.submit(job,row) for row in old['commands']]):
            item=task.result();report['commands'].append(item);report['commands'].sort(key=lambda row:row['command'])
            (a.out/'sfx-progress.json').write_text(json.dumps(report,indent=2)+'\n')
            print(json.dumps({'command':f'0x{item["command"]:02X}','historical_model_equal':item['historical_model_start_values_equal']}),flush=True)
    fixture=a.out/'sfx_original_fixture.h';fixture.write_text(fixture_text(report['commands']))
    exe=a.out/'test_sfx_original_starts'
    build=[a.cc,'-O2','-std=c99','-Wall','-Wextra','-Werror','-I',str(PACKAGE/'src'),'-I',str(a.out),
           str(PACKAGE/'tests/test_sfx_original_starts.c'),str(PACKAGE/'src/wf_scsp_sfx.c'),'-o',str(exe)]
    subprocess.run(build,check=True,capture_output=True,text=True)
    result=subprocess.run([str(exe),str(inputs/'SFX.DAT')],capture_output=True,text=True)
    (a.out/'native-original-gate.log').write_text(result.stdout+result.stderr)
    report.update(fixture_sha256=sha(fixture),native_source_sha256=native_before,native_build_command=build,
        native_original_start_gate_passed=result.returncode==0,native_original_gate_output=result.stdout+result.stderr,
        native_source_stable=all(sha(Path(p))==v for p,v in native_before.items()),
        original_source_stable=all(sha(Path(p))==v for p,v in baseline['source_sha256'].items()))
    report['legacy_model_mismatched_commands']=[r['command'] for r in report['commands'] if not r['historical_model_start_values_equal']]
    report['complete']=len(report['commands'])==98 and report['flag_encoding_audit']['passed'] and report['native_source_stable'] and report['original_source_stable']
    (a.out/'full-board-sfx-validation.json').write_text(json.dumps(report,indent=2)+'\n')
    print(result.stdout+result.stderr,end='')
    return 0 if report['complete'] and report['native_original_start_gate_passed'] else 1

if __name__=='__main__':raise SystemExit(main())
