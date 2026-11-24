#!/usr/bin/env python3
"""Cold-boot the private save probe in stock Mednafen; observe RAM read-only.

Reusing --profile preserves only normal battery files, never a saved machine
state. All tests must use a private profile created for this task.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import sys
import time

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from run_mednafen import sha,swap,symbols

def locate(pid,mem,raw,diagnostic):
    pattern=swap(raw[0x400:0x500]);second=swap(raw[0x1800:0x1900])
    for line in Path(f'/proc/{pid}/maps').read_text().splitlines():
        parts=line.split()
        if parts[1][:2]!='rw':continue
        begin,end=[int(v,16) for v in parts[0].split('-')]
        if end-begin>256*1024*1024:continue
        for chunk in range(begin,end,2*1024*1024):
            try:data=os.pread(mem,min(2*1024*1024+256,end-chunk),chunk)
            except OSError:continue
            at=data.find(pattern)
            while at>=0:
                base=chunk+at-0x4400
                try:
                    if os.pread(mem,len(second),base+0x5800)==second and swap(
                        os.pread(mem,4,base+diagnostic-0x06000000))==b'WBS1':return base
                except OSError:pass
                at=data.find(pattern,at+1)
    return None

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--disc',type=Path,required=True)
p.add_argument('--profile',type=Path,required=True)
p.add_argument('--out',type=Path,required=True)
p.add_argument('--bios',type=Path,required=True)
p.add_argument('--emulator',type=Path,default=Path('/usr/games/mednafen'))
p.add_argument('--expect',choices=('defaults','loaded','corrupt','full'),required=True)
p.add_argument('--rankings-file',type=Path,
               help='For loaded: compare with records produced by a completed game instead of the seed fixture')
a=p.parse_args();disc=a.disc.resolve();profile=a.profile.resolve();out=a.out.resolve()
out.mkdir(parents=True,exist_ok=False);profile.mkdir(parents=True,exist_ok=True)
syms=symbols(disc/'game.elf');raw=(disc/'game.raw').read_bytes()
command=[str(a.emulator),'-remote','WFREMOTE:','-video.driver','softfb','-video.fs','0',
    '-sound','0','-ss.bios_na_eu',str(a.bios.resolve()),'-ss.region_autodetect','0',
    '-ss.region_default','na','-ss.bios_sanity','1','-ss.cart','auto','-autosave','0',
    '-filesys.fname_sav','%f.%x','-nothrottle','1',str(disc/'game.cue')]
env=dict(os.environ,MEDNAFEN_HOME=str(profile),SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy')
env.pop('LD_PRELOAD',None)
report={'command':command,'cold_boot':True,'guest_writes':False,'state_loaded':False,
    'elf_sha256':sha(disc/'game.elf'),'raw_sha256':sha(disc/'game.raw'),
    'emulator_sha256':sha(a.emulator),'bios_sha256':sha(a.bios),
    'backup_before':{x.name:sha(x) for x in (profile/'sav').glob('*.bkr')},'status':'STARTING'}
proc=None;mem=None;ram=None;values=[]
try:
    with (out/'mednafen.log').open('wb') as log:
        proc=subprocess.Popen(command,env=env,stdin=subprocess.PIPE,stdout=log,stderr=subprocess.STDOUT)
        start=time.monotonic();next_search=0
        while time.monotonic()-start<120:
            if proc.poll() is not None:raise RuntimeError(f'Mednafen exited {proc.returncode}')
            if mem is None:mem=os.open(f'/proc/{proc.pid}/mem',os.O_RDONLY)
            if ram is None and time.monotonic()>=next_search:
                ram=locate(proc.pid,mem,raw,syms['wf_save_probe']);next_search=time.monotonic()+1
            if ram is not None:
                values=list(struct.unpack('>12I',swap(os.pread(mem,48,ram+syms['wf_save_probe']-0x06000000))))
                if values[1]==4:
                    rankings=swap(os.pread(mem,132,ram+syms['wf_save_probe_rankings']-0x06000000))
                    (out/'rankings.bin').write_bytes(rankings)
                    break
            time.sleep(.05)
        report['values']=values;report['wall_seconds']=time.monotonic()-start
        if len(values)!=12 or values[1]!=4:raise AssertionError(f'Probe incomplete: {values}')
        assert values[10]==0,values
        if a.expect=='defaults':assert values[3:8]==[1,5,1,0,1],values
        elif a.expect=='loaded':
            assert values[3]==2 and values[4]==2,values
            if a.rankings_file is None:assert values[6]==1,values
            else:
                expected=a.rankings_file.read_bytes()
                assert len(expected)==132 and rankings==expected
                report['expected_rankings_sha256']=hashlib.sha256(expected).hexdigest()
        elif a.expect=='corrupt':assert values[3:6]==[3,3,1] and values[7:9]==[0,1],values
        else:assert values[3:6]==[1,4,1] and values[7]==1 and values[9]>0,values
        report['status']='PASS'
finally:
    if mem is not None:os.close(mem)
    if proc is not None and proc.poll() is None:
        try:
            proc.stdin.write(b'exit\n');proc.stdin.flush();proc.wait(timeout=20)
        except Exception:proc.terminate();proc.wait(timeout=10)
    report['backup_after']={x.name:sha(x) for x in (profile/'sav').glob('*.bkr')}
    (out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
