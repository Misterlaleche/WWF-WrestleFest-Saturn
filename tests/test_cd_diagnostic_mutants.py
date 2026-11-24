#!/usr/bin/env python3
"""Reject concrete counter/latch/polling mutants in the source-derived TU."""
from pathlib import Path
import argparse,hashlib,json,subprocess,os
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--source-dir',type=Path,default=ROOT);p.add_argument('--input-dir',type=Path,default=ROOT/'build/cd-diagnostics');p.add_argument('--build-dir',type=Path,default=ROOT/'build/cd-diagnostic-mutants');args=p.parse_args()
out=args.build_dir;out.mkdir(parents=True,exist_ok=True)
program=(args.input_dir/'actual-source.cpp').read_text()
mutants=[
 ('missing-retry','++__jo_fs_cd_diag.fill_read_retries;','/* omitted retry */','retry_success'),
 ('reset-getter','if(out==JO_NULL)return;','if(out==JO_NULL)return;\n __jo_fs_cd_diag.fill_read_retries=0;','retry_success'),
 ('clear-latch','__jo_fs_cd_diag.last_command=cr1;',
  '__jo_fs_cd_diag.failure_reason=0;\n __jo_fs_cd_diag.last_command=cr1;','latched_failure'),
 ('per-poll-store','        if ((available = jo_cd_sector_count()) < 0)',
  '        ++diag_fad_writes;__jo_fs_cd_diag.last_fad=fad+done;\n        if ((available = jo_cd_sector_count()) < 0)','quiet_poll_timeout'),
 ('forget-max','if(elapsed>__jo_fs_cd_diag.max_read_fields)','if(1)','max_after_success'),
]
results=[]
for name,old,new,case in mutants:
 assert program.count(old)==1,(name,program.count(old))
 source=out/(name+'.cpp');source.write_text(program.replace(old,new));binary=out/name
 cmd=['c++','-std=c++11','-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer',
      '-I'+str(args.source_dir/'sdk/jo_engine'),str(source),'-o',str(binary)]
 p=subprocess.run(cmd,capture_output=True,text=True);assert p.returncode==0,p.stderr
 p=subprocess.run([str(binary),case,'1'],capture_output=True,text=True,
    env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0:abort_on_error=1',UBSAN_OPTIONS='halt_on_error=1'))
 assert p.returncode!=0 and 'FAIL '+case in p.stderr,(name,p.returncode,p.stdout,p.stderr)
 results.append({'name':name,'case':case,'returncode':p.returncode,'output':p.stdout+p.stderr,
                 'source_sha256':hashlib.sha256(source.read_bytes()).hexdigest()})
 print(name,'REJECTED',flush=True)
(out/'report.json').write_text(json.dumps({'status':'PASS','mutants':results},indent=2)+'\n')
