#include "wp1980_input.h"
#include "wf_r18_frontend_driver.h"

static unsigned launch_enabled;
static uint32_t royal_selected_clock;
static unsigned royal_selected;
static unsigned coin_state,credit_received;
static unsigned roster_release_observed;
volatile uint32_t wp1980_launch_stage;
#ifdef WP1980_NOINPUT
volatile uint32_t wp1980_demo_active=0u;
#else
volatile uint32_t wp1980_demo_active=1u;
#endif

void wp1980_frontend_input_begin(uint32_t clock)
{
    launch_enabled=1u;
    royal_selected_clock=clock;
    royal_selected=0u;
    coin_state=credit_received=0u;
    roster_release_observed=0u;
    wp1980_launch_stage=1u;
}

void wp1980_frontend_input_disable(void)
{
    launch_enabled=0u;
    wp1980_launch_stage=0u;
}

uint16_t wp1980_frontend_input(unsigned phase,const uint8_t *ram,
                              uint32_t clock,unsigned port,uint16_t physical)
{
    if(!launch_enabled||!ram)return physical;
    /* Keep DIP configuration and the real VBlank bit. Automatic launch
     * supplies a single player's controls and one ordinary coin pulse. */
    uint16_t word=(uint16_t)(physical|0x00ffu);
    if(port!=0u)return word;
    if(phase==WF_R18_DRIVER_BOOT||phase==WF_R18_DRIVER_TITLE){
        wp1980_launch_stage=1u;
        if(ram[0x4f])credit_received=1u;
        if(!credit_received){
            /* Original $048C accepts a coin on release after at least four
             * inactive polls. Observe its own debounce/duration bytes rather
             * than a display-clock window that CD or drawing can skip. */
            unsigned duration=((unsigned)ram[0x54]<<8)|ram[0x55];
            if(coin_state==0u&&ram[0x10]>=4u)coin_state=1u;
            if(coin_state==1u&&duration)coin_state=2u;
            if(coin_state==1u)word&=(uint16_t)~0x0100u;
        }else word&=(uint16_t)~0x0080u;
    }else if(phase==WF_R18_DRIVER_MODE){
        wp1980_launch_stage=2u;
        if(!(ram[0x161]&1u))word&=(uint16_t)~0x0002u;
        else{
            if(!royal_selected){royal_selected=1u;royal_selected_clock=clock;}
            if(clock-royal_selected_clock>=2u)word&=(uint16_t)~0x0010u;
        }
    }else if(phase==WF_R18_DRIVER_ROSTER){
        wp1980_launch_stage=3u;
        /* P1's original initial cursor is roster index0, identity0. Release
         * the mode-confirm button until $64E0 has actually sampled it, then
         * hold A until the original roster owner accepts the fresh edge.
         * A display-clock pulse can alias under long CD/audio transactions. */
        if(!ram[0x5b0u+0xa6u]&&!ram[0x5b0u+0xa7u])roster_release_observed=1u;
        if(roster_release_observed&&!(ram[0x5b0u+0x33u]&1u))word&=(uint16_t)~0x0010u;
    }else if(phase==WF_R18_DRIVER_AC0){
        wp1980_launch_stage=4u;
        /* The original card/prematch owners accept A to advance. */
        if((clock&7u)<4u)word&=(uint16_t)~0x0010u;
    }
    return word;
}

static unsigned distance(int value)
{
    return (unsigned)(value<0?-value:value);
}

uint8_t wp1980_input(const wf_runtime_t *runtime,uint8_t physical)
{
    if(physical!=0xffu)wp1980_demo_active=0u;
    if(!wp1980_demo_active||!runtime)return physical;
    const wf_actor_raw_t *player=&runtime->actors[0];
    if(!(wf_u8(player,WF_A_ACTIVE)&0x80u))return 0xffu;

    int px=(int16_t)wf_be16(player,WF_A_X);
    int py=(int16_t)wf_be16(player,WF_A_Y);
    int dx=0,dy=0;
    unsigned best=~0u;
    for(unsigned slot=1u;slot<WF_ACTOR_SLOTS;slot++){
        const wf_actor_raw_t *candidate=&runtime->actors[slot];
        if(!(wf_u8(candidate,WF_A_ACTIVE)&0x80u))continue;
        int cx=(int16_t)wf_be16(candidate,WF_A_X)-px;
        int cy=(int16_t)wf_be16(candidate,WF_A_Y)-py;
        unsigned score=distance(cx)+2u*distance(cy);
        if(score<best){best=score;dx=cx;dy=cy;}
    }
    if(best==~0u)return 0xffu;

    uint8_t input=0xffu;
    /* Native gameplay bits: right1,left2,down4,up8,A16,B32,C64.
     * These are human controller decisions, not replacement actor behavior. */
    if(dx>18)input&=(uint8_t)~1u;
    else if(dx< -18)input&=(uint8_t)~2u;
    if(dy>5)input&=(uint8_t)~4u;
    else if(dy< -5)input&=(uint8_t)~8u;

    uint32_t frame=runtime->g.frame_counter;
    if(distance(dx)<70u&&distance(dy)<28u){
        unsigned pulse=frame%24u;
        if(pulse<4u)input&=(uint8_t)~0x10u;
        else if(pulse>=12u&&pulse<16u)input&=(uint8_t)~0x20u;
    }
    return input;
}
