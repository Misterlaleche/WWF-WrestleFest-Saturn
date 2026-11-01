"""Prepare private host inputs and a generated trace copy; leave R04 intact."""
import argparse
import hashlib
import json
from pathlib import Path
import re
from zipfile import ZipFile

PINNED = {
    'opm.c': '983beecc471aebd8884fcc3aa48c50b1a26d4280ddbf1025b5196b540cf24188',
    'opm.h': '186e042d1cd80c8fdf2e1a07d7c123bbe9a0d42e39518c50889db140a9ec4c39',
    'z80.c': 'd82e99d025b4ec0d0d0dac29ef83e4bb41c7d2fc15592e13be4057bbf17f0698',
    'z80.h': 'd27e46daf7d3c661ace78918fe5315632bc82c79e351126709c541b80e01f5aa',
}
R04_SHA = 'f1500df193b7aec9470b965fdd72c2aa3784b54f8457ea3b5fe45bf1443892a6'


def prepare(repo, deps, candidate, output):
    hashes = {}
    for name, expected in PINNED.items():
        actual = hashlib.sha256((deps / name).read_bytes()).hexdigest()
        if actual != expected:
            raise ValueError(f'Pinned dependency mismatch: {name}: {actual}')
        hashes[name] = actual
    source_path = repo / 'src/audio/livevm/wf_audio_live_vm.c'
    source = source_path.read_bytes().replace(b'\r\n', b'\n')
    if hashlib.sha256(source).hexdigest() != R04_SHA:
        raise ValueError('R04 source differs from the audited version')
    text = source.decode('utf-8')
    needle = 'static void wb(void*u,uint16_t a,uint8_t val){ wf_audio_live_vm_t*v=u;'
    if text.count(needle) != 1:
        raise ValueError('R04 write hook signature changed')
    text = text.replace(needle, needle + '\n    wf_probe_capture(v,a,val);', 1)
    needle = 'if(v->ym_ctl&8u)z80_gen_int(&v->cpu,0xff);'
    if text.count(needle) != 1:
        raise ValueError('R04 timer hook signature changed')
    text = text.replace(needle, 'if(v->ym_ctl&8u){wf_probe_irq(v);z80_gen_int(&v->cpu,0xff);}', 1)
    text = text.replace('#include <string.h>', '#include <string.h>\n'
        'extern void wf_probe_capture(wf_audio_live_vm_t*,uint16_t,uint8_t);\n'
        'extern void wf_probe_irq(wf_audio_live_vm_t*);', 1)
    output.mkdir(parents=True, exist_ok=True)
    (output / 'wf_audio_live_vm_traced.c').write_text(text, encoding='utf-8', newline='\n')
    with ZipFile(candidate) as zipped:
        pcm = zipped.read('FINAL/source/cd/AUDPCM.PAK')
        index = zipped.read('FINAL/source/wf_audio_pcm_index.c').decode('utf-8')
    bank0 = index.split('wf_pcm_bank0[74]={', 1)[1].split('};', 1)[0]
    entries = re.findall(r'\{(\d+)u,(\d+)u\}', bank0)
    if len(entries) != 74 or len(pcm) != 980196:
        raise ValueError('Candidate AUDPCM/index does not match R04 input contract')
    for sector, count in entries:
        if int(sector) * 2048 + int(count) > len(pcm):
            raise ValueError('AUDPCM index outside the supplied pack')
    (output / 'AUDPCM.PAK').write_bytes(pcm)
    (output / 'bank0.csv').write_text(''.join(
        f'{sample},{int(sector)*2048},{count}\n'
        for sample, (sector, count) in enumerate(entries)), encoding='ascii')
    hashes.update(r04_source_lf_sha256=R04_SHA,
                  audpcm_sha256=hashlib.sha256(pcm).hexdigest(),
                  generated_trace_copy_sha256=hashlib.sha256(text.encode()).hexdigest())
    (output / 'provenance.json').write_text(json.dumps(hashes, indent=2) + '\n', encoding='utf-8')
    return hashes


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', type=Path, required=True)
    parser.add_argument('--deps', type=Path, required=True)
    parser.add_argument('--candidate', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    args = parser.parse_args()
    print(json.dumps(prepare(args.repo, args.deps, args.candidate, args.out), indent=2))
