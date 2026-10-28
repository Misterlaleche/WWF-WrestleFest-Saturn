"""Freeze the observed TAG pressure2800 raw47 guard; no state forcing."""
from pathlib import Path
import hashlib,json,struct,capstone
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent
BASE=ROOT.parents[1];source=BASE/'validation/local-20261024/tag-winner-pressure'
report=json.loads((source/'report.json').read_bytes());assert report['profile']=='pressure'and not report['royal']
assert 'frame=2800 runtime_frame=2800'in report['first_block']
row=(source/'trace.csv').read_text().splitlines()[-1].split(',');snapshot=source/row[-1];b=snapshot.read_bytes()
assert len(b)==16472;header=struct.unpack_from('=6I',b);assert header[:4]==(2800,1,1,0x1c03e)and header[-1]==1
actor=b[56+0x5b0+0x10c:56+0x5b0+2*0x10c];assert int.from_bytes(actor[0x60:0x62],'big')==0x4047 and actor[3]==7
target=HERE/'action47_snapshot_tag_pressure_2800.bin';assert not target.exists()or target.read_bytes()==b;target.write_bytes(b)
meta={'rom_sha1':'a726c46f350723ea73158bc78055d17d418b14e2','snapshot_sha256':hashlib.sha256(b).hexdigest(),'snapshot_bytes':len(b),'header':list(header),'source':str(snapshot.relative_to(BASE)),'source_report_sha256':hashlib.sha256((source/'report.json').read_bytes()).hexdigest(),'source_manifest_sha256':hashlib.sha256((source/'source-manifest.json').read_bytes()).hexdigest(),'provenance':'Real manual frontend TAG picks0/7 -> paidF8C; HOST validation-only pressure profile observes positions to produce pad bits, without changing actors/HP/RNG/results. Observed raw47 guard at frame2800/slot1/ID7. Snapshot format metadata24 +preD32 +preRAM8KB +postD32 +postRAM8KB. Only preRAM is input; unrepresented originalRAM zero.','limits':['HOST trajectory, not physical Saturn or native SH2 equivalence','No fixed pick or post-selection reseeding beyond normal frontend harness input','Snapshot post section is not a continuation of the preRAM buffer']}
(HERE/'action47_manifest.json').write_text(json.dumps(meta,indent=2)+'\n')
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()==meta['rom_sha1']
assert int.from_bytes(rom[0x12730:0x12734],'big')==0x14440
out=BASE/'validation/local-20261024/action47-research';out.mkdir(exist_ok=True)
md=capstone.Cs(capstone.CS_ARCH_M68K,capstone.CS_MODE_BIG_ENDIAN|capstone.CS_MODE_M68K_000)
text=[]
for start,end in((0x14458,0x14658),(0x11152,0x11192),(0x11192,0x111c8),(0x11412,0x11448)):
 text.append('\nOWNER '+hex(start))
 for i in md.disasm(rom[start:end],start):text.append(f'{i.address:06X}: {i.bytes.hex():24s} {i.mnemonic:10s} {i.op_str}')
(out/'original-rom-disassembly.txt').write_text('\n'.join(text)+'\n')
print(json.dumps(meta,indent=2));print('Descriptor',rom[0x14440:0x14458].hex())
