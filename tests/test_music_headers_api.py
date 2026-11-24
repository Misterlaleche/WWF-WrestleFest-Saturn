#!/usr/bin/env python3
"""Extract actual SDK preparation and parser bodies; replace only their FS edge."""
import argparse, hashlib, json, os, re, subprocess
from pathlib import Path

def function(s, name):
    m = re.search(r'^(?:static\s+)?(?:__jo_force_inline\s+)?(?:unsigned int|bool)\s+' + name + r'\([^;]+?\)\s*\{', s, re.M)
    if not m: raise ValueError(name)
    depth, end = 1, m.end()
    while depth:
        depth += (s[end] == '{') - (s[end] == '}'); end += 1
    return s[m.start():end]

PREFIX = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define JO_ZERO(x) ((x)=0)
#define JO_NULL NULL
#define JO_SCSP_BASE_SAMPLING_RATE 44100
#define JO_AUDIO_MUSIC_MAX_BLOCK 4096
#define JO_AUDIO_MUSIC_SECTOR_SIZE 2048
#define JO_DEFAULT_AUDIO_VOLUME 127
#define __jo_force_inline inline
static unsigned actual_size, actual_fad, reads, opens, closes;
static bool lookup_missing, open_failure;
static int read_length;
static unsigned char file_sector[2048];
static int __jo_fs_get_file_id(const char *name) { assert(name); return lookup_missing ? -1 : 7; }
static void *__jo_fs_open(int id) { assert(id==7); if(open_failure)return NULL; ++opens; return &actual_size; }
static unsigned __jo_fs_get_fad(void *h) { assert(h==&actual_size); return actual_fad; }
static int __jo_fs_get_file_size(void *h) { assert(h==&actual_size); return (int)actual_size; }
static void __jo_fs_close(void *h) { assert(h==&actual_size); ++closes; }
static int __jo_fs_read_sectors(void *h,int count,void *out,int bytes) {
    assert(h==&actual_size && count==1 && bytes==2048); ++reads;
    if(read_length>0)memcpy(out,file_sector,(unsigned)read_length); return read_length;
}
'''

def main():
    p=argparse.ArgumentParser(); p.add_argument('--source',type=Path,default=Path(__file__).resolve().parents[1]);p.add_argument('--assets',type=Path)
    p.add_argument('--output',type=Path,default=Path(__file__).resolve().parents[1]/'build/music-headers-api');p.add_argument('--red',action='store_true');a=p.parse_args();a.assets=(a.assets or a.source/'cd').resolve()
    s=(a.source/'sdk/jo_engine/audio.c').read_text(); h=(a.source/'sdk/jo_engine/jo/audio.h').read_text()
    enum=s.index('typedef enum\n{\n    JoAudioMusicPcm8'); end=s.index('\nstatic unsigned char',enum)
    prep=h.index('typedef struct\n{\n    unsigned int fad, data_start'); pe=h.index('} jo_audio_prepared_music;',prep)+len('} jo_audio_prepared_music;')
    body=PREFIX+s[enum:end]+h[prep:pe]+'\nstatic bool __jo_audio_music_pending;\nstatic unsigned __jo_audio_music_sector[512];\n'
    names=['jo_audio_wav_value','jo_audio_wav_format','jo_audio_music_header']
    if 'static bool jo_audio_prepare_music_header' in s:names+=['jo_audio_prepare_music_header']
    names+=['jo_audio_prepare_music']
    if not a.red:names+=['jo_audio_prepare_music_from_header']
    for name in names:body+='\n'+function(s,name)+'\n'
    fixture=Path(__file__).parent/'fixtures/music_headers/original_prepare.c'
    provenance=json.loads(fixture.with_name('provenance.json').read_text())
    assert hashlib.sha256(fixture.read_bytes()).hexdigest()==provenance['fixture_sha256']
    body+='\n'+fixture.read_text()+'\n'
    body+= ('static bool supplied(const char*n,const unsigned char*h,unsigned l,unsigned z,jo_audio_prepared_music*p)'
            '{(void)h;(void)l;(void)z;return jo_audio_prepare_music(n,p);}\n' if a.red else '#define supplied jo_audio_prepare_music_from_header\n')
    test=Path(__file__).with_name('test_music_headers_api.c');body+=test.read_text()
    out=a.output.resolve();out.mkdir(parents=True,exist_ok=True);src=out/'probe.c';src.write_text(body)
    cmd=['cc','-std=c11','-O1','-g','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-pie','-no-pie',str(src),'-o',str(out/'probe')]
    c=subprocess.run(cmd,capture_output=True,text=True);(out/'compile.log').write_text(c.stdout+c.stderr);c.check_returncode()
    r=subprocess.run([str(out/'probe'),str(a.assets.resolve()),str(int(a.red))],capture_output=True,text=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
    (out/'run.log').write_text(r.stdout+r.stderr)
    receipt={'returncode':r.returncode,'red_control':a.red,'result':r.stdout.strip(),'source_sha256':hashlib.sha256(s.encode()).hexdigest(),'probe_sha256':hashlib.sha256(src.read_bytes()).hexdigest(),'command':cmd,'scope':'Actual SDK functions; instrumented FS boundary, no target timing'}
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt))
    if a.red:assert r.returncode==1 and '"supplied_reads":18' in r.stdout
    else:r.check_returncode()
if __name__=='__main__':main()
