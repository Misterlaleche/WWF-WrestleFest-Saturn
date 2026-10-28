"""Compile assertion-enabled BOOT6FC metadata contract."""
from pathlib import Path
import argparse,subprocess,tempfile,os,re,json,hashlib
root=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser();p.add_argument('--report',type=Path);a=p.parse_args()
make=(root/'Makefile').read_text();sources=re.search(r'^CORE=(.*)$',make,re.M)[1].split()+re.search(r'^R18_HOST=(.*)$',make,re.M)[1].split()
for name in('frontend_driver','frontend_reset','frontend_boot','frontend_coin','frontend_irq','frontend_mode_plate','mode_select','roster_select','frontend_input','frontend_effect','frontend_fade','frontend_palette','frontend_board','frontend_map','frontend_io','frontend_render','frontend_leaf','title','title_palette'):
 f=f'src/wf_r18_{name}.c'
 if f not in sources:sources.append(f)
with tempfile.TemporaryDirectory(prefix='wf-frontend-driver-',ignore_cleanup_errors=os.name=='nt')as t:
 exe=Path(t)/'driver.exe';subprocess.run(['cc','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Werror','-Iinclude',*sources,'tests/r18/frontend_resume_boot_host.c','-o',str(exe)],cwd=root,check=True)
 result=subprocess.run([str(exe),str(root/'audit/runtime18-cd/ROM68K.BIN')],cwd=root,capture_output=True,text=True)
 print(result.stdout,end='');print(result.stderr,end='');assert result.returncode==0,result.returncode
 report={'status':'PASS','source_sha256':hashlib.sha256((root/'src/wf_r18_frontend_driver.c').read_bytes()).hexdigest(),'assertions':'enabled -UNDEBUG','stdout':result.stdout,'limits':'Only metadata transition tested; original BOOT6FC payload remains covered by its ROM oracle.'}
 if a.report:a.report.write_text(json.dumps(report,indent=2)+'\n')
