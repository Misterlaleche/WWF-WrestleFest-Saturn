"""Read-only Stage4 trace diagnosis; SH2 offsetof data from frozen headers."""
from pathlib import Path
import hashlib,json,struct,ctypes,subprocess,tempfile,os,re
import capstone
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent
BASE=ROOT.parents[1];EVIDENCE=BASE/'validation/local-20261024'
CAP=EVIDENCE/'hardware-input/beetle-full-flow-stage4-royal-basic'
BUILD=BASE/'validation/results/runtime18-full-flow-stage4-20261024'
OUT=EVIDENCE/'action04-research';OUT.mkdir(exist_ok=True)
j=json.loads((CAP/'report.json').read_bytes());raw=(CAP/'workram-at-block.bin').read_bytes()
assert(j['status'],j['fatal_owner'],j['fatal_dependency'],j['fatal_frame'])==('blocked',0xf4c2,0x12614,1840)
assert len(raw)==0x200000
d=bytearray(raw);d[::2],d[1::2]=raw[1::2],raw[::2]
elf=(BUILD/'wrestlefest-runtime18.elf').read_bytes();assert elf[5]==2
shoff=struct.unpack_from('>I',elf,32)[0];entsz,n,idx=struct.unpack_from('>HHH',elf,46)
sections=[struct.unpack_from('>10I',elf,shoff+i*entsz)for i in range(n)];symbols={}
for s in sections:
 if s[1]!=2:continue
 st=sections[s[6]];strings=elf[st[4]:st[4]+st[5]]
 for off in range(s[4],s[4]+s[5],s[9]):
  no,value,size,info,other,which=struct.unpack_from('>IIIBBH',elf,off)
  name=strings[no:].split(b'\0',1)[0].decode(errors='replace');symbols[name]=(value,size)
assert hashlib.sha256((BUILD/'A.BIN').read_bytes()).hexdigest()==j['program_sha256']
assert hashlib.sha256((BUILD/'wrestlefest-runtime18.bin').read_bytes()).hexdigest()==j['bin_sha256']
# SH2 GCC13.2 -m2 -mb -ffreestanding, frozen /home/builder/asan-stage4 headers.
layout=[4600,1144,4116,20,12,16,384,352,24,7340,7648]
assembly=(OUT/'layout-sh2.s').read_text(encoding='utf-8-sig')
sh2_constants=[int(value)for value in re.findall(r'^\s*\.long\s+(\d+)',assembly,re.M)]
assert sh2_constants[:11]==layout
gcname=next(name for name in symbols if name=='_gameplay' or name.startswith('_gameplay.'))
rtname=next(name for name in symbols if name=='_rt' or name.startswith('_rt.'))
gcaddr,gcsize=symbols[gcname];rtaddr,rtsize=symbols[rtname]
assert(gcsize,rtsize)==(layout[0],layout[10])
at=0x100000+gcaddr-0x06000000;rtat=0x100000+rtaddr-0x06000000
actors=b''.join(bytes.fromhex(s)for s in j['actors_at_block'])
assert d[rtat+layout[6]:rtat+layout[6]+len(actors)]==actors
assert int.from_bytes(d[rtat+24:rtat+28],'big')==1840
tr=[]
for slot in range(9):
 p=at+layout[2]+slot*layout[3];cb,desc,tick,dep=struct.unpack_from('>4I',d,p)
 tr.append(dict(slot=slot,callback=cb,descriptor=desc,tick_descriptor=tick,dependency=dep,selector=d[p+layout[5]]))
