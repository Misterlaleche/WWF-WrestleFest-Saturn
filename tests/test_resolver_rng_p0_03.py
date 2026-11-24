#!/usr/bin/env python3
"""P0-03: real production C vs original $280DC/$1129E/$DCDE instructions.

The opcode runner is limited and in-tree, NOT independent Musashi validation.
All new inputs are synthetic; the live fixture's byte image is never claimed
as the exact reported Hogan/Warrior game. Original ROM remains unmodified.
"""
import argparse, hashlib, itertools, json, os, random, struct, subprocess, sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tests/p0_03'))
from rom_subset import CPU,RAM,STOP
SIZE=0x4000;OUTSIZE=44+SIZE;REGS=[0xabcde000+i*0x101 for i in range(8)]
ROM_SHA1='a726c46f350723ea73158bc78055d17d418b14e2'
def sha(b):return hashlib.sha256(b).hexdigest()
def w(b,o,v,n=2):b[o:o+n]=(v&((1<<(8*n))-1)).to_bytes(n,'big')
def state(slot=0):
 b=bytearray(SIZE);a=0x5b0+slot*0x10c
 w(b,0x5c,0x8b5637a6,4);w(b,0x80,7217,4);w(b,0x161,1,1)
 w(b,a,0x8000);w(b,a+6,0x280);w(b,a+0xa,0x160);w(b,a+0xe,0x140)
 w(b,a+0x20,0x8005);w(b,a+0x60,0x7b);w(b,a+0xa3,1,1)
 return b,a

