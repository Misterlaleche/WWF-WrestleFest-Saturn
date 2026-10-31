#!/usr/bin/env python3
"""Build the shared session with locked HW1 owners; never write upstream."""
import argparse, hashlib, json, os, re, subprocess
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--out',type=Path,required=True)
    p.add_argument('--probe',type=Path)
    p.add_argument('--sanitize',action='store_true')
    p.add_argument('--sh2',action='store_true')
    p.add_argument('--entry',action='append',default=[],help='Additional native public root, without underscore')
    args=p.parse_args()
    root=Path(__file__).resolve().parents[1]
    native=root.parents[1]/'modo Royal/wp1980/work/hwfix01/native'
    subprocess.run(['python3',str(root/'tools/check_sources.py')],check=True)
    make=(native/'Makefile').read_text()
    names=[]
    for key in ('CORE','R18_HOST'):
        names+=re.search(r'^'+key+r'=(.*)$',make,re.M).group(1).split()
    names+=['src/wp1980_inflate.c','src/wp1980_lz4.c','src/wp1980_inflate_dynamic.c',
            'src/wp1980_vram_cache.c','src/wp1980_royal_init.c']
    sources=[native/n for n in names]+[root/'src/wf_session.c']
    out=args.out.resolve();out.mkdir(parents=True,exist_ok=True)
    flags=['-std=c11','-Wall','-Wextra','-Werror','-O2','-I'+str(native/'include'),
           '-I'+str(root/'src'),'-DWF_R18_ROM_EXTRA_CAPACITY=32u',
           '-DWF_SATURN_HWRAM_HOT_INDEX_BYTES=0x34000u']
    cc=os.environ.get('CC','cc')
    if args.sh2:
        if args.probe or args.sanitize:p.error('--sh2 builds a core archive, without host probes/sanitizers')
        sdk=Path(os.environ['YAUL_INSTALL_ROOT']);cc=str(sdk/'bin/sh2eb-elf-gcc')
        if subprocess.check_output([cc,'-dumpfullversion'],text=True).strip()!='13.2.0':
            raise ValueError('Expected supplied SH-2 GCC13.2.0')
        flags+=['-m2','-mb','-ffreestanding','-ffunction-sections','-fdata-sections',
                '-I'+str(sdk/'sh2eb-elf/include'),'-I'+str(sdk/'sh2eb-elf/include/yaul')]
    if args.sanitize:flags+=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-g']
    if args.probe:sources+=[root/'tests/session_host_audit.c',args.probe.resolve()]
    def build(source):
        obj=out/(source.stem+'.o')
        subprocess.run([cc,*flags,'-c',str(source),'-o',str(obj)],check=True)
        return obj
    if len({s.stem for s in sources})!=len(sources):raise ValueError('Duplicate object names')
    with ThreadPoolExecutor(max_workers=4) as pool:objects=list(pool.map(build,sources))
    if args.sh2:
        library=out/'libwf_session.a'
        combined=out/'wf_session_core.o'
        libgcc=subprocess.check_output([cc,'-m2','-mb','-print-libgcc-file-name'],text=True).strip()
        exports=['wf_session_init','wf_session_step','wf_session_prepare_match',*args.entry]
        if any(not re.fullmatch(r'[a-zA-Z_][a-zA-Z0-9_]*',e) for e in exports):raise ValueError('Invalid symbol')
        roots=[v for e in exports for v in ('-u','_'+e)]
        subprocess.run([str(sdk/'bin/sh2eb-elf-ld'),'-r','--gc-sections',*roots,
                        *map(str,objects),libgcc,'-o',str(combined)],check=True)
        # ld -r retains undefined symbol-table entries from discarded input
        # sections. Remove only symbols not needed by any relocation; retain
        # the public session API explicitly. Never whitelist missing owners.
        subprocess.run([str(sdk/'bin/sh2eb-elf-objcopy'),'--strip-unneeded',
                        *['--keep-symbol=_'+e for e in exports],str(combined)],check=True)
        undefined=subprocess.check_output([str(sdk/'bin/sh2eb-elf-nm'),'-u',str(combined)],text=True)
        (out/'unresolved.txt').write_text(undefined)
        unexpected={line.split()[-1] for line in undefined.splitlines()}-{'_memcpy','_memset','_memcmp'}
        if unexpected:raise ValueError('Unresolved session owners: '+repr(unexpected))
        # Replace this generated archive; ar would otherwise retain stale
        # members when the same build directory is reused after a change.
        temporary=out/'libwf_session.new.a'
        if temporary.exists():temporary.unlink()
        subprocess.run([str(sdk/'bin/sh2eb-elf-ar'),'rcsD',str(temporary),str(combined)],check=True)
        temporary.replace(library)
        output=library
    elif args.probe:
        output=out/args.probe.stem
        subprocess.run([cc,*flags,*map(str,objects),'-o',str(output)],check=True)
    else:p.error('Host requires --probe')
    report={'output':str(output),'sha256':hashlib.sha256(output.read_bytes()).hexdigest(),
            'bootable_disc':False,'compiler':subprocess.check_output([cc,'--version'],text=True).splitlines()[0],
            'flags':flags,'sources':{str(s.relative_to(root.parents[1])):{'sha256':hashlib.sha256(s.read_bytes()).hexdigest()} for s in sources}}
    (out/'build.json').write_text(json.dumps(report,indent=2)+'\n')
    print(output)

if __name__=='__main__':main()
