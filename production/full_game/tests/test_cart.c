#include "wf_cart.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
/* The bus model catches mirrored banks, stuck data, accidental 1MiB
 * promotion and un-restored probe contents in the real validator. */
typedef struct { uint32_t memory[16], original[16]; unsigned mirror, reads, writes; int stuck; } bus_t;
static unsigned index_of(uint32_t at) { assert(at>=0x22400000 && at<=0x227ffffc); unsigned o=at-0x22400000; assert((o%0x80000)==0 || (o%0x80000)==0x7fffc); return (o/0x80000)*2+(o%0x80000!=0); }
static uint32_t read_bus(void *p,uint32_t at){bus_t*b=p;unsigned i=index_of(at)%b->mirror;b->reads++;return b->stuck?(b->memory[i]&~1u):b->memory[i];}
static void write_bus(void*p,uint32_t at,uint32_t v){bus_t*b=p;b->writes++;b->memory[index_of(at)%b->mirror]=v;}
static void init(bus_t*b){memset(b,0,sizeof *b);b->mirror=16;for(unsigned i=0;i<16;i++)b->memory[i]=b->original[i]=0x13572460u+i*2;}
int main(void){
 bus_t b;wf_cart_bus_t io={read_bus,write_bus,&b};
 init(&b);assert(wf_cart_validate(0x5c,0x400000,0x22400000,&io));assert(!memcmp(b.memory,b.original,sizeof b.memory));
 const uint32_t ids[]={0,0x5a,0x5b};
 for(unsigned i=0;i<3;i++){init(&b);assert(!wf_cart_validate(ids[i],0x400000,0x22400000,&io));assert(b.reads==0&&b.writes==0);}
 init(&b);assert(!wf_cart_validate(0x5c,0x100000,0x22400000,&io));assert(!b.writes);
 assert(!wf_cart_validate(0x5c,0x400000,0x22580000,&io));assert(!b.writes);
 for(unsigned n=2;n<=8;n*=2){init(&b);b.mirror=n;assert(!wf_cart_validate(0x5c,0x400000,0x22400000,&io));assert(!memcmp(b.memory,b.original,n*4));}
 init(&b);b.stuck=1;assert(!wf_cart_validate(0x5c,0x400000,0x22400000,&io));assert(!memcmp(b.memory,b.original,sizeof b.memory));
 assert(!wf_cart_validate(0x5c,0x400000,0x22400000,0));
 puts("cart: valid4MiB, rejected1MiB/invalid/alias/stuck, contents restored PASS");
}
