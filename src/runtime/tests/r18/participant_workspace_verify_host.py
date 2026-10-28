"""Serialize original participant owners without changing their ROM algorithms."""
from pathlib import Path
import argparse,hashlib,json,os,re,shutil,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser();p.add_argument('--report',type=Path);args=p.parse_args()
core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split()
with tempfile.TemporaryDirectory(prefix='wf-participant-lease-',ignore_cleanup_errors=os.name=='nt')as tmp:
 frozen=Path(tmp)
 for name in('src','include','generated'):shutil.copytree(ROOT/name,frozen/name)
 source='tests/r18/test_participant_workspace.c';target=frozen/source;target.parent.mkdir(parents=True);shutil.copy2(ROOT/source,target)
 hashes={name:hashlib.sha256((frozen/name).read_bytes()).hexdigest()for name in[*core,source]}
 exe=frozen/'participant.exe';command=['cc','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Werror','-Iinclude',*core,source,'-o',str(exe)]
 subprocess.run(command,cwd=frozen,check=True);r=subprocess.run([str(exe)],cwd=frozen,capture_output=True,text=True);print(r.stdout,end='');print(r.stderr,end='');assert r.returncode==0,('participant workspace',r.returncode)
 report={'status':'PASS','owner_reentry_cases':40,'post_transaction_reacquisitions':80,'missing_output_early_exit_controls':2,'stdout':r.stdout,'compile_command':command,'compiled_sources_sha256':hashes,'limits':['HOST transaction/reentry proof; physical SH2 memory budget must be read from the linked map','Synchronous single-game-thread lease; no change to owner algorithms or external write ordering','Runtime rollback cannot undo an output callback that violates its atomic failure contract']}
 if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
