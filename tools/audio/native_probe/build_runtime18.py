"""Compile/link the FPS runtime with SCSP to ELF only; no disc tool is called."""
import argparse,hashlib,json,os,subprocess
from pathlib import Path

def build(repo,toolchain,sdk,out,baseline=False):
    repo,toolchain,sdk,out=[p.resolve() for p in (repo,toolchain,sdk,out)]
    out.mkdir(parents=True,exist_ok=True)
    runtime=repo/'src/runtime'
    manifest=json.loads((runtime/'FPS-SOURCE-MANIFEST.json').read_text())
    units=manifest['translation_units']
    if baseline:
        frozen=out/'frozen'
        for name in manifest['source_files']:
            target=frozen/name;target.parent.mkdir(parents=True,exist_ok=True)
            data=subprocess.check_output(['git','-C',str(repo),'show',
                  'a72456d5077da334a2dd461d7eafff51dedcce2e:src/runtime/'+name])
            target.write_bytes(data)
        runtime=frozen
    sources=[runtime/n for n in units]
    if not baseline:
        sources += [runtime/'src/wf_r18_scsp.c']
        sources += [repo/'src/audio/native'/n for n in
                    ['wf_native_host.c','wf_native_queue.c','wf_native_elf.c','wf_native_yaul.c','wf_sfx_upload.c','wf_music_elf.c']]
    env=dict(os.environ)
    env['PATH']=str(toolchain/'bin')+os.pathsep+env.get('PATH','')
    cc=toolchain/'bin/sh-elf-gcc.exe'
    binutils=toolchain/'sh-elf/bin'
    logs=[]
    def run(command,cwd=out):
        r=subprocess.run(list(map(str,command)),cwd=cwd,env=env,
                         text=True,capture_output=True)
        logs.append(dict(command=list(map(str,command)),exit_code=r.returncode,
                         stdout=r.stdout,stderr=r.stderr))
        (out/'commands.json').write_text(json.dumps(logs,indent=2)+'\n')
        if r.returncode:raise RuntimeError(r.stderr)
        return r.stdout
    flags=['-m2','-mb','-ffreestanding','-fno-builtin','-O2','-std=c11',
           '-fomit-frame-pointer','-ffast-math','-fstrict-aliasing',
           '-ffunction-sections','-fdata-sections','-Wall','-Wextra','-Werror',
           '-DWF_PLATFORM_SATURN=1','-DWF_R18_DIAG_RATE=1',
           '-B',str(binutils)+os.sep,'-I',str(runtime/'include'),
           '-I',str(repo/'src/audio/native'),
           '-isystem',str(sdk/'sh2eb-elf/include'),
           '-isystem',str(sdk/'sh2eb-elf/include/yaul')]
    objects=[]
    for i,source in enumerate(sources):
        obj=out/(source.stem+'.o')
        run([cc,*flags,'-c',source,'-o',obj])
        objects.append(obj)
        if i%25==0:print('Compiled',i+1,'/',len(sources),flush=True)
    lib=sdk/'sh2eb-elf/lib'
    gcc=sdk/'lib/gcc/sh2eb-elf/13.2.0/libgcc.a'
    elf=out/'wrestlefest-runtime18-scsp.elf'
    run([binutils/'ld.exe','-static','--gc-sections','-Map',out/'runtime18.map',
         '-T','ldscripts/yaul.x','--defsym','___master_stack=0x06004000',
         '--defsym','___slave_stack=0x06001E00',
         lib/'crt0.o',lib/'init.o',*objects,
         '--start-group',lib/'libyaul.a',gcc,'--end-group','-o',elf],lib)
    undefined=run([binutils/'nm.exe','-u',elf])
    if undefined.strip():raise ValueError('Unresolved symbols: '+undefined)
    symbols=run([binutils/'nm.exe','-n',elf])
    (out/'symbols.txt').write_text(symbols)
    end=None
    for line in symbols.splitlines():
        p=line.split()
        if len(p)==3 and p[2]=='___end':end=int(p[0],16)
    if end is None:raise ValueError('No heap boundary')
    report=dict(baseline=baseline,translation_units=len(sources),
                elf_sha256=hashlib.sha256(elf.read_bytes()).hexdigest(),
                elf_bytes=elf.stat().st_size,linked_end=end,
                heap_available=0x6100000-end,heap_reserved=16384,
                heap_gate_passed=(0x6100000-end)>=16384,
                unresolved_symbols=[],bin_cue_iso_generated=False,
                hardware_validation=False,
                embedded_firmware=False,cdfs_firmware_asset='SCSP.ELF',cdfs_music_module='MUSIC.ELF',cdfs_sfx_pack='SFX.DAT',vdp1_music_reserve=[0x05c5c000,0x05c80000])
    (out/'build-report.json').write_text(json.dumps(report,indent=2)+'\n')
    (out/'elf.txt').write_text(run([binutils/'readelf.exe','-h','-l',elf]))
    if not report['heap_gate_passed']:
        raise ValueError('Runtime exceeds the reserved HWRAM heap budget')
    print(json.dumps(report,indent=2))
    return report
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--repo',type=Path,default=Path(__file__).resolve().parents[3])
    for name in ['toolchain','sdk','out']:p.add_argument('--'+name,type=Path,required=True)
    p.add_argument('--baseline',action='store_true')
    a=p.parse_args();build(a.repo,a.toolchain,a.sdk,a.out,a.baseline)
