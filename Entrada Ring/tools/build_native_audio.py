#!/usr/bin/env python3
"""Compile original note/sample events to native SCSP register writes.

Runtime assets contain oscillator tables, original individual ADPCM samples,
and register events. No rendered musical channel or full-scene recording.
"""
from pathlib import Path
import csv,hashlib,json,struct,subprocess

ROOT=Path(__file__).resolve().parents[1]
FM=ROOT/'research_scsp'
OKI=ROOT/'research_inventory'
EVENTS=ROOT/'research_events/export'
OUT=ROOT/'source/cd'
EVIDENCE=ROOT/'evidence'

def ticks(sample):
    numerator=sample*120093750
    denominator=44100*143
    return (numerator+denominator//2)//denominator

def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    OUT.mkdir(exist_ok=True,parents=True)
    EVIDENCE.mkdir(exist_ok=True,parents=True)
    exe=ROOT/'tools/export_native_audio'
    command=['gcc','-std=c11','-O2','-Wall','-Wextra','-Werror',
        '-I',str(FM),'-I',str(OKI),'-I',str(OKI/'generated'),'-I',str(EVENTS),
        str(ROOT/'tools/export_native_audio.c'),str(FM/'wp01_fm.c'),str(OKI/'wp01_oki.c'),'-o',str(exe)]
    subprocess.run(command,check=True)
    result=subprocess.run([str(exe)],check=True,capture_output=True,text=True)
    csvpath=EVIDENCE/'native-registers.csv';csvpath.write_text(result.stdout)
    rows=list(csv.DictReader(result.stdout.splitlines()))
    initial=[r for r in rows if int(r['initial'])]
    events=[r for r in rows if not int(r['initial'])]
    sine=(FM/'FM-SINE.PCM').read_bytes()
    oki=(OKI/'generated/oki_volume_bank.bin').read_bytes()
    assert len(sine)==266240 and len(oki)==67680
    bank=bytearray(0x44000+len(oki));bank[:len(sine)]=sine;bank[0x44000:]=oki
    assert len(bank)<=0x80000
    (OUT/'SOUNDRAM.BIN').write_bytes(bank)
    last=0;keyexec=[]
    for r in rows:
        offset,value=int(r['offset']),int(r['value'])
        assert not offset&1 and 0<=offset<0x380 and offset%32<=0x16 and 0<=value<=65535
        if not int(r['initial']):
            t=int(r['sample']);assert t>=last;last=t
            if offset%32==0 and value&0x1000:keyexec.append(t)
    samples=313219
    data=bytearray(struct.pack('>8I',0x5750314e,1,len(initial),len(events),ticks(samples),len(bank),120093750,143))
    for r in initial:data.extend(struct.pack('>HH',int(r['offset']),int(r['value'])))
    for r in events:data.extend(struct.pack('>IHH',ticks(int(r['sample'])),int(r['offset']),int(r['value'])))
    (OUT/'AUDIOREG.BIN').write_bytes(data)
    report={'classification':'Native SCSP FM operators plus original individual OKI samples; no music PCM',
        'original_chip_events':748,'native_initial_writes':len(initial),'native_timed_writes':len(events),
        'source_samples':samples,'clock_frt_numerator':120093750,'clock_frt_denominator':143,
        'sound_ram_bytes':len(bank),'sound_ram_capacity':0x80000,'fm_slots':list(range(24)),
        'oki_slots':list(range(24,28)),'native_key_execute_writes':len(keyexec),
        'minimum_key_execute_gap_samples':min(b-a for a,b in zip(keyexec,keyexec[1:])),
        'key_edge_policy':'Adaptive FRT deadlines; native writer preserves at least2 SCSP samples between KYONEX writes',
        'exporter_result':result.stderr.strip(),
        'assets':{p.name:{'bytes':p.stat().st_size,'sha256':sha(p)} for p in [OUT/'SOUNDRAM.BIN',OUT/'AUDIOREG.BIN']}}
    (EVIDENCE/'native-audio-assets.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))

if __name__=='__main__':main()
