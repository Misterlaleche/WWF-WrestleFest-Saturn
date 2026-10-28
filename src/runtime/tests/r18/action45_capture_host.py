"""Freeze the unmodified pre-dispatch paired-rush HOST1407 raw45 state."""
from pathlib import Path
import hashlib,json,struct
root=Path(__file__).resolve().parents[4];here=Path(__file__).resolve().parent
source=root/'validation/local-20261024/tag-winner-rush-paired';snapshot=source/'snapshot-00387.bin';b=snapshot.read_bytes()
assert len(b)==16472 and struct.unpack_from('=6I',b)==(1407,1,1,0x1c03e,387,1)
actor=b[56+0x5b0+0x10c:56+0x5b0+2*0x10c]
assert int.from_bytes(actor[0x60:0x62],'big')==0x45 and int.from_bytes(actor[2:4],'big')==7
assert b[56+0x5b0:56+0x5b0+9*0x10c]==(source/'blocked-actors.bin').read_bytes()
assert b[24:56]==(source/'blocked-dregs.bin').read_bytes()
target=here/'action45_snapshot_paired_rush_1407.bin';assert not target.exists()or target.read_bytes()==b;target.write_bytes(b)
meta={'rom_sha1':'a726c46f350723ea73158bc78055d17d418b14e2','snapshot_sha256':hashlib.sha256(b).hexdigest(),'header':list(struct.unpack_from('=6I',b)),'source':str(snapshot.relative_to(root)),'source_report_sha256':hashlib.sha256((source/'report.json').read_bytes()).hexdigest(),'source_manifest_sha256':hashlib.sha256((source/'source-manifest.json').read_bytes()).hexdigest(),'trace_sha256':hashlib.sha256((source/'trace.csv').read_bytes()).hexdigest(),'host_log_sha256':hashlib.sha256((source/'host.log').read_bytes()).hexdigest(),'provenance':'Natural cold frontend TAG picks0/7 CPU6/3 -> HOST paired rush, frame1407 slot1 raw45, without reseed, actor/register/result forcing. Snapshot projects nine actors, live D8 and represented globals; unrepresented arcade RAM is zero. This is a HOST tape, not a native Saturn capture.','table_alias':{'selector':69,'descriptor':'0x128de','callback':'0x128fa','same_as_raw':'0x01'}}
(here/'action45_manifest.json').write_text(json.dumps(meta,indent=2)+'\n');print(json.dumps(meta))
