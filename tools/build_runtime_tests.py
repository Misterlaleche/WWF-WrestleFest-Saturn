#!/usr/bin/env python3
"""Compile the production Jo common core with the supplied native TAG/Royal fixtures.

The host replaces console input/output only. It loads the actual resident
asset pack and runs production C gameplay through complete match endpoints.
No 68000 interpreter, stored gameplay frames or simulator is linked.
"""
from pathlib import Path
import argparse, concurrent.futures, hashlib, json, os, re, subprocess
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--build-dir',type=Path)
parser.add_argument('--worker',action='store_true',
                    help='Enable the deferred host asset worker; default is the production serial fallback')
args=parser.parse_args()
root=Path(__file__).resolve().parents[1]
here=root/'tests/native'
build=(args.build_dir or root/'build/native-tests').resolve();build.mkdir(parents=True,exist_ok=True)
make=(root/'core_sources.mk').read_text()
relative=list(dict.fromkeys(re.findall(r'core/(?:src|generated)/\w+\.c',make)))
for name in ['wp1980_input','wp1980_royal_init']:
    path='core/src/'+name+'.c'
    if path not in relative:relative.append(path)
sources=[root/p for p in relative]
sources += [root/'port'/p for p in ['wf_jo_assets.c','wf_jo_video.c','wf_jo_backend.c','wf_jo_scene_prepare.c','wf_jo_save.c']]
obsolete=build/'wf_saturn_yaul_backend.o'
if obsolete.exists():obsolete.unlink()
sources += [root/'port/zlib'/p for p in ['inflate.c','inffast.c','inftrees.c','adler32.c','zutil.c']]
worker_object=build/'assets_worker_host.o'
if args.worker:sources.append(here/'assets_worker_host.c')
elif worker_object.exists():worker_object.unlink()
flags=['-std=c11','-O2','-g','-Wall','-Wextra','-Werror','-DWF_JO_ENGINE','-DWF_JO_ASSETS_HOST','-DZ_SOLO','-DNO_GZIP']
if args.worker:flags.append('-DWF_JO_ASSETS_TEST_WORKER')
flags += ['-I'+str(p) for p in [root/'core/include',root/'port',root/'port/zlib',here]]
cc=os.environ.get('CC','cc')
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
manifest=build/'runtime-build.json'
previous=json.loads(manifest.read_text()) if manifest.exists() else {}
dependencies=sorted({p for folder in [root/'core',root/'port',here]
                     for pattern in ['*.h','*.inc'] for p in folder.rglob(pattern)})
inputs=set(sources+dependencies+[root/'core_sources.mk',Path(__file__).resolve(),here/'assets_fixture.c'])
inputs.update(here/(name+'_probe.c') for name in ['tag','royal','frontend'])
hashes={str(p.relative_to(root)):sha(p) for p in sorted(inputs)}
dependency_hashes={str(p.relative_to(root)):hashes[str(p.relative_to(root))] for p in dependencies}
configuration=dict(compiler=cc,flags=flags,worker=args.worker,dependencies=dependency_hashes)
rebuild=previous.get('configuration')!=configuration
commands=[]
def run(argv,name):
    log=build/(name+'.build.log')
    with log.open('w') as stream:
        result=subprocess.run(argv,cwd=root,stdout=stream,stderr=subprocess.STDOUT)
    entry=dict(argv=argv,cwd=str(root),log=str(log),returncode=result.returncode,cached=False)
    if result.returncode:print(log.read_text(),end='')
    result.check_returncode()
    return entry
def compile_one(source):
    obj=build/(source.stem+'.o')
    extra=['-Wno-implicit-fallthrough'] if source.parent.name=='zlib' else []
    argv=[cc,*flags,*extra,'-c',str(source),'-o',str(obj)]
    changed=previous.get('sources',{}).get(str(source.relative_to(root)))!=hashes[str(source.relative_to(root))]
    if rebuild or changed or not obj.exists() or previous.get('objects',{}).get(obj.name)!=sha(obj):
        command=run(argv,source.stem)
    else:command=dict(argv=argv,cwd=str(root),returncode=0,cached=True)
    return str(obj),command
with concurrent.futures.ThreadPoolExecutor(max_workers=4) as workers:
    compiled=list(workers.map(compile_one,sources))
objects=[obj for obj,_ in compiled]
commands.extend(command for _,command in compiled)
for name in ['tag','royal','frontend']:
    commands.append(run([cc,*flags,str(here/(name+'_probe.c')),str(here/'assets_fixture.c'),*objects,'-o',str(build/(name+'_probe'))],name+'-link'))
changed=[name for name,value in hashes.items() if sha(root/name)!=value]
manifest.write_text(json.dumps(dict(configuration=configuration,sources=hashes,
    sources_changed_during_build=changed,objects={Path(p).name:sha(Path(p)) for p in objects},
    commands=commands),indent=2)+'\n')
if changed:raise SystemExit('Sources changed during native build: '+', '.join(changed))
print('Built TAG and Royal fixtures against',len(relative),'production core translation units in',build,
      '(deferred worker)' if args.worker else '(serial fallback)')