def cases_for(group):
 cases=[]
 def add(name,op,b,slot=0,regs=None,guard=None):cases.append(dict(name=name,op=op,raw=bytes(b),slot=slot,regs=list(REGS if regs is None else regs),guard=guard))
 if group in ('all','resolver'):
  # Literal boundaries named by the actual ROM comparisons, plus 16-bit wraps.
  sweeps={6:[0,0x1c6,0x1c7,0x1c8,0x21f,0x220,0x26f,0x270,0x293,0x294,0x2af,0x2b0,0x2e0,0x2e1,0x360,0x361,0x38b,0x38c,0x38f,0x390,0x3d0,0x3d1,0x470,0x471,0xffff],
          0xa:[0,0x10f,0x110,0x117,0x118,0x140,0x141,0x15f,0x160,0x161,0x198,0x199,0xffff],
          0xe:[0,0xff,0x100,0x101,0x13f,0x140,0x141,0x17f,0x180,0x181,0xffff]}
  for mode,side,face in itertools.product(range(7),(0,4),(0,0x80)):
   for off,values in sweeps.items():
    for v in values:
     b,a=state();w(b,0x7e,mode);w(b,a+0x32,side);w(b,a+0x2e,face,1);w(b,a+off,v)
     w(b,a+0x36,0x55a0);w(b,a+0x38,0x1234);w(b,a+0x3a,0x5678);w(b,a+0x3c,0xabcd)
     add(f'resolver-m{mode}-s{side}-f{face}-o{off:x}-{v:x}',0,b)
  r=random.Random(0x280dc)
  for i in range(768):
   slot=i%9;b,a=state(slot);w(b,0x7e,(i%7)|(r.randrange(8192)<<3))
   for o in (6,0xa,0xe,0x32,0x36,0x38,0x3a,0x3c,0x3e,0x40,0x42):w(b,a+o,r.randrange(65536))
   w(b,a+0x2e,r.randrange(256),1)
   add(f'resolver-boundary-random-{i}',0,b,slot,[r.getrandbits(32) for _ in range(8)])
  for mode,side,skip in itertools.product((7,15,0xffff),(0,4),(0,0x20)):
   b,a=state();w(b,0x7e,mode);w(b,a+0x32,skip,1);w(b,a+0x33,side,1);w(b,a+0x2e,0x80,1)
   w(b,a+0x3e,80);w(b,a+0x38,0x4321);w(b,a+0x3a,0x5678);w(b,a+0x3c,0xabcd)
   add(f'resolver-mode7-{mode:x}-{side}-{skip}',0,b,guard=0x280dc if not skip else None)
 if group in ('all','rng'):
  # The new gate is mode0 read of ROM[$56], not an actor substitution.
  for ptr,tier,progress,seed in itertools.product((0,0x80000000,0xff000000),range(3),range(10),range(16)):
   b,a=state();w(b,a+0x7a,ptr,4);w(b,a+0x70,tier);w(b,0x162,progress);w(b,0x5c,seed,4)
   regs=list(REGS);regs[1]=(regs[1]&0xffff0000)|0
   add(f'rng-null-{ptr:08x}-t{tier}-p{progress}-seed{seed}',1,b,regs=regs)
  # Literal boundaries independently checked in the original $21B4 arithmetic.
  # Each tier exercises threshold-1, equality and threshold+1. Mode0 is strict <.
  boundary = [(0,0x6,0xa48a64b2,1),(0,0x57,0x72453263,0),(0,0x5,0xc914c964,0),
              (1,0x48,0x72453261,1),(1,0x7,0xa48a64b2,0),(1,0x58,0x72453263,0),
              (2,0x0,0xa48a64b0,1),(2,0x49,0x72453261,0),(2,0x8,0xa48a64b2,0)]
  for ptr,(tier,tick,expected_rng,expected_carry) in itertools.product((0,0x80000000,0xff000000),boundary):
   b,a=state();w(b,a+0x7a,ptr,4);w(b,a+0x70,tier);w(b,0x80,tick,4)
   regs=list(REGS);regs[1]&=0xffff0000
   add(f'rng-strict-boundary-{ptr:08x}-t{tier}-tick{tick:x}',1,b,regs=regs)
   cases[-1].update(literal_rng=expected_rng,literal_carry=expected_carry)
  # Existing target actors, high pointer tags, all four RNG modes.
  for slot,mode,tag,role,tier in itertools.product(range(9),range(4),(0,0x80000000,0xff000000),(0,0x80),range(3)):
   b,a=state(slot);target=0x1c05b0+((slot+1)%9)*0x10c;w(b,a+0x7a,target|tag,4);w(b,a+0x26,target|tag,4)
   w(b,target-RAM+0x56,role,1);w(b,a+0x70,tier);w(b,0x162,slot)
   regs=list(REGS);regs[1]=(regs[1]&0xffff0000)|mode
   add(f'rng-actor-s{slot}-m{mode}-tag{tag:x}-r{role}-t{tier}',1,b,slot,regs)
  for ptr in (1,2,0x100,0x1c05b2,0x1c2000,0x80ffffff):
   b,a=state();w(b,a+0x7a,ptr,4);regs=list(REGS);regs[1]&=0xffff0000
   add(f'rng-invalid-{ptr:x}',1,b,regs=regs,guard=0x112c8)
  for tier,progress in ((3,0),(65535,0),(0,10),(0,65535)):
   b,a=state();w(b,a+0x70,tier);w(b,0x162,progress);regs=list(REGS);regs[1]&=0xffff0000
   add(f'rng-table-guard-{tier}-{progress}',1,b,regs=regs,guard=0x113ac)
 if group in ('all','human'):
  for slot,ptr,fresh,seed in itertools.product(range(9),(0,0x80000000,0xff000000),(1,2,3),range(16)):
   b,a=state(slot);w(b,a+0x7a,ptr,4);w(b,a+0xa3,fresh,1);w(b,0x5c,seed,4);w(b,a+0x70,slot%3);w(b,0x162,slot)
   w(b,a+2,(slot+seed)%12);w(b,a+0x60,0x127b)
   add(f'human-raw7b-s{slot}-{ptr:08x}-f{fresh}-seed{seed}',2,b,slot)
  # Already tested latch/direction cases must NOT request the RNG again.
  for ptr,latch,fresh in itertools.product((0,0x80000000),(0x807b,0x7b),(0,1,2,3)):
   b,a=state();w(b,a+0x7a,ptr,4);w(b,a+0x60,latch);w(b,a+0xa3,fresh,1)
   add(f'human-latch-{ptr:x}-{latch:x}-{fresh}',2,b)
 return cases

