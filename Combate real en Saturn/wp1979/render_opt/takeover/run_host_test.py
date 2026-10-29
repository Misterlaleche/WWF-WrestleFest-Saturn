#!/usr/bin/env python3
"""Host regression using the unmodified current native input sources."""
from pathlib import Path
import hashlib
import json
import re
import subprocess

here=Path(__file__).resolve().parent
project=here.parents[1]
runtime=project/'native/src/runtime'
build=here/'build'
build.mkdir(exist_ok=True)
entry=(runtime/'src/wp1979_entry.c').read_text()
match=re.search(r'static uint8_t controls\(const smpc_peripheral_digital_t \*sample\)\n\{.*?\n\}',entry,re.S)
assert match,'Unable to extract the actual controls() function'
(build/'controls.inc').write_text(match[0]+'\n')
flags=['-std=c11','-O2','-Wall','-Wextra','-Werror','-ffunction-sections','-fdata-sections',
       '-I'+str(runtime/'include'),'-I'+str(build)]
objects=[]
for variant in ['normal','noinput']:
 obj=build/(variant+'.o');objects.append(obj)
 defs=[f'-Dwp1979_input={variant}_input',f'-Dwp1979_demo_active={variant}_demo_active']
 if variant=='noinput':defs+=['-DWF_R18_REPLAY_NOINPUT=1']
 subprocess.run(['cc',*flags,*defs,'-c',str(runtime/'src/wp1979_input.c'),'-o',str(obj)],check=True)
exe=build/'test_input'
subprocess.run(['cc',*flags,str(here/'test_input.c'),str(runtime/'src/wf_input.c'),
                *(str(p) for p in objects),'-Wl,--gc-sections','-o',str(exe)],check=True)
result=subprocess.run([str(exe)],text=True,capture_output=True,check=True)
print(result.stdout,end='')
sources=[runtime/'src/wp1979_input.c',runtime/'src/wp1979_tape.inc',
         runtime/'src/wp1979_entry.c',runtime/'src/wf_input.c']
report={'status':'pass','kind':'host-native-source-regression','stdout':result.stdout,
        'controls_function_sha256':hashlib.sha256(match[0].encode()).hexdigest(),
        'source_sha256':{str(p.relative_to(project)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sources}}
(here/'host-test-result.json').write_text(json.dumps(report,indent=2)+'\n')
