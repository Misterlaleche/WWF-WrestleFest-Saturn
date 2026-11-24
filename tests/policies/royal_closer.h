/* Royal hunter controller: inspect state, return physical keys, never mutate
 * original runtime, actors, clock, score, RNG, records or match outcome. */
static unsigned royal_policy_abs(int value)
{return (unsigned)(value<0?-value:value);}
static unsigned campaign_pad_buttons(unsigned port)
{
    static unsigned initialized,switch_at,configured_phase,configured_period,configured_spacing,configured_pin_health,configured_variant,trace,last_trace=~0u;
    if(!initialized){
        const char *value=getenv("WF_POLICY_PERIOD");configured_period=value?(unsigned)strtoul(value,NULL,0):6u;
        value=getenv("WF_POLICY_SPACING");configured_spacing=value?(unsigned)strtoul(value,NULL,0):16u;
        value=getenv("WF_POLICY_PIN_HEALTH");configured_pin_health=value?(unsigned)strtoul(value,NULL,0):24u;
        value=getenv("WF_POLICY_VARIANT");configured_variant=value?(unsigned)strtoul(value,NULL,0):0u;
        value=getenv("WF_POLICY_TRACE");trace=value?(unsigned)strtoul(value,NULL,0):0u;
        value=getenv("WF_POLICY_PHASE");configured_phase=value?(unsigned)strtoul(value,NULL,0):0u;
        if(configured_period<2u)configured_period=2u;
        value=getenv("WF_POLICY_SWITCH_AT");switch_at=value?(unsigned)strtoul(value,NULL,0):8000u;
        initialized=1;
    }
    if(port||!running||frontend_state||!frontend.handed_off)return 0;
    /* Preserve a promising physical opening; vary subsequent pad choices. */
    unsigned late=rt.g.frame_counter>=switch_at;
    unsigned period=late?configured_period:8u,spacing=late?configured_spacing:16u;
    unsigned variant=late?configured_variant%12u:2u,pin_health=late?configured_pin_health:24u;
    unsigned phase=late?configured_phase:0u,grapple=late&&configured_variant>=12u;
    const wf_actor_raw_t *human=&rt.actors[0];
    if(!(wf_u8(human,WF_A_ACTIVE)&0x80u))return 0;
    int x=(int16_t)wf_be16(human,WF_A_X),y=(int16_t)wf_be16(human,WF_A_Y);
    unsigned selected=WF_ACTOR_SLOTS,best=~0u,enemies=0u;
    uint32_t link=wf_be32(human,WF_A_LINK)&0x00ffffffu;
    for(unsigned i=1u;i<WF_ACTOR_SLOTS;i++){
        const wf_actor_raw_t *a=&rt.actors[i];
        if(!(wf_u8(a,WF_A_ACTIVE)&0x80u)||(wf_u8(a,WF_A_FLAGS32)&0x10u))continue;
        ++enemies;
        int dx=(int16_t)wf_be16(a,WF_A_X)-x,dy=(int16_t)wf_be16(a,WF_A_Y)-y;
        unsigned score=royal_policy_abs(dx)+3u*royal_policy_abs(dy);
        if(variant/4u==1u)score+=4u*wf_be16(a,WF_A_STAMINA);
        else if(variant/4u>=2u)score+=16u*wf_be16(a,WF_A_STAMINA);
        if(link==wf_actor_rom_base(i))score=0;
        if(score<best){best=score;selected=i;}
    }
    if(selected==WF_ACTOR_SLOTS)return JO_KEY_A;
    const wf_actor_raw_t *enemy=&rt.actors[selected];
    int dx=(int16_t)wf_be16(enemy,WF_A_X)-x,dy=(int16_t)wf_be16(enemy,WF_A_Y)-y;
    unsigned frame=rt.g.frame_counter,keys=0;
    if(dx>(int)spacing)keys|=JO_KEY_RIGHT;else if(dx<-(int)spacing)keys|=JO_KEY_LEFT;
    if(dy>6)keys|=JO_KEY_UP;else if(dy< -6)keys|=JO_KEY_DOWN;
    const wf_actor_raw_t *action_target=enemy;
    uint32_t primary=wf_be32(human,WF_A_TARGET)&0x00ffffffu;
    for(unsigned i=1u;i<WF_ACTOR_SLOTS;i++)
        if(primary==wf_actor_rom_base(i))action_target=&rt.actors[i];
    unsigned state=wf_main_state(action_target),sub=wf_state4_subtype(action_target);
    unsigned prone=state==4u&&(sub==6u||sub==7u||sub==8u||sub==9u);
    unsigned action=wf_u8(human,WF_A_SELECTOR_LO),human_state=wf_main_state(human);
    unsigned close=royal_policy_abs(dx)<56u&&royal_policy_abs(dy)<20u;
    if(grapple&&(human_state==0u||human_state==1u)&&!close){
    }else if(grapple&&(human_state==12u||(human_state==5u&&(action==0x15u||action==0x16u)))){
        if((frame+phase)%period<period/2u)keys|=JO_KEY_A;
    }else if(human_state==127u||(human_state==5u&&action==0x4au)){
        /* Alternate buttons while held/pinned to create repeated fresh edges. */
        keys|=((frame+phase)%period<period/2u)?JO_KEY_A:JO_KEY_B;
    }else if(late&&enemies==1u&&prone){
        /* Finish damaging the final rival before attempting the pin. */
        if((frame+phase)%period<period/2u)keys|=wf_be16(action_target,WF_A_STAMINA)?JO_KEY_A:JO_KEY_B;
    }else if(prone&&wf_be16(action_target,WF_A_STAMINA)<=pin_health){
        if((frame+phase)%period<period/2u)keys|=JO_KEY_B;
    }else if(variant%4u==2u){
        keys|=((frame+phase)%period<period/2u)?JO_KEY_A:JO_KEY_B;
    }else if((frame+phase)%period<period/2u){
        if(variant%4u==1u)keys|=JO_KEY_B;
        else if(variant%4u==3u)keys|=JO_KEY_A|JO_KEY_B;
        else keys|=JO_KEY_A;
    }
    if(trace&&frame%512u==0u&&frame!=last_trace){
        fprintf(stderr,"ROYAL_POLICY frame=%u human_hp=%u state=%u/%u enemies=%u target=%u hp=%u state=%u/%u dx=%d dy=%d keys=%02x\n",
                frame,wf_be16(human,WF_A_STAMINA),wf_main_state(human),wf_u8(human,WF_A_SELECTOR_LO),
                enemies,selected,wf_be16(enemy,WF_A_STAMINA),wf_main_state(enemy),wf_state4_subtype(enemy),dx,dy,keys);
        last_trace=frame;
    }
    return keys;
}
