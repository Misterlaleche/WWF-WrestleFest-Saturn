#!/usr/bin/env python3
"""Execute actual SDK CD/read/refill bodies; only MMIO and DMA are mocked."""
from pathlib import Path
import argparse, hashlib, importlib.util, json, re, subprocess, os

ROOT=Path(__file__).resolve().parents[1]
REPO=ROOT
spec=importlib.util.spec_from_file_location('protocol', REPO/'tests/test_cd_protocol.py')
protocol=importlib.util.module_from_spec(spec);spec.loader.exec_module(protocol)
parser=argparse.ArgumentParser();parser.add_argument('--baseline',action='store_true');parser.add_argument('--source-dir',type=Path);parser.add_argument('--build-dir',type=Path);args=parser.parse_args()
src=args.source_dir or ROOT
if args.baseline:assert 'jo_fs_get_cd_diagnostics' not in (src/'sdk/jo_engine/fs.c').read_text(), 'baseline mode requires an explicit pre-diagnostic source tree'
out=args.build_dir or ROOT/'build'/('cd-diagnostics-red' if args.baseline else 'cd-diagnostics');out.mkdir(parents=True,exist_ok=True)
fs=(src/'sdk/jo_engine/fs.c').read_text();header=(src/'sdk/jo_engine/jo/fs.h').read_text()
types=(src/'sdk/jo_engine/jo/types.h').read_text()
def function(name):return protocol.function(fs,name)
diag_type='''typedef struct {
 unsigned read_attempts,start_read_attempts,sector_wait_timeouts,fill_read_retries,command_failures;
 unsigned last_fad,last_command,last_status,failure_fad,failure_command,failure_status,failure_reason;
 unsigned read_start_field,max_read_fields,max_read_fad,last_read_fields;
} jo_fs_cd_diagnostics;'''
if not args.baseline:
 m=re.search(r'typedef struct\s*\{[^}]+\}\s*jo_fs_cd_diagnostics;',header);assert m
 diag_type=m[0]
file_type=re.search(r'typedef struct\s*\{[^}]+\}\s*jo_file;',types)[0]
prefix=protocol.PREFIX.replace('static unsigned inject_event, services, clock_every;',
 'static unsigned inject_event, services, clock_every, spins, diag_fad_writes;\nstatic int response_remaining=-1;\nstatic uint64_t mmio_hash=1469598103934665603ull;\nstatic void event(unsigned kind,unsigned value){mmio_hash=(mmio_hash^kind)*1099511628211ull;mmio_hash=(mmio_hash^value)*1099511628211ull;}')
prefix=prefix.replace('++hirq_reads;', '++hirq_reads;event(0x10,hirq);')
prefix=prefix.replace('read_mask|=1u<<(index-1);','event(0x10+index,cr[index-1]);read_mask|=1u<<(index-1);')
prefix=prefix.replace('if(index==0) { hirq &= value;', 'event(index,value);if(index==0) { hirq &= value;')
prefix=prefix.replace('cr[0]=command==response_command?response_status:0x0100;',
 'cr[0]=(command==response_command && response_remaining!=0)?response_status:0x0100;\nif(command==response_command && response_remaining>0)--response_remaining;')
prefix+='\n#define _Static_assert static_assert\n#define WF_JO_ENGINE 1\n#define JO_READ_RETRY_COUNT 10\n#define JO_READ_SPIN_WAIT 16\nstatic void jo_spin_wait(unsigned n){spins+=n;}\n'
constants='\n'.join(line for line in fs[:fs.index('typedef struct')].splitlines()
 if re.match(r'^#\s*define JO_(CD_|FS_FILENAME)',line) and not re.match(r'^#\s*define JO_CD_(HIRQ|CR[1-4]|DATA|INFO)\s',line))
begin=fs.index('typedef struct\n{\n    unsigned short')
end=fs.index('#include "internal/cd_stream_async.h"',begin)
declarations=fs[begin:end]+'#include "internal/cd_stream_async.h"\n'
names=['jo_cd_command','jo_cd_end_transfer','jo_cd_reset_partition','jo_cd_sector_count',
       'jo_cd_init','jo_cd_stop_read','jo_cd_start_read','jo_cd_read_sectors',
       '__jo_fs_read_sectors','jo_fs_fill_read_buffer']
