#!/usr/bin/env python3
"""Read-only validation of the bounded residency extension against v4 gameplay.

Only this directory's verification/ outputs are written. Frozen v4 inputs,
gameplay sources, old atlas, new atlas and manifest are never changed.
"""
from pathlib import Path
import csv,hashlib,json,os,struct,subprocess
here=Path(__file__).resolve().parent;audit=here.parent;work=audit.parent
manifest=json.loads((here/'wp1979-residency-takeover-v1.json').read_text())
verified=[]
def check_hashes(value):
 if isinstance(value,dict):
  if {'path','bytes','sha256'}<=value.keys():
   p=audit/value['path'];b=p.read_bytes()
   assert len(b)==value['bytes'] and hashlib.sha256(b).hexdigest()==value['sha256'],p
   verified.append(value['path'])
  for v in value.values():check_hashes(v)
 elif isinstance(value,list):
  for v in value:check_hashes(v)
check_hashes(manifest)
out=here/'verification';out.mkdir(exist_ok=True)
builder=work/'reference_r18/src/runtime/tools/build_r18_hot_sprites.py'
subprocess.run(['python3',str(builder),str(here/'union-tile-ids.txt'),
                str(audit/'assets/SPRITES.PAK'),str(out/'rebuilt-atlas.bin'),
                '--format','dictionary','--chunk-bytes','32'],check=True)
atlas=audit/manifest['atlas']['path'];b=atlas.read_bytes()
assert (out/'rebuilt-atlas.bin').read_bytes()==b
source=(audit/'assets/SPRITES.PAK').read_bytes();count=int.from_bytes(b[4:6],'big')
chunks,mapping,dictionary,header=struct.unpack('>IIII',b[8:24])
assert count==6474 and dictionary//chunks==32
start=header+(mapping+31)//32*32
for i in range(count):
 tile=int.from_bytes(b[24+2*i:26+2*i],'big')
 indices=[int.from_bytes(b[header+2*(i*4+j):header+2*(i*4+j)+2],'big') for j in range(4)]
 restored=b''.join(b[start+k*32:start+(k+1)*32] for k in indices)
 assert restored==source[tile*128:(tile+1)*128],tile
checks=[]
for name,tape,limit,expected,complete in [
 ('manual',here/'takeover-input.csv',513,here/'sampled-roles.csv',False),
 ('main',audit/'host/tag-rom-v4-input.csv',24000,audit/'host/tag-rom-v4-resident-roles.csv',True)]:
 env={k:v for k,v in os.environ.items() if not (k.startswith('WP1979_') or k.startswith('WF_PROBE_'))}
 env.update(WP1979_SEED='1085',WF_PROBE_PAD_SCHEDULE=str(tape),WF_PROBE_FRAMES=str(limit),
            WF_PROBE_REQUIRE_RESIDENT='1',WP1979_TAG_ROLES=str(out/(name+'-roles.csv')))
 if complete:env['WF_PROBE_REQUIRE_COMPLETE']='1'
 run=subprocess.run([str(audit/'fixes/state6/guard_probe_v4'),str(audit/'assets/ROM68K.BIN'),
                     str(audit/'assets/SPRITES.PAK'),str(audit/'assets/R18RING.BIN'),str(atlas)],
                    cwd=audit/'host',env=env,capture_output=True,text=True)
 (out/(name+'.log')).write_text(run.stdout+run.stderr)
 assert run.returncode==0,(name,run.stdout[-2000:],run.stderr)
 assert 'PLAY CD reads ROM=0 SPRITES=0' in run.stdout and 'BLOCK frame=' not in run.stdout
 assert (out/(name+'-roles.csv')).read_bytes()==expected.read_bytes()
 if complete:assert 'MATCH COMPLETE frame=7372 ROM endpoint=0011B6 result=4001,4000,8000,8000' in run.stdout
 checks.append(dict(route=name,logical_steps=7372 if complete else 513,
                    full_role_trace_byte_identical=True,rom_reads=0,sprite_reads=0,blocked_owners=0))
host={(int(x['frame']),int(x['slot'])):x for x in csv.DictReader((out/'manual-roles.csv').open())}
observations=list(csv.DictReader((here/'emulator-actor-observations.csv').open()))
fields=[k for k in observations[0] if k not in ('emulator_frame','reported_game_frame','host_completed_frame','slot')]
assert len(observations)==2108 and len(fields)==17
for row in observations:
 expected=host[int(row['host_completed_frame']),int(row['slot'])]
 for k in fields:assert int(row[k])==int(expected[k]),(row['emulator_frame'],row['slot'],k)
result=dict(verified_artifacts=len(verified),exact_reconstructed_tiles=count,
            observed_actor_records=len(observations),compared_fields_per_record=len(fields),
            exact_observed_field_values=len(observations)*len(fields),
            resident_lwram_bytes=643456,cold_fallback_slots=53,routes=checks)
(out/'summary.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2))
