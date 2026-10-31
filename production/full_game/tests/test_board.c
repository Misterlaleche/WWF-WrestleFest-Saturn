#include "wf_board.h"
#include <assert.h>
#include <stdio.h>
static unsigned levels[32],count;
static bool irq(void*u,unsigned n){wf_board_t*b=u;levels[count++]=n;return wf_board_write(b,n==3?0x140000:0x140002,0,2);}
int main(void){
 wf_board_t b;wf_board_init(&b,0xef,0xff);
 uint32_t v;assert(wf_board_read(&b,0x140024,&v,2)&&v==0xefff);
 wf_board_input(&b,0,0x91,1);assert(wf_board_read(&b,0x140020,&v,2)&&v==0xfe6e);
 assert(wf_board_read(&b,0x140021,&v,1)&&v==0x6e);
 assert(!wf_board_read(&b,0x140027,&v,2));assert(!wf_board_read(&b,0xffffffff,&v,4));
 assert(wf_board_write(&b,0x100000,0x12345678,4));assert(wf_board_read(&b,0x100000,&v,4)&&v==0x12345678);
 assert(!wf_board_write(&b,0x14001f,1,2));
 assert(wf_board_write(&b,0x14000c,0x3132,2)&&b.sound_count==1&&b.last_sound==0x3132);
 assert(wf_board_advance(&b)==0);assert(b.irq2);assert(!wf_board_high(&b));
 assert(wf_board_dispatch(&b,0x2700,irq,&b)&&count==0&&b.irq2);
 assert(wf_board_dispatch(&b,0x2000,irq,&b)&&count==1&&!b.irq2);
 count=0;
 for(unsigned i=0;i<18;i++){
  unsigned line=wf_board_advance(&b);
  if(line==8)assert(wf_board_high(&b));
  if(line==248)assert(!wf_board_high(&b));
  assert(wf_board_dispatch(&b,0x2000,irq,&b));
 }
 assert(b.line==256);assert(count==17); /* IRQ2 lines16..256 (16), IRQ3 at248. */
 assert(levels[15]==3&&levels[16]==2);
 b.irq2=b.irq3=true;count=0;
 assert(wf_board_dispatch(&b,0x2200,irq,&b)&&count==1&&levels[0]==3&&b.irq2);
 assert(wf_board_dispatch(&b,0x2000,irq,&b)&&count==2&&levels[1]==2);
 puts("board: active-low VBlank, ordered raster/vblank, masks/ACK, DIP/input and bounded bus PASS");
}
