/* Physical tag-team decisions only. Wider held-button pulses cover the original
 * input sampling cadence; no original RAM, actors or progression are written. */
static unsigned campaign_pad_buttons(unsigned port)
{
    static unsigned initialized,configured_phase,configured_period,configured_spacing,configured_pin_health,configured_variant,trace,last_trace=~0u;
    if(!initialized){
        const char *value=getenv("WF_POLICY_PERIOD");configured_period=value?(unsigned)strtoul(value,NULL,0):8u;
        value=getenv("WF_POLICY_SPACING");configured_spacing=value?(unsigned)strtoul(value,NULL,0):24u;
        value=getenv("WF_POLICY_PIN_HEALTH");configured_pin_health=value?(unsigned)strtoul(value,NULL,0):255u;
        value=getenv("WF_POLICY_VARIANT");configured_variant=value?(unsigned)strtoul(value,NULL,0):0u;
        value=getenv("WF_POLICY_TRACE");trace=value?(unsigned)strtoul(value,NULL,0):0u;
        if(configured_period<2u)configured_period=2u;
        value=getenv("WF_POLICY_PHASE");configured_phase=value?(unsigned)strtoul(value,NULL,0):0u;
        initialized=1;
    }
    if(port||!running||frontend_state||!frontend.handed_off)return 0;
    /* The first three matches replay a naturally winning physical policy.
     * Only subsequent pad decisions vary; every trial still starts at boot. */
    unsigned period=rt.g.progression_0162<2u?8u:rt.g.progression_0162==2u?4u:configured_period;
    unsigned spacing=rt.g.progression_0162<2u?16u:rt.g.progression_0162==2u?24u:configured_spacing;
    unsigned pin_health=rt.g.progression_0162<3u?24u:configured_pin_health;
    unsigned variant=rt.g.progression_0162<2u?15u:rt.g.progression_0162==2u?8u:configured_variant;
    const wf_actor_raw_t *human=NULL,*enemy=NULL;unsigned human_slot=0,enemy_slot=0;
    for(unsigned i=0;i<4u;i++){
        const wf_actor_raw_t *a=&rt.actors[i];
        if((wf_u8(a,WF_A_ACTIVE)&128u)&&!(wf_u8(a,WF_A_ROLE)&0xc0u)){
            human=a;human_slot=i;break;
        }
    }
    if(!human)return 0;
    for(unsigned i=4u;i<WF_ACTOR_SLOTS;i++){
        const wf_actor_raw_t *a=&rt.actors[i];
        if((wf_u8(a,WF_A_ACTIVE)&128u)&&!(wf_u8(a,WF_A_ROLE)&0x40u)){
            enemy=a;enemy_slot=i;break;
        }
    }
    if(!enemy)return 0;
    int dx=(int16_t)(wf_be16(enemy,WF_A_X)-wf_be16(human,WF_A_X));
    int dy=(int16_t)(wf_be16(enemy,WF_A_Y)-wf_be16(human,WF_A_Y));
    unsigned frame=rt.g.frame_counter,keys=0;
    unsigned pulse_frame=frame+(rt.g.progression_0162>=3u?configured_phase:0u);
    if(dy>6)keys|=JO_KEY_UP;else if(dy< -6)keys|=JO_KEY_DOWN;
    if(dx>(int)spacing)keys|=JO_KEY_RIGHT;else if(dx< -(int)spacing)keys|=JO_KEY_LEFT;
    const wf_actor_raw_t *partner=NULL;
    uint32_t team=wf_be32(human,WF_A_TEAM_LINK)&0x00ffffffu;
    for(unsigned i=0;i<4u;i++)if(team==wf_actor_rom_base(i))partner=&rt.actors[i];
    unsigned hp=wf_be16(human,WF_A_STAMINA),threshold=20u+20u*(variant/4u);
    unsigned tag=partner&&hp<=threshold&&wf_be16(partner,WF_A_STAMINA)>hp+8u;
    if(tag){
        unsigned right=wf_u8(human,WF_A_FLAGS33)&0x80u;
        int gx=right?768:528,gy=right?288:400;
        int px=(int16_t)wf_be16(human,WF_A_X),py=(int16_t)wf_be16(human,WF_A_Y);
        keys=0;
        if(px<gx-2)keys|=JO_KEY_RIGHT;else if(px>gx+2)keys|=JO_KEY_LEFT;
        if(py<gy-2)keys|=JO_KEY_UP;else if(py>gy+2)keys|=JO_KEY_DOWN;
        unsigned in_zone=right?(px>=752&&py<302):(px<544&&py>=390);
        if(pulse_frame%period<period/2u&&(in_zone||(wf_main_state(human)!=0u&&wf_main_state(human)!=1u)))
            keys|=JO_KEY_A;
    }else if(rt.g.progression_0162>=3u){
        const wf_actor_raw_t *target=enemy;
        uint32_t primary=wf_be32(human,WF_A_TARGET)&0x00ffffffu;
        for(unsigned i=4u;i<WF_ACTOR_SLOTS;i++)
            if(primary==wf_actor_rom_base(i))target=&rt.actors[i];
        unsigned state=wf_main_state(target),sub=wf_state4_subtype(target);
        unsigned prone=state==4u&&(sub==6u||sub==7u||sub==8u||sub==9u);
        unsigned hs=wf_main_state(human),action=wf_u8(human,WF_A_SELECTOR_LO);
        unsigned close=dx>-56&&dx<56&&dy>-20&&dy<20;
        if((hs==0u||hs==1u)&&!close){
        }else if(hs==12u||(hs==5u&&(action==0x15u||action==0x16u))){
            if(pulse_frame%period<period/2u)keys|=JO_KEY_A;
        }else if(hs==127u||(hs==5u&&action==0x4au)){
            keys|=(pulse_frame%period<period/2u)?JO_KEY_A:JO_KEY_B;
        }else if(prone&&wf_be16(target,WF_A_STAMINA)<=pin_health){
            if(pulse_frame%period<period/2u)keys|=JO_KEY_B;
        }else if(variant%4u==2u){
            keys|=(pulse_frame%period<period/2u)?JO_KEY_A:JO_KEY_B;
        }else if(pulse_frame%period<period/2u){
            if(variant%4u==1u)keys|=JO_KEY_B;
            else if(variant%4u==3u)keys|=JO_KEY_A|JO_KEY_B;
            else keys|=JO_KEY_A;
        }
    }else if(pulse_frame%period<period/2u){
        unsigned state=wf_main_state(enemy),sub=wf_state4_subtype(enemy);
        unsigned prone=state==4u&&(sub==6u||sub==7u||sub==8u||sub==9u);
        if(prone&&wf_be16(enemy,WF_A_STAMINA)<=pin_health)keys|=JO_KEY_B;
        else if(variant%4u==1u)keys|=JO_KEY_B;
        else if(variant%4u==2u)keys|=(pulse_frame/period&1u)?JO_KEY_B:JO_KEY_A;
        else if(variant%4u==3u&&wf_main_state(human)!=0u&&wf_main_state(human)!=1u)keys|=JO_KEY_A|JO_KEY_B;
        else keys|=JO_KEY_A;
    }
    if(trace&&frame%512u==0u&&frame!=last_trace){
        fprintf(stderr,"PRESSURE frame=%u stage=%u human=%u hp=%u state=%u/%u enemy=%u hp=%u state=%u/%u dx=%d dy=%d keys=%02x tag=%u partner_hp=%u\n",
                frame,rt.g.progression_0162,human_slot,wf_be16(human,WF_A_STAMINA),wf_main_state(human),wf_u8(human,WF_A_SELECTOR_LO),
                enemy_slot,wf_be16(enemy,WF_A_STAMINA),wf_main_state(enemy),wf_state4_subtype(enemy),dx,dy,keys,tag,partner?wf_be16(partner,WF_A_STAMINA):0u);
        last_trace=frame;
    }
    return keys;
}
