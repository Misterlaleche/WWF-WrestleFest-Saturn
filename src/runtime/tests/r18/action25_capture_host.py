"""Freeze natural Royal3209 from exact nativeF8C HOST exploration, unchanged."""
from pathlib import Path
import hashlib,json,struct
root=Path(__file__).resolve().parents[4];here=Path(__file__).resolve().parent
source=root/'validation/local-20261024/full-flow-combat-native-f8c-royal-after24d5e-basic';snapshot=source/'snapshot-01266.bin';b=snapshot.read_bytes();report=json.loads((source/'report.json').read_text())
assert len(b)==16472 and struct.unpack_from('=6I',b)==(3209,8,1,0x1c03e,1266,1)
actor=b[56+0x5b0+8*0x10c:56+0x5b0+9*0x10c];assert int.from_bytes(actor[0x60:0x62],'big')==0x25
target=here/'action25_snapshot_native_f8c_royal_3209.bin';assert not target.exists()or target.read_bytes()==b;target.write_bytes(b)
m={'rom_sha1':'a726c46f350723ea73158bc78055d17d418b14e2','snapshot_sha256':hashlib.sha256(b).hexdigest(),'header':list(struct.unpack_from('=6I',b)),'source':str(snapshot.relative_to(root)),'source_report_sha256':hashlib.sha256((source/'report.json').read_bytes()).hexdigest(),'source_manifest_sha256':hashlib.sha256((source/'source-manifest.json').read_bytes()).hexdigest(),'captured_native_handoff':report['captured_native_handoff'],'provenance':'Exact Beetle stage3 nativeF8C Royal RAM/Dregs imported into HOST without reseeding, then HOSTbasic reaches3209 slot8 raw25. Literal pre-dispatch snapshot copies all9 actors/liveD8 and supported globals; unrepresented arcadeRAMzero. No claim that later HOST/SH2 trajectories or physical input latency match.'}
(here/'action25_manifest.json').write_text(json.dumps(m,indent=2)+'\n');print(json.dumps(m))
