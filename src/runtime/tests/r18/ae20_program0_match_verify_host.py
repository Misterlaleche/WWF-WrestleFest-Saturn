"""Explicit progress4/9 manual frontend fixtures -> shared match driver F8C."""
from pathlib import Path
import argparse,hashlib,json,os,re,shutil,subprocess,tempfile,zipfile
ROOT=Path(__file__).resolve().parents[2];p=argparse.ArgumentParser();p.add_argument('output',type=Path);p.add_argument('--red-driver-from-zip',type=Path,help='Explicit historical frozen driver without AE20 admission; RED only');a=p.parse_args();out=a.output.resolve();out.mkdir(parents=True,exist_ok=True);assert not(out/'host.log').exists()
make=(ROOT/'Makefile').read_text();core=re.search(r'^CORE=(.*)$',make,re.M)[1].split();host=re.search(r'^R18_HOST=(.*)$',make,re.M)[1].split();probe='tests/r18/ae20_program0_match_host.c';binary=out/'probe.exe'
with tempfile.TemporaryDirectory(prefix='wf-ae20-match-',ignore_cleanup_errors=os.name=='nt')as td:
 frozen=Path(td)
 for name in('src','include','generated'):shutil.copytree(ROOT/name,frozen/name)
 if a.red_driver_from_zip:
  with zipfile.ZipFile(a.red_driver_from_zip)as z:prior=z.read('src/wf_r18_match_driver.c')
  assert b'post_tag'in prior or b'0xae20'not in prior
  (frozen/'src/wf_r18_match_driver.c').write_bytes(prior)
 for name in(probe,'tests/r18/full_flow_combat_probe.c','tests/r18/frontend_driver_host.c'):
  dest=frozen/name;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(ROOT/name,dest)
 dest=frozen/'tests/r18/full_flow_combat_probe.c';s=dest.read_text();assert s.count('int main(int argc,char**argv)')==1;s=s.replace('int main(int argc,char**argv)','int unused_full_flow_combat_main(int argc,char**argv)');dest.write_text(s)
 hashes={str(x.relative_to(frozen)):hashlib.sha256(x.read_bytes()).hexdigest()for x in frozen.rglob('*')if x.is_file()};command=['cc','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Werror','-Iinclude',*core,*host,probe,'-o',str(binary)];subprocess.run(command,cwd=frozen,check=True)
 with(out/'host.log').open('w')as log:r=subprocess.run([str(binary),'audit/runtime18-cd/ROM68K.BIN'],cwd=ROOT,stdout=log,stderr=subprocess.STDOUT)
 text=(out/'host.log').read_text();(out/'report.json').write_text(json.dumps({'status':'PASS'if r.returncode==0 else'FAIL','returncode':r.returncode,'historical_RED_driver_archive':str(a.red_driver_from_zip)if a.red_driver_from_zip else None,'sources_sha256':hashes,'binary_sha256':hashlib.sha256(binary.read_bytes()).hexdigest(),'limits':['Explicit progression4/9 test fixtures at manualAC0, not natural tournament progress','Original shared match-driver/frontend callbacks; no private AE20 extension','HOST composition, not physical Saturn timing']},indent=2)+'\n');print(text[-4000:]);raise SystemExit(r.returncode)
