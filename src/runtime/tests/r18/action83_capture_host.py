"""Freeze observed TAG pressure3434 raw83; no state or seed changes."""
from pathlib import Path
import hashlib,json,struct,capstone
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent;BASE=ROOT.parents[1]
source=BASE/'validation/local-20261024/tag-winner-pressure-after47';report=json.loads((source/'report.json').read_bytes())
assert report['profile']=='pressure'and not report['royal']and 'frame=3434 runtime_frame=3434'in report['first_block']
row=(source/'trace.csv').read_text().splitlines()[-1].split(',');snapshot=source/row[-1];b=snapshot.read_bytes()
assert len(b)==16472;header=struct.unpack_from('=6I',b);assert header[:4]==(3434,0,1,0x1c03e)and header[-1]==1
actor=b[56+0x5b0:56+0x5b0+0x10c];assert int.from_bytes(actor[0x60:0x62],'big')==0x83
target=HERE/'action83_snapshot_tag_pressure_3434.bin';assert not target.exists()or target.read_bytes()==b;target.write_bytes(b)
meta={'rom_sha1':'a726c46f350723ea73158bc78055d17d418b14e2','snapshot_sha256':hashlib.sha256(b).hexdigest(),'snapshot_bytes':len(b),'header':list(header),'source':str(snapshot.relative_to(BASE)),'source_report_sha256':hashlib.sha256((source/'report.json').read_bytes()).hexdigest(),'source_manifest_sha256':hashlib.sha256((source/'source-manifest.json').read_bytes()).hexdigest(),'provenance':'Manual frontend TAG0/7 -> paidF8C -> HOST pressure validation pad policy. First raw83 guard frame3434 actor0/id0. No actor/HP/RNG/result rewriting. Snapshot24metadata+32preD+8192preRAM+32postD+8192postRAM; only preRAM used, remaining original RAM zero.','limits':['HOST trajectory, not physical/nativeSH2 timing proof','No later full-match or winner claim']}
(HERE/'action83_manifest.json').write_text(json.dumps(meta,indent=2)+'\n')
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()==meta['rom_sha1']
assert rom[0x12820:0x12824]==bytes.fromhex('0001a8a6')
md=capstone.Cs(capstone.CS_ARCH_M68K,capstone.CS_MODE_BIG_ENDIAN|capstone.CS_MODE_M68K_000)
text='\n'.join(f'{i.address:06X}: {i.bytes.hex():24s} {i.mnemonic:10s} {i.op_str}'for i in md.disasm(rom[0x1a8b6:0x1a940],0x1a8b6))
out=BASE/'validation/local-20261024/action83-research';out.mkdir(exist_ok=True);(out/'original-rom-disassembly.txt').write_text(text+'\n')
print(json.dumps(meta,indent=2));print(text)