body='\n'.join(function(n) for n in names)
if args.baseline:
 body+='\nstatic void jo_fs_get_cd_diagnostics(jo_fs_cd_diagnostics *out){if(out)memset(out,0,sizeof *out);}\n'
else:body+='\n'+function('jo_fs_get_cd_diagnostics')
program=(prefix+diag_type+'\n'+file_type+'\n'+constants+
 '\n#undef JO_CD_TIMEOUT\n#define JO_CD_TIMEOUT 64u\n'
 '#undef JO_CD_SECTOR_TIMEOUT\n#define JO_CD_SECTOR_TIMEOUT 128u\n'+declarations+
 '\nstatic unsigned __jo_fs_stat_vblanks,__jo_fs_stat_bytes;\n'+body+'\n'+
 (ROOT/'tests/cd_diagnostics.cpp.in').read_text())
program=program.replace('__jo_fs_cd_diag.last_fad=', '++diag_fad_writes;__jo_fs_cd_diag.last_fad=')
cpp=out/'actual-source.cpp';cpp.write_text(program);binary=out/'diagnostics'
cmd=['c++','-std=c++11','-O1','-g','-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-unused-variable',
 '-fsanitize=address,undefined','-fno-omit-frame-pointer','-I'+str(src/'sdk/jo_engine'),str(cpp),'-o',str(binary)]
p=subprocess.run(cmd,capture_output=True,text=True);(out/'compile.log').write_text(p.stdout+p.stderr)
if p.returncode:print(p.stderr);p.check_returncode()
cases=['success','retry_success','retry_exhaustion','max_after_success','field_timeout','clockless_timeout','quiet_poll_timeout','wrapped_timeout',
       'wait','reject','initial_cmok_timeout','cmok_timeout','selector_timeout','async_flush_failure',
       'latched_failure','sequential','skip_range','zero_read']
env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0:abort_on_error=1',UBSAN_OPTIONS='halt_on_error=1')
report={'status':'RUNNING','baseline':args.baseline,'command':cmd,'cases':[]}
hashes={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in
 [src/'sdk/jo_engine/fs.c',src/'sdk/jo_engine/jo/fs.h',src/'sdk/jo_engine/jo/types.h',
 src/'sdk/jo_engine/internal/cd_stream_async.h',Path(__file__),ROOT/'tests/cd_diagnostics.cpp.in',REPO/'tests/test_cd_protocol.py']}
for case in cases:
 control=subprocess.run([str(binary),case,'0'],text=True,capture_output=True,env=env,timeout=20)
 assert control.returncode==0,(case,control.stdout,control.stderr)
 check=subprocess.run([str(binary),case,'1'],text=True,capture_output=True,env=env,timeout=20)
 # Baseline is deliberately red only for the new counters, never for I/O behavior.
 expected_red=args.baseline and case!='zero_read'
 assert(check.returncode!=0)==expected_red,(case,check.returncode,check.stdout,check.stderr)
 result={'name':case,'behavior':control.stdout.strip(),'check_returncode':check.returncode,
         'check_output':check.stdout+check.stderr}
 if not args.baseline:
  baseline=json.loads((ROOT/'tests/fixtures/cd_diagnostics/reference-behavior.json').read_text())
  original=next(c for c in baseline['cases'] if c['name']==case)
  assert result['behavior']==original['behavior'],('MMIO/behavior drift',case,result['behavior'],original['behavior'])
 report['cases'].append(result);print(case,'EXPECTED_RED' if expected_red else 'PASS',flush=True)
report.update(status='EXPECTED_RED' if args.baseline else 'PASS',source_sha256=hashes,
 source_drift=[p for p,h in hashes.items() if hashlib.sha256(Path(p).read_bytes()).hexdigest()!=h],
 binary_sha256=hashlib.sha256(binary.read_bytes()).hexdigest())
assert not report['source_drift']
(out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
