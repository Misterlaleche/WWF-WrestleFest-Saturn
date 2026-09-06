#include "arcade_attract_records.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint16_t be16(const uint8_t*p){return (uint16_t)((p[0]<<8)|p[1]);}
static void check_table(const uint8_t*b,unsigned off,const arcade_record_entry_t*r){
 for(unsigned i=0;i<ARCADE_RECORD_ROWS;i++){
  const uint8_t*p=b+off+i*6;assert(r[i].metric_raw==p[0]);assert(!memcmp(r[i].initials,p+1,3));assert(r[i].wrestler_a==p[4]);assert(r[i].wrestler_b==p[5]);
 }
}
int main(void){
 FILE*f=fopen("program.bin","rb");assert(f);uint8_t*b=malloc(0x7000);assert(b);assert(fread(b,1,0x7000,f)==0x7000);fclose(f);
 const uint8_t seq[]={0x4e,0xb9,0x00,0x00,0xae,0x20,0x4e,0xb9,0x00,0x02,0x64,0xe2,0x4e,0xb9,0x00,0x00,0x69,0xc8,0x30,0x3c,0x01,0x00,0x4e,0xb9,0x00,0x00,0x21,0xe6};
 assert(!memcmp(b+0x0b40,seq,sizeof seq));
 assert(!memcmp(b+0x6a26,(const uint8_t[]){0x45,0xf9,0x00,0x1c,0x00,0xc2,0x08,0x39,0x00,0x00,0x00,0x1c,0x01,0x61,0x67,0x06,0x45,0xf9,0x00,0x1c,0x01,0x04},22));
 assert(arcade_record_tables[0].ram_address_low==0x00c2&&arcade_record_tables[1].ram_address_low==0x0104);
 assert(arcade_record_tables[0].hold_ticks==0x100&&arcade_record_tables[1].hold_ticks==0x100);
 assert(arcade_record_tables[0].tilemap_variant_x==0&&arcade_record_tables[1].tilemap_variant_x==8);
 check_table(b,0x6db0,arcade_default_tag_records);check_table(b,0x6df2,arcade_default_royal_records);
 assert(!memcmp(b+0x6d82,(const uint8_t[]){0x41,0xf9,0x00,0x00,0x6d,0xb0,0x43,0xf9,0x00,0x00,0x6d,0xf2,0x45,0xf9,0x00,0x1c,0x00,0xc2,0x47,0xf9,0x00,0x1c,0x01,0x04},24));
 for(unsigned i=0;i<ARCADE_RECORD_ROWS;i++){assert(arcade_record_name_offsets[i]==be16(b+0x6b42+i*2));assert(arcade_record_metric_offsets[i]==be16(b+0x6b58+i*2));}
 free(b);puts("PASS Test255: TAG/Royal attract records tables, state-entry renderer path, 11x6 defaults and row tilemap offsets match ROM");return 0;
}
