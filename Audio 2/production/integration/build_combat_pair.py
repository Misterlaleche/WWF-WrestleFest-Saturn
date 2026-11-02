#!/usr/bin/env python3
"""Build an isolated, identical-code TAG or Royal control/Audio2 pair.

Copies all inputs to a new output directory. The only game build substitution
is its existing audio implementation and the explicit audio linker reserve.
ROM assets, firmware and game sources are required local inputs, never shipped
by this script. This is diagnostic integration, not a production acceptance.
"""
import argparse, hashlib, importlib.util, json, os, re, shutil, struct, subprocess
from pathlib import Path

def sha(p): return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def symbols(nm,elf):
    out=subprocess.check_output([str(nm),'-n',str(elf)],text=True)
    return {p[2]:int(p[0],16) for line in out.splitlines() if len(p:=line.split())==3}
def patch_data_word(elf,address,old,new):
    b=bytearray(elf);table=struct.unpack_from('>I',b,32)[0]
    stride,count=struct.unpack_from('>HH',b,46)
    for i in range(count):
        h=struct.unpack_from('>10I',b,table+i*stride)
        if h[1]==1 and h[2]&1 and h[3]<=address and address+4<=h[3]+h[5]:
            off=h[4]+address-h[3]
            assert struct.unpack_from('>I',b,off)[0]==old
            struct.pack_into('>I',b,off,new)
            return bytes(b),off
    raise ValueError('Enabled flag is not in a writable file-backed ELF section')

