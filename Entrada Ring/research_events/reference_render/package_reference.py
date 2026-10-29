#!/usr/bin/env python3
"""Create validation-only per-channel WAVs and analysis from YMFM native output."""
from pathlib import Path
import csv
import hashlib
import json
import math
import numpy as np
from scipy import signal
from scipy.io import wavfile

HERE=Path(__file__).resolve().parent
OUT=HERE/'waves';OUT.mkdir(exist_ok=True)
NATIVE=55930;RATE=44100;COUNT=313219;START=22.10816
raw=np.fromfile(HERE/'entrance_stems.f32le',dtype='<f4').reshape(-1,9)
audio=signal.resample_poly(raw,4410,5593,axis=0,padtype='constant')[:COUNT]
assert len(audio)==COUNT
names=[f'ym_channel_{i}' for i in range(8)]+['ym_mono_mix']
manifest={'role':'Validation-only reference waveforms. Never use as Saturn runtime assets.',
          'implementation':'Unmodified YMFM from MAME tagmame0264; source hashes in ymfm/provenance.json',
          'clock_hz':3579545,'native_rate_integer_hz':NATIVE,'resampled_rate_hz':RATE,
          'native_first_sample_index':1236510,
          'native_first_sample_offset_seconds':1236510/NATIVE-START,
          'samples':COUNT,'start_original_seconds':START,
          'window_duration_seconds':COUNT/RATE,
          'routes':'original MAME0.264 ddragon3.cpp YMleft0.45+YMright0.45 into mono',
          'native_stem_tap_validation':'0 output mismatches versus unmodified ym2151::generate over all rendered native samples from boot through29.210624s',
          'limitations':['Channel stems are before the common YM3012 DAC quantization; the mixed FM file is after that quantization.',
                         'WAV conversion uses scipy resample_poly FIR; MAME mixer/resampler details may differ, so these WAVs are not claimed byte-identical to MAME WAV output.',
                         'Original register times are applied at the first following YM output sample (<=17.88us quantization).',
                         'PCM16 WAV clips only if amplitude exceeds numeric full scale; peak/clipped counts are reported. Float WAV retains the full signal.'],
          'channels':[]}
for ch,name in enumerate(names):
    x=audio[:,ch];floatpath=OUT/(name+'_float.wav');path=OUT/(name+'.wav')
    wavfile.write(floatpath,RATE,x.astype(np.float32))
    s=np.clip(np.rint(x*32768),-32768,32767).astype('<i2');wavfile.write(path,RATE,s)
    manifest['channels'].append({'name':name,'peak':float(np.max(np.abs(x))),
        'rms':float(np.sqrt(np.mean(x*x))),'clipped_samples':int(np.sum(np.abs(x)>=1)),
        'pcm16_sha256':hashlib.sha256(path.read_bytes()).hexdigest()})

events=json.loads((HERE.parent/'export/entrance_audio_events.json').read_text())['events']
firstnotes=[]
for ch in [0,1,2,7]:
    keys=[e for e in events if e['chip']=='YM' and e['address']==8 and e['value']&7==ch]
    ons=[e for e in keys if e['value']>>3==15 and e['sample']>=783]
    on=ons[0]['sample']
    off=next((e['sample'] for e in keys if e['sample']>on and e['value']>>3==0),COUNT)
    nxt=next((e['sample'] for e in ons if e['sample']>on),COUNT)
    end=min(nxt,off+RATE//4,COUNT)
    x=audio[on:end,ch]
    stem=f'note_ch{ch}_first'
    wavfile.write(OUT/(stem+'.wav'),RATE,np.clip(np.rint(x*32768),-32768,32767).astype('<i2'))
    # Hann FFT over the held part, useful for timbre/pitch comparisons without
    # mistaking the strongest FM harmonic for the fundamental note.
    held=audio[on:min(off,on+RATE//2),ch]
    if len(held)>=64:
        fft=np.abs(np.fft.rfft(held*np.hanning(len(held)),n=65536))
        freq=np.fft.rfftfreq(65536,1/RATE)
        peaks,_=signal.find_peaks(fft)
        peaks=sorted(peaks,key=lambda p:fft[p],reverse=True)[:12]
        peaks=[{'hz':float(freq[p]),'relative_db':float(20*np.log10(max(fft[p],1e-20)/max(fft)))} for p in peaks]
    else:peaks=[]
    firstnotes.append({'channel':ch,'start_sample':on,'keyoff_sample':off,'end_before_next_note_sample':end,
                       'file':stem+'.wav','spectral_peaks_not_fundamental_claims':peaks})
manifest['representative_first_notes']=firstnotes

# RMS envelopes are deterministic reference data for calibration; no matching
# or normalization of the Saturn result is applied here.
block=441
with (HERE/'rms_envelopes_10ms.csv').open('w') as f:
    w=csv.writer(f);w.writerow(['time_seconds']+names)
    for i in range(0,len(audio),block):
        x=audio[i:i+block]
        w.writerow([i/RATE]+[float(v) for v in np.sqrt(np.mean(x*x,axis=0))])
(HERE/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
print(json.dumps({'channels':manifest['channels'],'first_notes':firstnotes},indent=2))
