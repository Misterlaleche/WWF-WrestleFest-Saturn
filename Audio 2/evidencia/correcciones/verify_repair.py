"""Consolidate component repairs, preserved media, and the failed release gate."""
from pathlib import Path
import hashlib,importlib.util,json,struct
HERE=Path(__file__).resolve().parent
RUNTIME=HERE.parents[2]
def read(rel):return json.loads((HERE/rel).read_text())
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
pcm=read('pcm/green/pcm-report.json')
assert pcm['cases']==144 and pcm['neighbors_failures']==pcm['interpolation_failures']==pcm['moving_failures']==0
assert len(pcm['moving_rows'])==12
firmware=read('pcm/firmware-tests/FIRMWARE-TEST-SUMMARY.json')
assert firmware['passed'] and firmware['commands_passed']==18 and firmware['pcm_underruns']==0
assert firmware['all_operator_masks_match'] and firmware['nominal_music_seconds_per_command']==3
assert read('final/host-tests/host-tests.json')['passed']
spec=importlib.util.spec_from_file_location('stack_gate',RUNTIME/'validation/verify_saturn_stack_layout.py')
gate=importlib.util.module_from_spec(spec);spec.loader.exec_module(gate)
master=HERE/'final/wrestlefest-runtime18.elf';contract=gate.verify(master.read_bytes())
assert contract['slave_stack_bytes']==8192 and contract['heap_available']>=16384
assert sha(master)=='53eaf24ee5d62152ca788c1de99cb9af5bb5bb98bf3dede23d272205bc4cced0'
scsp=HERE/'final/SCSP.ELF';assert sha(scsp)==firmware['firmware_sha256']
# Compare the actually linked DSP words with the current assembler output.
spec=importlib.util.spec_from_file_location('dsp',RUNTIME/'tools/audio/native_probe/build_dsp_mixer.py')
dsp=importlib.util.module_from_spec(spec);spec.loader.exec_module(dsp)
expected=struct.pack('>512H',*(word for row in dsp.build_program() for word in row))
symbols=read('firmware/symbols.json');address=symbols['wf_dsp_program'];data=scsp.read_bytes()
ph=struct.unpack_from('>I',data,28)[0];n=struct.unpack_from('>H',data,44)[0];observed=None
for i in range(n):
 h=struct.unpack_from('>8I',data,ph+i*32)
 if h[0]==1 and h[2]<=address and address+len(expected)<=h[2]+h[4]:
  offset=h[1]+address-h[2];observed=data[offset:offset+len(expected)]
