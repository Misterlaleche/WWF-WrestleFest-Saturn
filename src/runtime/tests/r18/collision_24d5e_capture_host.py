"""Read-only projection of the captured natural HOST Royal2398 failed pair."""
from pathlib import Path
import ctypes,hashlib,json,os,struct,subprocess,tempfile
root=Path(__file__).resolve().parents[2];dest=Path(__file__).resolve().parent
v=root.parents[1]/'validation/local-20261024/full-flow-combat-native-f8c-royal-collision2398-diagnostic'
g=(v/'blocked-globals.bin').read_bytes();actors=(v/'blocked-actors.bin').read_bytes();regs=(v/'blocked-dregs.bin').read_bytes()
with tempfile.TemporaryDirectory(prefix='wf-collisioncapture-',ignore_cleanup_errors=os.name=='nt')as tmp:
 lib=Path(tmp)/'project.so'
 subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude','tests/r18/collision_24d5e_oracle_bridge.c','src/wf_actor.c','src/wf_collision_dispatch.c','src/wf_collision_runtime.c','src/wf_collision.c','generated/wf_collision_tables.c','-o',str(lib)],cwd=root,check=True)
 n=ctypes.CDLL(str(lib));assert n.collision_24d5e_globals_size()==len(g)
 n.collision_24d5e_project.argtypes=[ctypes.c_void_p,ctypes.c_void_p,ctypes.c_void_p]
 ram=ctypes.create_string_buffer(0x2000);n.collision_24d5e_project(ram,ctypes.create_string_buffer(g),ctypes.create_string_buffer(actors))
 b=struct.pack('=6I',2398,5,8,0x24d5e,3,4)+regs+ram.raw
assert len(regs)==32 and len(actors)==9*0x10c and int.from_bytes(ram.raw[0x80:0x84],'big')==2398
target=dest/'collision_24d5e_snapshot_native_f8c_royal_2398.bin';assert not target.exists()or target.read_bytes()==b;target.write_bytes(b)
d={'rom_sha1':'a726c46f350723ea73158bc78055d17d418b14e2','snapshot_sha256':hashlib.sha256(b).hexdigest(),'header':[2398,5,8,0x24d5e,3,4],'header_fields':['frame','source_slot','target_slot','second_callback','retained_source_row','target_row'],'source_report_sha256':hashlib.sha256((v/'report.json').read_bytes()).hexdigest(),'actors_sha256':hashlib.sha256(actors).hexdigest(),'globals_sha256':hashlib.sha256(g).hexdigest(),'dregs_sha256':hashlib.sha256(regs).hexdigest(),'provenance':'Natural nativeF8C Royal import -> HOSTbasic2398; production failed pair restores src/dst/linked before returningBLOCKED. Diagnostic wrapper independently identifies src5/ID2 row3 -> dst8/ID8 row4, first2ACCEPT then second12BLOCKED on BYTEfacing00/80. All9 actor records and liveD8 copied literally; supported globals projected, other arcadeRAMzero. No seed/actor/frame/register forcing.'}
(dest/'collision_24d5e_manifest.json').write_text(json.dumps(d,indent=2)+'\n');print(json.dumps(d))