def build(a):
    royal = a.scenario == 'royal'
    royal_vdp1_hot = royal and not a.royal_cart
    if a.royal_cart and not royal:raise ValueError('--royal-cart applies only to Royal HW1')
    program = 'audio2-royal' if royal else 'audio2-tag'
    artifact = ('AUDIO2ROYAL4M' if a.royal_cart else 'AUDIO2ROYAL') if royal else 'AUDIO2TAG'
    memory_profile=({'name':'royal-vdp2-bank1','base':0x05e26000,'state':0x05e3b000,
                     'shared':0x05e3e000,'end':0x05e3f000} if royal else
                    {'name':'tag-vdp1-tail','base':0x05c5c000,'state':0x05c7c000,
                     'shared':0x05c7f000,'end':0x05c80000})
    for name in ('game_source','audio2','sdk','resources','firmware','out'):
        setattr(a,name,getattr(a,name).resolve())
    here=Path(__file__).resolve().parent
    out=a.out;out.mkdir(parents=True,exist_ok=a.resume)
    game=out/'game';audio=out/'audio2';adapter=out/'adapter';adapter.mkdir(exist_ok=a.resume)
    # Runtime compile inputs only; the original and copied sources stay intact.
    if a.resume:
        original=json.loads((out/'input-manifest.json').read_text())['game_sources']
        if any(sha(game/p)!=h for p,h in original.items()):raise ValueError('Refuse resume with changed game sources')
    else:
        shutil.copytree(a.game_source,game,ignore=shutil.ignore_patterns('audit','__pycache__'))
        original={str(p.relative_to(game)):sha(p) for p in game.rglob('*') if p.is_file()}
    for sub in ('src','include'):shutil.copytree(a.audio2/sub,audio/sub,dirs_exist_ok=a.resume)
    for name in ('wf_music_elf.c','wf_music_elf.h','wf_music_module.h'):
        shutil.copyfile(here/name,audio/'src'/name)
    for name in ('wf_audio2_runtime_adapter.c','wf_audio2_game_queue.c','wf_audio2_game_queue.h','wf_audio2_cadence.h'):
        shutil.copyfile(here/name,adapter/name)
    legacy_headers={}
    if royal:
        # The unchanged game public audio header has a relative include to
        # these declarations. No legacy audio object is compiled or loaded.
        legacy_source=a.game_source.parent/'audio/native'
        legacy_destination=out/'audio/native';legacy_destination.mkdir(parents=True,exist_ok=a.resume)
        for name in ('wf_native_queue.h','wf_native_host.h','wf_native_driver.h'):
            shutil.copyfile(legacy_source/name,legacy_destination/name)
            legacy_headers[name]=sha(legacy_destination/name)
    runtime=game if royal else game/'src/runtime';src=audio/'src'
    source_cd=a.game_source/('audit/wp1980-release-cd' if royal else 'src/runtime/audit/runtime18-cd')
    cd=out/'cd';shutil.copytree(source_cd,cd,dirs_exist_ok=a.resume)
    shutil.copyfile(a.firmware,cd/'SCSP.ELF')
    shutil.copyfile(a.resources/'SFX.DAT',cd/'SFX.DAT')
    (out/'input-manifest.json').write_text(json.dumps({
        'firmware':{'path':str(a.firmware),'sha256':sha(a.firmware)},
        'audio_sources':{str(p.relative_to(audio)):sha(p) for p in audio.rglob('*') if p.is_file()},
        'game_sources':original,
        'legacy_declarations_only':legacy_headers,
        'disc_inputs':{p.name:sha(p) for p in cd.iterdir() if p.is_file()},
    },indent=2)+'\n')
    env=dict(os.environ,YAUL_INSTALL_ROOT=str(a.sdk),YAUL_ARCH_SH_PREFIX='sh2eb-elf',YAUL_PROG_SH_PREFIX='sh2eb-elf')
    cc=a.sdk/'bin/sh2eb-elf-gcc';ld=a.sdk/'bin/sh2eb-elf-ld';nm=a.sdk/'bin/sh2eb-elf-nm'
    objcopy=a.sdk/'bin/sh2eb-elf-objcopy';logs=[]
    def run(cmd,cwd=out):
        cmd=list(map(str,cmd))
        p=subprocess.run(cmd,cwd=cwd,env=env,text=True,capture_output=True)
        logs.append({'command':cmd,'cwd':str(cwd),'returncode':p.returncode,'stdout':p.stdout,'stderr':p.stderr})
        (out/'commands.json').write_text(json.dumps(logs,indent=2)+'\n')
        if p.returncode:raise RuntimeError(p.stderr or p.stdout)
        return p.stdout
    linker=(a.sdk/'sh2eb-elf/lib/ldscripts/yaul.x').read_text()
    old='  ___end = ___bss_end + SIZEOF (.uncached);'
    assert linker.count(old)==1
    reserve='''  ___audio2_reserve_base = ALIGN(___bss_end + SIZEOF(.uncached), 32);
  .audio2_owned ___audio2_reserve_base (NOLOAD) : AT(___audio2_reserve_base)
  {
    ___audio2_hot_start = .; . += 18432; ___audio2_hot_end = .;
    ___audio2_slave_stack_bottom = .; . += 8192; ___audio2_slave_stack_top = .;
  }
  ___end = ___audio2_slave_stack_top;
  ASSERT(___end + 4096 <= 0x06100000, "Audio reserve leaves no bounded user heap")
'''
    if royal_vdp1_hot:
        reserve=reserve.replace(
            '    ___audio2_hot_start = .; . += 18432; ___audio2_hot_end = .;\n', '')
        reserve+='''  ___audio2_hot_start = 0x05c78020;
  ___audio2_hot_end = 0x05c7c820;
'''
    linker=linker.replace(old,reserve)
    (out/'master.ld').write_text(linker)
    specs=(a.sdk/'sh2eb-elf/lib/yaul.specs').read_text().replace('-T ldscripts/yaul.x','-T '+str(out/'master.ld'))
    (out/'master.specs').write_text(specs)
    mk=(runtime/('Makefile.wp1980' if royal else 'Makefile.wp1979')).read_text()
    old_sources='SH_SRCS+= src/wf_r18_scsp.c ../audio/native/wf_native_host.c ../audio/native/wf_native_queue.c ../audio/native/wf_native_elf.c ../audio/native/wf_native_yaul.c ../audio/native/wf_sfx_upload.c ../audio/native/wf_music_elf.c'
    assert mk.count(old_sources)==1
    master_sources=[adapter/'wf_audio2_runtime_adapter.c',adapter/'wf_audio2_game_queue.c']+[src/n for n in (
        'wf_audio2.c','wf_native_host.c','wf_native_elf.c','wf_native_yaul.c','wf_scsp_sfx.c','wf_music_elf.c')]
    mk=mk.replace(old_sources,'SH_SRCS+= '+' '.join(map(str,master_sources)))
    mk=mk.replace('SH_PROGRAM:= wrestlefest-'+('wp1980' if royal else 'wp1979'),'SH_PROGRAM:= '+program)
    mk=mk.replace('SH_BUILD_DIR:= audit/'+('wp1980' if royal else 'runtime18')+'-build','SH_BUILD_DIR:= audit/audio2-build')
    mk=mk.replace('SH_OUTPUT_DIR:= audit/'+('wp1980' if royal else 'runtime18')+'-disc','SH_OUTPUT_DIR:= audit/audio2-disc')
    mk=mk.replace('___slave_stack=0x060FF000','___slave_stack=___audio2_slave_stack_top')
    # ip.bin.mk appends another --defsym from this setting even for ELF-only
    # builds; it must agree with the explicit linker-owned stack symbol.
    mk=mk.replace('IP_SLAVE_STACK_ADDR:= 0x060FF000','IP_SLAVE_STACK_ADDR:= ___audio2_slave_stack_top')
    mk=mk.replace('SH_CFLAGS+= -O2 -Iinclude',f'SH_CFLAGS+= -O2 -I{audio}/include -I{src} -I{adapter} -Iinclude')
    if royal:
        # Preserve HW1's VDP1 texture pool and legacy music-disable contract.
        # The new audio module owns a distinct VDP2 bank1 gap, not MUSIC.ELF.
        profile_flags='-DWF_AUDIO2_REGION_VDP2=1 '
        if royal_vdp1_hot:profile_flags+='-DWF_AUDIO2_WORKSPACE_VDP1=1 '
        mk=mk.replace('SH_CFLAGS+= -O2 ', 'SH_CFLAGS+= '+profile_flags+'-O2 ')
    mk=mk.replace('include $(YAUL_INSTALL_ROOT)/share/build.post.iso-cue.mk',
        f'SH_SPECS:= {out}/master.specs yaul-main.specs\ninclude $(YAUL_INSTALL_ROOT)/share/build.post.iso-cue.mk')
    (runtime/'Makefile.audio2').write_text(mk)
    master=runtime/'audit/audio2-build'/(program+'.elf')
    print('Building unchanged gameplay with the isolated audio adapter (-j1)',flush=True)
    scenario_options=(['WP1980_AUDIO=1','WP1980_CART='+str(a.royal_cart),'WP1980_DISABLE_MUSIC=1',
                       'R18_ENTRY=src/wp1980_combat_entry.c'] if royal else
                      ['WP1979_AUDIO=1','R18_ENTRY=src/wp1979_entry.c'])
    run(['make','-f','Makefile.audio2','-j1',str(master),*scenario_options,
         'R18_PROFILE_START=','R18_PROFILE_STOP=','R18_DIAG_RATE='],runtime)
    syms=symbols(nm,master);(out/'master-symbols.json').write_text(json.dumps(syms,indent=2)+'\n')
    # Observer offsets come from the actual game types and target ABI, never
    # from a host compiler or guessed offsets. This object is not game code.
    run([cc,'-m2','-mb','-std=c11','-ffreestanding','-I',runtime/'include',
         '-c',here/'game_layout.c','-o',out/'game-layout.o'])
    run([objcopy,'-j','.rodata','-O','binary',out/'game-layout.o',out/'game-layout.bin'])
    hot=syms['___audio2_hot_start'];hot_end=syms['___audio2_hot_end'];stack=syms['___audio2_slave_stack_top']
    stack_bottom=syms['___audio2_slave_stack_bottom']
    assert hot_end-hot==18432 and stack-stack_bottom==8192 and syms['___end']==stack<0x06100000
    assert syms['___slave_stack']==stack and syms['___bss_end']<=stack_bottom
    if royal_vdp1_hot:assert hot==0x05c78020 and hot_end==0x05c7c820
    else:assert stack_bottom==hot_end and syms['___bss_end']<=hot
    print(f'Master-owned hot {hot:#010x}..{hot_end:#010x}, slave stack to {stack:#010x}',flush=True)
    module=out/'module';module.mkdir(exist_ok=a.resume);(module/'include').mkdir(exist_ok=a.resume)
    (module/'include/stdio.h').write_text('#include <stddef.h>\ntypedef struct FILE FILE; extern FILE *stderr; int fprintf(FILE *,const char *,...); int printf(const char *,...);\n')
    shutil.copyfile(here/'wf_audio2_module_memory.c',module/'wf_audio2_module_memory.c')
    sound=(a.resources/'SOUND.ROM').read_bytes()
    if len(sound)!=65536 or hashlib.sha1(sound).hexdigest()!='30073963e965250d94f0dc3bd261a054850adf95':
        raise ValueError('Sound ROM differs from the supplied WrestleFest ROM authority')
    (module/'SOUND.ROM').write_bytes(sound[:0xc000])
    (module/'module-data.S').write_text('''.section .wf_music_header,"a"
.long 0x57464d53
.word 4,32
.long 0x05c5c000,0x05c80000,_wf_music_module_prepare,_wf_music_module_run,0x25c7f000,_wf_hotcode_start
.section .rodata.music_rom,"a"
.global _wf_music_sound_rom
_wf_music_sound_rom:
.incbin "SOUND.ROM"
''')
    (module/'module.ld').write_text('''OUTPUT_FORMAT("elf32-sh")
OUTPUT_ARCH(sh)
ENTRY(_wf_music_module_prepare)
PHDRS {program PT_LOAD FLAGS(5); state PT_LOAD FLAGS(6); shared PT_LOAD FLAGS(6); hot PT_LOAD FLAGS(7);}
SECTIONS {
 .header 0x05c5c000 : {KEEP(*(.wf_music_header))} :program
 .text : {*(.text .text.*)} :program
 .rodata : {*(.rodata .rodata.*) *(.wf_fm_tables) . = ALIGN(4);} :program
 ASSERT(. <= 0x05c7c000,"Audio cold code/ROM exceeds the existing VDP1 audio window")
 .wf_hotcode HOT_START : {
  _wf_hotcode_start = .; _wf_hotcode_load = .;
  *(.wf_hotcode .wf_hotcode.*) *(.wf_hotdata_const .wf_hotdata_const.*)
  . = ALIGN(4); _wf_hotcode_end = .;
 } :hot
 .wf_hotstate (NOLOAD) : {. += 3220;} :hot
 ASSERT(. <= HOT_END,"Audio hot code/state exceeds the owned 18KiB window")
 .bss 0x05c7c000 (NOLOAD) : {*(.bss .bss.* COMMON) . = ALIGN(4);} :state
 ASSERT(. <= 0x05c7f000,"Audio state exceeds the existing private12KiB window")
 .shared 0x05c7f000 (NOLOAD) : {. += 4096;} :shared
 /DISCARD/ : {*(.comment) *(.note*) *(.eh_frame*)}
}
'''.replace('HOT_START',hex(hot)).replace('HOT_END',hex(hot_end)))
    if royal:
        replacements={'0x05c5c000':hex(memory_profile['base']),
                      '0x05c7c000':hex(memory_profile['state']),
                      '0x05c7f000':hex(memory_profile['shared']),
                      '0x25c7f000':hex(memory_profile['shared']|0x20000000),
                      '0x05c80000':hex(memory_profile['end'])}
        for name in ('module-data.S','module.ld'):
            data=(module/name).read_text()
            for old,new in replacements.items():data=data.replace(old,new)
            (module/name).write_text(data.replace('existing VDP1 audio window','owned video RAM window'))
    flags=['-m2','-mb','-std=c11','-ffreestanding','-fno-builtin','-ffunction-sections','-fdata-sections','-fstack-usage',
           '-Wall','-Wextra','-Werror','-DWF_MUSIC_NATIVE_CONTROL=1','-I',module/'include','-I',src,'-I',src/'reference_driver']
    if royal:flags+=['-DWF_AUDIO2_REGION_VDP2=1']
    if royal_vdp1_hot:flags+=['-DWF_AUDIO2_WORKSPACE_VDP1=1']
    sources=[src/n for n in ['wf_music_module.c','wf_music_seq.c','wf_scsp_ym.c','reference_driver/z80-native.c','reference_driver/z80-cold.c']]+[module/'wf_audio2_module_memory.c']
    objects=[]
    for f in sources:
        obj=module/(f.stem+'.o');objects.append(obj)
        # These generated driver splits retain unused declarations by design;
        # use the same warning policy as the pinned production build.
        options=[x for x in flags if x!='-Werror'] if f.parent.name=='reference_driver' else flags
        run([cc,*options,'-O2' if f.stem == 'z80-native' else '-Os','-c',f,'-o',obj],module)
    for f in (src/'wf_music_entry.S',module/'module-data.S'):
        obj=module/(f.stem+'.o');objects.append(obj);run([cc,'-m2','-mb','-c',f,'-o',obj],module)
    debug=module/'MUSIC4.DEBUG.ELF'
    run([ld,'--gc-sections','-static','-T',module/'module.ld','-Map',module/'module.map',*objects,
         a.sdk/'lib/gcc/sh2eb-elf/13.2.0/libgcc.a','-o',debug],module)
    if run([nm,'-u',debug]).strip():raise ValueError('Undefined audio module symbols')
    spec=importlib.util.spec_from_file_location('compact_music_elf',here/'compact_music_elf.py')
    compact=importlib.util.module_from_spec(spec);spec.loader.exec_module(compact)
    packed=compact.compact(debug.read_bytes());(cd/'MUSIC4.ELF').write_bytes(packed)
    ms=symbols(nm,debug);(out/'module-symbols.json').write_text(json.dumps(ms,indent=2)+'\n')
    assert ms['_wf_hotcode_start']==hot and ms['_wf_hotcode_end']+3220<=hot_end
    headers=run([a.sdk/'bin/sh2eb-elf-readelf','-h','-l','-S',debug]);(out/'module-layout.txt').write_text(headers)
    original_elf=master.read_bytes()
    control_elf,patch_offset=patch_data_word(original_elf,syms['_wf_audio2_enabled'],1,0)
    assert sum(x!=y for x,y in zip(original_elf,control_elf))==1
    report={'scope':'isolated native '+a.scenario.upper()+' diagnostic, NOT AUDIO ACCEPTED',
            'scenario':a.scenario,'artifact_basename':artifact,
            'music_memory_profile':memory_profile,
            'game_input_scope':('Unchanged HW1 ordinary-input policy and ROM F94 initial state' if royal else 'Unchanged WP1979 logical-frame input tape'),
            'expected_gameplay_endpoint':8474 if royal else 7372,
            'expansion_ram_required':bool(a.royal_cart),
            'emulator_cartridge':'extram4' if a.royal_cart else 'none',
            'hot_start':hot,'hot_end':hot_end,
            'hot_memory':'VDP1 audited tail (experimental)' if royal_vdp1_hot else 'HWRAM',
            'hot_code_const_bytes':ms['_wf_hotcode_end']-hot,'hot_state_bytes':3220,
            'slave_stack_bottom':stack_bottom,'slave_stack_top':stack,'user_heap_start':syms['___end'],
            'user_heap_bytes':0x06100000-syms['___end'],'module_bytes':len(packed),
            'control_elf_difference_bytes':1,'control_elf_difference_offset':patch_offset,
            'control_change':'wf_audio2_enabled data word 1 to 0; all executable bytes identical',
            'firmware_sha256':sha(a.firmware),'pairs':{}}
    for profile,data in [('audio2',original_elf),('control',control_elf)]:
        dest=out/'profiles'/profile;dest.mkdir(parents=True,exist_ok=a.resume);elf=dest/(artifact+'.ELF');elf.write_bytes(data)
        run([objcopy,'-O','binary',elf,dest/(artifact+'.bin')])
        run([a.sdk/'bin/make-ip',dest/(artifact+'.bin'),'V1.981' if royal else 'V1.979','20261029','JTUBKAEL','J',
             'WWF WrestleFest Audio2 Royal' if royal else 'WWF WrestleFest WP1979 TAG','0x06004000',hex(stack),'0x06004000','-1'],dest)
        image=dest/'cd';shutil.copytree(cd,image,dirs_exist_ok=a.resume)
        shutil.copyfile(dest/(artifact+'.bin'),image/'A.BIN')
        run([a.sdk/'bin/make-iso',image,dest/'IP.BIN',dest,artifact])
        (dest/(artifact+'.cue')).write_text('FILE "'+artifact+'.iso" BINARY\n  TRACK 01 MODE1/2048\n    INDEX 01 00:00:00\n')
        report['pairs'][profile]={artifact+ext:sha(dest/(artifact+ext)) for ext in ('.ELF','.bin','.iso','.cue')}
    # Every archive source is identical, including the entry and renderer.
    changed=[p for p,h in original.items() if sha(game/p)!=h]
    if changed:raise ValueError('Original copied source changed: '+str(changed))
    report['original_game_files_unchanged']=len(original)
    graph_names=[p.name for p in cd.iterdir() if p.is_file() and p.name not in ('A.BIN','MUSIC4.ELF','SCSP.ELF','SFX.DAT')]
    report['graphics_assets_unchanged']=all(sha(cd/n)==sha(source_cd/n) for n in graph_names)
    (out/'integration-build.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--scenario',choices=('tag','royal'),default='tag',
                   help='Royal requires the pristine WP1980 HW1 work/hwfix01/native source, with its staged assets')
    p.add_argument('--royal-cart',type=int,choices=(0,1),default=0,
                   help='Select the existing HW1 graphics variant; 1 requires 4MiB and does not certify the base-memory acceptance gate')
    p.add_argument('--resume',action='store_true',help='Resume an unaccepted private build; verifies all copied game sources first')
    for n in ('game-source','audio2','sdk','resources','firmware','out'):p.add_argument('--'+n,type=Path,required=True)
    build(p.parse_args())
