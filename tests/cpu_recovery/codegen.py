from pathlib import Path
import subprocess,re,json,hashlib
from common import verify_reference
root=Path(__file__).resolve().parents[2];here=Path(__file__).parent;out=root/'build/cpu-recovery/codegen';out.mkdir(parents=True,exist_ok=True)
base=root/'tests/native/cpu_reference';cc=root/'sdk/Compiler/LINUX/bin/sh-none-elf-gcc-8.2.0'; objcopy=root/'sdk/Compiler/LINUX/bin/sh-none-elf-objcopy';size='size'
verify_reference(base)
flags=['-m2','-std=gnu99','-O2','-fno-builtin','-fno-peephole2','-Wall','-Wextra','-Werror','-I'+str(root/'sdk/Compiler/WINDOWS/sh-elf/include')]
report={}
for label,tree in [('baseline',base),('candidate',root)]:
 inc=['-I'+str(tree/'core/include'),'-I'+str(tree/'port')]
 dest=out/(label+'-helpers.s');subprocess.run([str(cc),*flags,*inc,'-S',str(here/'helpers_wrapper.c'),'-o',str(dest)],check=True)
 text=dest.read_text();report[label]={}
 for name in ['get16_6','get32_6','move']:
  match=re.search(r'^_candidate_'+name+r':\n(.*?)(?=\n\s*\.size\s+_candidate_'+name+r',)',text,re.M|re.S)
  section=match.group(1) if match else ''
  count=sum(bool(re.match(r'^\s+[a-z][a-z0-9./]*(?:\s|$)',l)) for l in section.splitlines())
  report[label][name]={'instructions':count,'assembly':section}
 layout=here/'helpers_wrapper.c';s=layout.read_text();start=s.index(' const size_t v[]=');end=s.index(';\n size_t i;',start)
 data=s[start:end+1].replace(' const size_t v[]=','const unsigned wf_abi[]=')
 abi=out/(label+'-abi.c');abi.write_text('#include "wf_runtime.h"\n#include "wf_r17_actions_r11.h"\n#include <stddef.h>\n#define R(x) offsetof(wf_runtime_t,x)\n#define B(x) offsetof(wf_r17_actions_r11_bridge_t,x)\n'+data+'\n')
 obj=out/(label+'-abi.o');subprocess.run([str(cc),*flags,*inc,'-c',str(abi),'-o',str(obj)],check=True)
 binary=out/(label+'-abi.bin');subprocess.run([str(objcopy),'-O','binary','-j','.rodata',str(obj),str(binary)],check=True)
 raw=binary.read_bytes().hex()
 report[label]['abi_hex']=raw
 for name in ['wf_physics.c','wf_r17_post7a00.c','wf_r18_frontend_coin.c']:
  src=tree/'core/src'/name
  if not src.exists():continue
  obj=out/(label+'-'+name+'.o');subprocess.run([str(cc),*flags,*inc,'-c',str(src),'-o',str(obj)],check=True)
  report[label][name]=subprocess.check_output([str(size),str(obj)],text=True).splitlines()[-1].split()[:4]
assert report['baseline']['abi_hex']==report['candidate']['abi_hex'],report
report['status']='PASS';(out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
for k in ['baseline','candidate']:
 print(k,{n:report[k][n]['instructions'] for n in ['get16_6','get32_6','move']},'SH2 ABI equal')
