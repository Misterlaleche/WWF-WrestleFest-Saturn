/* Run actual main/frontend from RESET654 through six natural demo matches,
 * the original $8490 title and paid gameplay. Only physical buttons and host
 * console/time boundaries are supplied. The controller is sampled once per
 * field; no original RAM, RNG, actor, credit or scene value is written.
 * inject_title_fault is a negative control for an actual dependency failure.
 */
#define main unused_probe_main
#define __wrap_wf_jo_frontend_run unused_entry_wrapper
#define __wrap_wf_jo_frontend_after_combat_at_level unused_terminal_wrapper
#include "native/jo_main_probe.c"
#undef main
#undef __wrap_wf_jo_frontend_run
#undef __wrap_wf_jo_frontend_after_combat_at_level
static unsigned terminals,entries,credited_entries,input_epoch=~0u,last_key=~0u;
static unsigned wanted_royal,inject_title_fault,credited_frames,last_credited_frame,mode_epoch=~0u;
static unsigned audit_keys(void){
 if(input_epoch==~0u&&terminals&&frontend.driver.phase==WF_R18_DRIVER_AC0&&
    frontend.driver.pc>=0x8300u&&frontend.driver.pc<0x8490u){
  input_epoch=host_fields;fprintf(stderr,"PAD_BEGIN fields=%u ticks=%u pc=%x generation=%u terminals=%u\n",host_fields,frontend.arcade_ticks,frontend.driver.pc,frontend.driver.generation,terminals);
 }
 unsigned keys=0;
 if(input_epoch!=~0u){
  unsigned elapsed=host_fields-input_epoch;
  if(elapsed<18u)keys=JO_KEY_C;
  else if(elapsed>=55u&&elapsed<73u)keys=JO_KEY_START;
  else if(frontend.driver.phase==WF_R18_DRIVER_MODE){
   if(mode_epoch==~0u)mode_epoch=host_fields;
   if((frontend.driver.ram[0x161]&1u)!=wanted_royal)keys=wanted_royal?JO_KEY_LEFT:JO_KEY_RIGHT;
   else if(host_fields-mode_epoch>=16u&&(host_fields&7u)<4u)keys=JO_KEY_A;
  }else if(frontend.driver.phase==WF_R18_DRIVER_ROSTER&&(host_fields&7u)<4u)keys=JO_KEY_A;
 }
 if(keys!=last_key){fprintf(stderr,"PAD fields=%u ticks=%u phase=%u pc=%x keys=%x\n",host_fields,frontend.arcade_ticks,frontend.driver.phase,frontend.driver.pc,keys);last_key=keys;}
 return keys;
}
static bool audit_pad_read(unsigned port,unsigned key){
 static unsigned sampled_field=~0u,sampled_keys;
 if(port)return false;
 if(sampled_field!=host_fields){sampled_field=host_fields;sampled_keys=audit_keys();}
 return (sampled_keys&key)!=0;
}
bool __wrap_jo_is_input_key_pressed(unsigned port,unsigned key){return audit_pad_read(port,key);}
int __real_wf_r18_title_step(wf_r18_title_t*,wf_r18_title_context_t*);
int __wrap_wf_r18_title_step(wf_r18_title_t*t,wf_r18_title_context_t*c){
 if(inject_title_fault&&terminals&&frontend.driver.phase==WF_R18_DRIVER_AC0&&t->pc==0x83a4u){
  t->blocked_dependency=0xdead8490u;return WF_R18_TITLE_BLOCKED;
 }
 return __real_wf_r18_title_step(t,c);
}
wf_status_t __wrap_wf_jo_frontend_run(wf_jo_frontend_t*f){
 fprintf(stderr,"RUN_BEFORE fields=%u phase=%u pc=%x gen=%u handed=%u matchgen=%u blocked=%x/%x\n",host_fields,f->driver.phase,f->driver.pc,f->driver.generation,f->handed_off,f->match.generation,f->blocked_pc,f->blocked_dependency);
 wf_status_t status=__real_wf_jo_frontend_run(f);
 fprintf(stderr,"RUN_AFTER status=%u fields=%u phase=%u pc=%x gen=%u handed=%u matchgen=%u blocked=%x/%x\n",status,host_fields,f->driver.phase,f->driver.pc,f->driver.generation,f->handed_off,f->match.generation,f->blocked_pc,f->blocked_dependency);
 if(status==WF_OK&&f->handed_off){++entries;credited_entries+=!!rt.g.credited_route;if(rt.g.credited_route)host_snapshot("paid-entry");fprintf(stderr,"ENTRY number=%u credited=%u world=%u stage=%u\n",entries,rt.g.credited_route,rt.g.global_007e,rt.g.progression_0162);}
 return status;
}
wf_status_t __real_wf_jo_frontend_after_combat_at_level(wf_jo_frontend_t*,uint32_t,unsigned);
wf_status_t __wrap_wf_jo_frontend_after_combat_at_level(wf_jo_frontend_t*f,uint32_t owner,unsigned high){
 ++terminals;fprintf(stderr,"TERMINAL owner=%x frame=%u fields=%u\n",owner,rt.g.frame_counter,host_fields);
 return __real_wf_jo_frontend_after_combat_at_level(f,owner,high);
}
int main(int argc,char**argv){
 assert(argc==4);host_output=argv[1];wanted_royal=(unsigned)strtoul(argv[2],NULL,0);inject_title_fault=(unsigned)strtoul(argv[3],NULL,0);assert(wanted_royal<=1u);
 void*ram=mmap((void*)0x00200000u,0x100000u,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE,-1,0);assert(ram==(void*)0x00200000u);
 wf_host_assets_init();wf_jo_assets_set_service_hook(service_during_assets,NULL);
 selected_demo=0;selected_royal=0;loaded=1;wf_jo_clock_start(&arcade_clock,host_fields);
 assert(init_platform());assert(init_frontend());wf_host_assets_play();
 while(!failed&&host_fields<30000u&&credited_frames<600u){
  __jo_core_synch();game_frame();wf_host_assets_frame(rt.g.frame_counter);
  if(credited_entries&&running&&rt.g.credited_route&&rt.g.frame_counter!=last_credited_frame){
   last_credited_frame=rt.g.frame_counter;++credited_frames;
  }
 }
 fprintf(stderr,"SUMMARY failed=%u fault=%x/%x fields=%u front=%u/%x ticks=%u state=%u running=%u entries=%u credited=%u terminals=%u gen=%u handed=%u matchgen=%u\n",failed,wf_jo_diagnostic.fault_owner,wf_jo_diagnostic.fault_dependency,host_fields,frontend.driver.phase,frontend.driver.pc,frontend.arcade_ticks,frontend_state,running,entries,credited_entries,terminals,frontend.driver.generation,frontend.handed_off,frontend.match.generation);
 host_snapshot(failed?"blocked":"complete");host_bytes("final","frontend-host.bin",&frontend,sizeof frontend);
 FILE*out=host_file("result","summary.json","w");
 fprintf(out,"{\"failed\":%u,\"driver_phase\":%u,\"driver_pc\":%u,\"driver_dependency\":%u,\"generation\":%u,\"match_generation\":%u,\"terminals\":%u,\"credited_entries\":%u,\"credited_frames\":%u,\"mode\":%u,\"wanted_royal\":%u,\"injected_fault\":%u,\"host_fields\":%u,\"frontend_ticks\":%u}\n",failed,frontend.driver.phase,frontend.driver.pc,frontend.driver.blocked_dependency,frontend.driver.generation,frontend.match.generation,terminals,credited_entries,credited_frames,rt.g.mode_0161,wanted_royal,inject_title_fault,host_fields,frontend.arcade_ticks);assert(!fclose(out));
 if(inject_title_fault)return failed&&frontend.driver.phase==WF_R18_DRIVER_FATAL&&frontend.driver.blocked_dependency==0xdead8490u&&frontend.driver.generation==0u?0:4;
 return failed?2:credited_frames==600u&&(rt.g.mode_0161&1u)==wanted_royal?0:3;
}
