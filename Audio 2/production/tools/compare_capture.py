#!/usr/bin/env python3
"""Extract a native music window and quantify raw-level differences.

The source is Mednafen's real soundrecord WAV. No renderer, gain adjustment,
resampling, time stretching or waveform optimization is applied to that WAV.
Alignment uses the native final SCSP clock and diagnostic command epoch plus
the existing 4096-frame music lead. It is a documented command-clock estimate,
not a claim of sample-exact DAC alignment or owner listening approval.
"""
from pathlib import Path
import argparse,hashlib,json,shutil,struct,wave
import numpy as np
import scipy
from scipy import signal

def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def read_wav(p):
    with wave.open(str(p)) as f:
        if f.getsampwidth()!=2 or f.getframerate()!=44100:raise ValueError('Need PCM16/44100 WAV')
        return np.frombuffer(f.readframes(f.getnframes()),dtype='<i2').reshape(-1,f.getnchannels()).copy()
def main():
    p=argparse.ArgumentParser(description=__doc__)
    for key in ('capture','reference','build','out'):p.add_argument('--'+key,type=Path,required=True)
    p.add_argument('--command',type=lambda x:int(x,0),required=True)
    a=p.parse_args();a.out.mkdir(parents=True,exist_ok=True)
    report=json.loads((a.capture/'report.json').read_text());d=report['final_watch_values']
    build=json.loads((a.build/'saturn-build.json').read_text())
    is_music=1<=a.command<=18
    if not (is_music or 0x20<=a.command<=0x81) or build['command']!=a.command:
        raise ValueError('Original sound command/build mismatch')
    if d.get('magic')!=0x57464132 or d.get('state')!=4:raise ValueError('Native window did not finish')
    native=read_wav(a.capture/'native.wav');reference=read_wav(a.reference)
    ram=(a.capture/'SCSP-RAM-saturn-order.bin').read_bytes()
    clock=struct.unpack_from('>H',ram,0xb010)[0]
    residual=(clock-d['DSPclock16'])&0xffff
    if residual>1475:raise ValueError('Final diagnostic clock is more than two video frames stale')
    command_to_end=d['SCSPelapsed']+residual
    lead=4096 if is_music else 0
    first=len(native)-command_to_end+lead
    if first<0 or first+len(reference)>len(native):raise ValueError('Insufficient captured native samples')
    excerpt=native[first:first+len(reference)]
    identifier=f'{a.command:02d}' if is_music else f'{a.command:02X}'
    ref_name=f'ARCADE-REFERENCIA-{identifier}.wav'
    sat_name=f'SATURN-AUDIO2-{identifier}.wav'
    shutil.copyfile(a.reference,a.out/ref_name)
    with wave.open(str(a.out/sat_name),'wb') as f:
        f.setnchannels(excerpt.shape[1]);f.setsampwidth(2);f.setframerate(44100);f.writeframes(excerpt.astype('<i2').tobytes())
    x=reference.astype(float).mean(axis=1)/32768;y=excerpt.astype(float).mean(axis=1)/32768
    def rms(v):return float(np.sqrt(np.mean(v*v)))
    block=441;count=len(x)//block
    ex=np.sqrt(np.mean(x[:count*block].reshape(-1,block)**2,axis=1))
    ey=np.sqrt(np.mean(y[:count*block].reshape(-1,block)**2,axis=1))
    corr=signal.correlate(ey-ey.mean(),ex-ex.mean(),mode='full')
    lags=signal.correlation_lags(len(ey),len(ex),mode='full');allow=np.abs(lags)<=25
    best=int(lags[allow][np.argmax(corr[allow])])
    freq,px=signal.welch(x,44100,nperseg=4096);_,py=signal.welch(y,44100,nperseg=4096)
    selected=(freq>=40)&(freq<=16000)
    def correlation(left,right):
        if np.std(left)==0 or np.std(right)==0:return None
        return float(np.corrcoef(left,right)[0,1])
    metrics={
      'reference_rms_dbfs':20*np.log10(max(rms(x),1e-12)),
      'saturn_rms_dbfs_analysis_mono':20*np.log10(max(rms(y),1e-12)),
      'saturn_minus_reference_rms_db':20*np.log10(max(rms(y),1e-12)/max(rms(x),1e-12)),
      'reference_peak':int(np.abs(reference.astype(np.int32)).max()),
      'saturn_peak':int(np.abs(excerpt.astype(np.int32)).max()),
      'raw_waveform_correlation':correlation(x,y),
      'raw_waveform_error_rms':rms(y-x),
      'envelope_10ms_correlation':correlation(ex,ey),
      'envelope_best_lag_ms_diagnostic_only':best*10,
      'spectral_centroid_reference_hz':float((freq*px).sum()/px.sum()) if px.sum() else None,
      'spectral_centroid_saturn_hz':float((freq*py).sum()/py.sum()) if py.sum() else None,
      'spectral_log_power_rmse_db_40_to_16000hz':float(np.sqrt(np.mean((10*np.log10(py[selected]+1e-20)-10*np.log10(px[selected]+1e-20))**2))),
      'clipped_reference_samples':int(np.count_nonzero((reference==-32768)|(reference==32767))),
      'clipped_saturn_samples':int(np.count_nonzero((excerpt==-32768)|(excerpt==32767)))}
    result={'command':f'0x{a.command:02X}','seconds':len(reference)/44100,
      'reference_kind':('A53 host software reference; not a physical arcade recording' if is_music else
                        'A15 original-ROM command/sample model; not a physical arcade or full Z80 capture'),
      'saturn_kind':'Real standalone Mednafen 1.32.1 execution',
      'alignment':{'method':'native final SCSP clock minus elapsed since command, plus documented command lead',
        'command_lead_frames':lead,
        'source_start_sample':first,'source_end_sample_exclusive':first+len(reference),
        'final_diagnostic_staleness_frames':residual,'residual_DAC_alignment':'not independently calibrated'},
      'processing':{'normalization':False,'gain':1,'resampling':False,'time_stretch':False,'metric_mono':'arithmetic average only; delivered native WAV remains stereo'},
      'metrics':metrics,'diagnostics':d,
      'identities':{'source_capture_wav':sha(a.capture/'native.wav'),'capture_report':sha(a.capture/'report.json'),
        'reference_wav':sha(a.out/ref_name),'saturn_wav':sha(a.out/sat_name),
        'source_executable':build['files']['AUDIO2.ELF']['sha256'],'source_iso':build['files']['AUDIO2.iso']['sha256']},
      'analysis_dependencies':{'numpy':np.__version__,'scipy':scipy.__version__},
      'acceptance':'PENDING; metrics cannot establish fidelity or owner approval','hardware_status':'HARDWARE PENDING'}
    (a.out/f'COMPARACION-{identifier}.json').write_text(json.dumps(result,indent=2,allow_nan=False)+'\n')
    print(json.dumps(result,indent=2));return 0
if __name__=='__main__':raise SystemExit(main())