def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--rom',type=Path,default=ROOT/'source_assets/ROM68K.BIN');p.add_argument('--output',type=Path,required=True)
 p.add_argument('--group',choices=('all','resolver','rng','human'),default='all');p.add_argument('--sanitize',action='store_true')
 p.add_argument('--rng-source',type=Path,default=ROOT/'core/src/wf_r17_rng1129e.c');args=p.parse_args()
 out=args.output.resolve();out.mkdir(parents=True,exist_ok=True);rom=args.rom.read_bytes();assert hashlib.sha1(rom).hexdigest()==ROM_SHA1
 assert rom[0x56]==0xff, "ROM null-target flag byte"
 sources=[ROOT/'core'/s for s in ('src/wf_actor.c','src/wf_r18_runtime_handoff.c','src/wf_r17_m68k_shadow.c',
           'src/wf_r17_resolver_280dc.c','src/wf_r17_human_raw23.c','src/wf_r17_human_ddc8.c')]+[args.rng_source.resolve()]
 hashes={str(s):sha(s.read_bytes()) for s in sources}
 cmd=[os.environ.get('CC','cc'),'-std=c11','-O1','-g','-Wall','-Wextra','-Werror','-I'+str(ROOT/'core/include')]
 if args.sanitize:cmd+=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-pie','-no-pie']
 cmd+=['-ffunction-sections','-fdata-sections',str(ROOT/'tests/p0_03/probe.c'),*map(str,sources),'-Wl,--gc-sections','-o',str(out/'probe')]
 cp=subprocess.run(cmd,text=True,capture_output=True);(out/'compile.log').write_text(cp.stdout+cp.stderr);cp.check_returncode()
 cases=cases_for(args.group);inputs=b''.join(struct.pack('>II8I',c['op'],c['slot'],*c['regs'])+c['raw'] for c in cases)
 (out/'inputs.bin').write_bytes(inputs);cp=subprocess.run([str(out/'probe'),str(out/'inputs.bin'),str(out/'outputs.bin')],text=True,capture_output=True,
 env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1'))
 (out/'native.log').write_text(cp.stdout+cp.stderr);cp.check_returncode();outputs=(out/'outputs.bin').read_bytes();assert len(outputs)==len(cases)*OUTSIZE
 results=[];failures=[];pcs=set();oracle_outputs=hashlib.sha256();carry_counts={0:0,1:0};exceptions=[]
 for i,c in enumerate(cases):
  record=outputs[i*OUTSIZE:(i+1)*OUTSIZE];status,dep,carry,*regs=struct.unpack('>11I',record[:44]);ram=record[44:]
  expected_regs=c['regs'];expected_ram=c['raw'];expected_carry=None
  if c['guard']:
   if c['op']==0:
    expect=bytearray(c['raw']);a=0x5b0+c['slot']*0x10c
    expect[a+0x38:a+0x3e]=bytes(6);w(expect,a+0x3e,(-80)&65535)
    expected_ram=bytes(expect)
    good=status==1 and ram==expected_ram and regs==expected_regs
   else:
    good=status==1 and dep==c['guard'] and ram==expected_ram and regs==expected_regs
  else:
   cpu=CPU(rom,c['raw'],c['regs']);cpu.a[0]=0x1c05b0+c['slot']*0x10c
   try:
    expected_ram,expected_regs=cpu.execute_to((0x280dc,0x1129e,0xdcde)[c['op']]);pcs.update(cpu.trace)
   except Exception as exc:
    exceptions.append(dict(name=c['name'],pc=hex(cpu.pc),exception=str(exc)));break
   if c['op']==1:expected_carry=int(cpu.c);carry_counts[expected_carry]+=1
   if 'literal_rng' in c:
    assert int.from_bytes(expected_ram[0x5c:0x60],'big')==c['literal_rng'],c['name']
    assert expected_carry==c['literal_carry'],c['name']
   good=status==0 and dep==0 and ram==expected_ram and regs==expected_regs and (expected_carry is None or carry==expected_carry)
   oracle_outputs.update(expected_ram+struct.pack('>8I',*expected_regs)+bytes([int(cpu.c)]))
   assert cpu.rom==rom
  row=dict(name=c['name'],op=c['op'],passed=good,status=status,dependency=hex(dep),carry=carry,output_sha256=sha(record))
  results.append(row)
  if not good:
   failures.append(dict(**row,ram_differences=[dict(address=hex(RAM+j),native=x,rom=y) for j,(x,y) in enumerate(zip(ram,expected_ram)) if x!=y][:12],
      register_differences=[dict(register=j,native=hex(x),rom=hex(y)) for j,(x,y) in enumerate(zip(regs,expected_regs)) if x!=y],expected_carry=expected_carry))
 drift=[str(s) for s in sources if hashes[str(s)]!=sha(s.read_bytes())]
 table=[hex(int.from_bytes(rom[0x28154+i*4:0x28158+i*4],'big')) for i in range(16)]
 report=dict(status='FAIL' if failures or exceptions or drift else 'PASS_SCOPED_HOST',group=args.group,total=len(cases),evaluated=len(results),passed=sum(r['passed'] for r in results),failed=len(failures),
  oracle='Limited in-tree opcode interpreter; NOT independent CPU validation',rom_sha1=ROM_SHA1,rom_sha256=sha(rom),source_hashes=hashes,source_drift=drift,
  sanitize=args.sanitize,command=cmd,carry_counts=carry_counts,table_at_28154=table,original_pcs_executed=[hex(x) for x in sorted(pcs)],
  inputs_sha256=sha(inputs),outputs_sha256=sha(outputs),oracle_outputs_sha256=oracle_outputs.hexdigest(),failures=failures,exceptions=exceptions,cases=results)
 (out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps({k:report[k] for k in ('status','group','total','evaluated','passed','failed','carry_counts','exceptions')}))
 if failures:print(json.dumps(failures[:2],indent=2))
 return int(report['status']=='FAIL')
if __name__=='__main__':raise SystemExit(main())
