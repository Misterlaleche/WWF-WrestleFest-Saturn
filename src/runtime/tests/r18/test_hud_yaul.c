#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <yaul.h>
#include "wf_r18_hud.h"
#include "wf_saturn_memory.h"
uint8_t mock_vdp2_vram[0x80000];uint32_t mock_cram[1024];
unsigned mock_screen_priorities[3],mock_display;
static wf_r18_video_t video;static FILE *font;static unsigned reads;
static int read_file(const char*n,uint32_t at,void*out,size_t bytes){
 assert(!strcmp(n,"FONT8.BIN")&&bytes==2048&&at==reads*2048);++reads;
 return fseek(font,(long)at,SEEK_SET)||fread(out,1,bytes,font)!=bytes?-1:0;
}
int main(int argc,char**argv){
 assert(argc==2);font=fopen(argv[1],"rb");assert(font);
 memset(mock_vdp2_vram,0xa5,sizeof mock_vdp2_vram);
 assert(wf_r18_hud_bind(&video,read_file)==WF_OK&&reads==64);
 rewind(font);
 for(unsigned i=0;i<131072;i++)assert(mock_vdp2_vram[0x60000+i]==(unsigned)fgetc(font));
 assert(fgetc(font)==EOF);fclose(font);
 /* Full boot upload changes only font bank and the HUD map window. */
 wf_saturn_cram_reset();assert(wf_r18_hud_commit(&video)==WF_OK);
 assert(wf_saturn_cram_writes==256&&mock_screen_priorities[2]==4);
 wf_r17_fg0_write_t w[]={{0x180003,15,1},{0xc0001,0x34,1},{0xc0003,0x12,1}};
 assert(wf_r18_video_batch(NULL,w,3,&video)==WF_OK);
 assert(wf_r18_hud_commit(&video)==WF_OK&&wf_saturn_cram_writes==257);
 volatile uint16_t *map=(volatile uint16_t*)(mock_vdp2_vram+0x24000);
 assert(map[0]==0x1234&&map[2048]==0x1234&&mock_cram[513]==255);
 assert(wf_r18_hud_commit(&video)==WF_OK&&wf_saturn_cram_writes==257);
 for(unsigned i=0;i<0x60000;i++)if(i<0x24000||i>=0x26000)assert(mock_vdp2_vram[i]==0xa5);
 puts("PASS HUD hardware boundary:64 sector boot reads, exact native font, fixed transparent NBG2, isolated map and CRAM, no repeated colors");
}
