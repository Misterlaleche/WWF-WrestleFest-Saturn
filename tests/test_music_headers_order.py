#!/usr/bin/env python3
"""Exercise the real init loop, including changed order with identical headers."""
import argparse, hashlib, json, os, subprocess
from pathlib import Path

def main():
    p=argparse.ArgumentParser();p.add_argument('--output',type=Path,required=True);p.add_argument('--red',action='store_true');p.add_argument('--source',type=Path,default=Path(__file__).resolve().parents[1]);p.add_argument('--assets',type=Path);a=p.parse_args()
    here=Path(__file__).resolve().parent;source=a.source.resolve();root=source;assets=(a.assets or root/'cd').resolve();out=a.output.resolve();out.mkdir(parents=True,exist_ok=True)
    shim=r'''
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define JO_COMPILE_WITH_AUDIO_SUPPORT
#define JO_COMPILE_WITH_FS_SUPPORT
#include "jo/audio.h"
bool jo_audio_prepare_music_from_header(const char *name,const unsigned char *h,unsigned n,unsigned size,jo_audio_prepared_music *p) {
    assert(name && h && p && n==44); const char *cd=getenv("WF_TEST_CD"); assert(cd);
    char path[4096];snprintf(path,sizeof path,"%s/%s",cd,name);
    FILE*f=fopen(path,"rb");assert(f);assert(!fseek(f,0,SEEK_END));assert((unsigned)ftell(f)==size);
    rewind(f);unsigned char expected[44];assert(fread(expected,1,44,f)==44);fclose(f);
    assert(!memcmp(expected,h,44));return jo_audio_prepare_music(name,p);
}
'''
    shim=shim.replace('bool jo_audio_prepare_music_from_header(', 'unsigned table_calls; unsigned table_mask;\nbool jo_audio_prepare_music_from_header(')
    shim=shim.replace('assert(name && h && p && n==44);', 'assert(name && h && p && n==44); unsigned cue; assert(sscanf(name,"MU%2X.WAV",&cue)==1); ++table_calls; table_mask |= 1u<<cue;')
    (out/'edge.c').write_text(shim)
    # Include existing fake SDK and reset() unchanged; replace only its main.
    test='#define jo_audio_prepare_music_from_header existing_prepare_stub\n#define main existing_service_main\n#include "'+str(root/'tests/test_audio.c')+'"\n#undef main\n#undef jo_audio_prepare_music_from_header\n'
    test+='''
extern unsigned table_calls,table_mask;
int main(int argc,char **argv) {
 assert(argc==3);size_t n;uint8_t *data=readfile(argv[1],&n);
 table_calls=table_mask=0;reset(data,n);
 unsigned missing=(unsigned)strtoul(argv[2],NULL,0);
 assert(wf_jo_audio_diagnostic.music_missing_mask==missing);
 assert(wf_jo_audio_diagnostic.music_prepared_mask==(0x7FFFEu & ~missing));
 assert(table_mask==(0x7FFFEu & ~missing));
 assert(table_calls==(missing?16u:18u));free(data);
 printf("PASS adapter init: calls=%u prepared=%08x missing=%08x\\n",table_calls,table_mask,missing);return 0;
}
'''
    (out/'test.c').write_text(test)
    flags=['-std=c11','-O1','-g','-Wall','-Wextra','-Werror','-DWF_JO_AUDIO_HOST','-DWF_JO_ENGINE','-ffunction-sections','-fdata-sections','-Wl,--gc-sections','-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-pie','-no-pie']
    flags+=['-I'+str(x) for x in [source/'port',source/'sdk/jo_engine',root/'port',root/'core/include',root/'core/src']]
    adapter=(source/'port/wf_jo_audio.c').read_text();runs=[]
    for label,changed in [('normal',False),('swapped-identical-headers',True),('invalid-cue-ids',True)]:
        text=adapter
        if changed:
            assert 'music_ids[MUSIC_COUNT]={1,2,3' in text
            replacement='music_ids[MUSIC_COUNT]={255,0,3' if label=='invalid-cue-ids' else 'music_ids[MUSIC_COUNT]={2,1,3'
            text=text.replace('music_ids[MUSIC_COUNT]={1,2,3',replacement,1)
        path=out/(label+'.c');path.write_text(text)
        cmd=['cc',*flags,str(out/'test.c'),str(out/'edge.c'),str(path),str(root/'tests/test_audio_core_hooks.c'),str(root/'core/src/wf_r18_ring.c'),str(root/'core/src/wf_r18_video.c'),'-o',str(out/label)]
        c=subprocess.run(cmd,capture_output=True,text=True);(out/(label+'-compile.log')).write_text(c.stdout+c.stderr);c.check_returncode()
        r=subprocess.run([str(out/label),str(assets/'SFX.DAT'),'6' if changed else '0'],capture_output=True,text=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0','WF_TEST_CD':str(assets)})
        (out/(label+'.log')).write_text(r.stdout+r.stderr);runs.append({'label':label,'rc':r.returncode,'stdout':r.stdout.strip()})
    receipt={'red':a.red,'source_sha256':hashlib.sha256(adapter.encode()).hexdigest(),'runs':runs}
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt))
    assert runs[0]['rc']==0
    assert all(r['rc']!=0 if a.red else r['rc']==0 for r in runs[1:])
if __name__=='__main__':main()
