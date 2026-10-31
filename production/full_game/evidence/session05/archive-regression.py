import hashlib,shutil,subprocess
from pathlib import Path
base=Path('/work/session05');out=base/'sh2-release';out.mkdir(exist_ok=False)
ar='/sdk/bin/sh2eb-elf-ar';lib=out/'libwf_session.a'
shutil.copy2(base/'sh2-verified/libwf_session.a',lib)
subprocess.run([ar,'rcsD',str(lib),str(base/'sh2-verified/wf_actor.o')],check=True)
assert 'wf_actor.o' in subprocess.check_output([ar,'t',str(lib)],text=True).splitlines()
subprocess.run(['python3','/work/repo/production/full_game/tools/build_session.py','--sh2','--out',str(out)],check=True)
assert subprocess.check_output([ar,'t',str(lib)],text=True).splitlines()==['wf_session_core.o']
assert hashlib.sha256(lib.read_bytes()).hexdigest()=='86f9259f148d56f9f326c5111ebf3a9bd02da68a5d3f2e69d2744f54bf66a452'
print('PASS stale archive member removed; native session archive unchanged and verified')
