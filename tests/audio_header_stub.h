/* Validate the real adapter's generated header at the fake SDK boundary.
 * This checks inputs only; SDK parsing is exercised by test_music_headers. */
#ifndef WF_TEST_AUDIO_HEADER_STUB_H
#define WF_TEST_AUDIO_HEADER_STUB_H
static void wf_test_check_music_header(const char *name,const unsigned char *header,
                                       unsigned bytes,unsigned file_size)
{
    static const char hex[]="0123456789ABCDEF";
    assert(name && header && bytes==44 && file_size>=46);
    assert(strlen(name)==8 && name[0]=='M' && name[1]=='U' && !strcmp(name+4,".WAV"));
    const char *high=strchr(hex,name[2]),*low=strchr(hex,name[3]);
    assert(high && low);
    unsigned cue=(unsigned)(high-hex)*16u+(unsigned)(low-hex);
    assert(cue>=1 && cue<=18);
    const char *assets=getenv("WF_TEST_CD");assert(assets && *assets);
    char path[4096];int length=snprintf(path,sizeof path,"%s/%s",assets,name);
    assert(length>0 && (unsigned)length<sizeof path);
    FILE *file=fopen(path,"rb");assert(file);
    assert(!fseek(file,0,SEEK_END));long actual_size=ftell(file);
    assert(actual_size>=0 && (unsigned long)actual_size==file_size);
    rewind(file);unsigned char expected[44];
    assert(fread(expected,1,sizeof expected,file)==sizeof expected);
    assert(!fclose(file) && !memcmp(header,expected,sizeof expected));
}
#endif
