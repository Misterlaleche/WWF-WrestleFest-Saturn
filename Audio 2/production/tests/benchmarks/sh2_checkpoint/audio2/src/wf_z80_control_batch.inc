/* Isolated compact dispatcher experiment. Existing fast/scalar routines retain
 * their own budget, signed cycle-limit and interrupt admission checks. Every
 * memory-mapped I/O callback increments io_epoch before observable effects.
 * Return after any such activity so seq_run recomputes timer control/limit.
 * Returning at 003A preserves the outer exact idle-loop skip opportunity. */
WF_SMALL __attribute__((section(".wf_hotcode")))
unsigned wf_z80_control_batch(z80 *z,unsigned budget,uint32_t limit)
{
 wf_music_seq_t *v=(wf_music_seq_t *)z->userdata;unsigned total=0;
 while(!v->error){
  uint32_t io=v->io_epoch;
  unsigned n=wf_z80_control_fast_step(z,budget,limit);
  if(!n)n=wf_z80_scalar_step(z,budget,limit);
  if(!n)break;
  total+=n;budget-=n;
  if(z->pc==0x003a||v->io_epoch!=io)break;
 }
 return total;
}
