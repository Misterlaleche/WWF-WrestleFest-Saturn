"""Independent GAME OVER detector branch/ACK review; tests only."""
from pathlib import Path
import argparse,ctypes,hashlib,itertools,json,os,shutil,subprocess,tempfile
import machine68k as mus
import capstone
ROOT=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser();p.add_argument('--report',type=Path);args=p.parse_args()
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
sources=['src/wf_r18_match_result.c','src/wf_scheduler.c','src/wf_r17_postack_2017a.c','src/wf_r17_m68k_shadow.c','tests/r18/royal_endpoint_oracle_bridge.c']
count=0;gates=0;sequences=0;branches={};coverage=set()
with tempfile.TemporaryDirectory(prefix='wf-royal-endpoint-',ignore_cleanup_errors=os.name=='nt')as tmp:
 f=Path(tmp);shutil.copytree(ROOT/'include',f/'include')
 for name in sources:dest=f/name;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(ROOT/name,dest)
 hashes={name:hashlib.sha256((f/name).read_bytes()).hexdigest()for name in sources}
 lib=f/'endpoint.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Werror','-Iinclude',*sources,'-o',str(lib)],cwd=f,check=True)
 native=ctypes.CDLL(str(lib));native.royal_endpoint_observe.argtypes=[ctypes.c_uint]*6;native.royal_endpoint_ack_sequence.argtypes=[ctypes.c_uint]*3
 assert native.royal_endpoint_null()==0
 machine=mus.Machine(mus.CPUType.M68000,2048);cpu,mem=machine.cpu,machine.mem;mem.w_block(0,rom)
 md=capstone.Cs(capstone.CS_ARCH_M68K,capstone.CS_MODE_BIG_ENDIAN|capstone.CS_MODE_M68K_000)
 disassembly='\n'.join(f'{i.address:06X}: {i.bytes.hex():24s} {i.mnemonic:10s} {i.op_str}'for i in md.disasm(rom[0x115c:0x1178],0x115c))
 print(disassembly)
 for flag in range(65536):
  mem.w16(0x1c16c4,flag);cpu.w_pc(0x115c)
  for _ in range(12):
   pc=cpu.r_pc();coverage.add(pc)
   if pc in(0x1b4e,0x1afc,0x1172):break
   cpu.execute(1)
  assert cpu.r_pc()in(0x1b4e,0x1afc,0x1172),('unexpected ROM branch',flag,hex(cpu.r_pc()))
  target=cpu.r_pc();branches[hex(target)]=branches.get(hex(target),0)+1
  expected=int(target==0x1afc);got=native.royal_endpoint_observe(flag,2,1,1,0x8000,1);assert got==expected,(flag,got,hex(target));count+=1
 for flag,stage,ready,ack,credit,mode in itertools.product((0,1,2,3,4,0x103,0x8003,0xff03),(0,1,2,3,255),(0,1,2),(0,1,2),(0,1,0x8000),(0,1,2,3,0x80,0x81,0xff)):
  expected=int(flag==3 and stage==2 and ready!=0 and ack!=0 and credit!=0 and mode&1)
  assert native.royal_endpoint_observe(flag,stage,ready,ack,credit,mode)==expected,(flag,stage,ready,ack,credit,mode);gates+=1
 for flag,after,endpoint in((0,1,0),(1,3,1),(3,3,1),(0x100,0x101,0),(0x101,0x103,0),(0x103,0x103,0),(0x8003,0x8003,0),(0xff03,0xff03,0)):
  assert native.royal_endpoint_ack_sequence(flag,after,endpoint)==0,(flag,after,endpoint);sequences+=1
 assert bytes(mem.r_block(0,len(rom)))==rom;machine.cleanup()
 for name in sources:assert hashlib.sha256((ROOT/name).read_bytes()).hexdigest()==hashes[name],('source changed',name)
 report={'status':'PASS','whole_ROM_115C_branches':count,'context_gate_purity_checks':gates,'real_scheduler_postack_ACK_sequences':sequences,'null_contract_checks':1,'branch_counts':branches,'coverage':[hex(pc)for pc in sorted(coverage)],'rom_sha1':hashlib.sha1(rom).hexdigest(),'sources_sha256':hashes,'caller_source_sha256':{name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest()for name in('src/wf_saturn_entry.c','tests/r18/full_flow_combat_probe.c')},'original_branch_disassembly':disassembly,'intercepts':[],'limits':['Exhaustive original115C branch tail only; 2017A separately exercised by linked native scheduler sequences','Saturn entry inspected as source; no independent SH2 execution or physical hardware timing claim','Bounded presentation1AFC..1B1D verified separately; winner1B4E remains unsupported']}
 if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
 print('PASS',count,'ROM branch words,',gates,'context/purity checks,',sequences,'real scheduler ACK sequences')
