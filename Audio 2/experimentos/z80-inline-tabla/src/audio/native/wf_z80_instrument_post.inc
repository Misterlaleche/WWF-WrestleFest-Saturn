   /* Separate continuation: never crosses the preceding C801 callback. */
   case 0x0f5f:{
    if(budget<5u||z->sp<0xc000||z->sp>0xc7fe)return 0;
    bool taken=z->b!=1u;unsigned ticks=taken?43u:38u;
    if((int32_t)(limit-(uint32_t)z->cyc)<(int32_t)ticks)return 0;
    wf_music_seq_t*v=(wf_music_seq_t*)z->userdata;unsigned at=z->sp-0xc000u;
    if(v->ram[at]!=0x71||v->ram[at+1]!=0x0e)return 0;
    uint8_t a=z->a,f=get_f(z);
    z->a=z->a_;set_f(z,z->f_);z->a_=a;z->f_=f;
    z->sp+=2;++z->ix;set_hl(z,(uint16_t)(get_hl(z)+1u));--z->b;
    z->pc=taken?0x0e6a:0x0e76;z->mem_ptr=taken?0x0e6a:0x0e71;
    z->cyc+=ticks;z->r=(uint8_t)((z->r&128u)|((z->r+6u)&127u));
    return 5;
   }
