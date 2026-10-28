"""Freeze natural HOST pressure frame3632 CPU5 before1D6EE."""
from pathlib import Path
import hashlib,json,struct
root=Path(__file__).resolve().parents[4];here=Path(__file__).resolve().parent
source=root/'validation/local-20261024/tag-winner-pressure-after83';path=source/'snapshot-01085.bin';b=path.read_bytes()
assert len(b)==16472 and struct.unpack_from('=6I',b)==(3632,5,0,0x1c8f4,1085,1)
a=b[56+0x5b0+5*0x10c:56+0x5b0+6*0x10c];assert a[0x60:0x62]==bytes.fromhex('0070')and a[0x76:0x7a]==bytes.fromhex('001c0f1c')
target=here/'cpu_1d6ee_snapshot_pressure3632.bin';assert not target.exists()or target.read_bytes()==b;target.write_bytes(b)
meta={'rom_sha1':'a726c46f350723ea73158bc78055d17d418b14e2','snapshot_sha256':hashlib.sha256(b).hexdigest(),'header':list(struct.unpack_from('=6I',b)),'source':str(path.relative_to(root)),'source_report_sha256':hashlib.sha256((source/'report.json').read_bytes()).hexdigest(),'source_manifest_sha256':hashlib.sha256((source/'source-manifest.json').read_bytes()).hexdigest(),'provenance':'Natural HOST full frontend pressure3632, mode02, CPU5/ID3 AE10 raw70; fixed objectF1C. No outcome, health, seed, input or registers changed for the natural fixture. All9 actors/fixed records/liveD8 represented in snapshot; unrepresented arcadeRAMzero. HOST and SH2 trajectories not claimed identical.'}
(here/'cpu_1d6ee_manifest.json').write_text(json.dumps(meta,indent=2)+'\n');print(json.dumps(meta))
