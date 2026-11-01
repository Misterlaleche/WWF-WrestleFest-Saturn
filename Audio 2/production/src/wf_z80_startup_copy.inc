/* Isolated experiment. Exact pinned ED B0 at037C, never a generic LDIR.
 * Precondition: z belongs to the compatible wf_music_seq_t memory adapter.
 * Source10F0..123A is ROM; destinationC057..C1A1 is ordinary RAM.
 * 330 repeated21T iterations + final16T; no I/O or interrupt boundary. */
WF_SMALL __attribute__((section(".wf_hotcode")))
unsigned wf_z80_startup_copy(z80*z,unsigned budget,uint32_t limit)
{
 if(z->pc!=0x037c||budget<331u||(int32_t)(limit-(uint32_t)z->cyc)<6946||
    z->nmi_pending||(z->int_pending&&z->iff1)||z->iff_delay||z->halted||
    z->b!=1||z->c!=0x4b||z->h!=0x10||z->l!=0xf0||z->d!=0xc0||z->e!=0x57)return 0;
 wf_music_seq_t*v=(wf_music_seq_t*)z->userdata;
 for(unsigned i=0;i<331u;i++)v->ram[0x57u+i]=v->rom[0x10f0u+i];
 const uint8_t sum=(uint8_t)(z->a+v->rom[0x123a]);
 z->xf=(sum>>3)&1u;z->yf=(sum>>1)&1u;z->hf=0;z->nf=0;z->pf=0;
 set_hl(z,0x123b);set_de(z,0xc1a2);set_bc(z,0);
 z->pc=0x037e;z->mem_ptr=0x037d;
 z->r=(uint8_t)((z->r&128u)|((z->r+662u)&127u));
 z->cyc=(uint32_t)((uint32_t)z->cyc+6946u);
 return 331u;
}
