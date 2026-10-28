"""Read-only projection of the captured natural HOST970 failed pair."""
from pathlib import Path
import ctypes,hashlib,json,os,struct,subprocess,tempfile
root=Path(__file__).resolve().parents[2];dest=Path(__file__).resolve().parent
v=root.parents[1]/'validation/local-20261024/full-flow-combat-native-f8c-tag-after3c-basic'
g=(v/'blocked-globals.bin').read_bytes();actors=(v/'blocked-actors.bin').read_bytes();regs=(v/'blocked-dregs.bin').read_bytes()
with tempfile.TemporaryDirectory(prefix='wf-collisioncapture-',ignore_cleanup_errors=os.name=='nt')as tmp:
 lib=Path(tmp)/'project.so'
 subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude','tests/r18/collision_24924_oracle_bridge.c','src/wf_actor.c','src/wf_collision_dispatch.c','src/wf_collision_runtime.c','src/wf_collision.c','generated/wf_collision_tables.c','-o',str(lib)],cwd=root,check=True)
 n=ctypes.CDLL(str(lib));assert n.collision_24924_globals_size()==len(g)
 n.collision_24924_project.argtypes=[ctypes.c_void_p,ctypes.c_void_p,ctypes.c_void_p]
 ram=ctypes.create_string_buffer(0x2000);n.collision_24924_project(ram,ctypes.create_string_buffer(g),ctypes.create_string_buffer(actors))
 b=struct.pack('=6I',970,5,4,0x24924,7,10)+regs+ram.raw
assert len(regs)==32 and len(actors)==9*0x10c and int.from_bytes(ram.raw[0x80:0x84],'big')==970
target=dest/'collision_24924_snapshot_native_f8c_tag_970.bin';assert not target.exists()or target.read_bytes()==b;target.write_bytes(b)
d={'rom_sha1':'a726c46f350723ea73158bc78055d17d418b14e2','snapshot_sha256':hashlib.sha256(b).hexdigest(),'header':[970,5,4,0x24924,7,10],'header_fields':['frame','source_slot','target_slot','second_callback','retained_source_row','target_row'],'source_report_sha256':hashlib.sha256((v/'report.json').read_bytes()).hexdigest(),'actors_sha256':hashlib.sha256(actors).hexdigest(),'globals_sha256':hashlib.sha256(g).hexdigest(),'dregs_sha256':hashlib.sha256(regs).hexdigest(),'provenance':'Natural nativeF8C TAG import -> HOSTbasic970; production failed pair restores src/dst/linked before returningBLOCKED. Diagnostic wrapper independently identifies src5/ID3 row7 -> dst4/ID9 row10, first1ACCEPT then second2BLOCKED on BYTEfacing00/80. Source currentprofile0 is retained; original dispatcher still uses cached row7. All9 actor records and liveD8 copied literally; supported globals projected, other arcadeRAMzero. No seed/actor/frame/register forcing.'}
(dest/'collision_24924_manifest.json').write_text(json.dumps(d,indent=2)+'\n');print(json.dumps(d))
