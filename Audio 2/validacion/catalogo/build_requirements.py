"""Read-only catalogue audit and planned coverage; executes no audio/emulator."""
from pathlib import Path
import csv,hashlib,json,struct,collections
HERE=Path(__file__).resolve().parent;CONTROL=HERE.parents[1];REPAIR=CONTROL.parent;RUNTIME=REPAIR.parents[2]
pack=REPAIR/'pcm/firmware-tests/input/inputs/SFX.DAT';data=pack.read_bytes();sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
assert data[:4]==b'WFSX'and int.from_bytes(data[8:12],'big')==len(data)
samples,commands,events,raw=struct.unpack_from('>4I',data,12);catalog=[]
for i in range(98):
 at=commands+i*6;priority,bank,count,stop,off=struct.unpack_from('>4BH',data,at)
 ev=[dict(zip(['sample','ticks','volume','flags'],data[events+off+j*4:events+off+j*4+4]))for j in range(count)]
 catalog.append(dict(command=0x20+i,hex=f'{0x20+i:02X}',priority=priority,bank_selector=bank,event_count=count,stop_on_sequence_end=bool(stop),events=ev))
with(HERE/'SFX-CATALOGUE.csv').open('w',newline='')as f:
 w=csv.writer(f);w.writerow(['command','priority','bank','events','stop','ticks_total','samples','flags'])
 for c in catalog:w.writerow([c['hex'],c['priority'],c['bank_selector'],c['event_count'],int(c['stop_on_sequence_end']),sum(e['ticks']for e in c['events']),','.join(str(e['sample'])for e in c['events']),','.join(str(e['flags'])for e in c['events'])])
with(HERE/'HOST-MUSIC-SFX-MATRIX.csv').open('w',newline='')as f:
 w=csv.writer(f);w.writerow(['music','sfx','stage','status','route','expected'])
 for music in range(1,19):
  for c in catalog:w.writerow([f'{music:02X}',c['hex'],'HOST_ROUTE_AND_STATE','PLANNED_NOT_EXECUTED','wf_r18_scsp_audio->native_queue->ARCADE_WORD->sfx_command','A15/A16 priority/bank/voice state; native music events unchanged'])
with(HERE/'COMMAND-ROUTES.csv').open('w',newline='')as f:
 w=csv.writer(f);w.writerow(['low_byte','route_in_music_profile','expected_result','positive_catalogue'])
 for c in range(256):
  if c==0:route,expected,positive='worker FIFO + mailbox FIFO','ordered music/effects stop',True
  elif 1<=c<=18:route,expected,positive='worker FIFO','start/reset native score and new generation',True
  elif c==31:route,expected,positive='worker FIFO','native fade if playing; effects unaffected',True
  elif 0x20<=c<=0x81:route,expected,positive='mailbox FIFO','SFX accepted/legitimate priority reject;20=no-op',True
  elif c<=0x81:route,expected,positive='mailbox FIFO','UNIMPLEMENTED counted, not a covered music command',False
  else:route,expected,positive='queue rejects','invalid/dropped_sfx counted; not silently covered',False
  w.writerow([f'{c:02X}',route,expected,positive])
