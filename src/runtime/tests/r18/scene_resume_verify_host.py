from pathlib import Path
import subprocess,tempfile,os
root=Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix='wf-scene-resume-',ignore_cleanup_errors=os.name=='nt')as t:
 exe=Path(t)/'test-scene.exe'
 subprocess.run(['cc','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Werror','-Iinclude','src/wf_r18_runtime_handoff.c','src/wf_r17_bootstrap.c','src/wf_r17_m68k_shadow.c','tests/r18/test_scene_resume.c','-o',str(exe)],cwd=root,check=True)
 subprocess.run([str(exe)],check=True)
