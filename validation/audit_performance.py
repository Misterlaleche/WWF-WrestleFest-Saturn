"""Recompute evidence; never treats emulator profiles as a hardware test."""
from pathlib import Path
import argparse, hashlib, json, statistics, sys, zipfile, difflib

ROOT=Path(__file__).resolve().parents[1]
def sha(data): return hashlib.sha256(data).hexdigest()
def summarize(path):
    d=json.loads(path.read_text(encoding='utf-8'))
    r={k:d[k] for k in ('bin_sha256','program_sha256','cue_sha256','hardware_test','fps','status','pad_policy','last_frame','fatal_owner') if k in d}
    samples=[s for s in d.get('samples',[]) if s['frame']>0]
    if len(samples)>1 and samples[-1]['clock']>samples[0]['clock'] and samples[-1]['frame']>samples[0]['frame']:
        first,last=samples[0],samples[-1]
        frames=last['frame']-first['frame']; edges=last['clock']-first['clock']
        gaps=[b['clock']-a['clock'] for a,b in zip(samples,samples[1:]) if b['frame']==a['frame']+1]
        r['observed']={'first_frame':first['frame'],'last_frame':last['frame'],'frames':frames,'vblanks':edges,'fps_nominal60':60*frames/edges,'fps_core':d['fps']*frames/edges,
            'gap_count':len(gaps),'gap_max':max(gaps,default=0),'gap_p95':sorted(gaps)[int(.95*(len(gaps)-1))] if gaps else None,
            'missed_edge_count':sum(max(0,x-1) for x in gaps),
            'counters_delta':{k:last[k]-first[k] for k in ('texture_bytes','texture_transfers','texture_reused','cd_window_misses') if k in first},
            'counter_warning':'cd_window_misses is window_clock: increments on fetch hits too; not a physical CD-miss count'}
    if d.get('profile_frames'):
        n=d['profile_frames']; v=d['profile_vblanks']
        r['profile']={'frames':n,'vblanks':v,'fps_nominal60':60*n/v,'fps_core':d['fps']*n/v,
            'passes_mean_one_edge':v<=n,'file_calls':d['profile_calls'][7:9],
            'totals_frt128':d['profile_totals'],
            'equivalent_master_cycles_per_frame':[round(t*128/n,2) for t in d['profile_totals']]}
    return r

def main():
    p=argparse.ArgumentParser();p.add_argument('--out',type=Path,required=True);a=p.parse_args();a.out.mkdir(parents=True,exist_ok=True)
    reports={str(f.relative_to(ROOT)):summarize(f) for f in (ROOT/'validation/local-20261025').rglob('report.json') if 'samples' in json.loads(f.read_text())}
    (a.out/'measurements.json').write_text(json.dumps(reports,indent=2)+'\n')
    # Hash all candidate disc images, extract ISO payload independently of names.
    sys.path.insert(0,str(ROOT/'validation'))
    from verify_disc_exact import directory_files
    discs=[]
    paths=list((ROOT/'validation').rglob('wrestlefest-runtime18.bin'))+list((ROOT.parent/'entrega').rglob('wrestlefest-runtime18.bin'))
    for f in paths:
        raw=f.read_bytes(); row={'path':str(f.relative_to(ROOT.parent)),'bytes':len(raw),'sha256':sha(raw)}
        cue=f.with_suffix('.cue');row['cue_sha256']=sha(cue.read_bytes()) if cue.exists() else None
        if len(raw)%2352==0:
            iso=b''.join(raw[i+16:i+2064] for i in range(0,len(raw),2352))
            try:
                files=directory_files(iso)
                row['files']={n:{'bytes':x['size'],'sha256':sha(x['data'])} for n,x in files.items()}
                row['program_sha256']=sha(files['A.BIN']['data'])
                row['matching_reports']=[k for k,v in reports.items() if v.get('bin_sha256')==row['sha256']]
            except (AssertionError,KeyError,ValueError) as e:row['parse_error']=repr(e)
        discs.append(row)
    (a.out/'disc-inventory.json').write_text(json.dumps(discs,indent=2)+'\n')
    # Archives are read in place, with no bulk extraction or ROM redistribution.
    archives=[]
    for f in ROOT.parent.glob('*.zip'):
        with zipfile.ZipFile(f) as z:
            images=[]
            for info in z.infolist():
                if info.filename.lower().endswith(('.cue','/a.bin','/wrestlefest-runtime18.bin')):
                    with z.open(info) as stream:
                        h=hashlib.sha256()
                        while chunk:=stream.read(1024*1024):h.update(chunk)
                    images.append({'name':info.filename,'bytes':info.file_size,'sha256':h.hexdigest()})
            archives.append({'path':f.name,'entries':len(z.infolist()),'images':images})
    (a.out/'archives.json').write_text(json.dumps(archives,indent=2)+'\n')
    # Existing performance changes, isolated against the preserved source freeze.
    base=ROOT/'validation/local-20261024/core-stage9b-final-src'
    candidates=list(base.rglob('wf_r18_present.c'))
    changes=[]
    if candidates:
        oldroot=candidates[0].parent.parent
        names=['src/wf_r18_present.c','src/wf_vdp1_plan.c','include/wf_vdp1_plan.h','src/wf_saturn_yaul_backend.c','include/wf_saturn_yaul_backend.h','include/wf_saturn_memory.h','src/wf_r18_cdfs.c','tests/r18/test_sprite_crop.c']
        for name in names:
            old=oldroot/name;new=ROOT/'src/runtime'/name
            if old.exists() and new.exists():
                changes.extend(difflib.unified_diff(old.read_text().splitlines(True),new.read_text().splitlines(True),fromfile='a/'+name,tofile='b/'+name))
    (a.out/'existing-performance-vs-stage9b.patch').write_text(''.join(changes))
    print(json.dumps({'reports':len(reports),'discs':len(discs),'archives':len(archives),'existing_patch_lines':len(changes)}))
if __name__=='__main__': main()
