"""Assertion-enabled paid frontend through the shared hardware-tape transport."""
from pathlib import Path
import argparse,hashlib,json,os,re,subprocess,tempfile
root=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser();p.add_argument('--report',type=Path);a=p.parse_args()
make=(root/'Makefile').read_text();sources=re.search(r'^CORE=(.*)$',make,re.M)[1].split()+re.search(r'^R18_HOST=(.*)$',make,re.M)[1].split()
with tempfile.TemporaryDirectory(prefix='wf-paid-driver-',ignore_cleanup_errors=os.name=='nt')as t:
 exe=Path(t)/'paid.exe';subprocess.run(['cc','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Werror','-Iinclude',*sources,'tests/r18/match_driver_host.c','-o',str(exe)],cwd=root,check=True)
 r=subprocess.run([str(exe),str(root/'audit/runtime18-cd/ROM68K.BIN')],cwd=root,text=True,capture_output=True);print(r.stdout,end='',flush=True);print(r.stderr,end='',flush=True);assert r.returncode==0,r.returncode
 report={'status':'PASS','source_sha256':hashlib.sha256((root/'src/wf_r18_match_driver.c').read_bytes()).hexdigest(),'stdout':r.stdout,'limits':'Explicit host event tape, no claim of physical Saturn scanout/timing.'}
 if a.report:a.report.write_text(json.dumps(report,indent=2)+'\n')
