#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wf_r18_result_screen.h"

#define CHECK(c) do {if(!(c)){fprintf(stderr,"result-screen:%u: %s\n",__LINE__,#c);return 1;}}while(0)

typedef struct host_probe {
    const uint8_t *rom;
    size_t rom_size;
    unsigned waits,commits,reads,erase_reads;
    unsigned fail_wait,fail_commit,fail_read;
    uint16_t frames[3],texts[2];
    wf_runtime_t *rt;
} host_probe_t;

static wf_status_t read_rom(void *user,uint32_t address,void *out,size_t length){
    host_probe_t *p=user;
    if(++p->reads==p->fail_read)return WF_BLOCKED;
    if(address>p->rom_size||length>p->rom_size-address)return WF_BLOCKED;
    memcpy(out,p->rom+address,length);return WF_OK;
}
static wf_status_t wait_frames(wf_runtime_t *rt,uint16_t count,void *user){
    host_probe_t *p=user;
    if(p->rt!=rt||p->waits>=3)return WF_BAD_STATE;
    p->frames[p->waits++]=count;
    return p->waits==p->fail_wait?WF_BLOCKED:WF_OK;
}
static wf_status_t commit(void *user){
    host_probe_t *p=user;
    if(p->commits>=2)return WF_BAD_STATE;
    p->texts[p->commits++]=(uint16_t)p->rt->m68k_regs.d[0];
    if(p->commits==1)p->erase_reads=p->reads;
    return p->commits==p->fail_commit?WF_BAD_STATE:WF_OK;
}
static void setup(wf_runtime_t *rt,wf_r18_video_t *video,host_probe_t *p,
                  const uint8_t *rom,size_t rom_size){
    memset(rt,0,sizeof *rt);memset(video,0,sizeof *video);memset(p,0,sizeof *p);
    rt->m68k_regs.valid=1;rt->m68k_regs.d[0]=0xabcdeeffu;rt->m68k_regs.d[1]=0x12345678u;
    rt->g.global_0092=0x13579bdfu;
    memset(video->fg0,0x5a,sizeof video->fg0);
    p->rt=rt;p->rom=rom;p->rom_size=rom_size;video->read_rom=read_rom;video->read_user=p;
}

