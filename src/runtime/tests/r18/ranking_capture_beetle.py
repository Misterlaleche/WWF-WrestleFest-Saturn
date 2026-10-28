"""Reproduce the actual raw16KB/D8/A7/FG lease at native9b's65C0 guard.

Read-only extraction of an already recorded Beetle run. No emulator, combat,
RAM/input/result write or typed-state projection is performed.
"""
from pathlib import Path
import hashlib,json,re,struct
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent
EVIDENCE=ROOT.parents[1]/'validation/local-20261024'
CAP=EVIDENCE/'hardware-input/beetle-full-flow-stage9b-human-tag'
BUILD=ROOT.parents[1]/'validation/results/runtime18-full-flow-stage9b-20261024'
meta=json.loads((HERE/'ranking_capture_manifest.json').read_text())
report=(CAP/'report.json').read_bytes();j=json.loads(report)
assert(j['status'],j['fatal_owner'],j['fatal_dependency'],j['fatal_frame'])==('blocked',0x19e2,0x65c0,3566)
assert hashlib.sha256(report).hexdigest()==meta['source_report_sha256']
raw=(CAP/'workram-at-block.bin').read_bytes();assert len(raw)==0x200000 and hashlib.sha256(raw).hexdigest()==meta['wram_sha256']
swapped=bytearray(raw);swapped[::2],swapped[1::2]=raw[1::2],raw[::2]
def read(at,size):
 if 0x00200000<=at<=0x00300000-size:offset=at-0x00200000
 else:assert 0x06000000<=at<=0x06100000-size;offset=0x100000+at-0x06000000
 return bytes(swapped[offset:offset+size])
def word(at):return int.from_bytes(read(at,4),'big')
elf=(BUILD/'wrestlefest-runtime18.elf').read_bytes();assert elf[5]==2 and hashlib.sha256(elf).hexdigest()==meta['elf_sha256']
shoff=struct.unpack_from('>I',elf,32)[0];entsz,n,_=struct.unpack_from('>HHH',elf,46)
sections=[struct.unpack_from('>10I',elf,shoff+i*entsz)for i in range(n)];symbols={}
for section in sections:
 if section[1]!=2:continue
 st=sections[section[6]];strings=elf[st[4]:st[4]+st[5]]
 for off in range(section[4],section[4]+section[5],section[9]):
  no,value,size,*_=struct.unpack_from('>IIIBBH',elf,off)
  symbols[strings[no:].split(b'\0',1)[0].decode(errors='replace')]=(value,size)
for name,key in(('A.BIN','program_sha256'),('wrestlefest-runtime18.bin','bin_sha256'),('wrestlefest-runtime18.cue','cue_sha256')):
 assert hashlib.sha256((BUILD/name).read_bytes()).hexdigest()==j[key]
for short in('frontend','frontend_match','video'):assert list(symbols['_'+short])==meta['symbols'][short]
layout_source=(EVIDENCE/'ranking65c0-layout.c').read_bytes();layout_asm=(EVIDENCE/'ranking65c0-layout-sh2.s').read_bytes()
assert hashlib.sha256(layout_source).hexdigest()==meta['layout_source_sha256'] and hashlib.sha256(layout_asm).hexdigest()==meta['layout_sh2_sha256']
layout=[int(x)for x in re.findall(rb'\.long\s+(\d+)',layout_asm)];assert layout==meta['layout']==[356,0,52,88,108,4,8,12]
frozen=(EVIDENCE/'core-stage9b-final-src/include/wf_r18_frontend_driver.h').read_bytes();live=(ROOT/'include/wf_r18_frontend_driver.h').read_bytes()
assert hashlib.sha256(frozen).hexdigest()==meta['frozen_header_sha256']
pattern=rb'struct wf_r18_frontend_driver\s*\{.*?\n\};'
assert re.search(pattern,frozen,re.S)[0]==re.search(pattern,live,re.S)[0]
front=symbols['_frontend'][0];match=symbols['_frontend_match'][0];video=symbols['_video'][0]
assert word(front)==word(match+4)==0x002fc000 and word(match+8)==front+52
D=list(struct.unpack('>8I',read(front+52,32)));A=list(struct.unpack('>7I',read(match+12,28)))
assert read(match+12,28)==read(front+88,28)and D==meta['D']and A==meta['A']
ram=read(0x002fc000,0x4000);fg=read(video+12,0x4000)
assert hashlib.sha256(ram[0x5b0:0x5b0+9*0x10c]).hexdigest()==meta['all9actors_sha256']
assert ram[0x92:0x96]==bytes.fromhex('001c05b0')and ram[0x66:0x68]==bytes.fromhex('0300')
capture=struct.pack('=15I',*(D+A))+ram+fg
assert hashlib.sha256(capture).hexdigest()==meta['snapshot_sha256']
dest=HERE/'ranking_snapshot_stage9b_guard.bin';assert not dest.exists()or dest.read_bytes()==capture;dest.write_bytes(capture)
print(json.dumps({'status':'PASS','snapshot_sha256':meta['snapshot_sha256'],'all9actors_sha256':meta['all9actors_sha256'],'layout':layout,'raw_pointer':'002FC000','D8':D,'A7':A,'read_only_recorded_capture':True}))
