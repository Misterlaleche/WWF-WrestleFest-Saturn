from pathlib import Path
import subprocess,json,hashlib,sys
from common import verify_reference
root=Path(__file__).resolve().parents[2];here=Path(__file__).parent;out=root/'build/cpu-recovery/helper-gate';out.mkdir(parents=True,exist_ok=True)
base=root/'tests/native/cpu_reference';cc='cc';flags=['-std=c11','-O2','-g','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-fno-builtin','-fsanitize=address,undefined','-fno-omit-frame-pointer']
verify_reference(base)
rename=['wf_r17_m68k_'+n for n in ['software_reset','move_b','move_w','move_l','moveq','clr_l']]+['wf_r17_rng21b4_step','wf_r17_rng21b4_live']
cmds=[]
def run(c):cmds.append(c);subprocess.run(c,check=True)
objects=[]
for prefix,tree in [('reference_',base),('candidate_',root)]:
 inc=['-I'+str(tree/'core/include'),'-I'+str(tree/'port')]
 for name,source,extra in [('wrapper',here/'helpers_wrapper.c',['-DWF_TEST_PREFIX='+prefix]),('shadow',tree/'core/src/wf_r17_m68k_shadow.c',[]),('clock',tree/'port/wf_jo_clock.c',[])]:
  obj=out/(prefix+name+'.o');defs=[]
  if prefix=='reference_':defs=['-D'+n+'=reference_'+n for n in rename]+['-Dwf_jo_clock_'+n+'=reference_wf_jo_clock_'+n for n in ['start','advance','take']]
  run([cc,*flags,*inc,*defs,*extra,'-c',str(source),'-o',str(obj)]);objects.append(str(obj))
run([cc,*flags,'-I'+str(root/'core/include'),'-I'+str(root/'port'),str(here/'helpers_test.c'),*objects,'-o',str(out/'helpers')])
r=subprocess.run([str(out/'helpers')],text=True,capture_output=True);print(r.stdout,end='');print(r.stderr,end='',file=sys.stderr);r.check_returncode()
paths=['core/include/wf_actor.h','core/include/wf_r17_m68k_shadow.h','core/src/wf_r17_m68k_shadow.c','port/wf_jo_clock.c']
report={'status':'PASS','baseline':'805483a','commands':cmds,'output':r.stdout,'hashes':{p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in paths},'scope':'Independent B12 byte and register/clock code; fresh reconstruction gate, not historical result.'}
(out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
