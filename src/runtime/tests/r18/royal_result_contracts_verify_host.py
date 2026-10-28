"""Freeze and execute independent GAME OVER failure-contract tests."""
from pathlib import Path
import argparse,hashlib,json,os,shutil,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[2];p=argparse.ArgumentParser();p.add_argument('--report',type=Path);args=p.parse_args()
sources=['src/wf_r18_result_screen.c','src/wf_r18_video.c','src/wf_r18_frontend_palette.c','src/wf_r17_m68k_shadow.c','tests/r18/royal_result_contracts.c']
with tempfile.TemporaryDirectory(prefix='wf-royal-result-contracts-',ignore_cleanup_errors=os.name=='nt')as tmp:
 f=Path(tmp);shutil.copytree(ROOT/'include',f/'include')
 for name in sources:dest=f/name;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(ROOT/name,dest)
 manifest={str(x.relative_to(f)):hashlib.sha256(x.read_bytes()).hexdigest()for x in f.rglob('*')if x.is_file()};exe=f/'contracts.exe'
 subprocess.run(['cc','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Werror','-Iinclude',*sources,'-o',str(exe)],cwd=f,check=True)
 result=subprocess.run([str(exe),str((ROOT/'audit/runtime18-cd/ROM68K.BIN').resolve())],capture_output=True,text=True,check=True);print(result.stdout,end='')
 report={'status':'PASS','output':result.stdout.strip(),'sources_sha256':manifest,'rom_sha1':hashlib.sha1((ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes()).hexdigest(),'limits':['Tests only bounded1AFC..1B1D native failure contract; intact-ROM positive matrix is separate','Failure callbacks are terminal and not a resumable continuation; no later operation allowed']}
 if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
