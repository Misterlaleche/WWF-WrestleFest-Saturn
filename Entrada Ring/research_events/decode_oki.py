#!/usr/bin/env python3
"""Decode original individual OKIM6295 samples, retaining all 12 signal bits."""
from pathlib import Path
import hashlib
import json
import math
import struct
import subprocess
import wave

HERE=Path(__file__).resolve().parent
OUT=HERE/'export'
REFERENCE=HERE.parent/'research_inventory/mame0264'
steps=[math.floor(16*(11/10)**i) for i in range(49)]
shift=(-1,-1,-1,-1,2,4,6,8)

def decode(data):
    signal=0;index=0;result=[]
    for byte in data:
        for nib in (byte>>4,byte&15):
            step=steps[index]
            diff=step//8
            if nib&1:diff+=step//4
            if nib&2:diff+=step//2
            if nib&4:diff+=step
            signal=max(-2048,min(2047,signal+(-diff if nib&8 else diff)))
            index=max(0,min(48,index+shift[nib&7]))
            result.append(signal*16)
    return result

verify=HERE/'verify_oki';verify.mkdir(exist_ok=True)
(verify/'emu.h').write_text('#include <cstdint>\n#include <cmath>\n')
(verify/'main.cpp').write_text('''#include <cstdint>
#include <cstdio>
#include "okiadpcm.h"
int main(int ac,char**av){
 if(ac!=3)return 2;FILE*i=fopen(av[1],"rb"),*o=fopen(av[2],"wb");if(!i||!o)return 3;
 oki_adpcm_state s;int b;
 while((b=fgetc(i))!=EOF)for(int shift=4;shift>=0;shift-=4){
  uint16_t v=static_cast<uint16_t>(s.clock((b>>shift)&15)*16);
  fputc(v>>8,o);fputc(v&255,o);
 }
 fclose(i);fclose(o);return 0;
}
''')
subprocess.run(['g++','-std=c++17','-O2','-I',str(verify),'-I',str(REFERENCE),
                str(verify/'main.cpp'),str(REFERENCE/'okiadpcm.cpp'),'-o',str(verify/'mame_oki_decode')],check=True)
meta=json.loads((OUT/'oki_samples.json').read_text())
for row in meta:
    sid=row['sample_id'];stem=f'oki_bank0_sample{sid:03d}'
    data=(OUT/(stem+'.adpcm')).read_bytes()
    pcm=decode(data);raw=struct.pack('>'+str(len(pcm))+'h',*pcm)
    (OUT/(stem+'.pcm16be')).write_bytes(raw)
    with wave.open(str(OUT/(stem+'.wav')),'wb') as wav:
        wav.setnchannels(1);wav.setsampwidth(2);wav.setframerate(8000)
        wav.writeframes(struct.pack('<'+str(len(pcm))+'h',*pcm))
    ref=verify/(stem+'.pcm16be')
    subprocess.run([str(verify/'mame_oki_decode'),str(OUT/(stem+'.adpcm')),str(ref)],check=True)
    assert ref.read_bytes()==raw
    row.update({'pcm_format':'PCM16BE','pcm_sha256':hashlib.sha256(raw).hexdigest(),
                'signal_initial':0,'index_initial':0,'native_signal_bits':12,
                'pcm_scaling':'original signed 12-bit signal *16; no clipping or normalization',
                'mame0264_decoder_byte_exact':True,'pcm_bytes':len(raw),
                'original_volume_nibble':3 if sid==1 else 2,
                'original_volume_gain':11/32 if sid==1 else 16/32,
                'original_mame_mono_route_gain':0.90})
(OUT/'oki_samples.json').write_text(json.dumps(meta,indent=2)+'\n')
print(json.dumps(meta,indent=2))
