"""Freeze the unmodified natural TAG pressure3747 raw1F observation."""
from pathlib import Path
import hashlib,json,struct,capstone
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent;BASE=ROOT.parents[1]
source=BASE/'validation/local-20261024/tag-winner-pressure-after1d032';report=json.loads((source/'report.json').read_bytes())
assert report['profile']=='pressure'and not report['royal']and 'frame=3747 runtime_frame=3747'in report['first_block']
row=(source/'trace.csv').read_text().splitlines()[-1].split(',');snapshot=source/row[-1];b=snapshot.read_bytes()
assert len(b)==16472;header=struct.unpack_from('=6I',b);assert header[:4]==(3747,5,1,0x1c03e)and header[-1]==1
actor=b[56+0x5b0+5*0x10c:56+0x5b0+6*0x10c];assert int.from_bytes(actor[0x60:0x62],'big')==0x1f
target=HERE/'action1f_snapshot_tag_pressure_3747.bin';assert not target.exists()or target.read_bytes()==b;target.write_bytes(b)
meta={'rom_sha1':'a726c46f350723ea73158bc78055d17d418b14e2','snapshot_sha256':hashlib.sha256(b).hexdigest(),'snapshot_bytes':len(b),'header':list(header),'source':str(snapshot.relative_to(BASE)),'source_report_sha256':hashlib.sha256((source/'report.json').read_bytes()).hexdigest(),'source_manifest_sha256':hashlib.sha256((source/'source-manifest.json').read_bytes()).hexdigest(),'provenance':'Manual frontend TAG0/7 -> paidF8C -> HOST pressure validation pad policy. First raw1F guard frame3747 actor5/id3 HP0 mode02. No actor/HP/RNG/result rewriting. Snapshot24metadata+32preD+8192preRAM+32postD+8192postRAM; only preRAM used, remaining original RAM zero.','limits':['HOST trajectory, not physical/nativeSH2 timing proof','No later full-match or winner claim']}
(HERE/'action1f_manifest.json').write_text(json.dumps(meta,indent=2)+'\n')
alias_source=BASE/'validation/local-20261024/tag-winner-pressure-after1f'
if alias_source.exists():
 alias_report=json.loads((alias_source/'report.json').read_bytes());assert 'frame=4189 runtime_frame=4189'in alias_report['first_block']
 alias_row=(alias_source/'trace.csv').read_text().splitlines()[-1].split(',');alias=alias_source/alias_row[-1];data=alias.read_bytes();h=struct.unpack_from('=6I',data)
 assert len(data)==16472 and h[:4]==(4189,1,1,0x1c03e)and h[-1]==1
 assert int.from_bytes(data[56+0x6bc+0x60:56+0x6bc+0x62],'big')==0x1e
 dest=HERE/'action1f_snapshot_tag_pressure_4189_raw1e.bin';assert not dest.exists()or dest.read_bytes()==data;dest.write_bytes(data)
 meta['alias1e']={'snapshot_sha256':hashlib.sha256(data).hexdigest(),'header':list(h),'source':str(alias.relative_to(BASE)),'source_report_sha256':hashlib.sha256((alias_source/'report.json').read_bytes()).hexdigest(),'source_manifest_sha256':hashlib.sha256((alias_source/'source-manifest.json').read_bytes()).hexdigest(),'provenance':'Same manual frontend/pressure trajectory after raw1F. Natural raw1E frame4189 actor1/id7. Only preRAM8KB projected, no health/RNG/result edits.'}
 (HERE/'action1f_manifest.json').write_text(json.dumps(meta,indent=2)+'\n')
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()==meta['rom_sha1']
assert rom[0x12690:0x12694]==bytes.fromhex('000159f8')
md=capstone.Cs(capstone.CS_ARCH_M68K,capstone.CS_MODE_BIG_ENDIAN|capstone.CS_MODE_M68K_000)
text='\n'.join(f'{i.address:06X}: {i.bytes.hex():24s} {i.mnemonic:10s} {i.op_str}'for i in md.disasm(rom[0x15a20:0x15b24],0x15a20))
out=BASE/'validation/local-20261024/action1f-research';out.mkdir(exist_ok=True);(out/'original-rom-disassembly.txt').write_text(text+'\n')
print(json.dumps(meta,indent=2));print(text)
