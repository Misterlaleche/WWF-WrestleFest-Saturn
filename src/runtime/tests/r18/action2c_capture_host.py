"""Freeze exact pre-dispatch HOST558 raw2C, without actor/register forcing."""
from pathlib import Path
import hashlib,json,struct
root=Path(__file__).resolve().parents[4];here=Path(__file__).resolve().parent
source=root/'validation/local-20261024/post-tag-natural-host';snapshot=source/'snapshot-00180.bin';b=snapshot.read_bytes();report=json.loads((source/'report.json').read_text())
assert len(b)==16472 and struct.unpack_from('=6I',b)==(558,4,1,0x1c03e,180,1)
actor=b[56+0x5b0+4*0x10c:56+0x5b0+5*0x10c];assert int.from_bytes(actor[0x60:0x62],'big')==0x2c
target=here/'action2c_snapshot_native_f8c_tag_558.bin';assert not target.exists()or target.read_bytes()==b;target.write_bytes(b)
meta={'rom_sha1':'a726c46f350723ea73158bc78055d17d418b14e2','snapshot_sha256':hashlib.sha256(b).hexdigest(),'header':list(struct.unpack_from('=6I',b)),'source':str(snapshot.relative_to(root)),'source_report_sha256':hashlib.sha256((source/'report.json').read_bytes()).hexdigest(),'source_manifest_sha256':hashlib.sha256((source/'source-manifest.json').read_bytes()).hexdigest(),'captured_native_handoff':report['captured_native_handoff'],'provenance':'Exact nativeF8C TAG capture imported into HOST, no reseed; current alias10120 fix reaches558 slot4 raw2C. Literal pre-dispatch snapshot copies9 actors/liveD8/supported globals; unrepresented arcadeRAMzero. Later HOST/SH2 trajectories are not claimed identical.'}
(here/'action2c_manifest.json').write_text(json.dumps(meta,indent=2)+'\n');print(json.dumps(meta))
