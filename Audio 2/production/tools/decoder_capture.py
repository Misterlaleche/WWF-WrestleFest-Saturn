#!/usr/bin/env python3
"""Capture the native decoder probe with unmodified standalone Mednafen1.32.1."""
import argparse
import json
from pathlib import Path
import subprocess
import struct
import sys

p=argparse.ArgumentParser(description=__doc__)
for name in ('build','capture-tool','environment-root','bios','out'):
    p.add_argument('--'+name,type=Path,required=True)
p.add_argument('--display',type=int,default=96)
p.add_argument('--seconds',type=int,default=240)
a=p.parse_args();build=json.loads((a.build/'decoder-build.json').read_text())
command=[sys.executable,str(a.capture_tool.resolve()),'--environment-root',str(a.environment_root.resolve()),
         '--bios',str(a.bios.resolve()),'--cue',str((a.build/'DECODERSAT.cue').resolve()),
         '--out',str(a.out.resolve()),'--display',str(a.display),'--seconds',str(a.seconds),
         '--audio-only','--dump-wram','--ready-watch','host_boot','--ready-value','1',
         '--stop-watch','state','--stop-value','3','--watch',f'host_boot:{build["host_boot"]:#x}:4']
names={0:'magic',1:'state',2:'error',3:'edge_cases',4:'sample_frames',5:'sample_entries',
       6:'baseline_fs',7:'candidate_fs',8:'bench_blocks',9:'baseline_min',10:'baseline_max',
       11:'candidate_min',12:'candidate_max',13:'state_checks',14:'crc_checks',15:'phase',
       16:'failure_at',17:'expected',18:'actual',19:'gain_gate'}
for offset,name in names.items():command+=['--watch',f'{name}:{build["host_diag"]+offset*4:#x}:4']
(a.build/'decoder-capture-command.json').write_text(json.dumps(command,indent=2)+'\n')
with (a.build/'decoder-capture.log').open('w') as log:
    subprocess.run(command,stdout=log,stderr=subprocess.STDOUT,check=True)
r=json.loads((a.out/'report.json').read_text());v=r.get('final_watch_values',{})
summary={'build':build,'capture_sha256':r['emulator_sha256'],'stop_reason':r['stop_reason'],
         'native_state':r.get('native_state'),'observed':v,
         'correctness_pass':v.get('state')==3 and v.get('error')==0 and v.get('sample_frames')==1034226 and v.get('sample_entries')==114,
         'performance_gate_pass':v.get('gain_gate')==1}
if v.get('bench_blocks'):
    summary.update({'baseline_fs_per_128':v['baseline_fs']/v['bench_blocks'],
                    'candidate_fs_per_128':v['candidate_fs']/v['bench_blocks'],
                    'decoder_time_reduction_percent':100*(1-v['candidate_fs']/v['baseline_fs'])})
if 'benchmark_records_address' in build:
    ram=(a.out/'SCSP-RAM-saturn-order.bin').read_bytes();at=build['benchmark_records_address']
    records=[]
    for i in range(4):
        words=struct.unpack_from('>8I',ram,at+i*32)
        record=dict(zip(('samples_per_block','blocks','baseline_fs','candidate_fs','baseline_min','baseline_max','candidate_min','candidate_max'),words))
        if record['blocks']:
            record['baseline_mean_fs']=record['baseline_fs']/record['blocks']
            record['candidate_mean_fs']=record['candidate_fs']/record['blocks']
            record['time_reduction_percent']=100*(1-record['candidate_fs']/record['baseline_fs'])
        records.append(record)
    summary['block_benchmarks']=records
(a.out/'decoder-results.json').write_text(json.dumps(summary,indent=2)+'\n')
print(json.dumps(summary,indent=2))
