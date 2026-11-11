#include "wf_cart.h"
/* Yaul dram-cart.h: 1MiB is two512KiB banks at22400000/22600000;
 * intervening512KiB windows mirror their bank. SDK area_get uses22580000
 * for its contiguous1MiB view; probes still use canonical physical banks. */
static uint32_t address(unsigned i,unsigned bank_step){return 0x22400000u+(i/2u)*bank_step+((i&1u)?0x7fffcu:0u);}
bool wf_cart_validate(uint32_t id,uint32_t bytes,uint32_t base,const wf_cart_bus_t *b){
 uint32_t saved[16];bool valid=true;
 const bool one_mib=id==0x5au&&bytes==0x100000u;
 if((!one_mib&&(id!=0x5cu||bytes!=0x400000u))||base!=(one_mib?0x22580000u:0x22400000u)||!b||!b->read||!b->write)return false;
 const unsigned probes=one_mib?4u:16u,bank_step=one_mib?0x200000u:0x80000u;
 for(unsigned i=0;i<probes;i++)saved[i]=b->read(b->context,address(i,bank_step));
 for(unsigned pass=0;pass<2;pass++){
  for(unsigned i=0;i<probes;i++){
   uint32_t pattern=0x5a39c6e1u^(0x01010101u*i);
   b->write(b->context,address(i,bank_step),pass?~pattern:pattern);
  }
  for(unsigned i=0;i<probes;i++){
   uint32_t pattern=0x5a39c6e1u^(0x01010101u*i);
   if(b->read(b->context,address(i,bank_step))!=(pass?~pattern:pattern))valid=false;
  }
 }
 for(unsigned i=0;i<probes;i++)b->write(b->context,address(i,bank_step),saved[i]);
 for(unsigned i=0;i<probes;i++)if(b->read(b->context,address(i,bank_step))!=saved[i])valid=false;
 return valid;
}