assert [(x['slot'],x['selector'])for x in tr if x['dependency']==0x12614]==[(6,4)]
assert tr[5]['selector']==0x3d and tr[5]['dependency']==0
report={'status':'CONFIRMED','fatal_frame':1840,'blocked_slot':6,'blocked_raw':4,
 'trace':tr,'sh2_layout':layout,'gameplay_elf_symbol':[gcaddr,gcsize],
 'runtime_elf_symbol':[rtaddr,rtsize],'gameplay_wram_offset':at,'runtime_wram_offset':rtat,
 'report_sha256':hashlib.sha256((CAP/'report.json').read_bytes()).hexdigest(),
 'workram_sha256':hashlib.sha256(raw).hexdigest(),'program_sha256':j['program_sha256'],
 'elf_sha256':hashlib.sha256(elf).hexdigest(),'layout_source_sha256':hashlib.sha256((HERE/'action04_layout.c').read_bytes()).hexdigest(),'layout_assembly_sha256':hashlib.sha256((OUT/'layout-sh2.s').read_bytes()).hexdigest(),
 'layout_command':'sh2eb-elf-gcc -m2 -mb -ffreestanding -I/home/builder/asan-stage4/src/runtime/include -S /tmp/wf-action04-layout-20261024.c -o -',
 'limits':['Beetle emulation capture, not physical Saturn','No state writes or ROM mutation','Stage4 SH2 symbols and ABI are pinned; not inferred from HOST pointer widths']}
(OUT/'stage4-trace-diagnosis.json').write_text(json.dumps(report,indent=2)+'\n')
assert (ROOT/'include/wf_runtime.h').read_bytes()==(EVIDENCE/'core-stage4-src/src/runtime/include/wf_runtime.h').read_bytes()
with tempfile.TemporaryDirectory(prefix='wf-raw04-layout-',ignore_cleanup_errors=os.name=='nt')as td:
 lib=Path(td)/'layout.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Iinclude','tests/r18/action04_layout.c','-o',str(lib)],cwd=ROOT,check=True)
 native=ctypes.CDLL(str(lib));glob=list((ctypes.c_uint32*8).in_dll(native,'wf_action04_globals'))
assert glob==sh2_constants[11:],'HOST scalar prefix differs from SH2'
ram=bytearray(0x4000);ram[0x5b0:0x5b0+len(actors)]=actors
for dest,off,nbytes in [(0x5c,352,4),(0x80,24,4)]+list(zip((0x161,0x7c,0x7e,0x1670,0x1150,0x1180,0x11b0,0x11e0),glob,(1,2,2,4,2,2,2,2))):ram[dest:dest+nbytes]=d[rtat+off:rtat+off+nbytes]
regs=j['shadow_d_at_block'];assert bytes(d[rtat+7340:rtat+7372])==struct.pack('>8I',*regs)
capture=struct.pack('=6I',1840,6,1,0x1c03e,0,1)+struct.pack('=8I',*regs)+ram
path=HERE/'action04_snapshot_beetle_frame1840.bin';assert not path.exists()or path.read_bytes()==capture;path.write_bytes(capture)
meta={**report,'rom_sha1':'a726c46f350723ea73158bc78055d17d418b14e2','snapshot_sha256':hashlib.sha256(capture).hexdigest(),'snapshot_bytes':len(capture),'header':[1840,6,1,0x1c03e,0,1],'global_layout_offsets':glob,'projection_limits':['Typed native model projected to supported arcade globals and all9actors; unrepresented original RAM zero','No state/seed/pick forcing or timing-equivalence claim']}
(HERE/'action04_manifest.json').write_text(json.dumps(meta,indent=2)+'\n')
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
md=capstone.Cs(capstone.CS_ARCH_M68K,capstone.CS_MODE_BIG_ENDIAN|capstone.CS_MODE_M68K_000)
lines=[]
for start,end in((0x12c22,0x12f08),(0x110e0,0x1110e),(0x1108c,0x110e0),(0x10b9a,0x10bd0),(0x258e,0x25c8)):
 lines.append('\nOWNER '+hex(start))
 for i in md.disasm(rom[start:end],start):lines.append(f'{i.address:06X}: {i.bytes.hex():24s} {i.mnemonic:10s} {i.op_str}')
(OUT/'original-rom-disassembly.txt').write_text('\n'.join(lines)+'\n')
print(json.dumps(report,indent=2))
print('Descriptors',rom[0x12bda:0x12c22].hex())
