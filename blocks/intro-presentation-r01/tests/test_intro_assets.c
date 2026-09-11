#include "wf_intro_presentation.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int argc,char **argv){
    assert(argc==2);FILE *f=fopen(argv[1],"rb");assert(f!=NULL);
    uint8_t *blob=malloc(WF_INTRO_ASSET_BYTES),*bitmap=malloc(WF_INTRO_BITMAP_BYTES),fade[WF_INTRO_PALETTE_BYTES];
    assert(blob&&bitmap);assert(fread(blob,1,WF_INTRO_ASSET_BYTES,f)==WF_INTRO_ASSET_BYTES);assert(fgetc(f)==EOF);fclose(f);
    assert(wf_intro_fnv1a(blob,WF_INTRO_ASSET_BYTES)==0x69055E2BU);
    for(unsigned scene=0;scene<WF_INTRO_SCENE_COUNT;scene++){
        const wf_intro_asset_t *a=&wf_intro_assets[scene];
        assert(a->offset+a->size<=WF_INTRO_ASSET_BYTES);assert(a->palette_offset+WF_INTRO_PALETTE_BYTES<=WF_INTRO_ASSET_BYTES);
        assert(wf_intro_unpack(blob+a->offset,a->size,bitmap,WF_INTRO_BITMAP_BYTES));
        assert(wf_intro_fnv1a(bitmap,WF_INTRO_BITMAP_BYTES)==a->bitmap_fnv1a);
        assert(wf_intro_fnv1a(blob+a->palette_offset,WF_INTRO_PALETTE_BYTES)==a->palette_fnv1a);
        wf_intro_build_fade_palette(blob+a->palette_offset,7U,fade);assert(memcmp(fade,blob+a->palette_offset,WF_INTRO_PALETTE_BYTES)==0);
        wf_intro_build_fade_palette(blob+a->palette_offset,0U,fade);
        for(unsigned i=0;i<256U;i++)assert(fade[i*2U]==0x80U&&fade[i*2U+1U]==0x00U);
    }
    free(bitmap);free(blob);puts("PASS: three exact intro assets, hashes, identity fade, black fade");return 0;
}
