/* Exact 0F2D..0F4A unsigned 8x8 multiplication and RET. No I/O.
 * Complete admission precedes every CPU/RAM mutation. */
WF_SMALL __attribute__((section(".wf_hotcode")))
unsigned wf_score_multiply(z80 *z,unsigned budget,uint32_t limit)
{
 if(z->pc!=0x0f2d||z->nmi_pending||(z->int_pending&&z->iff1)||
    z->iff_delay||z->halted||z->sp<0xc004||z->sp>0xc7fe)return 0;
 unsigned pop=z->e;
 pop-=((pop>>1)&0x55u);
 pop=(pop&0x33u)+((pop>>2)&0x33u);
 pop=(pop+(pop>>4))&15u;
 unsigned steps=60u+pop,ticks=511u+6u*pop;
 if(budget<steps||(int32_t)(limit-(uint32_t)z->cyc)<(int32_t)ticks)return 0;
 wf_music_seq_t *v=(wf_music_seq_t *)z->userdata;
 unsigned at=z->sp-0xc000u,product=(unsigned)z->b*z->e;
 uint16_t back=(uint16_t)(v->ram[at]|((unsigned)v->ram[at+1]<<8));
 v->ram[at-2]=get_f(z);v->ram[at-1]=z->a;
 v->ram[at-4]=z->l;v->ram[at-3]=z->h;
 z->b=(uint8_t)(product>>8);z->c=(uint8_t)product;z->e=0;
 z->sp+=2;z->pc=back;z->mem_ptr=back;z->cyc+=ticks;
 z->r=(uint8_t)((z->r&128u)|((z->r+86u+pop)&127u));
 return steps;
}
