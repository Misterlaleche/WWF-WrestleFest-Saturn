#!/usr/bin/env python3
"""Original-ROM state9 ($1EEA0) through the real state5 handoff ($1DFE2).

Tests the preserved Royal failure snapshot plus explicit ID/target/RNG/wait
variants. Only the next native owner is intercepted at the matched ROM boundary;
all state9 instructions and its RNG dependency execute intact original ROM.
Full16KiB RAM and D0-D7 must match, with no ROM modifications.
"""
import argparse,hashlib,json,os,subprocess,sys
from pathlib import Path
import machine68k as mus
from run_tag_campaign_main import ROOT,sha
from test_tag_campaign_cpu import put
FIXTURE=ROOT/'tests/fixtures/royal_cpu_state9'
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--build-dir',type=Path,default=ROOT/'build/royal-cpu-state9-20261113/runtime');p.add_argument('--output',type=Path,default=ROOT/'build/royal-cpu-state9-current');p.add_argument('--source',type=Path,default=ROOT/'core/src/wf_cpu_runtime.c');p.add_argument('--skip-build',action='store_true');a=p.parse_args();build=a.build_dir.resolve();out=a.output.resolve();build.mkdir(parents=True,exist_ok=True);out.mkdir(parents=True,exist_ok=True)
 fm=json.loads((FIXTURE/'manifest.json').read_text())
 for name,digest in fm['files'].items():assert sha(FIXTURE/name)==digest
 if not a.skip_build:subprocess.run([sys.executable,str(ROOT/'tools/build_runtime_tests.py'),'--build-dir',str(build)],cwd=ROOT,check=True)
 bm=json.loads((build/'runtime-build.json').read_text());assert not bm['sources_changed_during_build'];objects=[]
 for name,digest in bm['sources'].items():
  if name!='core/src/wf_cpu_runtime.c':assert sha(ROOT/name)==digest,('stale source',name)
 for name,digest in sorted(bm['objects'].items()):
  assert sha(build/name)==digest
  if name!='wf_cpu_runtime.o':objects.append(str(build/name))
 binary=out/'state9';source_hash=sha(a.source)
 subprocess.run([os.environ.get('CC','cc'),*bm['configuration']['flags'],'-Wl,--wrap=wf_state5_execute',str(ROOT/'tests/test_royal_cpu_state9.c'),str(a.source.resolve()),str(ROOT/'tests/native/assets_fixture.c'),*objects,'-o',str(binary)],check=True)
 rom=(ROOT/'source_assets/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
 raw=(FIXTURE/'arcade-ram.bin').read_bytes();meta=json.loads((FIXTURE/'state.json').read_text());regs=meta['d'];actor=0x5b0+7*0x10c;target=0x5b0+8*0x10c
 assert raw[actor+0x57]==11 and raw[actor+0x21]==9 and raw[target+0x61]==0x4a
 cases=[('observed-retry',raw)]
 for identity in (4,7,11):
  for seed in range(32):
   v=bytearray(raw);put(v,actor+0x57,identity,1);put(v,0x5c,(0x9e3779b9*seed+0x12345678)&0xffffffff,4);v[actor+0xbd]=7;cases.append((f'id{identity}-rng{seed}',bytes(v)))
 for identity in range(12):
  for target_state,selector,sub in ((5,0x7b,0),(5,0x61,0),(4,0,8),(4,0,9),(0,0,0)):
   v=bytearray(raw);v[actor+0x57]=identity;v[target+0x21]=target_state;v[target+0x61]=selector;v[target+0x65]=sub;cases.append((f'id{identity}-target{target_state}-{selector:02x}-{sub}',bytes(v)))
 for bd in (0,6,7,254,255):
  v=bytearray(raw);v[actor+0xbd]=bd;cases.append((f'wait{bd}',bytes(v)))
 v=bytearray(raw);partner=0x5b0+6*0x10c;put(v,actor+0x86,0x1c0000+partner,4);v[partner+0x21]=5;v[partner+0x61]=0x89;cases.append(('partner89',bytes(v)))
 machine=mus.Machine(mus.CPUType.M68000,2048);cpu,mem=machine.cpu,machine.mem;mem.w_block(0,rom);reports=[];coverage=set()
 try:
  for name,data in cases:
   inp=out/(name+'-input.bin');inp.write_bytes(data);actual_file=out/(name+'-native.bin');result=subprocess.run([str(binary),str(inp),str(actual_file),'7',*[str(r) for r in regs]],capture_output=True,text=True)
   mem.w_block(0x1c0000,data);cpu.w_sr(0x2700)
   for i,r in enumerate(regs):cpu.w_reg(i,r)
   for i in range(8,15):cpu.w_reg(i,0)
   cpu.w_reg(8,0x1c0000+actor);cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_pc(0x1eea0);trace=[]
   while cpu.r_pc() not in (0x1dfe2,0x70000):
    trace.append(cpu.r_pc());assert len(trace)<10000,(name,'ROM runaway',hex(cpu.r_pc()));cpu.execute(1)
   coverage.update(trace);expected=mem.r_block(0x1c0000,0x4000)+b''.join(cpu.r_reg(i).to_bytes(4,'big') for i in range(8));(out/(name+'-oracle.bin')).write_bytes(expected);(out/(name+'-trace.json')).write_text(json.dumps([hex(pc) for pc in trace])+'\n')
   assert result.returncode==0,(name,'native blocked',result.stdout,result.stderr)
   assert ('boundary=1dfe2' in result.stdout)==(cpu.r_pc()==0x1dfe2),(name,'wrong boundary',result.stdout,hex(cpu.r_pc()))
   actual=actual_file.read_bytes();assert actual==expected,(name,'RAM/D-register mismatch',[(hex(i+0x1c0000),x,y) for i,(x,y) in enumerate(zip(actual,expected)) if x!=y][:12]);assert mem.r_block(0,len(rom))==rom
   reports.append(dict(name=name,endpoint=hex(cpu.r_pc()),instructions=len(trace),sha256=hashlib.sha256(actual).hexdigest()));print('PASS',name,flush=True)
 finally:machine.cleanup()
 assert {0x1ef48,0x1ef54,0x1ef74,0x1eece,0x1ef8e}<=coverage
 assert sha(a.source)==source_hash
 (out/'summary.json').write_text(json.dumps(dict(status='PASS',scope=__doc__,source_sha256=source_hash,binary_sha256=sha(binary),fixture=fm,rom_sha256=hashlib.sha256(rom).hexdigest(),cases=reports,coverage=[hex(pc) for pc in sorted(coverage)]),indent=2)+'\n')
if __name__=='__main__':main()
