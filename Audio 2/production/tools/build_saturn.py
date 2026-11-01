#!/usr/bin/env python3
"""Build an isolated real-Saturn diagnostic; no legacy ABI ELF is loaded."""
from pathlib import Path
import argparse, hashlib, json, os, shutil, struct, subprocess

def build(package,out,resources,firmware,sdk,command,seconds,replay=None):
    package,out,resources,firmware,sdk=map(lambda x:Path(x).resolve(),(package,out,resources,firmware,sdk))
    # Reject aliases before writing: an in-place build would truncate the
    # complete source sound ROM to the module's 48KiB executable window.
    inputs=[resources/'SOUND.ROM',resources/'SFX.DAT',firmware]
    outputs=[out/'SOUND.ROM',out/'SFX.DAT',out/'SCSP.ELF']
    if any(dst.resolve()==source.resolve() or
           (dst.exists() and source.exists() and os.path.samefile(dst,source))
           for dst in outputs for source in inputs):
        raise ValueError('Build output must not alias any input resource or firmware')
    out.mkdir(parents=True,exist_ok=True);src=package/'src';log=[]
    replay_identity=None
    if replay is not None:
        replay=Path(replay).resolve();plan=json.loads(replay.read_text())
        events=plan['events']
        if not events or events[0]['sample']!=0 or any(
                not isinstance(e['sample'],int) or e['sample']<0 or
                not isinstance(e['word'],int) or not 0<=e['word']<=65535 or
                (e['word']&255)>0x81 or (i and e['sample']<events[i-1]['sample'])
                for i,e in enumerate(events)):
            raise ValueError('Invalid diagnostic replay schedule')
        if events[-1]['sample']>=seconds*44100:raise ValueError('Replay exceeds capture window')
        (out/'audio2_replay.h').write_text(
            '#define AUDIO2_REPLAY_COUNT '+str(len(events))+'u\n'+
            'static const struct {uint32_t sample;uint16_t word;} audio2_replay[]={\n'+
            ''.join('{%du,0x%04x},\n'%(e['sample'],e['word']) for e in events)+'};\n')
        replay_identity={'sha256':hashlib.sha256(replay.read_bytes()).hexdigest(),
                         'events':len(events),'source':str(replay),'scope':plan.get('scope')}
    cc=sdk/'bin/sh2eb-elf-gcc'
    env=dict(os.environ,YAUL_INSTALL_ROOT=str(sdk),YAUL_ARCH_SH_PREFIX='sh2eb-elf',YAUL_PROG_SH_PREFIX='sh2eb-elf')
    def run(args):
        p=subprocess.run(list(map(str,args)),cwd=out,env=env,text=True,capture_output=True)
        log.append(dict(command=list(map(str,args)),returncode=p.returncode,stdout=p.stdout,stderr=p.stderr))
        (out/'build-commands.json').write_text(json.dumps(log,indent=2)+'\n')
        if p.returncode:raise RuntimeError(p.stderr or p.stdout)
        return p.stdout
    (out/'SOUND.ROM').write_bytes((resources/'SOUND.ROM').read_bytes()[:0xc000])
    shutil.copyfile(resources/'SFX.DAT',out/'SFX.DAT');shutil.copyfile(firmware,out/'SCSP.ELF')
    (out/'assets.S').write_text(''' .section .rodata.audio2_assets,"a"
 .balign 4
 .global _audio2_firmware,_audio2_firmware_end,_audio2_sfx,_audio2_sfx_end,_wf_music_sound_rom
_audio2_firmware: .incbin "SCSP.ELF"
_audio2_firmware_end:
 .balign 4
_audio2_sfx: .incbin "SFX.DAT"
_audio2_sfx_end:
 .balign 4
_wf_music_sound_rom: .incbin "SOUND.ROM"
''')
    link=(sdk/'sh2eb-elf/lib/ldscripts/yaul.x').read_text()
    link=link.replace('  .rodata :','''  .wf_hotcode :
  {
    . = ALIGN(4); _wf_hotcode_start = .; _wf_hotcode_load = .;
    *(.wf_hotcode)
    . = ALIGN(4); _wf_hotcode_end = .;
    . += 3220;
  }
  ASSERT((_wf_hotcode_end-_wf_hotcode_start)+3220 <= 18432, "Audio hot code/state exceeds historical 18KiB contract")
  .rodata :''')
    (out/'audio2.ld').write_text(link)
    # Same Yaul ABI, with a diagnostic-only owned hot section. No historical pins changed.
    specs=(sdk/'sh2eb-elf/lib/yaul.specs').read_text().replace('-T ldscripts/yaul.x','-T '+str(out/'audio2.ld'))
    (out/'audio2.specs').write_text(specs)
    flags=['-m2','-mb','-std=c11','-ffreestanding','-fno-builtin','-ffunction-sections','-fdata-sections',
           '-fstack-usage','-Wall','-Wextra','-I',package/'include','-I',src,'-I',src/'reference_driver',
           '-I',sdk/'sh2eb-elf/include/yaul','-DWF_MUSIC_NATIVE_CONTROL=1']
    sources=[package/'tests/saturn_harness.c',src/'wf_audio2.c',src/'wf_audio2_yaul.c',src/'wf_native_host.c',src/'wf_native_elf.c',src/'wf_native_yaul.c',
             src/'wf_scsp_sfx.c',src/'wf_music_module.c',src/'wf_music_seq.c',src/'wf_scsp_ym.c',
             src/'reference_driver/z80-native.c',src/'reference_driver/z80-cold.c']
    objects=[]
    firmware_symbols=json.loads((firmware.parent/'symbols.json').read_text())
    for f in sources:
        obj=out/(f.stem+'.o');objects.append(obj)
        is_native=f.stem in ['z80-native','wf_music_seq']
        options=[*flags,'-O2' if is_native else '-Os']
        if f.name=='saturn_harness.c':
            options+=['-DAUDIO2_COMMAND='+str(command),'-DAUDIO2_SECONDS='+str(seconds),
                '-DAUDIO2_SFX_ENGINE_ADDR='+str(firmware_symbols['wf_sfx_engine'])]
            if replay is not None:options+=['-DAUDIO2_REPLAY=1','-I',out]
        run([cc,*options,'-c',f,'-o',obj])
    for f in [src/'wf_music_entry.S',out/'assets.S']:
        obj=out/(f.stem+'.o');objects.append(obj);run([cc,'-m2','-mb','-c',f,'-o',obj])
    elf=out/'AUDIO2.ELF'
    run([cc,'-specs='+str(out/'audio2.specs'),'-specs=yaul-main.specs',*objects,
         '-static','-Wl,--gc-sections','-Wl,--defsym=___master_stack=0x06004000',
         '-Wl,--defsym=___slave_stack=0x06001e00','-Wl,-Map,'+str(out/'AUDIO2.map'),'-o',elf])
    run([sdk/'bin/sh2eb-elf-objcopy','-O','binary',elf,out/'AUDIO2.bin'])
    (out/'AUDIO2.headers.txt').write_text(run([sdk/'bin/sh2eb-elf-readelf','-h','-l','-S',elf]))
    syms={}
    for line in run([sdk/'bin/sh2eb-elf-nm','-n',elf]).splitlines():
        s=line.split()
        if len(s)==3:syms[s[2]]=int(s[0],16)
    (out/'symbols.json').write_text(json.dumps(syms,indent=2)+'\n')
    # Official Yaul boot-header generator, then ISO9660 data image only.
    # This SDK emits fixed J/T/U/E area blocks. A lone U in its header points
    # the NA BIOS at the wrong block. The SDK's complete ordered area string
    # was isolated by a region-only real-Mednafen boot experiment.
    run([sdk/'bin/make-ip',out/'AUDIO2.bin','V0.001','20261101','JTUBKAEL','J','AUDIO 2 DIAGNOSTIC',
         '0x06004000','0x06001E00','0x06004000','-1'])
    ip=(out/'IP.BIN').read_bytes()
    if ip[0x60:0xd0].rstrip()!=b'AUDIO 2 DIAGNOSTIC' or ip[0x40:0x50].rstrip()!=b'JTUBKAEL':
        raise ValueError('Boot title/area metadata differs from the validated SDK profile')
    if struct.unpack_from('>II',ip,0xf0)!=(0x06004000,max(0x20000,(out/'AUDIO2.bin').stat().st_size)):
        raise ValueError('BIOS first-read address/size does not match the executable')
    cd=out/'cd';cd.mkdir(exist_ok=True);shutil.copyfile(out/'AUDIO2.bin',cd/'A.BIN')
    for n in ('ABS','BIB','CPY'):(cd/(n+'.TXT')).write_text('Audio 2 private diagnostic\n')
    run([sdk/'bin/make-iso',cd,out/'IP.BIN',out,'AUDIO2'])
    (out/'AUDIO2.cue').write_text('FILE "AUDIO2.iso" BINARY\n  TRACK 01 MODE1/2048\n    INDEX 01 00:00:00\n')
    report=dict(profile='isolated-static-HWRAM',music_abi='static link, not ABI3/ABI4 ELF interchange',
                command=command,seconds=seconds,hot_bytes=syms['_wf_hotcode_end']-syms['_wf_hotcode_start'],
                diag_address=syms['_audio2_diag'],replay=replay_identity,hardware_status='HARDWARE PENDING',
                files={n:dict(bytes=(out/n).stat().st_size,sha256=hashlib.sha256((out/n).read_bytes()).hexdigest()) for n in ['AUDIO2.ELF','AUDIO2.map','AUDIO2.bin','AUDIO2.iso','AUDIO2.cue']})
    (out/'saturn-build.json').write_text(json.dumps(report,indent=2)+'\n');return report

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--package',type=Path,default=Path(__file__).resolve().parents[1])
    for key in ['out','resources','firmware','sdk']:p.add_argument('--'+key,type=Path,required=True)
    p.add_argument('--command',type=lambda x:int(x,0),default=1);p.add_argument('--seconds',type=int,default=35)
    p.add_argument('--replay',type=Path)
    a=p.parse_args();print(json.dumps(build(a.package,a.out,a.resources,a.firmware,a.sdk,a.command,a.seconds,a.replay),indent=2))
