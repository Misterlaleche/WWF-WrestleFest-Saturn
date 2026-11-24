/* Exact adapter BIOS call order with MMIO/function vectors substituted.
 * The low data-read address is independently fixed by original BIOS bytes,
 * checked in test_save_ubc.py, not inferred from the candidate implementation. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static volatile uint16_t test_bbra;
static uintptr_t test_bios_init_entry;
static uintptr_t test_bios_entry(unsigned offset);
#include "adapter-under-test.c"
static unsigned null_access[3],init_calls,disables,enables,entry_calls;
static unsigned find_calls,read_calls,write_calls,verify_calls;
static unsigned saved_mode,failure,opened;
static uint8_t *expected_workspace;
static void data_read(unsigned address) {
 if(test_bbra==0x006cu&&address<0x1000u){
  if(!null_access[0]){null_access[1]=0x7d618u;null_access[2]=0x601559eu;}
  ++null_access[0];
 }
}
void jo_core_disable_reset(void){assert(test_bbra==saved_mode&&!opened);++disables;opened=1;}
void jo_core_enable_reset(void){assert(test_bbra==saved_mode&&opened);++enables;opened=0;}
void jo_getdate(jo_datetime*d){memset(d,0,sizeof *d);}
static void mock_init(void *library,void *work,bios_config_t *config) {
 assert(opened&&library==expected_workspace&&work==expected_workspace+16384u);
 assert(config);++init_calls;data_read(0x5f8u);
}
static unsigned partition(unsigned device,unsigned short part){assert(!device&&!part&&test_bbra==saved_mode);return failure==1u;}
static int stat(unsigned device,unsigned bytes,bios_stat_t*out){assert(!device&&test_bbra==saved_mode&&out);(void)bytes;return failure==2u;}
static int directory(unsigned device,const char*name,unsigned short count,bios_file_t*out) {
 assert(!device&&!strcmp(name,"WFESTSAT001")&&count==1&&test_bbra==saved_mode);++find_calls;
 if(failure==3u)return -1;
 if(failure==4u){memcpy(out->filename,name,12);out->bytes=WF_R18_SAVE_IMAGE_BYTES;return 1;}
 return 0;
}
static int read_file(unsigned device,const char*name,void*out){(void)device;(void)name;(void)out;++read_calls;return 1;}
static int write_file(unsigned device,bios_file_t*file,const void*in,unsigned char mode){
 assert(!device&&file&&in&&!mode&&test_bbra==saved_mode);++write_calls;return failure==5u;
}
static unsigned date(bios_date_t*d){assert(d&&test_bbra==saved_mode);return 0;}
static int verify(unsigned device,const char*name,const void*in){assert(!device&&name&&in&&test_bbra==saved_mode);++verify_calls;return failure==6u;}
static uintptr_t test_bios_entry(unsigned offset) {
 ++entry_calls;assert(test_bbra==saved_mode);
 switch(offset){case 4:return(uintptr_t)partition;case 12:return(uintptr_t)stat;case 28:return(uintptr_t)directory;
 case 20:return(uintptr_t)read_file;case 16:return(uintptr_t)write_file;case 32:return(uintptr_t)verify;case 40:return(uintptr_t)date;default:assert(0);return 0;}
}
int main(void){
 _Alignas(4) uint8_t workspace[WF_JO_SAVE_WORKSPACE_BYTES+4u],rankings[132];
 expected_workspace=workspace;test_bios_init_entry=(uintptr_t)mock_init;
 const unsigned modes[]={0,0x6c,0x44,0xffff};unsigned cases=0;
 for(unsigned m=0;m<sizeof modes/sizeof modes[0];m++)for(unsigned f=0;f<7;f++)for(unsigned prior=0;prior<2;prior++){
  saved_mode=modes[m];test_bbra=saved_mode;failure=f;opened=0;
  memset(rankings,0x15,sizeof rankings);wf_jo_save_t state;wf_jo_save_init(&state,NULL,NULL);
  unsigned expected[3]={prior?7u:0u,prior?0x123456u:0u,prior?0xabcdefu:0u};memcpy(null_access,expected,sizeof expected);
  unsigned before=init_calls,old_entries=entry_calls;
  assert(wf_jo_save_load(&state,rankings,NULL,sizeof workspace)==WF_JO_SAVE_UNAVAILABLE);
  assert(wf_jo_save_load(&state,rankings,workspace,WF_JO_SAVE_WORKSPACE_BYTES-1u)==WF_JO_SAVE_UNAVAILABLE);
  assert(wf_jo_save_load(&state,rankings,workspace+1u,WF_JO_SAVE_WORKSPACE_BYTES)==WF_JO_SAVE_UNAVAILABLE);
  assert(init_calls==before&&entry_calls==old_entries&&test_bbra==saved_mode&&!memcmp(null_access,expected,sizeof expected));
  wf_jo_save_status_t status=wf_jo_save_load(&state,rankings,workspace,WF_JO_SAVE_WORKSPACE_BYTES);
  assert(status==(f==1u||f==2u?WF_JO_SAVE_UNAVAILABLE:f==3u||f==4u?WF_JO_SAVE_CORRUPT:WF_JO_SAVE_DEFAULTS));
  assert(init_calls==before+1u&&!opened&&test_bbra==saved_mode);
  if(memcmp(null_access,expected,sizeof expected)){fprintf(stderr,"FAIL: legitimate BIOS ROM[$5F8] changed Jo NULL evidence\n");return 2;}
  if(status==WF_JO_SAVE_DEFAULTS){
   rankings[0]++;
   status=wf_jo_save_store(&state,rankings,workspace,WF_JO_SAVE_WORKSPACE_BYTES);
   assert(status==(f==5u||f==6u?WF_JO_SAVE_WRITE_ERROR:WF_JO_SAVE_SAVED));
   assert(!opened&&test_bbra==saved_mode&&!memcmp(null_access,expected,sizeof expected));
  }
  if(saved_mode==0x6c){data_read(0);assert(null_access[0]==expected[0]+1u);}
  ++cases;
 }
 assert(disables==enables&&read_calls&&write_calls&&verify_calls&&find_calls);
 printf("PASS UBC: %u mode/failure/prior-fault cases; exact restore, BIOS5F8 excluded, subsequent game NULL detected\n",cases);
}
