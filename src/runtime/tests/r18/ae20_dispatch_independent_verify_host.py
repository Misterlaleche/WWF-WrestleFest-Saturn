"""Frozen real AE20/shared-driver transport controls, not a ROM oracle.

Only the root fixture's unused main is renamed. Production sources and the
actual post cursor are compiled unchanged; injected failures are explicit
device-contract probes. The B2E4 payload is additionally covered by its intact
ROM oracle, which this script does not replace.
"""
from pathlib import Path
import argparse,hashlib,json,os,re,shutil,subprocess,tempfile,zipfile
ROOT=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser();p.add_argument('output',type=Path);a=p.parse_args()
out=a.output.resolve();out.mkdir(parents=True,exist_ok=True);assert not(out/'host.log').exists()
make=(ROOT/'Makefile').read_text();sources=re.search(r'^CORE=(.*)$',make,re.M)[1].split()+re.search(r'^R18_HOST=(.*)$',make,re.M)[1].split()
probe='tests/r18/ae20_dispatch_independent_host.c';binary=out/'probe.exe'
with tempfile.TemporaryDirectory(prefix='wf-ae20-review-',ignore_cleanup_errors=os.name=='nt')as td:
 frozen=Path(td)
 for name in('src','include','generated'):shutil.copytree(ROOT/name,frozen/name)
 for name in(probe,'tests/r18/match_post_dispatch_host.c','tests/r18/frontend_driver_host.c'):
  dest=frozen/name;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(ROOT/name,dest)
 dest=frozen/'tests/r18/match_post_dispatch_host.c';s=dest.read_text();assert s.count('int main(int argc,char**argv)')==1
 dest.write_text(s.replace('int main(int argc,char**argv)','int unused_match_post_dispatch_main(int argc,char**argv)'))
 hashes={str(x.relative_to(frozen)):hashlib.sha256(x.read_bytes()).hexdigest()for x in frozen.rglob('*')if x.is_file()}
 with zipfile.ZipFile(out/'frozen-sources.zip','w',zipfile.ZIP_DEFLATED)as z:
  for name in hashes:z.write(frozen/name,name)
 command=['cc','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Werror','-Iinclude',*sources,probe,'-o',str(binary)]
 subprocess.run(command,cwd=frozen,check=True)
 with(out/'host.log').open('w')as log:r=subprocess.run([str(binary),'audit/runtime18-cd/ROM68K.BIN'],cwd=ROOT,stdout=log,stderr=subprocess.STDOUT)
 print((out/'host.log').read_text()[-4500:])
 report={'status':'PASS'if r.returncode==0 else'FAIL','returncode':r.returncode,'scenarios':7,'early_return_shared_dispatch_controls':4,'B2E4_shared_dispatch_control':1,'sources_sha256':hashes,'binary_sha256':hashlib.sha256(binary.read_bytes()).hexdigest(),'limits':['Explicit direct AE20(0) public-owner fixtures after legitimate manual frontend','Transport failures and ACK/cancellation contracts are deliberately injected device controls','Actual production cursor/shared driver compiled unchanged; not an intact-ROM oracle or physical Saturn timing test','AE20 does not issue READ8/READ16 requests: generic branches remain outside these real-cursor scenarios']}
 (out/'report.json').write_text(json.dumps(report,indent=2)+'\n');raise SystemExit(r.returncode)
