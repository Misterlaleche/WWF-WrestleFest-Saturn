#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wf_r18_fault_pages.h"

static uint16_t pixels[512u*240u/2u+4u];

int main(void)
{
    wf_fault_snapshot_t s={0};
    s.owner=0xF4C2u;s.dep=0x12614u;s.frame=0x1581u;
    s.slot=0u;s.actor[0].id=0u;s.actor[0].state=5u;
    s.actor[0].action=0x56u;s.actor[0].target=1u;
    s.actor[1].id=5u;s.trace_action=0x56u;
    s.descriptor=0x19076u;s.callback=0x19086u;
    for(unsigned page=0;page<5u;page++){
        for(unsigned i=0;i<sizeof pixels/sizeof pixels[0];i++)pixels[i]=0xeeeeu;
        wf_fault_draw((volatile uint16_t *)pixels,&s,page);
        assert(pixels[512u*240u/2u]==0xeeeeu);
        assert(pixels[512u*240u/2u+3u]==0xeeeeu);
        unsigned lit=0;
        for(unsigned i=0;i<512u*240u/2u;i++){
            uint8_t hi=(uint8_t)(pixels[i]>>8),lo=(uint8_t)pixels[i];
            assert((hi==1u||hi==3u||hi==4u||hi==5u)&&
                   (lo==1u||lo==3u||lo==4u||lo==5u));
            lit+=(hi!=1u)+(lo!=1u);
            /* The bitmap pitch is 512, but NTSC output is only 320 pixels.
             * Keep text inside CRT margins, not merely inside allocated VRAM. */
            unsigned y=i/256u,x=(i%256u)*2u;
            if(y<16u||y>=224u||x<16u||x>=304u)
                assert(hi==1u&&lo==1u);
        }
        assert(lit>1000u);
        const char *dump=getenv("WF_FAULT_DUMP");
        if(dump){
            char name[256];
            snprintf(name,sizeof name,"%s/page%u.raw",dump,page);
            FILE *file=fopen(name,"wb");assert(file);
            assert(fwrite(pixels,2u,512u*240u/2u,file)==512u*240u/2u);
            assert(fclose(file)==0);
        }
    }
    /* A collision failure used to report only first callback $24408 and no
     * participant. The snapshot must preserve BOTH callbacks and raw pair. */
    {
        wf_fault_snapshot_t c={0};
        volatile uint32_t probe[12]={4,0,7,12,1,3,7,12,0x001c0aec,2,3,0x80000010};
        uint32_t first[15]={0},second[13]={0};
        first[1]=0x24408;second[3]=0x24984;
        wf_fault_capture_collision(&c,0x24062,0x24408,probe,first,second);
        assert(c.collision_valid && c.collision_probe[0]==4 && c.collision_probe[1]==0);
        assert(c.collision_first_callback==0x24408 && c.collision_second_callback==0x24984);
        assert(c.collision_probe[8]==0x001c0aec && c.collision_probe[11]==0x80000010);
        memset(pixels,0,sizeof pixels);wf_fault_draw(pixels,&c,1);
        for(unsigned i=0;i<512u*240u/2u;i++){
            uint8_t hi=(uint8_t)(pixels[i]>>8),lo=(uint8_t)pixels[i];
            unsigned y=i/256u,x=(i%256u)*2u;
            assert((hi==1u||hi==3u||hi==4u||hi==5u)&&(lo==1u||lo==3u||lo==4u||lo==5u));
            if(y<16u||y>=224u||x<16u||x>=304u)assert(hi==1u&&lo==1u);
        }
        static uint16_t expected[512u*240u/2u];
        memset(expected,1,sizeof expected);
        wf_fault_line_t line;wf_fault_reset(&line);
        wf_fault_text(&line,"FIRST 024408 SECOND 024984");wf_fault_row(expected,5,3,&line);
        assert(memcmp(pixels+(18u+5u*14u)*256u,expected+(18u+5u*14u)*256u,7u*256u*sizeof(uint16_t))==0);
        const char *collision_dump=getenv("WF_FAULT_COLLISION_DUMP");
        if(collision_dump){
            FILE *file=fopen(collision_dump,"wb");assert(file);
            assert(fwrite(pixels,2u,512u*240u/2u,file)==512u*240u/2u);
            assert(fclose(file)==0);
        }
        wf_fault_snapshot_t invalid={0};
        wf_fault_capture_collision(&invalid,0x24062,0x241e8,probe,first,second);
        assert(!invalid.collision_valid);
        wf_fault_capture_collision(&invalid,0xf4c2,0x24408,probe,first,second);
        assert(!invalid.collision_valid);
        probe[5]=13;
        wf_fault_capture_collision(&invalid,0x24062,0x24408,probe,first,second);
        assert(!invalid.collision_valid);
        probe[5]=3;
        probe[4]=15;
        wf_fault_capture_collision(&invalid,0x24062,0x24408,probe,first,second);
        assert(!invalid.collision_valid);
    }
    puts("PASS fault pages visible, palette bounded, framebuffer guarded");
}
