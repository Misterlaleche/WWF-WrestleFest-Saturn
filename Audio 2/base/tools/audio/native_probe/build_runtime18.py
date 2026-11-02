"""Compile/link the production runtime with SCSP to ELF only; no disc tool is called."""
import argparse,hashlib,json,os,re,subprocess,sys
from pathlib import Path

def current_sources(repo):
    """Read the production graphics/audio translation units from its Makefile."""
    runtime=repo/'src/runtime'
    text=(runtime/'Makefile.saturn-disc').read_text().replace('\\\n',' ')
    defaults={name:value for name,value in
              re.findall(r'^(R18_ENTRY|R18_PRESENT)\?=\s*(\S+)',text,re.M)}
    units=[]
    for line in text.splitlines():
        match=re.match(r'^SH_SRCS(?::=|\+=)\s*(.*)',line)
        if not match:continue
        for unit in match[1].split():
            for name,value in defaults.items():unit=unit.replace('$('+name+')',value)
            if '$' in unit:raise ValueError('Unsupported source expression: '+unit)
            if unit not in units:units.append(unit)
    if not units:raise ValueError('No production sources in Makefile.saturn-disc')
    return [runtime/unit for unit in units]

def build(repo,toolchain,sdk,out,baseline=False):
    repo,toolchain,sdk,out=[p.resolve() for p in (repo,toolchain,sdk,out)]
    out.mkdir(parents=True,exist_ok=True)
    runtime=repo/'src/runtime'
    if baseline:
        manifest=json.loads((runtime/'FPS-SOURCE-MANIFEST.json').read_text())
        units=manifest['translation_units']
        frozen=out/'frozen'
        for name in manifest['source_files']:
            target=frozen/name;target.parent.mkdir(parents=True,exist_ok=True)
            data=subprocess.check_output(['git','-C',str(repo),'show',
                  'a72456d5077da334a2dd461d7eafff51dedcce2e:src/runtime/'+name])
            target.write_bytes(data)
        runtime=frozen
        sources=[runtime/n for n in units]
        sources += [repo/'src/runtime/src/wf_saturn_allocator_partition.c']
    else:
        sources=current_sources(repo)
        sources += [runtime/'src/wf_saturn_allocator_partition.c']
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
    elf=out/'wrestlefest-runtime18-scsp.elf'
    nm_tool=binutils/'nm.exe'
    readelf_tool=binutils/'readelf.exe'
    if os.name != 'nt' and (toolchain/'bin/sh2eb-elf-gcc').is_file() and not baseline:
        # The installed Linux SDK already has the production compiler/specs.
        # Invoke only an explicit ELF target through the same Makefile as the
        # graphical build, so source lists, LTO and allocator ownership agree.
        env.update(YAUL_INSTALL_ROOT=str(sdk),YAUL_ARCH_SH_PREFIX='sh2eb-elf',
                   YAUL_ARCH_M68K_PREFIX='m68keb-elf',YAUL_OPTION_MALLOC_IMPL='tlsf',
                   R18_DIAG_RATE='1',SILENT='1',NOCOLOR='1')
        run(['make','-f','Makefile.saturn-disc','-j4',
             'SH_BUILD_DIR='+str(out),'SH_PROGRAM=wrestlefest-runtime18-scsp',elf],runtime)
        (out/'runtime18.map').write_bytes((out/'wrestlefest-runtime18-scsp.map').read_bytes())
        nm_tool=toolchain/'bin/sh2eb-elf-nm'
        readelf_tool=toolchain/'bin/sh2eb-elf-readelf'
    else:
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
        native_allocator=out/'wf_sdk_allocator_internal.o'
        run([sys.executable,'-B',repo/'tools/audio/native_probe/prepare_allocator_sdk.py',
             '--archive',lib/'libyaul.a','--objcopy',binutils/'objcopy.exe','--out',native_allocator])
        objects.append(native_allocator)
        elf=out/'wrestlefest-runtime18-scsp.elf'
        run([binutils/'ld.exe','-static','--gc-sections','-Map',out/'runtime18.map',
             '-T',repo/'src/runtime/ldscripts/wf_memory.x','--defsym','___master_stack=0x06004000',
             '--wrap=tlsf_pool_create','--wrap=tlsf_pool_get',
             '--undefined=_wf_saturn_tlsf_pool_create','--undefined=_wf_saturn_tlsf_pool_get',
             '--undefined=_tlsf_size','--undefined=_tlsf_pool_overhead',
             lib/'crt0.o',lib/'init.o',*objects,
             '--start-group',lib/'libyaul.a',gcc,'--end-group','-o',elf],lib)
    undefined=run([nm_tool,'-u',elf])
    if undefined.strip():raise ValueError('Unresolved symbols: '+undefined)
    symbols=run([nm_tool,'-n',elf])
    (out/'symbols.txt').write_text(symbols)
    run([sys.executable,'-B',repo/'validation/verify_saturn_stack_layout.py',elf])
    run([sys.executable,'-B',repo/'validation/verify-hwram-budget.py',
         out/'runtime18.map','--elf',elf,'--json',out/'hwram-budget.json'])
    heap_report=json.loads((out/'hwram-budget.json').read_text())
    end=None
    for line in symbols.splitlines():
        p=line.split()
        if len(p)==3 and p[2]=='___end':end=int(p[0],16)
    if end is None:raise ValueError('No heap boundary')
    report=dict(baseline=baseline,translation_units=len(sources),
                elf_sha256=hashlib.sha256(elf.read_bytes()).hexdigest(),
                elf_bytes=elf.stat().st_size,linked_end=end,
                heap_available=heap_report['heap_available'],heap_reserved=16384,
                heap_raw_bytes=heap_report['heap_raw_bytes'],
                slave_stack_bytes=heap_report['slave_stack_bytes'],
                heap_gate_passed=heap_report['status']=='PASS',
                unresolved_symbols=[],bin_cue_iso_generated=False,
                hardware_validation=False,
                embedded_firmware=False,cdfs_firmware_asset='SCSP.ELF',cdfs_music_module='MUSIC.ELF',cdfs_sfx_pack='SFX.DAT',vdp1_music_reserve=[0x05c5c000,0x05c80000])
    (out/'build-report.json').write_text(json.dumps(report,indent=2)+'\n')
    (out/'elf.txt').write_text(run([readelf_tool,'-h','-l',elf]))
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
