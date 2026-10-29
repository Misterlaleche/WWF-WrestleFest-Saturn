#!/usr/bin/env python3
"""Export actual MAME writes for the original WP01 entrance, without audio rendering."""
from pathlib import Path
import collections
import csv
from decimal import Decimal, ROUND_HALF_UP
import hashlib
import json
import struct
import zipfile

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
START = Decimal('22.108160000000')
END = Decimal('29.210624000000')
RATE = 44100

def sample(t):
    return int(((t-START)*RATE).quantize(Decimal(1),rounding=ROUND_HALF_UP))

rows = list(csv.DictReader((HERE/'capture/audio_writes.csv').open()))
regs = [0]*256
keys = [0]*8
bank = 0
initial = None
events = []
commands = []
snapshots = []
seen = collections.defaultdict(set)
max_full = 0
full = [0]*8
for raw in rows:
    t=Decimal(raw['time_seconds'])
    chip=raw['chip']; address=int(raw['address']); value=int(raw['value'])
    if initial is None and t>=START:
        initial={'ym_registers':regs.copy(),'ym_key_masks':keys.copy(),'oki_bank':bank,
                 'limitation':'Registers only: phase/envelope/audio tail state before the scene is not reconstructed by this snapshot.'}
    previous=regs[address] if chip=='YM' else None
    if chip=='YM':
        regs[address]=value
        if address==8:
            keys[value&7]=value>>3
            full[value&7]=int(value>>3==15)
    elif chip=='BANK':
        bank=value&1
    if not START<=t<END:
        continue
    rel={'sample':sample(t),'time_seconds':str(t-START),'original_time_seconds':str(t),
         'original_frame':int(raw['frame']),'chip':chip,'address':address,'value':value}
    if chip=='COMMAND':
        commands.append(rel)
        continue
    if chip=='YM':
        # Timers drive the original Z80 driver; their resulting musical events
        # are already present in this capture. Do not run that clock twice.
        if address in (0x10,0x11,0x12,0x14):
            continue
        # Ordinary register writes are state-setting. KEYON stays explicit;
        # repeated KEYOFF is retained for a faithful executable event stream.
        if value==previous and address!=8:
            continue
        if address==8 and value>>3==15:
            ch=value&7
            if t>=Decimal('22.125913916578'):
                patch={'channel':ch,'sample':sample(t),'algorithm':regs[0x20+ch]&7,
                       'feedback':(regs[0x20+ch]>>3)&7,'panning':regs[0x20+ch]>>6,
                       'kc_at_keyon':regs[0x28+ch],'kf_at_keyon':regs[0x30+ch],
                       'pms_ams':regs[0x38+ch],
                       'operators_register_order_M1_M2_C1_C2':[
                           {'dt1_mul':regs[0x40+ch+o*8],'tl':regs[0x60+ch+o*8]&127,
                            'ks_ar':regs[0x80+ch+o*8],'am_d1r':regs[0xa0+ch+o*8],
                            'dt2_d2r':regs[0xc0+ch+o*8],'d1l_rr':regs[0xe0+ch+o*8]}
                           for o in range(4)]}
                snapshots.append(patch)
                seen[ch].add(patch['algorithm'])
        if t>=Decimal('22.125913916578'):
            max_full=max(max_full,sum(full))
    events.append(rel)

out=HERE/'export';out.mkdir(exist_ok=True)
manifest={'source':'MAME0.264 input-only original-ROM capture; write taps do not change emulated values',
          'source_capture_sha256':hashlib.sha256((HERE/'capture/audio_writes.csv').read_bytes()).hexdigest(),
          'scene_start_original_seconds':str(START),'scene_end_original_seconds':str(END),
          'sample_rate':RATE,'scene_samples':sample(END),'initial':initial,
          'commands':commands,'events':events,
          'fm_full_keyon_channels':sorted(seen),'fm_algorithms':{str(k):sorted(v) for k,v in seen.items()},
          'max_simultaneous_full_key_masks':max_full,
          'initialization_only_partial_key_channels':[5,6],
          'notes':'Channels5/6 receive only partial initialization keys; no complete-carrier notes in the entrance. Native FM translation must preserve register order: KC/KF may follow KEYON. This stream contains events, never rendered FM waveform data.'}
(out/'entrance_audio_events.json').write_text(json.dumps(manifest,indent=2)+'\n')
(out/'entrance_note_patches.json').write_text(json.dumps(snapshots,indent=2)+'\n')
with (out/'entrance_audio_events.csv').open('w') as f:
    writer=csv.DictWriter(f,fieldnames=list(events[0]));writer.writeheader();writer.writerows(events)
chipids={'YM':0,'OKI':1,'BANK':2}
with (out/'entrance_audio_events.h').open('w') as f:
    f.write('/* Generated from original MAME input-only write taps. No mixed PCM. */\n#pragma once\n#include <stdint.h>\n')
    f.write('typedef struct { uint32_t sample; uint8_t chip, reg, value, reserved; } wp01_audio_event_t;\n')
    f.write('enum { WP01_AUDIO_YM=0, WP01_AUDIO_OKI=1, WP01_AUDIO_BANK=2, WP01_AUDIO_RATE=44100, WP01_AUDIO_SAMPLES=%du };\n'%sample(END))
    f.write('static const uint8_t wp01_audio_initial_ym[256]={\n')
    for i in range(0,256,16): f.write(' '+','.join(str(v) for v in initial['ym_registers'][i:i+16])+',\n')
    f.write('};\nstatic const wp01_audio_event_t wp01_audio_events[]={\n')
    for e in events:
        f.write(' {%du,%du,%du,%du,0},\n'%(e['sample'],chipids[e['chip']],e['address'],e['value']))
    f.write('};\n#define WP01_AUDIO_EVENT_COUNT (sizeof(wp01_audio_events)/sizeof(wp01_audio_events[0]))\n')
with (out/'entrance_audio_events.bin').open('wb') as f:
    for e in events:f.write(struct.pack('>IBBBB',e['sample'],chipids[e['chip']],e['address'],e['value'],0))

# Original OKI individual ADPCM assets. The exact PCM16 decoder is a separate
# step, so their unmodified source bytes are available for independent checks.
with zipfile.ZipFile(ROOT/'project_sources/01-wwfwfest.zip') as z:
    rom=z.read('31j10.ic73')
samples=[]
for sid in (1,18):
    a=int.from_bytes(rom[sid*8:sid*8+3],'big')&0x3ffff
    b=int.from_bytes(rom[sid*8+3:sid*8+6],'big')&0x3ffff
    data=rom[a:b+1]
    path=out/f'oki_bank0_sample{sid:03d}.adpcm';path.write_bytes(data)
    samples.append({'bank':0,'sample_id':sid,'start':a,'end_inclusive':b,'bytes':len(data),
                    'decoded_samples':len(data)*2,'source_clock_hz':8000,
                    'sha256':hashlib.sha256(data).hexdigest()})
(out/'oki_samples.json').write_text(json.dumps(samples,indent=2)+'\n')
print(json.dumps({'events':len(events),'event_counts':dict(collections.Counter(e['chip'] for e in events)),
                  'channels':manifest['fm_full_keyon_channels'],'max_full_keys':max_full,'samples':samples},indent=2))