source_paths=[RUNTIME/p for p in ['src/runtime/src/wf_r18_scsp_runtime.c','src/audio/native/wf_native_queue.c','src/audio/native/wf_music_firmware.c','src/audio/native/wf_scsp_sfx.c','src/audio/native/wf_scsp_sfx.h','src/audio/native/wf_scsp_dsp.c','src/audio/native/wf_scsp_ym.c','tests/audio/native-driver/test_sfx.c','tests/audio/native-driver/test_music_catalogue.c','tests/audio/native-driver/test_music_firmware.py','tests/audio/native-driver/reference/WF_OKI_AUDIT.json']]
requirements=[
 dict(id='INPUTS',require='Exact master/module/debug/SCSP/debug/SFX pack/ROM/source/layout/harness/core/cue/BIN/BIOS/input script hashes and compiler identities; reject changes before first job'),
 dict(id='ROUTES',require='All98 SFX commands traverse wf_r18_scsp_audio, host FIFO and firmware ARCADE_WORD, not only direct OKI_START;18 native scores and00/1F lifecycle commands explicitly observed'),
 dict(id='HOST_MATRIX',require='18x98 short HOST routing/state matrix, reference A15/A16 priority/bank oracle and R01 musical events; not1764 long emulator boots'),
 dict(id='PCM_ALL',require='114 bank entries/95 unique samples complete PCM vs audited golden PAK, original sequence tails, all98 sequences and30000 mixed ticks'),
 dict(id='PRIORITY',require='Three occupied logical voices: lower rejection, equal/higher preemption and deterministic victim; retain bank selector2, F1 suppression and no-op20'),
 dict(id='LONG_SCORE',require='All18 R01 intro/observed-loop or natural-end coverage, exact CPU/RAM/timer/native events;0B/0C/11 remain finite600s observations with explicit incomplete-loop label'),
 dict(id='FADE_END',require='Observe fade flagC062 clear, native-activeC006 clear, FM key mask/levels and final OKI stop against R01; no new score activity after closure, allow existing SFX tails'),
 dict(id='STOP_RACE',require='00/new-track/late old mailbox stop; generation ACK discards stale music, stopsPCM3, preserves current effects and newer FM ownership; stop pressure fails closed while gameplay continues'),
 dict(id='MIX_LONG',require='Representative long full-system stress: startup-heavy02, write-burst12, long0B/0C/11 as needed from measured worst windows; three SFX logical voices plus musicalPCM3, bursts/fades/new generations'),
 dict(id='DEADLINES',require='Zero FM drops, real32-bit PCM underruns, mailbox underruns, unsupported/invalid required commands, queue loss, ignored or disabled audio; expected priority rejection must match oracle'),
 dict(id='CONTINUITY',require='Event/cycle progress and expected note/PCM activity in oracle-active windows; no blanket nonzero RMS rule for intentional rests or natural end; RMS alone never certifies a score'),
 dict(id='GRAPHICS',require='Same pinned master/seed/input/assets, same completed game-frame scene hashes/render workload; actual stable combat>=600game advances; no extra video blanks for same advances'),
 dict(id='OBSERVATION',require='Exact M68K compiler-derived engine sizeof/offsetof and debug/release load identity, native profiles4D46/53F1, live32-bit counters plus mailbox; missing data blocks certification'),
 dict(id='NEGATIVE_CONTAINMENT',require='Deliberate starvation stops affected PCM voice and increments underrun without setting engine fault or blocking port; that negative run must still fail delivery gate'),
 dict(id='MEDIA',require='No BIN/CUE before complete evidence and user authorization; R04/canonical untouched; no relaxed tolerance, mute, resampling, guest-memory/state edits or substituted audio')]
result=dict(status='REQUIREMENTS_AND_PLAN_NOT_EXECUTED',music_commands=list(range(1,19)),sfx_commands=list(range(0x20,0x82)),host_pairs=1764,long_emulator_pairs=0,pack=dict(path=str(pack),bytes=len(data),sha256=sha(pack),command_count=98,event_count=sum(c['event_count']for c in catalog),sample_entries=114,distinct_samples=95,priority_distribution=dict(collections.Counter(c['priority']for c in catalog)),bank_distribution=dict(collections.Counter(c['bank_selector']for c in catalog))),source_sha256={str(p):sha(p)for p in source_paths},requirements=requirements,catalogue=catalog,known_gaps=['Current test_sfx.c starvation assertion expects fatal; implementation intentionally contains per-voice underrun. Test-only correction authorized separately.','Existing M68K/DSP mix test covers3s per score and3 direct OKI effects; no full FM waveform emulation or runtime98-command route.','Original cold audio_gate omitted realPCM underruns; validation observer/assessor now closes gap with compiler-pinned counter.','600s and an8s advance alone do not assert fade closure or complete looping tracks.','Hold-first suite deliberately suppresses real STOP/fade/track changes, requiring a separate unmodified/remap lifecycle case.','V25 failed during combat:284 FMdrops in verified9600-frame run; later1761 reported by root requires its own artifact pin.'],production_modified=False,disc_images_created=False,emulator_run=False)
(HERE/'REQUIREMENTS.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(status=result['status'],host_pairs=1764,sfx_events=result['pack']['event_count'],pack_sha256=sha(pack),requirements=len(requirements)),indent=2))
