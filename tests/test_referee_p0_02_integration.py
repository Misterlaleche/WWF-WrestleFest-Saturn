#!/usr/bin/env python3
"""Native phase integration and parity of two inherited fixture families.

This is NOT an independent ROM test. The inherited snapshots are replayed with
production callbacks using baseline and candidate referee code; outputs must
remain identical. Only harness/referee TUs receive sanitizers in this runner.
"""
import argparse, hashlib, json, os, subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--build-dir',type=Path,required=True)
 p.add_argument('--baseline',type=Path,required=True)
 p.add_argument('--candidate',type=Path,default=ROOT/'core/src/wf_r17_referee.c')
 p.add_argument('--output',type=Path,required=True)
 args=p.parse_args();out=args.output.resolve();out.mkdir(parents=True,exist_ok=True)
 build=args.build_dir.resolve();manifest=json.loads((build/'runtime-build.json').read_text())
 objects=[]
 for name,digest in manifest['objects'].items():
  assert sha(build/name)==digest,('stale object',name)
  if name!='wf_r17_referee.o':objects.append(str(build/name))
 flags=manifest['configuration']['flags']
 cc=os.environ.get('CC','cc');reports=[]
 def compile(harness,src,label):
  binary=out/label
  cmd=[cc,*flags,'-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-pie','-no-pie',str(harness),str(src),str(ROOT/'tests/native/assets_fixture.c'),*objects,'-o',str(binary)]
  result=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True)
  (out/(label+'-compile.log')).write_text(result.stdout+result.stderr);result.check_returncode()
  return binary
 env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1')
 phase=compile(ROOT/'tests/p0_02/phase_probe.c',args.candidate,'phase')
 r=subprocess.run([str(phase)],capture_output=True,text=True,env=env)
 (out/'phase.log').write_text(r.stdout+r.stderr)
 phase_ok=r.returncode==0
 if not phase_ok:
  (out/'report.json').write_text(json.dumps(dict(status='FAIL_PHASE',returncode=r.returncode,stdout=r.stdout,stderr=r.stderr),indent=2)+'\n')
  print(r.stdout+r.stderr);return 1
 phase_result=json.loads(r.stdout)
 for kind in ('tag_referee_null','royal_referee_count'):
  fix=ROOT/'tests/fixtures'/kind;fm=json.loads((fix/'manifest.json').read_text())
  for name,digest in fm['files'].items(): assert sha(fix/name)==digest
  raw=(fix/'arcade-ram.bin').read_bytes();regs=json.loads((fix/'state.json').read_text())['d']
  cases=[('inherited',raw)]
  if kind=='tag_referee_null':
   for ptr in (0x80000000,0xff000000,0x1c05b0,0x801c05b0):
    data=bytearray(raw);data[0x1684:0x1688]=ptr.to_bytes(4,'big');cases.append((f'target-{ptr:08x}',data))
   for x in (0x100,0x280,0x604,0x680):
    data=bytearray(raw);data[0x11fa:0x11fc]=x.to_bytes(2,'big');cases.append((f'x-{x:04x}',data))
  else:
   holder=int.from_bytes(raw[0x124a:0x124e],'big')-0x1c0000
   for ptr in (0x80000000,0x100,0x80000100,0x7fe00,0x1c0aec,0x801c0aec):
    data=bytearray(raw);data[holder+0x26:holder+0x2a]=ptr.to_bytes(4,'big');cases.append((f'target-{ptr:08x}',data))
  binaries=[compile(ROOT/'tests'/('test_'+kind+'.c'),src,kind+'-'+label) for label,src in [('baseline',args.baseline),('candidate',args.candidate)]]
  for name,data in cases:
   case=kind+'-'+name;inputp=out/(case+'-input.bin');inputp.write_bytes(data);outputs=[]
   for label,binary in zip(('baseline','candidate'),binaries):
    outputp=out/(case+'-'+label+'.bin')
    r=subprocess.run([str(binary),str(inputp),str(outputp),str(ROOT/'source_assets/ROM68K.BIN'),*map(str,regs)],capture_output=True,text=True,env=env)
    (out/(case+'-'+label+'.log')).write_text(r.stdout+r.stderr)
    assert r.returncode==0,(case,label,r.returncode,r.stdout,r.stderr)
    outputs.append(outputp.read_bytes())
   assert outputs[0]==outputs[1],('inherited fixture regression',case)
   reports.append(dict(name=case,passed=True,bytes=len(outputs[1]),output_sha256=hashlib.sha256(outputs[1]).hexdigest()))
 report=dict(status='PASS_NATIVE_INTEGRATION_AND_PARITY',scope=__doc__,phase=phase_result,parity_cases=reports,
             baseline_sha256=sha(args.baseline),candidate_sha256=sha(args.candidate),
             dependency_manifest_sha256=sha(build/'runtime-build.json'),external_ROM_validation=False)
 (out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps(dict(status=report['status'],phase=phase_result,fixture_cases=len(reports))))
 return 0
if __name__=='__main__':raise SystemExit(main())
