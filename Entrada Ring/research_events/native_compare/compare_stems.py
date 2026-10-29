#!/usr/bin/env python3
"""Compare native-register SCSP model output to original YMFM references."""
from pathlib import Path
import argparse
import json
import numpy as np
from scipy import signal
from scipy.io import wavfile

HERE=Path(__file__).resolve().parent
parser=argparse.ArgumentParser();parser.add_argument('--native-dir',type=Path,default=HERE)
NATIVE=parser.parse_args().native_dir
REF=HERE.parent/'reference_render'
RATE=44100
notes={r['channel']:r for r in json.loads((REF/'manifest.json').read_text())['representative_first_notes']}
def read(path):
 rate,x=wavfile.read(path);assert rate==RATE
 x=x.astype(np.float64)/32768
 return x.mean(axis=1) if x.ndim==2 else x
def envelopes(x):
 x=x[:len(x)//441*441].reshape(-1,441)
 return np.sqrt(np.mean(x*x,axis=1))
def peaks(x,n=6):
 x=x-x.mean();w=np.hanning(len(x));fft=np.abs(np.fft.rfft(x*w,n=131072));freq=np.fft.rfftfreq(131072,1/RATE)
 loc,_=signal.find_peaks(fft);loc=sorted((p for p in loc if freq[p]>=15),key=lambda p:fft[p],reverse=True)[:n]
 return [{'hz':round(float(freq[p]),3),'relative_db':round(float(20*np.log10(max(fft[p],1e-20)/max(fft))),2)} for p in loc]
rows=[]
for ch in [0,1,2,3,4,7]:
 a=read(NATIVE/f'ch{ch}.wav');b=read(REF/'waves'/f'ym_channel_{ch}.wav')
 ea=envelopes(a);eb=envelopes(b);rmsa=float(np.sqrt(np.mean(a*a)));rmsb=float(np.sqrt(np.mean(b*b)))
 row={'channel':ch,'scsp_rms':rmsa,'ym_rms':rmsb,'scsp_over_ym_rms_db':float(20*np.log10(rmsa/rmsb)),
      'rms_envelope_correlation':float(np.corrcoef(ea,eb)[0,1]),
      'rms_envelope_nrmse':float(np.sqrt(np.mean((ea-eb)**2)/np.mean(eb**2))),
      'scsp_peak':float(np.max(np.abs(a))),'ym_peak':float(np.max(np.abs(b)))}
 if ch in notes:
  n=notes[ch];begin=n['start_sample'];end=min(n['keyoff_sample'],begin+22050)
  row['first_passage']={'start':begin,'end':end,'scsp_spectral_peaks':peaks(a[begin:end]),'ym_spectral_peaks':peaks(b[begin:end])}
 rows.append(row)
result={'role':'Host audio comparison only; not physical Saturn timing proof.',
        'scsp_model':'unchanged libretro Beetle Saturn Mednafen SCSP source copied with hashes; state/IRQ adapter only',
        'reference':'MAME0.264 YMFM, actual register writes, original route gain; see reference_render/manifest.json',
        'method':'No gain fit, time stretch, pitch shift, or sample alignment adjustment. RMS envelopes use10ms blocks. Spectrum peaks are not asserted fundamental pitches.',
        'rows':rows}
(NATIVE/'comparison.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2))
