"""Shared driver routes the original scene instead of losing AE20 at BF6."""
from pathlib import Path
import argparse,hashlib,json,os,re,subprocess,tempfile
root=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser();p.add_argument('--report',type=Path);a=p.parse_args()
make=(root/'Makefile').read_text();sources=re.search(r'^CORE=(.*)$',make,re.M)[1].split()+re.search(r'^R18_HOST=(.*)$',make,re.M)[1].split()
with tempfile.TemporaryDirectory(prefix='wf-post-dispatch-',ignore_cleanup_errors=os.name=='nt')as t:
 exe=Path(t)/'post-dispatch.exe'
 subprocess.run(['cc','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Werror','-Iinclude',*sources,'tests/r18/match_post_dispatch_host.c','-o',str(exe)],cwd=root,check=True)
 q=subprocess.run([str(exe),str(root/'audit/runtime18-cd/ROM68K.BIN')],cwd=root,text=True,capture_output=True)
 print(q.stdout,end='',flush=True);print(q.stderr,end='',flush=True);assert q.returncode==0,q.returncode
 report={'status':'PASS','source_sha256':hashlib.sha256((root/'src/wf_r18_match_driver.c').read_bytes()).hexdigest(),'stdout':q.stdout,'limits':'Controlled AE20(3) caller fixture; not a natural campaign victory or physical timing test.'}
 if a.report:a.report.write_text(json.dumps(report,indent=2)+'\n')
