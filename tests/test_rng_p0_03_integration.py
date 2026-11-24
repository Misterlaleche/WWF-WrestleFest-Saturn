#!/usr/bin/env python3
"""P0-03 integration at the actual $F18A phase, from an inherited live snapshot.

Only the observed case is the inherited snapshot unmodified. All variants are
synthetic boundaries derived from it. The test opcode runner is in-tree, not an
independent 68000 core and not a Saturn benchmark. Device resources unused here.
"""
import argparse,hashlib,itertools,json,os,struct,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tests/p0_03'))
from rom_subset import CPU
from test_resolver_rng_p0_03 import w

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--build-dir',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
 p.add_argument('--rng-source',type=Path,default=ROOT/'core/src/wf_r17_rng1129e.c');args=p.parse_args();out=args.output.resolve();out.mkdir(parents=True,exist_ok=True)
 build=args.build_dir.resolve();mf=build/'runtime-build.json';manifest=json.loads(mf.read_text());assert not manifest['sources_changed_during_build']
 for name,digest in manifest['sources'].items():
  if name!='core/src/wf_r17_rng1129e.c':assert sha(ROOT/name)==digest,('stale dependency source',name)
 objects=[]
 for name,digest in sorted(manifest['objects'].items()):
  assert sha(build/name)==digest,('stale object',name)
  if name!='wf_r17_rng1129e.o':objects.append(str(build/name))
 source=args.rng_source.resolve();source_before=sha(source);cmd=[os.environ.get('CC','cc'),*manifest['configuration']['flags'],'-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-pie','-no-pie',
 str(ROOT/'tests/test_royal_facing_target.c'),str(source),str(ROOT/'tests/native/assets_fixture.c'),*objects,'-o',str(out/'phase')]
 cp=subprocess.run(cmd,capture_output=True,text=True,cwd=ROOT);(out/'compile.log').write_text(cp.stdout+cp.stderr);cp.check_returncode()
 fix=ROOT/'tests/fixtures/royal_human_active';fm=json.loads((fix/'manifest.json').read_text())
 for name,digest in fm['files'].items():assert sha(fix/name)==digest
 rom=(ROOT/'source_assets/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
 raw=(fix/'arcade-ram.bin').read_bytes();regs=json.loads((fix/'state.json').read_text())['d'];a=0x5b0
 cases=[('inherited-observed',raw)]
 seeds=(0,15,0xffffffff,int.from_bytes(raw[0x5c:0x60],'big'))
 for ptr,fresh,tier,progress,seed in itertools.product((0,0x80000000,0xff000000),(1,2,3),range(3),(0,9),seeds):
  b=bytearray(raw);w(b,a+0x60,0x7b);w(b,a+0x7a,ptr,4);w(b,a+0xa3,fresh,1);w(b,a+0x70,tier);w(b,0x162,progress);w(b,0x5c,seed,4)
  cases.append((f'raw7b-{ptr:08x}-f{fresh}-t{tier}-p{progress}-r{seed:08x}',bytes(b)))
 for sel in (6,0x72,0x807b):
  for fresh in (0,1,2,3):
   b=bytearray(raw);w(b,a+0x60,sel);w(b,a+0xa3,fresh,1);cases.append((f'unchanged-sel{sel:x}-f{fresh}',bytes(b)))
 results=[];failures=[];pcs=set();continuations={};inputhash=hashlib.sha256();outputhash=hashlib.sha256();oraclehash=hashlib.sha256()
 for name,b in cases:
  inp=out/'input.bin';native=out/'native.bin';inp.write_bytes(b);native.unlink(missing_ok=True)
  cp=subprocess.run([str(out/'phase'),str(inp),str(native),'0',*map(str,regs)],capture_output=True,text=True,
    env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1'))
  cpu=CPU(rom,b,regs);cpu.a=[0]*7+[0x1d07fc];ram,d=cpu.execute_to(0xf18a,limit=10000);pcs.update(cpu.trace)
  expected=ram+struct.pack('>8I',*d);actual=native.read_bytes() if native.exists() else b''
  ok=cp.returncode==0 and actual==expected
  result=dict(name=name,passed=ok,returncode=cp.returncode,instructions=len(cpu.trace),output_sha256=hashlib.sha256(actual).hexdigest(),stdout=cp.stdout.strip())
  results.append(result);inputhash.update(b);outputhash.update(actual);oraclehash.update(expected)
  selector=int.from_bytes(ram[a+0x60:a+0x62],'big');continuations[hex(selector)]=continuations.get(hex(selector),0)+1
  if not ok:failures.append(dict(**result,stderr=cp.stderr,byte_differences=[dict(offset=hex(i),native=x,rom=y) for i,(x,y) in enumerate(zip(actual,expected)) if x!=y][:12]))
 assert sha(source)==source_before,'RNG source changed during test'
 report=dict(status='PASS_PHASE' if not failures else 'FAIL_PHASE',scope=__doc__,total=len(cases),passed=sum(x['passed'] for x in results),failed=len(failures),
 source_sha256=sha(source),build_manifest_sha256=sha(mf),fixture_manifest_sha256=sha(fix/'manifest.json'),rom_sha256=hashlib.sha256(rom).hexdigest(),
 sanitizers_scope='harness and RNG source; dependency objects built without sanitizers',command=cmd,input_sha256=inputhash.hexdigest(),output_sha256=outputhash.hexdigest(),oracle_sha256=oraclehash.hexdigest(),
 result_selectors=continuations,original_pcs_executed=[hex(x) for x in sorted(pcs)],failures=failures,cases=results)
 (out/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k] for k in ('status','total','passed','failed','result_selectors')}))
 if failures:print(json.dumps(failures[:2],indent=2))
 return bool(failures)
if __name__=='__main__':raise SystemExit(main())
