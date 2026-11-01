#!/usr/bin/env python3
"""Build a private same-firmware native68000 decoder A/B probe from local assets."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import zlib


def sha(p):
    return hashlib.sha256(Path(p).read_bytes()).hexdigest()


def build(a):
    here=Path(__file__).resolve().parent
    for key in ('candidate','baseline','production','resources','sdk','template_firmware','runtime_source','out'):
        setattr(a,key,getattr(a,key).resolve())
    a.out.mkdir(parents=True,exist_ok=False);out=a.out
    if sha(a.baseline)!='5e97ad36a59fd374886bf3aad571e231a302a2b5ba413a53cc1d72d7cb04bf48':
        raise ValueError('Baseline decoder differs from the preserved original')
    candidate_identity=sha(a.candidate)
    logs=[];env=dict(os.environ,YAUL_INSTALL_ROOT=str(a.sdk),YAUL_ARCH_SH_PREFIX='sh2eb-elf',YAUL_PROG_SH_PREFIX='sh2eb-elf')
    def run(cmd):
        cmd=list(map(str,cmd));r=subprocess.run(cmd,cwd=out,env=env,capture_output=True,text=True)
        logs.append({'command':cmd,'returncode':r.returncode,'stdout':r.stdout,'stderr':r.stderr})
        (out/'decoder-build-commands.json').write_text(json.dumps(logs,indent=2)+'\n')
        if r.returncode:raise RuntimeError(r.stderr or r.stdout)
        return r.stdout
    def syms(nm,elf):
        return {p[2]:int(p[0],16) for line in run([nm,'-n',elf]).splitlines() if len(p:=line.split())==3}
    src=out/'source';src.mkdir()
    tests=here if (here/'decoder_native.c').is_file() else a.production/'tests'
    for n in ('decoder_native.c','decoder_saturn.c'):
        shutil.copyfile(tests/n,src/n)
    for n in ('wf_native_host.c','wf_native_elf.c','wf_native_yaul.c','wf_native_host.h','wf_native_elf.h',
              'wf_native_yaul.h','wf_native_driver.h','wf_scsp_sfx.h','wf_scsp_dsp.c','wf_scsp_dsp.h','wf_sfx_memory.c'):
        shutil.copyfile(here/n if (here/n).exists() else a.production/'src'/n,src/n)
    for n in ('oki-lut.inc','wf_dsp_program.h','sine.pcm'):
        shutil.copyfile(a.template_firmware/n,out/n)
    pack=(a.resources/'SFX.DAT').read_bytes();gold=(a.resources/'PCM16BE.PAK').read_bytes()
    audit=json.loads((a.production/'tools/reference/oracle_sfx/WF_OKI_AUDIT.json').read_text())
    if hashlib.sha256(gold).hexdigest()!=audit['pak_sha256']:
        raise ValueError('Golden full PCM pack does not match independent pinned audit')
    fixtures=[]
    for entry in audit['samples']:
        bank,sid=entry['bank'],entry['sample_id'];at=32+(bank*74+sid)*8
        offset,encoded=struct.unpack_from('>IH',pack,at)
        if encoded*2!=entry['frames']:raise ValueError('Pack entry frame count mismatch')
        pcm=gold[entry['offset']:entry['offset']+entry['bytes']]
        if hashlib.sha256(pcm).hexdigest()!=entry['sha256_pcm16be']:
            raise ValueError('Golden entry digest mismatch')
        fixtures.append((offset,entry['frames'],zlib.crc32(pcm)))
    crc=[]
    for n in range(256):
        v=n
        for _ in range(8):v=(v>>1)^(0xedb88320 if v&1 else 0)
        crc.append(v)
    (out/'decoder_fixture.h').write_text('typedef struct {uint32_t offset,frames,crc;} decoder_fixture_t;\n'
        +'static const decoder_fixture_t decoder_fixtures[114] __attribute__((section(".decoder_fixture")))={\n'
        +''.join('{%du,%du,0x%08xu},\n'%f for f in fixtures)+'};\n'
        +'static const uint32_t decoder_crc_table[256] __attribute__((section(".decoder_fixture")))={\n'
        +','.join('0x%08xu'%v for v in crc)+'};\n')
    original=a.baseline.read_text();candidate=a.candidate.read_text()
    original=original.replace('wf_sfx_decode_block','decoder_baseline').replace('wf_sfx_lut','decoder_lut')
    original='.global decoder_lut\n'+original
    candidate=candidate.split('.section .rodata.wf_sfx_lut',1)[0]
    candidate=candidate.replace('wf_sfx_decode_block','decoder_candidate').replace('wf_sfx_lut','decoder_lut')
    (src/'decoder_baseline.S').write_text(original);(src/'decoder_candidate.S').write_text(candidate)
    startup=(a.production/'src/wf_sfx_start.S').read_text().replace('wf_sfx_main','decoder_main')
    (src/'decoder_start.S').write_text(startup)
    (src/'decoder_wave.S').write_text('.section .wf_native_wave,"a"\n.balign 2\n.incbin "sine.pcm"\n')
    linker='''OUTPUT_FORMAT("elf32-m68k")
OUTPUT_ARCH(m68k)
ENTRY(wf_m68k_reset)
PHDRS {low PT_LOAD FLAGS(5); workspace PT_LOAD FLAGS(7); wave PT_LOAD FLAGS(4);}
SECTIONS {
 .vectors 0 : {KEEP(*(.vectors))} :low
 .text 0x400 : {*decoder_start.o(.text*) *decoder_baseline.o(.text*) *decoder_candidate.o(.text*) *(.text .text.*)} :low
 .rodata : {*(.rodata .rodata.*) . = ALIGN(2);} :low
 ASSERT(. <= 0x3000,"decoder probe low code overlaps output buffer")
 .high 0xc100 : {LONG(0x44454331); *(.decoder_fixture) . = ALIGN(4);} :workspace
 .bss (NOLOAD) : {__bss_start = .; *(.bss .bss.* COMMON) . = ALIGN(4); __bss_end = .;} :workspace
 ASSERT(. <= 0x10000,"decoder probe data overlaps wave")
 .wave 0x10000 : {KEEP(*(.wf_native_wave))} :wave
 ASSERT(SIZEOF(.wave)==9216,"wave contract")
 __stack_bottom = 0x12800; __stack_top = 0x14000;
 /DISCARD/ : {*(.comment) *(.note*) *(.eh_frame*)}
}
'''
    (out/'decoder_m68k.ld').write_text(linker)
    cc=shutil.which('m68k-linux-gnu-gcc');nm=shutil.which('m68k-linux-gnu-nm')
    if not cc or not nm:raise ValueError('Source tools-env/env.sh before building')
    flags=['-m68000','-msoft-float','-Os','-std=c11','-ffreestanding','-fno-builtin','-fno-pic','-fno-pie',
           '-ffunction-sections','-fdata-sections','-fomit-frame-pointer','-Wall','-Wextra','-Werror',
           '-Wno-misleading-indentation','-I',src,'-I',out]
    flags+=['-DDECODER_BENCH_ONLY='+str(int(a.bench_only))]
    objects=[]
    for n in ('decoder_native.c','wf_scsp_dsp.c','wf_sfx_memory.c','decoder_baseline.S','decoder_candidate.S','decoder_start.S','decoder_wave.S'):
        obj=out/(Path(n).stem+'.o');objects.append(obj)
        run([cc,*(flags if n.endswith('.c') else ['-m68000','-I',out]),'-c',src/n,'-o',obj])
    if sha(a.runtime_source)!='851f082a547137b70dfe686764c9312aed489df0ff00d60e2dd6c6fe64d5ba00':
        raise ValueError('Unexpected baseline68000 runtime source')
    for name in ('mulsi3','udivsi3','divsi3'):
        obj=out/('decoder_'+name+'.o');objects.append(obj)
        run([cc,'-m68000','-msoft-float','-fno-pic','-fno-pie','-DL_'+name,'-c',a.runtime_source,'-o',obj])
    firmware=out/'DECODER.ELF'
    run([cc,'-m68000','-msoft-float','-nostdlib','-no-pie','-Wl,--build-id=none','-Wl,--gc-sections',
         '-Wl,-T,'+str(out/'decoder_m68k.ld'),'-Wl,-Map,'+str(out/'decoder_m68k.map'),*objects,'-o',firmware])
    if run([nm,'-u',firmware]).strip():raise ValueError('Undefined native probe symbols')
    raw=firmware.read_bytes();phoff=struct.unpack_from('>I',raw,28)[0]
    phsize,phnum=struct.unpack_from('>HH',raw,42)
    loads=[struct.unpack_from('>8I',raw,phoff+i*phsize) for i in range(phnum)]
    loads=[p for p in loads if p[0]==1]
    if ([p[2] for p in loads]!=[0,0xc100,0x10000] or loads[0][5]>0x3000 or
            loads[1][5]>0x3f00 or loads[2][4:6]!=(9216,9216)):
        raise ValueError('Probe ELF violates the existing three-segment native loader contract')
    firmware_symbols=syms(nm,firmware);(out/'decoder-firmware-symbols.json').write_text(json.dumps(firmware_symbols,indent=2)+'\n')
    (out/'decoder_m68k.disassembly').write_text(run(['m68k-linux-gnu-objdump','-d','-m','m68k:68000',firmware]))
    shutil.copyfile(a.resources/'SFX.DAT',out/'SFX.DAT')
    (src/'decoder_assets.S').write_text('''.section .rodata.decoder_assets,"a"
.balign 4
.global _decoder_firmware,_decoder_firmware_end,_decoder_pack,_decoder_pack_end
_decoder_firmware: .incbin "DECODER.ELF"
_decoder_firmware_end:
.balign 4
_decoder_pack: .incbin "SFX.DAT"
_decoder_pack_end:
''')
    shcc=a.sdk/'bin/sh2eb-elf-gcc';objects=[]
    flags=['-m2','-mb','-Os','-std=c11','-ffreestanding','-fno-builtin','-ffunction-sections','-fdata-sections',
           '-Wall','-Wextra','-Werror','-I',src,'-I',a.sdk/'sh2eb-elf/include/yaul',
           '-DDECODER_DIAG_ADDRESS='+str(firmware_symbols['decoder_diag'])]
    for n in ('decoder_saturn.c','wf_native_host.c','wf_native_elf.c','wf_native_yaul.c'):
        obj=out/(Path(n).stem+'.sh.o');objects.append(obj);run([shcc,*flags,'-c',src/n,'-o',obj])
    obj=out/'decoder_assets.sh.o';objects.append(obj);run([shcc,'-m2','-mb','-c',src/'decoder_assets.S','-o',obj])
    elf=out/'DECODERSAT.ELF'
    run([shcc,'-specs=yaul.specs','-specs=yaul-main.specs',*objects,'-static','-Wl,--gc-sections',
         '-Wl,--defsym=___master_stack=0x06004000','-Wl,--defsym=___slave_stack=0x06001e00',
         '-Wl,-Map,'+str(out/'DECODERSAT.map'),'-o',elf])
    host_symbols=syms(a.sdk/'bin/sh2eb-elf-nm',elf)
    (out/'decoder-host-symbols.json').write_text(json.dumps(host_symbols,indent=2)+'\n')
    run([a.sdk/'bin/sh2eb-elf-objcopy','-O','binary',elf,out/'DECODERSAT.bin'])
    run([a.sdk/'bin/make-ip',out/'DECODERSAT.bin','V0.001','20261101','JTUBKAEL','J','DECODER NATIVE REGRESSION',
         '0x06004000','0x06001E00','0x06004000','-1'])
    cd=out/'cd';cd.mkdir();shutil.copyfile(out/'DECODERSAT.bin',cd/'A.BIN')
    for n in ('ABS','BIB','CPY'):(cd/(n+'.TXT')).write_text('Private decoder native probe\n')
    run([a.sdk/'bin/make-iso',cd,out/'IP.BIN',out,'DECODERSAT'])
    (out/'DECODERSAT.cue').write_text('FILE "DECODERSAT.iso" BINARY\n  TRACK 01 MODE1/2048\n    INDEX 01 00:00:00\n')
    report={'scope':'native68000 decoder test only; not gameplay/audio acceptance','baseline_sha256':sha(a.baseline),
            'candidate_sha256':sha(a.candidate),'pack_sha256':sha(a.resources/'SFX.DAT'),
            'independent_golden_pcm_sha256':hashlib.sha256(gold).hexdigest(),'sample_entries':114,
            'sample_frames':sum(f[1] for f in fixtures),'firmware_sha256':sha(firmware),
            'iso_sha256':sha(out/'DECODERSAT.iso'),'host_diag':host_symbols['_decoder_host_diag'],
            'host_boot':host_symbols['_decoder_host_boot'],'m68k_diag':firmware_symbols['decoder_diag'],
            'baseline_code':firmware_symbols['decoder_baseline'],'candidate_code':firmware_symbols['decoder_candidate'],
            'shared_lut':firmware_symbols['decoder_lut']}
    report['benchmark_only']=a.bench_only
    report['benchmark_records_address']=firmware_symbols['decoder_bench_records']
    report['source_sha256']={str(p.relative_to(out)):sha(p) for p in src.iterdir() if p.is_file()}
    report['generated_lut_sha256']=sha(out/'oki-lut.inc')
    report['runtime_source_sha256']=sha(a.runtime_source)
    if candidate_identity!=sha(a.candidate):raise ValueError('Candidate changed while building the native probe')
    (out/'decoder-build.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for n in ('candidate','baseline','production','resources','sdk','template-firmware','runtime-source','out'):
        p.add_argument('--'+n,type=Path,required=True)
    p.add_argument('--bench-only',action='store_true',help='Additional timing/PCM comparison only; full114-entry proof is a separate required capture')
    build(p.parse_args())
