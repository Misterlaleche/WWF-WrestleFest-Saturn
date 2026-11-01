"""No emulator: prove fail-closed PCM gates and read-only32-bit sampling."""
from pathlib import Path
import importlib.util,hashlib,types,unittest
BASE=Path(__file__).resolve().parents[1]
def module(name):
 spec=importlib.util.spec_from_file_location(name,BASE/(name+'.py'));m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m
observe=module('observe_cold');assess=module('assess_tracks')
def row(counter=0,mail=0):
 return dict(native_profile=0x53f1,mcu_state=2,game_frame=450,audio_rms=1,accepted_sfx_word=0x21,
  loaded_audio_code={'SCSP.ELF':{'executable_samples':[{'matches':True}]}},
  pcm_engine_underruns=counter,pcm_underruns_mail=mail,
  pcm_engine_observation=dict(validated=True,counter_bytes=4,engine_address=0xc200,counter_address=0xc220,
   layout_sha256='a'*64,debug_elf_sha256='b'*64,release_elf_sha256='c'*64,load_segments_sha256='d'*64))
def profile(rows):
 report=dict(observations=rows,load_success=True,existing_media_unchanged=True,guest_memory_edits=False,state_loads=0,disc_images_created=False,vfs={'errors':[]},accepted_options=assess.OPTIONS)
 trace=[dict(video_clock=1,audio_diagnostics=dict(disabled=False,reason=0,dropped_sfx=0,dropped_music=0,ignored=0))]
 return assess.assess_profile(report,trace,{},'sfx')
class Gates(unittest.TestCase):
 def test_real_counter_over_mailbox_zero_must_fail(self):
  result=profile([row(0x10000,0)]);self.assertFalse(result['passed']);self.assertIn('pcm_engine_underruns',result['blockers'])
 def test_missing_real_counter_must_fail(self):
  r=row();del r['pcm_engine_underruns'];result=profile([r]);self.assertFalse(result['passed']);self.assertIn('pcm_engine_underruns_unobserved',result['blockers'])
 def test_missing_mail_counter_must_fail(self):
  r=row();del r['pcm_underruns_mail'];self.assertFalse(profile([r])['passed'])
 def test_missing_layout_binding_must_fail(self):
  r=row();del r['pcm_engine_observation'];self.assertFalse(profile([r])['passed'])
 def test_valid_zero_counters_pass(self):self.assertTrue(profile([row()])['passed'])
 def test_mail_counter_alone_is_failure(self):self.assertFalse(profile([row(0,1)])['passed'])
 def test_bad_counter_types_fail(self):
  for v in [None,False,-1,2**32,'0']:
   with self.subTest(v=v):self.assertFalse(profile([row(v)])['passed'])
 def test_real_counter_reset_fails(self):self.assertFalse(profile([row(2),row(0)])['passed'])
 def test_both_profiles_use_same_pcm_gate(self):
  for native in [0x4d46,0x53f1]:
   r=row(1);r['native_profile']=native;self.assertFalse(assess.assess_pcm_observation([r])['passed'])
class Observation(unittest.TestCase):
 def fixture(self):
  elf=types.SimpleNamespace(data=b'pinned-elf',symbols={'wf_sfx_engine':(0xc200,262)},ph=[(1,0,0xc100,0xc100,0,0x1000,6,2)])
  layout=dict(schema='wf_scsp_sfx_counter_v1',debug_elf_sha256=hashlib.sha256(elf.data).hexdigest(),release_elf_sha256=hashlib.sha256(elf.data).hexdigest(),load_segments_sha256=observe.scsp_load_digest(elf),engine_symbol='wf_sfx_engine',engine_bytes=262,underruns_offset=32,underruns_bytes=4,compiler_id='exact-test-compiler',compiler_sha256='e'*64,header_sha256='f'*64,probe_source_sha256='0'*64)
  return elf,layout
 def test_counter_is32bit_target_endian_and_readonly(self):
  elf,layout=self.fixture();binding=observe.bind_scsp_counter(elf,elf,layout,'a'*64);state=bytearray(0x80000);state[0xc220:0xc224]=bytes.fromhex('34127856');state[0xc05c:0xc05e]=b'\0\0';before=bytes(state)
  result=observe.sample_scsp_counter(state,0,binding);self.assertEqual(result['pcm_engine_underruns'],0x12345678);self.assertEqual(result['pcm_underruns_mail'],0);self.assertEqual(bytes(state),before)
 def test_layout_mismatches_rejected(self):
  for key,value in [('engine_bytes',264),('underruns_offset',260),('underruns_bytes',2),('debug_elf_sha256','0'*64),('release_elf_sha256','0'*64),('load_segments_sha256','0'*64),('engine_symbol','guess'),('schema','old')]:
   elf,layout=self.fixture();layout[key]=value
   with self.subTest(key=key),self.assertRaises(ValueError):observe.bind_scsp_counter(elf,elf,layout,'a'*64)
 def test_outside_target_memory_rejected(self):
  elf,layout=self.fixture();elf.symbols={'wf_sfx_engine':(0x7fff0,262)}
  with self.assertRaises(ValueError):observe.bind_scsp_counter(elf,elf,layout,'a'*64)
 def test_missing_or_changed_target_load_rejected(self):
  elf,layout=self.fixture();other=types.SimpleNamespace(data=elf.data,symbols=elf.symbols,ph=[(1,0,0xc100,0xc100,0,0x1002,6,2)])
  with self.assertRaises(ValueError):observe.bind_scsp_counter(elf,other,layout,'a'*64)
 def test_sampler_rejects_missing_or_out_of_range_binding(self):
  for binding in [{},{'validated':True,'counter_bytes':4,'counter_address':0x80000},{'validated':True,'counter_bytes':2,'counter_address':0xc220}]:
   with self.subTest(binding=binding),self.assertRaises(ValueError):observe.sample_scsp_counter(bytearray(0x80000),0,binding)
class Abi4Identity(unittest.TestCase):
 def test_integrated_inserted_source_reads_hardware_regions(self):
  ns={'__file__':str(observe.ORIGINAL),'__name__':'observer_test'};exec(compile(observe.inserted_source(),str(observe.ORIGINAL),'exec'),ns)
  regions={'DSP.MPROG':0,'RAM':0,'VRAM':0x80000,'WorkRAMH':0x100000};ns['field']=lambda state,name,size,*args:(regions[name],size)
  state=bytearray(0x200000)
  def add(region,at,data):
   x=bytearray(data);x[::2],x[1::2]=data[1::2],data[::2];start=regions[region]+at;state[start:start+len(x)]=x
  sound=bytes(range(64));video=bytes(range(64,128));hot=bytes(range(128,192))
  add('RAM',0,sound);add('VRAM',0x5c000,video);add('WorkRAMH',0x88b30,hot)
  def load(address,data,offset=0):return(1,offset,address,address,len(data),len(data),5,4)
  elves={'SCSP.ELF':types.SimpleNamespace(data=sound,ph=[load(0,sound)]),'MUSIC.ELF':types.SimpleNamespace(data=video+hot,ph=[load(0x05c5c000,video),load(0x06088b30,hot,64)])}
  before=bytes(state);result=ns['code_identity'](state,elves)
  self.assertTrue(all(x['matches']for item in result.values()for x in item['executable_samples']));self.assertEqual(bytes(state),before)
if __name__=='__main__':unittest.main()