assert expected==observed
comparison=read('review/final-contained-vs-sfx-comparison.json')
stable=comparison['stable_comparison']
assert stable['equivalent_under_observed_keys'] and stable['music_vs_sfx_video_blank_ratio']==1
assert stable['music_minus_sfx_median_render_estimate']==0
assert not comparison['software_audio_gate_passed_for_both']
contained=read('emulator/final-contained-deadline/report.json')
last=contained['observations'][-1]
assert last['audio_diagnostics']['disabled'] and last['music_error']==10 and last['dropped_fm']==1
assert last['game_frame']==849 and not last['fatal_owner']
live=last['allocator_live_stats'];assert live['captures'] and not live['faults']
assert live['private_consistency']==live['user_consistency']==0
active=read('emulator/candidate-stats-music/report.json')['observations'][-1]
assert active['dropped_fm']==257 and active['game_frame']==0
negative=read('emulator/injected-boot-failure/report.json')['observations'][-1]
assert negative['game_frame']>0 and negative['audio_diagnostics']['reason']==1 and not negative['fatal_owner']
oldzip=Path('C:/Users/VersusPC/Documents/Codex/2026-10-25/https-github-com-misterlaleche-wwf-wrestlefest/outputs/WF-SCSP-NATIVE-MUSIC-R02.zip')
assert sha(oldzip)=='8f1d9a90dae167ad2871baa952775e1ea6de728760f139dcfe04cf5eac16f02b'
original=RUNTIME/'entregas/WrestleFest-Saturn-frontend-r02-20261026/wrestlefest-runtime18.bin'
assert sha(original)=='55baf7c5a9f61a7dd9ff6e230d744e03d371c050d61f45895be98f7bcdb5f126'
images=[str(p.relative_to(HERE)) for p in HERE.rglob('*') if p.is_file() and p.suffix.lower() in ('.bin','.cue','.iso')]
assert not images,images
owned=[
 'src/audio/native/wf_music_module.c','src/audio/native/wf_music_module.h',
 'src/audio/native/wf_music_firmware.c','src/audio/native/wf_scsp_dsp.c',
 'src/runtime/src/wf_r18_scsp.c','src/runtime/src/wf_r18_scsp_runtime.c',
 'src/runtime/include/wf_r18_scsp.h','src/runtime/include/wf_r18_scsp_runtime.h',
 'src/runtime/src/wf_saturn_entry.c','src/runtime/src/wf_saturn_frontend.inc',
 'src/runtime/src/wf_saturn_allocator_partition.c','src/runtime/ldscripts/wf_memory.x',
 'src/runtime/ldscripts/wf_memory.mk','src/runtime/ldscripts/wf_memory.specs',
 'src/runtime/Makefile.saturn-link','src/runtime/Makefile.saturn-disc',
 'validation/verify_saturn_stack_layout.py','validation/verify-hwram-budget.py',
 'tools/audio/native_probe/build_runtime18.py','tools/audio/native_probe/prepare_allocator_sdk.py',
 'tools/audio/native_probe/build_music_module.py','tools/audio/native_probe/build_music_driver.py',
 'tools/audio/native_probe/build_dsp_mixer.py','tools/audio/native_probe/build_music_host.py',
 'tools/audio/native_probe/adapt_dsp_oracle.py',
 'tests/audio/native-probe/test_runtime_containment.c','tests/audio/native-probe/test_music_waits.c',
 'tests/audio/native-probe/test_pcm_signed_address_beetle.py','tests/audio/native-driver/test_dsp_mixer.py']
report=dict(release_status='BLOCKED_NATIVE_MUSIC_PERFORMANCE',component_repairs_verified=True,
 full_native_music_ready=False,active_music_no_graphics_regression_verified=False,
 physical_validation=False,bin_cue_iso_created=False,r04_and_previous_deliveries_edited=False,
 containment=dict(host_tests=True,full_cold_boot_failure_game_continues=True,
                  missed_fm_deadline_faults_stream=True,muted_fallback_is_not_audio_approval=True),
 pcm=dict(independent_cases=144,moving_cases=12,failures=0,linked_dsp_matches_assembler=True,
          dsp_steps=120,dsp_ram_accesses=12,m68k_music_windows=18,seconds_per_window=3,
          concurrent_effects=3,pcm_underruns=0,fm_waveform_hardware_certified=False),
 memory=contract,observed_boot_allocator=live,
 muted_fallback_graphics=dict(game_window=[300,600],vblank_ratio=1.0,median_render_estimate_delta=0,
                             samples_are_not_bit_identical=True,active_music_certification=False),
 unresolved=dict(control_musical_deadlines=True,active_baseline_dropped_fm=257,
                 active_baseline_game_frame_at_9600=0,reason='Full cache integration fails; performance experiments were not promoted'),
 files={str(p.relative_to(HERE)):dict(bytes=p.stat().st_size,sha256=sha(p)) for p in [master,scsp,HERE/'final/module/MUSIC.ELF']},
 source_hashes_at_verification={name:sha(RUNTIME/name) for name in owned},
 source_scope='Audio hunks and allocator/build only. Concurrent CDFS/metasprite-cache changes remain untouched. Tested master uses pinned graphical snapshot, not every later checkout edit.',
 original_audio_r02_zip_unchanged=True,original_image_unchanged=True)
(HERE/'VERIFIED-RESULT.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps({k:report[k] for k in ('release_status','component_repairs_verified','full_native_music_ready','bin_cue_iso_created','memory','muted_fallback_graphics')},indent=2))
