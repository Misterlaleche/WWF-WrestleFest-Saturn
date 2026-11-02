/* 0E6A DD56,5E,CALL0F4B plus the existing exact12-step YM writer.
 * Called only by the common fast dispatcher with its IRQ/PC admission.
 * All local admission precedes reads, stack stores and CPU mutation. */
WF_SMALL __attribute__((section(".wf_hotcode")))
unsigned wf_score_instrument_pre(z80*z,unsigned budget,uint32_t limit)
{
 wf_music_seq_t*v=(wf_music_seq_t*)z->userdata;
 uint16_t hl=get_hl(z);
 if(budget<15u||(int32_t)(limit-(uint32_t)z->cyc)<124||
    z->sp<0xc002||z->sp>0xc800||z->ix>=0xc800||hl>=0xc800||(v->status&128u))return 0;
 uint8_t d=rb(z,z->ix),e=rb(z,hl);
 unsigned at=z->sp-0xc002u;
 z->d=d;z->e=e;
 v->ram[at]=0x71;v->ram[at+1]=0x0e;
 z->sp-=2;z->pc=0x0f4b;z->mem_ptr=0x0f4b;z->cyc+=43u;
 z->r=(uint8_t)((z->r&128u)|((z->r+4u)&127u));
 unsigned n=wf_cost_macro(z,budget-3u,limit);
 wf_hot_process_tail(z);
 return n+3u;
}
