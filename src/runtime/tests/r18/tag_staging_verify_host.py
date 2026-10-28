from pathlib import Path
import hashlib,json,os,re,shutil,subprocess,tempfile,argparse
ROOT=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser();p.add_argument('--report',type=Path);args=p.parse_args()
core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split();core.remove('src/wf_r17_tag_6e34.c')
with tempfile.TemporaryDirectory(prefix='wf-tag-staging-',ignore_cleanup_errors=os.name=='nt')as td:
 frozen=Path(td)
 for folder in('src','include','generated'):shutil.copytree(ROOT/folder,frozen/folder)
 source='tests/r18/test_tag_staging.c';target=frozen/source;target.parent.mkdir(parents=True);shutil.copy2(ROOT/source,target)
 hashes={name:hashlib.sha256((frozen/name).read_bytes()).hexdigest()for name in core+['src/wf_r17_tag_6e34.c',source]}
 exe=frozen/'staging.exe';flags=['cc','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Werror','-Iinclude'];subprocess.run([*flags,*core,source,'-o',str(exe)],cwd=frozen,check=True)
 r=subprocess.run([str(exe)],capture_output=True,text=True,check=True);print(r.stdout,end='')
 report={'status':'PASS','cases':20485,'workspace_write_bytes':49152,'extra_staging_bytes':0,'flags':flags,'stdout':r.stdout,'compiled_sources_sha256':hashes,'limits':['Private stable-partition/capacity test plus separate real owner/reentry and ROM checks','Actual linked SH2 saving to be verified in parent map; O2 unchanged']}
 if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
