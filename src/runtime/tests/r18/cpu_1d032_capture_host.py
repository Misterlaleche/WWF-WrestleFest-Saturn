"""Freeze natural CPU1D032 pressure3674 snapshot, without derived state."""
from pathlib import Path
import hashlib,json,struct
root=Path(__file__).resolve().parents[4];here=Path(__file__).resolve().parent
source=root/'validation/local-20261024/tag-winner-pressure-after1d6ee';snapshot=source/'snapshot-01102.bin';b=snapshot.read_bytes()
assert len(b)==16472 and struct.unpack_from('=6I',b)==(3674,5,0,0x1c8f4,1102,1)
target=here/'cpu_1d032_snapshot_pressure3674.bin';assert not target.exists()or target.read_bytes()==b;target.write_bytes(b)
meta={'rom_sha1':'a726c46f350723ea73158bc78055d17d418b14e2','snapshot_sha256':hashlib.sha256(b).hexdigest(),'header':list(struct.unpack_from('=6I',b)),'source':str(snapshot.relative_to(root)),'source_report_sha256':hashlib.sha256((source/'report.json').read_bytes()).hexdigest(),'source_manifest_sha256':hashlib.sha256((source/'source-manifest.json').read_bytes()).hexdigest(),'provenance':'Natural HOST fullfrontend pressure3674 actor5/ID3 AE0 mode02; no result/health/seed/input/register forcing. Snapshot retains actors/fixed/liveD8/represented globals; unrepresented RAMzero. No HOST/SH2 trajectory identity claim.'}
(here/'cpu_1d032_manifest.json').write_text(json.dumps(meta,indent=2)+'\n');print(json.dumps(meta))
