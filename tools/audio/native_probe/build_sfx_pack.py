"""Build the resident native SFX pack from the user's ROM and audited tables."""
import argparse,csv,hashlib,json,re,struct,zlib
from pathlib import Path
RATE_NUM=3579545
RATE_DEN=64*253
BASE=0x14000
CAPACITY=0x80000-BASE
def build(rom_path,manifest_path,model_path,output):
    rom=rom_path.read_bytes()
    if len(rom)!=0x80000:raise ValueError('Expected 512 KiB OKI ROM')
    with manifest_path.open(newline='',encoding='utf-8') as source:
        rows=list(csv.DictReader(source))
    expected={(b,i) for b,n in [(0,73),(1,41)] for i in range(1,n+1)}
    if len(rows)!=114 or {(int(r['bank']),int(r['index'])) for r in rows}!=expected:
        raise ValueError('114 exact bank entries required')
    matches=re.findall(r'^ \{(\d+)u,(\d+)u,(\d+)u,(\d+)u,\{(.+)\}\},?$',
                       model_path.read_text(),re.M)
    if len(matches)!=98:raise ValueError('Expected 98 recovered commands')
    commands=bytearray();events=bytearray()
    for pri,bank,count,cleanup,body in matches:
        ev=re.findall(r'\{(\d+)u,(\d+)u,(\d+)u,(\d+)u\}',body)
        pri,bank,count,cleanup=map(int,(pri,bank,count,cleanup))
        if len(ev)!=9 or count>9 or bank>2:raise ValueError('Invalid command')
        commands+=struct.pack('>4BH',pri,bank,count,cleanup,len(events))
        for a in ev[:count]:
            values=list(map(int,a))
            if not 1<=values[0]<=73 or values[2]>15 or values[3]>3:
                raise ValueError('Invalid event')
            events+=bytes(values)
    sample_offset=32
    command_offset=sample_offset+116*8
    event_offset=command_offset+len(commands)
    raw_offset=event_offset+len(events)
    table=bytearray(116*8);raw=bytearray();seen={};audit=[]
    for r in sorted(rows,key=lambda r:(int(r['bank']),int(r['index']))):
        b,sid=int(r['bank']),int(r['index'])
        a,n=int(r['adpcm_start']),int(r['adpcm_bytes'])
        if a<0 or n<=0 or n>65535 or a+n>0x40000 or a+n-1!=int(r['adpcm_end_inclusive']):
            raise ValueError('Sample span')
        encoded=rom[(b<<18)+a:(b<<18)+a+n]
        h=hashlib.sha1(encoded).hexdigest()
        if h!=r['sha1'] or int(r['pcm16_frames'])!=n*2:raise ValueError('ROM mismatch')
        if h not in seen:seen[h]=raw_offset+len(raw);raw+=encoded
        offset=seen[h]
        ticks=(n*2*RATE_NUM+8000*RATE_DEN-1)//(8000*RATE_DEN)
        idx=sid if b==0 else 74+sid
        struct.pack_into('>IHH',table,idx*8,offset,n,ticks)
        audit.append(dict(bank=b,sample=sid,offset=offset,adpcm_bytes=n,ticks=ticks,
                          sha1_adpcm=h))
    if len(seen)!=95:raise ValueError('Expected 95 distinct samples')
    payload=table+commands+events+raw
    payload+=b'\0'*(-len(payload)%2)
    header=struct.pack('>4sHH6I',b'WFSX',1,116,32+len(payload),sample_offset,
                       command_offset,event_offset,raw_offset,zlib.crc32(payload))
    pack=header+payload
    if len(pack)>CAPACITY:raise ValueError('Resident pack exceeds sound RAM')
    output.mkdir(parents=True,exist_ok=True)
    (output/'SFX.DAT').write_bytes(pack)
    report=dict(pack_bytes=len(pack),capacity=CAPACITY,unused_bytes=CAPACITY-len(pack),
                sample_entries=114,distinct_samples=95,commands=98,event_count=len(events)//4,
                raw_adpcm_bytes=len(raw),sha256=hashlib.sha256(pack).hexdigest(),
                source_rom_sha256=hashlib.sha256(rom).hexdigest(),
                model_sha256=hashlib.sha256(model_path.read_bytes()).hexdigest(),samples=audit)
    (output/'sfx-pack.json').write_text(json.dumps(report,indent=2)+'\n')
    return report
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for n in ['rom','manifest','model','out']:p.add_argument('--'+n,type=Path,required=True)
    a=p.parse_args();r=build(a.rom,a.manifest,a.model,a.out)
    print(json.dumps({k:r[k] for k in ['pack_bytes','unused_bytes','event_count','raw_adpcm_bytes']},indent=2))
