"""Scalar feedback experiment, not a reference renderer or Saturn emulator.

Original pinned-core phase and attenuation drive the same transfer equation.
Only the feedback sample interval changes in the counterfactual.
"""
from pathlib import Path
import argparse,hashlib,json,re,subprocess
import numpy as np
from scipy.signal import welch

HERE=Path(__file__).resolve().parent
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--trace',type=Path,required=True,help='Original full-Z80 command01 trace from the reference wrapper')
parser.add_argument('--opm-dir',type=Path,required=True)
parser.add_argument('--out',type=Path,required=True)
parser.add_argument('--cc',default='cc')
args=parser.parse_args()
B=args.out.resolve()
package=HERE.parents[1]
if B==package or package in B.parents:
    raise ValueError('Generated original operator states must remain outside the source package')
P=args.opm_dir.resolve()/'opm.c'
header=P.with_name('opm.h')
if hashlib.sha256(header.read_bytes()).hexdigest()!='186e042d1cd80c8fdf2e1a07d7c123bbe9a0d42e39518c50889db140a9ec4c39':
    raise ValueError('Pinned OPM header identity mismatch')
raw=P.read_bytes()
assert hashlib.sha256(raw).hexdigest()=='983beecc471aebd8884fcc3aa48c50b1a26d4280ddbf1025b5196b540cf24188'
B.mkdir(parents=True,exist_ok=False)
exe=B/'trace_original_feedback'
build=[args.cc,'-std=c99','-O2','-I',str(P.parent),str(HERE/'trace_original_feedback.c'),str(P),'-o',str(exe)]
subprocess.run(build,check=True)
with (B/'original-m1-feedback-state.csv').open('w') as stream:
    subprocess.run([str(exe),str(args.trace.resolve())],stdout=stream,check=True)
s=raw.decode()
def table(name):
    return [int(v,0) for v in re.findall(r'0x[0-9a-f]+|\d+',re.search(name+r'\[256\] = \{(.*?)\};',s,re.S)[1])]
ls,ex=table('logsinrom'),table('exprom')
def transfer(phase,level):
    quarter=(phase&255)^(255 if phase&256 else 0)
    attenuation=min(4095,ls[quarter]+4*level)
    value=(ex[attenuation&255]<<2)>>(attenuation>>8)
    return -value if phase&512 else value

d=np.genfromtxt(B/'original-m1-feedback-state.csv',delimiter=',',names=True,dtype=np.int64)
calculated=np.array([transfer((int(r['phase10'])+int(r['modulation']))&1023,int(r['attenuation'])) for r in d])
mod=np.array([0 if r['feedback']==0 else ((int(r['previous0'])+int(r['previous1']))>>1)>>(9-int(r['feedback'])) for r in d])
observed_mod=(d['modulation']+32768)%65536-32768
report={'kind':'ORIGINAL_OPERATOR_SCALAR_RECURRENCE_DIAGNOSTIC_NOT_REFERENCE_AUDIO',
 'OPM_source_sha256':hashlib.sha256(raw).hexdigest(),
 'input_trace_sha256':hashlib.sha256(args.trace.read_bytes()).hexdigest(),
 'probe_source_sha256':hashlib.sha256((HERE/'trace_original_feedback.c').read_bytes()).hexdigest(),
 'build_command':build,
 'state_trace_sha256':hashlib.sha256((B/'original-m1-feedback-state.csv').read_bytes()).hexdigest(),
 'samples':len(d),'transfer_output_mismatch':int(np.count_nonzero(calculated!=d['output'])),
 'feedback_formula_mismatch':int(np.count_nonzero(mod!=observed_mod))}
if report['transfer_output_mismatch'] or report['feedback_formula_mismatch']:
    print(json.dumps(report,indent=2));raise SystemExit(1)

t=(d['cycle']-d['origin']-18)/3579545
segment=(t>=.015)&(t<.31)
r=d[segment];times=t[segment]
if len(r)<1000 or not np.all(r['feedback']==7) or not np.all(np.diff(r['cycle'])==64):
    raise ValueError('Need original command01/channel1 first-note feedback7 window')
hist=[int(r[0]['previous0']),int(r[0]['previous1'])]
out=[]
for row in r:
    m=((hist[0]+hist[1])>>1)>>(9-int(row['feedback']))
    v=transfer((int(row['phase10'])+m)&1023,int(row['attenuation']))
    hist=[v,hist[0]];out.append(v)
report['free_recurrence_same_rate_mismatch']=int(np.count_nonzero(np.array(out)!=r['output']))
if report['free_recurrence_same_rate_mismatch']:
    print(json.dumps(report,indent=2));raise SystemExit(1)

phase=np.unwrap(r['phase20']*2*np.pi/1048576)*1048576/(2*np.pi)
if not np.all((np.diff(phase)>0)&(np.diff(phase)<100000)):
    raise ValueError('Selected first-note phase window contains a reset or incompatible pitch')
def metrics(y,ty,fs):
    x=np.asarray(y)[(ty>=.04)&(ty<.21)]
    f,p=welch(x,fs,nperseg=min(4096,len(x)),nfft=131072)
    peaks=np.where((p[1:-1]>p[:-2])&(p[1:-1]>p[2:]))[0]+1
    high=sorted((i for i in peaks if f[i]>10000),key=lambda i:p[i],reverse=True)[:6]
    top=sorted(peaks,key=lambda i:p[i],reverse=True)[:8]
    return {'rate_hz':fs,'centroid_hz':float(np.sum(f*p)/np.sum(p)),
            'rms_operator_units':float(np.sqrt(np.mean(x*x))),
            'top_peaks_hz_power':[[float(f[i]),float(p[i])] for i in top],
            'high_peaks_hz_power':[[float(f[i]),float(p[i])] for i in high],
            'power_above10k_fraction':float(np.sum(p[f>10000])/np.sum(p))}
report['original']=metrics(r['output'],times,3579545/64)
fs=44100
nt=np.arange(times[0],times[-1],1/fs)
np_phase=np.interp(nt,times,phase)
np_level=np.interp(nt,times,r['attenuation'])
hist=[int(r[0]['previous0']),int(r[0]['previous1'])]
native_interval=[]
for pp,ll in zip(np_phase,np_level):
    m=((hist[0]+hist[1])>>1)>>2
    v=transfer(((int(pp)>>10)+m)&1023,int(round(ll)))
    hist=[v,hist[0]];native_interval.append(v)
report['counterfactual_44k1_interval_only']=metrics(native_interval,nt,fs)
report['limitations']=['Original physical phase trajectory and attenuation are interpolated onto44.1kHz. Only feedback history interval changes; feedback7 remains original.',
 'This scalar counterfactual does not include SCSP amplitude, interpolation, or envelope equations and is not a Saturn capture.',
 'The real native capture is compared separately. This is a causal mechanism experiment, not a fidelity score.']
(B/'feedback-interval-experiment.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
