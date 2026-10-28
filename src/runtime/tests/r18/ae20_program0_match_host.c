/* Explicit isolated progression fixtures, not naturally won campaigns. */
#include "full_flow_combat_probe.c"
int main(int argc,char**argv){
 assert(argc==2);setvbuf(stdout,NULL,_IOLBF,0);
 FILE*f=fopen(argv[1],"rb");assert(f);assert(fread(h.rom,1,sizeof h.rom,f)==sizeof h.rom);fclose(f);
 for(unsigned test=0;test<2;test++){
  start_case(0,1);unsigned progress=test?9u:4u;
  /* Deliberately declared TEST FIXTURE at the true manual AC0 boundary.
   * This is never used by the input-only natural combat harness. */
  write_be(h.ram+0x162,progress,2);
  assert(wf_r18_match_driver_init(&combat_paid,&h.driver,combat_extension,NULL)==WF_OK);
  wf_status_t st=wf_r18_match_driver_run(&combat_paid);
  printf("EXPLICIT AE20(0) progression fixture%u status=%u pc=%06x dep=%06x flow=%06x seed=%08x\n",progress,st,combat_paid.blocked_pc,combat_paid.blocked_dependency,combat_paid.flow.pc,read_be(h.ram+0x5c,4));
  fflush(stdout);
  assert(st==WF_OK&&combat_paid.flow.pc==0xf8cu);
 }
 puts("PASS explicit progression4/9 manual-frontend fixtures through shared AE20(0)/B2E4 to originalF8C; no natural campaign claim");return 0;
}
