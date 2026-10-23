#!/usr/bin/env python3
"""Natural HOST replay on frozen a4be15f dependencies plus the CPU2 patch.

The three explicit overrides are wf_cpu_runtime.c, its header and the
controller16 helper reuse in wf_state5_exec.c. In-progress action22 or other
working-tree sources are not compiled into this probe.
"""
import argparse,hashlib,io,json,os,re,subprocess,sys,tarfile,tempfile
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('output',type=Path);p.add_argument('--frames',type=int,default=20000);p.add_argument('--attack',action='store_true');p.add_argument('--source-root',type=Path)
args=p.parse_args();here=Path(__file__).resolve().parent;root=here.parents[1];repo=root.parents[1];out=args.output.resolve()
out.mkdir(parents=True,exist_ok=True);assert not (out/'host.log').exists(),'Preserve prior evidence by using a new output directory'
base=json.loads((here/'cpu_state2_manifest.json').read_text())['base_commit']
names=('src/wf_cpu_runtime.c','include/wf_cpu_runtime.h','src/wf_state5_exec.c')
overrides={name:((args.source_root or root)/name).read_bytes() for name in names}
assets=root/'audit/runtime18-cd';asset_hash={name:hashlib.sha256((assets/name).read_bytes()).hexdigest() for name in ('ROM68K.BIN','SPRITES.PAK','R18RING.BIN')}
with tempfile.TemporaryDirectory() as temp:
 frozen=Path(temp);archive=subprocess.run(['git','archive',base,'src/runtime'],cwd=repo,stdout=subprocess.PIPE,check=True).stdout
 with tarfile.open(fileobj=io.BytesIO(archive)) as tar:tar.extractall(frozen,filter='data')
 fr=frozen/'src/runtime'
 for name,data in overrides.items():(fr/name).write_bytes(data)
 # The frozen loader cannot consume future enlarged HOTSPR headers. Rebuild
 # its historical pack with the builder and tile list from the same commit.
 frozen_assets=fr/'audit/runtime18-cd';frozen_assets.mkdir(parents=True,exist_ok=True)
 for name in asset_hash:(frozen_assets/name).symlink_to(assets/name)
 subprocess.run([sys.executable,str(fr/'tools/build_r18_hot_sprites.py'),str(fr/'validation/runtime18-hot-tiles.txt'),str(assets/'SPRITES.PAK'),str(frozen_assets/'HOTSPR.BIN')],check=True)
 frozen_hot_hash=hashlib.sha256((frozen_assets/'HOTSPR.BIN').read_bytes()).hexdigest()
 make=(fr/'Makefile').read_text();core=re.search(r'^CORE=(.*)$',make,re.M)[1].split();host=re.search(r'^R18_HOST=(.*)$',make,re.M)[1].split()
 for name in ('src/wf_cpu_runtime.c','src/wf_r17_dispatch_1c03e.c'):core.remove(name)
 binary=out/'cpu_state2_host_probe'
 subprocess.run(['cc','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,*host,'tests/r18/allies_cpu_trace.c','tests/r18/allies_dispatch_trace.c','tests/r18/allies_trace.c','tests/r17/direct_combat_probe.c','-o',str(binary)],cwd=fr,check=True)
 env={**os.environ,'WF_ALLIES_OUT':str(out),'WF_PROBE_FRAMES':str(args.frames),'WF_PROBE_REQUIRE_COMPLETE':'1',
      'WF_PROBE_ROM_TRACE':str(out/'rom-pages.csv'),'WF_PROBE_SPRITE_TRACE':str(out/'sprite-reads.csv')}
 env.pop('WF_PROBE_ATTACK',None);env.pop('WF_PROBE_REQUIRE_RESIDENT',None)
 if args.attack:env['WF_PROBE_ATTACK']='1'
 with (out/'host.log').open('w') as log:
  run=subprocess.run([str(binary),'audit/runtime18-cd/ROM68K.BIN','audit/runtime18-cd/SPRITES.PAK','audit/runtime18-cd/R18RING.BIN','audit/runtime18-cd/HOTSPR.BIN'],cwd=fr,env=env,stdout=log,stderr=subprocess.STDOUT)
metadata={'base_commit':base,'overrides_sha256':{name:hashlib.sha256(data).hexdigest() for name,data in overrides.items()},
          'assets_sha256':{**asset_hash,'HOTSPR.BIN':frozen_hot_hash},'hot_pack_source_commit':base,'host_binary_sha256':hashlib.sha256(binary.read_bytes()).hexdigest(),
          'frames_budget':args.frames,'input':'RIGHT60-179/A180-183' if args.attack else 'none','reset_seed':0,'exit_code':run.returncode}
for name,want in asset_hash.items():assert hashlib.sha256((assets/name).read_bytes()).hexdigest()==want,('Asset changed while running',name)
(out/'manifest.json').write_text(json.dumps(metadata,indent=2)+'\n')
print((out/'host.log').read_text()[-6000:]);raise SystemExit(run.returncode)