int main(int argc,char **argv){
    static wf_runtime_t rt,before;
    static wf_r18_video_t video;
    host_probe_t p;
    FILE *file;
    uint8_t *rom;
    long length;
    CHECK(argc==2);file=fopen(argv[1],"rb");CHECK(file);
    CHECK(fseek(file,0,SEEK_END)==0);length=ftell(file);CHECK(length>0);
    rewind(file);rom=malloc((size_t)length);CHECK(rom);
    CHECK(fread(rom,1,(size_t)length,file)==(size_t)length);CHECK(fclose(file)==0);

    /* Invalid contracts must not start the presentation or alter caller state. */
    setup(&rt,&video,&p,rom,(size_t)length);before=rt;
    CHECK(wf_r18_result_in_ring(NULL,&video,wait_frames,commit,&p)==WF_BAD_STATE);
    CHECK(wf_r18_result_in_ring(&rt,NULL,wait_frames,commit,&p)==WF_BAD_STATE);
    CHECK(wf_r18_result_in_ring(&rt,&video,NULL,commit,&p)==WF_BAD_STATE);
    CHECK(wf_r18_result_in_ring(&rt,&video,wait_frames,NULL,&p)==WF_BAD_STATE);
    CHECK(!memcmp(&rt,&before,sizeof rt)&&!p.reads&&!p.waits&&!p.commits);
    rt.m68k_regs.valid=0;before=rt;
    CHECK(wf_r18_result_in_ring(&rt,&video,wait_frames,commit,&p)==WF_BAD_STATE);
    CHECK(!memcmp(&rt,&before,sizeof rt)&&!p.reads&&!p.waits&&!p.commits);
    setup(&rt,&video,&p,rom,(size_t)length);video.read_rom=NULL;before=rt;
    CHECK(wf_r18_result_in_ring(&rt,&video,wait_frames,commit,&p)==WF_BLOCKED);
    CHECK(!memcmp(&rt,&before,sizeof rt)&&!p.reads&&!p.waits&&!p.commits);

    /* A blocked physical wait cannot advance into text or later waits. */
    for(unsigned failure=1;failure<=3;failure++){
        setup(&rt,&video,&p,rom,(size_t)length);p.fail_wait=failure;
        CHECK(wf_r18_result_in_ring(&rt,&video,wait_frames,commit,&p)==WF_BLOCKED);
        CHECK(p.waits==failure&&p.commits==failure-1u);
        CHECK((uint16_t)rt.m68k_regs.d[0]==(failure==3?0x40u:0x20u));
        CHECK(rt.g.global_0092==0x13579bdfu);
    }
    /* Text must complete before committing; a failed commit stops immediately. */
    for(unsigned failure=1;failure<=2;failure++){
        setup(&rt,&video,&p,rom,(size_t)length);p.fail_commit=failure;
        CHECK(wf_r18_result_in_ring(&rt,&video,wait_frames,commit,&p)==WF_BAD_STATE);
        CHECK(p.waits==failure&&p.commits==failure);
        CHECK(p.texts[failure-1u]==(failure==1?0x8050u:0x50u));
        CHECK(p.reads>0&&rt.g.global_0092==0x13579bdfu);
    }
    /* A ROM read failure is explicit, with no partial FG write or commit. */
    setup(&rt,&video,&p,rom,(size_t)length);p.fail_read=2;
    CHECK(wf_r18_result_in_ring(&rt,&video,wait_frames,commit,&p)==WF_BLOCKED);
    CHECK(p.waits==1&&!p.commits&&p.reads==2);
    for(size_t i=0;i<sizeof video.fg0;i++)CHECK(video.fg0[i]==0x5a);
    setup(&rt,&video,&p,rom,(size_t)length);p.fail_commit=1;
    CHECK(wf_r18_result_in_ring(&rt,&video,wait_frames,commit,&p)==WF_BAD_STATE);
    {
        static uint8_t erased[0x4000];
        unsigned first_text_reads=p.erase_reads;
        memcpy(erased,video.fg0,sizeof erased);
        setup(&rt,&video,&p,rom,(size_t)length);p.fail_read=first_text_reads+1u;
        CHECK(wf_r18_result_in_ring(&rt,&video,wait_frames,commit,&p)==WF_BLOCKED);
        CHECK(p.waits==2&&p.commits==1&&p.reads==p.fail_read);
        CHECK(!memcmp(erased,video.fg0,sizeof erased));
        CHECK(rt.m68k_regs.d[0]==0xabcd0050u&&rt.m68k_regs.d[1]==0x1234ffffu);
    }

    /* Winner slot two uses +$A0.bit6; a low result-byte bit is not a winner. */
    setup(&rt,&video,&p,rom,(size_t)length);
    wf_set_u8(&rt.actors[0],WF_A_RESULT_LO,0x40u);
    wf_set_u8(&rt.actors[2],WF_A_RESULT,0x40u);rt.g.progression_0162=0x0900u;
    before=rt;
    CHECK(wf_r18_result_in_ring(&rt,&video,wait_frames,commit,&p)==WF_OK);
    CHECK(p.waits==3&&p.commits==2&&p.texts[0]==0x8050u&&p.texts[1]==0x51u);
    CHECK(p.frames[0]==33&&p.frames[1]==33&&p.frames[2]==65);
    CHECK(rt.m68k_regs.d[0]==0xabcd0040u&&rt.m68k_regs.d[1]==0x12340001u);
    CHECK(rt.g.global_0092==0x13579bdfu);
    before.m68k_regs=rt.m68k_regs;CHECK(!memcmp(&before,&rt,sizeof rt));
    setup(&rt,&video,&p,rom,(size_t)length);
    wf_set_u8(&rt.actors[3],WF_A_RESULT,0x40u);rt.g.progression_0162=0xab09u;
    CHECK(wf_r18_result_in_ring(&rt,&video,wait_frames,commit,&p)==WF_OK);
    CHECK(p.waits==3&&p.commits==1&&p.texts[0]==0x8050u);
    CHECK(rt.m68k_regs.d[1]==0x12340000u&&rt.g.global_0092==0x13579bdfu);
    free(rom);
    puts("PASS result-screen HOST: contracts, real text, failure boundaries and preserved state");
    return 0;
}
