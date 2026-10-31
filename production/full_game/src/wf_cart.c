#include "wf_cart.h"
static uint32_t address(unsigned i){return 0x22400000u+(i/2u)*0x80000u+((i&1u)?0x7fffcu:0u);}
bool wf_cart_validate(uint32_t id,uint32_t bytes,uint32_t base,const wf_cart_bus_t *b){
 uint32_t saved[16];bool valid=true;
 if(id!=0x5cu||bytes!=0x400000u||base!=0x22400000u||!b||!b->read||!b->write)return false;
 for(unsigned i=0;i<16;i++)saved[i]=b->read(b->context,address(i));
 for(unsigned pass=0;pass<2;pass++){
  for(unsigned i=0;i<16;i++){
   uint32_t pattern=0x5a39c6e1u^(0x01010101u*i);
   b->write(b->context,address(i),pass?~pattern:pattern);
  }
  for(unsigned i=0;i<16;i++){
   uint32_t pattern=0x5a39c6e1u^(0x01010101u*i);
   if(b->read(b->context,address(i))!=(pass?~pattern:pattern))valid=false;
  }
 }
 for(unsigned i=0;i<16;i++)b->write(b->context,address(i),saved[i]);
 for(unsigned i=0;i<16;i++)if(b->read(b->context,address(i))!=saved[i])valid=false;
 return valid;
}
