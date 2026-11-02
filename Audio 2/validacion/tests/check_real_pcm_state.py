"""Read an existing frozen9600 state; never loads it or invokes an emulator."""
from pathlib import Path
import hashlib,importlib.util,json,sys
base=Path(__file__).resolve().parents[1];repair=base.parents[1]
spec=importlib.util.spec_from_file_location('observe_pcm_real',base/'observe_cold.py');observe=importlib.util.module_from_spec(spec);spec.loader.exec_module(observe)
spec=importlib.util.spec_from_file_location('assess_pcm_real',base/'assess_tracks.py');assess=importlib.util.module_from_spec(spec);spec.loader.exec_module(assess)
sys.path.insert(0,str(repair/'emulator'));from cold_overlay import Elf,field
layout_path=base/'pcm-layout/music-layout.json';layout=json.loads(layout_path.read_text());sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
binding=observe.bind_scsp_counter(Elf(Path(layout['debug_path'])),Elf(Path(layout['release_path'])),layout,sha(layout_path))
folder=base.parent/'saturn-real-v25/cold-9600-tag';path=folder/'frame-9600.state';state=path.read_bytes();report=json.loads((folder/'report.json').read_text())
assert next(x['new_sha256']for x in report['replacements']if x['name']=='SCSP.ELF')==binding['release_elf_sha256']
mp,_=field(state,'DSP.MPROG',1024);sound,_=field(state,'RAM',524288,mp)
observed=observe.sample_scsp_counter(state,sound,binding);assert sha(path)==hashlib.sha256(state).hexdigest()
fixture=bytearray(state);at=sound+binding['counter_address'];fixture[at:at+4]=bytes.fromhex('01000000');fixture[sound+0xc05c:sound+0xc05e]=b'\0\0'
negative=observe.sample_scsp_counter(fixture,sound,binding);assert negative['pcm_engine_underruns']==65536 and negative['pcm_underruns_mail']==0
gate=assess.assess_pcm_observation([negative]);assert not gate['passed']and'pcm_engine_underruns'in gate['blockers']
result=dict(passed=True,state_path=str(path),state_sha256=sha(path),real_state_observation=observed,private_bytearray_negative_fixture=negative,negative_gate=gate,real_guest_edited=False,state_loaded=False,emulator_invoked=False,original_state_unchanged=sha(path)==hashlib.sha256(state).hexdigest(),note='A zero PCM counter in this existing state does not override its284 FM discards.')
(base/'tests/real-pcm-state-result.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(passed=True,real_counter=observed['pcm_engine_underruns'],mail_counter=observed['pcm_underruns_mail'],negative_counter=negative['pcm_engine_underruns'],negative_gate_passed=gate['passed'],state_unchanged=True),indent=2))
