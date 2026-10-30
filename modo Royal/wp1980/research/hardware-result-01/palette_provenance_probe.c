/* The genuine palette owner must identify a failing logical ROM address.
 * Only its external ROM reader is fault-injected; payloads come from ROM. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "wf_r18_frontend_palette.h"
static uint8_t rom[0x80000];
static uint32_t fail_at;
static unsigned calls;
static wf_status_t read_rom(void *user,uint32_t at,void *out,size_t bytes)
{
    (void)user;++calls;
    if(at==fail_at)return WF_BLOCKED;
    if(at>sizeof rom||bytes>sizeof rom-at)return WF_BAD_STATE;
    memcpy(out,rom+at,bytes);return WF_OK;
}
int main(int argc,char **argv)
{
    if(argc!=2)return 2;
    FILE *f=fopen(argv[1],"rb");if(!f)return 2;
    int loaded=fread(rom,1,sizeof rom,f)==sizeof rom&&fgetc(f)==EOF;fclose(f);
    if(!loaded)return 2;
    static const uint32_t targets[]={0x2a98u,0x3922u};
    int failed=0;
    for(unsigned i=0;i<2u;++i){
        wf_runtime_t runtime={0};wf_r18_video_t video={0};
        runtime.m68k_regs.valid=1;video.read_rom=read_rom;
        fail_at=targets[i];calls=0;
        wf_status_t status=wf_r18_frontend_palette_2a06(&runtime,&video,0u,1u);
        int ok=status==WF_BLOCKED&&video.failed_address==fail_at;
        printf("{\"case\":\"%s\",\"status\":%u,\"read_calls\":%u,\"expected_address\":%u,\"actual_address\":%u,\"pass\":%s}\n",
            i?"payload":"pointer",status,calls,fail_at,video.failed_address,ok?"true":"false");
        if(!ok)failed=1;
    }
    return failed;
}
