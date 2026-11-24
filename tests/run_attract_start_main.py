#!/usr/bin/env python3
"""Physical C/START after natural attract: RESET -> paid TAG/Royal, no state edits."""
from pathlib import Path
import argparse, hashlib, json, subprocess

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--root',type=Path,default=Path(__file__).resolve().parents[1])
p.add_argument('--runtime-build',type=Path,required=True)
p.add_argument('--build-dir',type=Path,required=True)
p.add_argument('--assets',type=Path)
p.add_argument('--driver-source',type=Path)
p.add_argument('--expect-bug',action='store_true',help='Require the original generation-cancellation failure (red control)')
a=p.parse_args();root=a.root.resolve();out=a.build_dir.resolve();out.mkdir(parents=True,exist_ok=True)
runtime=a.runtime_build.resolve();assets=(a.assets or root/'cd').resolve();driver=(a.driver_source or root/'core/src/wf_r18_frontend_driver.c').resolve()
sha=lambda f:hashlib.sha256(Path(f).read_bytes()).hexdigest()
m=json.loads((runtime/'runtime-build.json').read_text())
# Frozen objects may be reused only when their recorded sources/dependencies
# and objects are verified. The driver is always rebuilt from the selected
# current source, so its former source hash is the sole permitted difference.
assert not m['sources_changed_during_build']
for name,digest in m['sources'].items():
 if name!='core/src/wf_r18_frontend_driver.c':assert sha(root/name)==digest,name
for name,digest in m['objects'].items():assert sha(runtime/name)==digest,name
flags=['-std=c11','-O2','-g','-Wall','-Wextra','-Werror','-Wno-unused-variable',
       '-DWF_JO_ENGINE','-DWF_JO_ASSETS_HOST','-DWF_JO_FRONTEND_TEST',
       '-DWF_JO_AUTORUN_MODE=1','-DWF_JO_FRONTEND','-DZ_SOLO','-DNO_GZIP']
# AUTORUN itself is deliberately absent: the actual manual controller path runs.
flags+=['-I'+str(root/n)for n in ['tests/native/jo_host','tests','core/include','port','port/zlib','tests/native']]
boundary=out/'main_boundary.c'
boundary.write_text('#include "jo/jo.h"\nstatic bool audit_pad_read(unsigned port,unsigned key);\n#define jo_is_input_key_pressed audit_pad_read\n#include '+json.dumps(str(root/'port/main.c'))+'\n#undef jo_is_input_key_pressed\n')
obj=out/'wf_r18_frontend_driver.o';binary=out/'attract_start_probe'
commands=[['cc',*flags,'-c',str(driver),'-o',str(obj)]]
objects=[str(obj)if n=='wf_r18_frontend_driver.o'else str(runtime/n)for n in sorted(m['objects'])]
test=Path(__file__).with_name('test_attract_start_main.c')
commands.append(['cc',*flags,'-DWF_JO_PROBE_MAIN='+json.dumps(str(boundary)),
 '-Wl,--wrap=wf_jo_frontend_run','-Wl,--wrap=wf_jo_frontend_after_combat_at_level',
 '-Wl,--wrap=jo_is_input_key_pressed','-Wl,--wrap=wf_r18_title_step',
 str(test),str(root/'tests/native/assets_fixture.c'),str(root/'port/wf_jo_frontend.c'),
 str(root/'port/wf_jo_clock.c'),*objects,'-o',str(binary)])
for cmd in commands:subprocess.run(cmd,check=True)
identity={'runtime_manifest_sha256':sha(runtime/'runtime-build.json'),'binary_sha256':sha(binary),
          'commands':commands,'sources':{str(n):sha(n)for n in [test,Path(__file__),driver,boundary,root/'port/main.c',root/'port/wf_jo_frontend.c']}}
(out/'build.json').write_text(json.dumps(identity,indent=2)+'\n')
cases=[]
for name,mode,fault in [('tag',0,0),('royal',1,0),('real-fault',0,1)]:
 dest=out/name;dest.mkdir(exist_ok=True)
 cmd=[str(binary),str(dest),str(mode),str(fault)]
 import os
 env=dict(os.environ,WF_JO_TEST_CACHE_BYTES='565248')
 with (dest/'native.log').open('w')as log:result=subprocess.run(cmd,cwd=assets,env=env,stdout=log,stderr=subprocess.STDOUT)
 summary=json.loads((dest/'result-summary.json').read_text());summary.update(name=name,returncode=result.returncode,command=cmd)
 if fault:
  okay=result.returncode==0 and summary['failed']==1 and summary['generation']==0 and summary['driver_dependency']==0xdead8490
 elif a.expect_bug:
  okay=result.returncode==2 and summary['failed']==1 and summary['driver_phase']==7 and summary['driver_pc']==0x52be and summary['driver_dependency']==0x83a4 and summary['generation']==1 and summary['credited_frames']==0
 else:
  okay=result.returncode==0 and summary['failed']==0 and summary['generation']==1 and summary['match_generation']==1 and summary['credited_entries']==1 and summary['credited_frames']==600 and summary['mode']&1==mode
 summary['passed']=bool(okay);cases.append(summary)
 print(name,'PASS'if okay else 'FAIL',summary)
# Verify inputs did not drift while running, including source/driver overrides.
for n,digest in identity['sources'].items():assert sha(n)==digest,n
report={'status':'PASS'if all(c['passed']for c in cases)else'FAIL','expect_bug':a.expect_bug,'cases':cases,'build_identity_sha256':sha(out/'build.json'),'scope':'Host control-flow and real gameplay; no Saturn timing claim'}
(out/'summary.json').write_text(json.dumps(report,indent=2)+'\n')
raise SystemExit(0 if report['status']=='PASS'else 1)
