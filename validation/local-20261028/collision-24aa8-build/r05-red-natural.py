import os,shutil,subprocess,sys,tarfile,tempfile
from pathlib import Path
root=Path('.').resolve();runtime=root/'src/runtime';out=root/'validation/local-20261028/collision-24aa8-natural-capture';test=runtime/'tests/r18/collision_24aa8_natural_verify_rom.py'
with tempfile.TemporaryDirectory(prefix='wf-natural-r05-',ignore_cleanup_errors=True) as td:
 project=Path(td)/'runtime18';frozen=project/'src/runtime'
 for name in ('src','include','generated'):shutil.copytree(runtime/name,frozen/name)
 for name in ('Makefile','tests/r18/collision_249be_oracle_bridge.c','tests/r18/collision_24aa8_natural_verify_rom.py','audit/runtime18-cd/ROM68K.BIN'):
  p=frozen/name;p.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(runtime/name,p)
 with tarfile.open(root/'validation/local-20261028/eval-r05-build/r05-sources.tar.gz') as tar:
  (frozen/'src/wf_collision_dispatch.c').write_bytes(tar.extractfile('runtime18/src/runtime/src/wf_collision_dispatch.c').read())
 dest=project/'validation/local-20261028/collision-24aa8-natural-capture/royal-rush';dest.parent.mkdir(parents=True,exist_ok=True);shutil.copytree(out/'royal-rush',dest,ignore=shutil.ignore_patterns('snapshot-*','*.csv'))
 result=subprocess.run([sys.executable,str(frozen/'tests/r18/collision_24aa8_natural_verify_rom.py')],cwd=project,capture_output=True,text=True)
 (out/'natural-r05-red.log').write_text(result.stdout+result.stderr,encoding='utf8');print('R05 natural replay exit',result.returncode);print((result.stdout+result.stderr)[-1600:])
 assert result.returncode!=0 and 'AssertionError' in result.stderr
