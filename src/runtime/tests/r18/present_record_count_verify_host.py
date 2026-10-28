from pathlib import Path
import subprocess,tempfile,os
root=Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix='wf-record-count-',ignore_cleanup_errors=os.name=='nt')as t:
 exe=Path(t)/'test-count.exe'
 files=['src/wf_r18_present.c','src/wf_r18_metasprite.c','src/wf_actor.c','src/wf_saturn_yaul_backend.c','src/wf_saturn_memory.c','src/wf_saturn_render_bridge.c','src/wf_vdp1_plan.c','tests/r18/test_present_record_count.c']
 subprocess.run(['cc','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Werror','-Iinclude',*files,'-o',str(exe)],cwd=root,check=True)
 subprocess.run([str(exe)],check=True)
