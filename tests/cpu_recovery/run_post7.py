from pathlib import Path
import subprocess,re,json,hashlib,sys
from common import verify_reference
root=Path(__file__).resolve().parents[2];here=Path(__file__).parent;out=root/'build/cpu-recovery/post7-gate';out.mkdir(parents=True,exist_ok=True);base=root/'tests/native/cpu_reference'
verify_reference(base)
flags=['-std=c11','-O2','-g','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-fno-builtin','-ffunction-sections','-fdata-sections','-fsanitize=address,undefined','-fno-omit-frame-pointer'];objects=[];cmds=[]
def run(c):cmds.append(c);subprocess.run(c,check=True)
files=['wf_r17_post7a00.c','wf_r17_m68k_shadow.c','wf_r17_render_contract.c']
# Namespace all exported B12 functions; its internal calls retain its original implementations.
names=[]
for n in files:names+=re.findall(r'^\w+(?: \*)?\s+(wf_\w+)\s*\(', (base/'core/src'/n).read_text(),re.M)
for prefix,tree in [('reference_',base),('candidate_',root)]:
 for n in files:
  obj=out/(prefix+n+'.o');defs=['-D'+n+'=reference_'+n for n in names] if prefix=='reference_' else []
  run(['cc',*flags,'-I'+str(tree/'core/include'),*defs,'-c',str(tree/'core/src'/n),'-o',str(obj)]);objects.append(str(obj))
run(['cc',*flags,'-I'+str(root/'core/include'),str(here/'post7_test.c'),*objects,'-Wl,--gc-sections','-o',str(out/'post7')])
r=subprocess.run([str(out/'post7')],capture_output=True,text=True);print(r.stdout,end='');print(r.stderr,end='',file=sys.stderr);r.check_returncode()
(out/'report.json').write_text(json.dumps({'status':'PASS','baseline':'805483a','commands':cmds,'output':r.stdout,'source_sha256':hashlib.sha256((root/'core/src/wf_r17_post7a00.c').read_bytes()).hexdigest(),'scope':'Fresh independent namespaced B12 runtime/queue/callback-state comparison with faults.'},indent=2)+'\n')
