#include "ring_rope_rom_exact.h"

typedef struct { wwf_u8 selector, ticks; } phase_t;
static const phase_t s1_a[6]={{3,6},{4,6},{5,6},{6,6},{2,6},{4,6}};
static const phase_t s1_b[6]={{7,6},{8,6},{10,6},{9,6},{10,6},{8,6}};
static const phase_t s2_a[6]={{3,16},{4,6},{2,6},{6,6},{2,6},{4,6}};
static const phase_t s2_b[6]={{7,16},{8,6},{10,6},{9,6},{10,6},{8,6}};
static const phase_t s3[3]={{4,10},{5,10},{6,6}};
static const phase_t s4[2]={{6,14},{2,12}};

static void load_phase(wwf_rope_record_t *r,const phase_t *p){r->selector=p[r->phase].selector;r->timer=p[r->phase].ticks;}

void wwf_rope_quartet_reset(wwf_rope_quartet_t *q){
    static const wwf_u16 x[4]={0x01C0,0x01C0,0x0330,0x0330};
    static const wwf_u16 cls[4]={0x0001,0x0003,0x0001,0x0003};
    static const wwf_u16 ori[4]={0x0000,0x0000,0x8000,0x8000};
    unsigned i;
    for(i=0;i<4;i++){
        wwf_rope_record_t *r=&q->r[i];
        r->state_word=0;r->timer=0;r->phase=0;r->gfx_set=0x000E;
        r->world_x=x[i];r->world_y=0x0178;r->world_z=0x0140;
        r->class_word=cls[i];r->orient=ori[i];r->selector=0;
    }
}
void wwf_rope_write_state(wwf_rope_quartet_t *q,unsigned record,wwf_u16 state){if(record<4)q->r[record].state_word=(wwf_u16)(state&0x000F);}
void wwf_rope_write_pair_state(wwf_rope_quartet_t *q,unsigned side,wwf_u16 state){unsigned b=side==WWF_ROPE_RIGHT?2U:0U;wwf_rope_write_state(q,b,state);wwf_rope_write_state(q,b+1U,state);}
unsigned wwf_rope_record_side(unsigned record){return record>=2U?WWF_ROPE_RIGHT:WWF_ROPE_LEFT;}

void wwf_rope_step_record(wwf_rope_record_t *r){
    unsigned state=(unsigned)(r->state_word&0x000FU),count=0;
    const phase_t *table=0;
    if((r->state_word&0x0080U)==0){
        r->state_word|=0x0080U;
        if(state==0){r->selector=(r->class_word&0x0002U)?10U:2U;return;}
        r->phase=0;
        if(state==1){table=(r->class_word&0x0002U)?s1_b:s1_a;count=6;}
        else if(state==2){table=(r->class_word&0x0002U)?s2_b:s2_a;count=6;}
        else if(state==3){table=s3;count=3;}
        else if(state==4){table=s4;count=2;}
        if(table&&count)load_phase(r,table);
        return;
    }
    if(state==0)return;
    if(r->timer!=0)r->timer--;
    if(r->timer!=0)return;
    r->phase++;
    if(state==1){table=(r->class_word&0x0002U)?s1_b:s1_a;count=6;}
    else if(state==2){table=(r->class_word&0x0002U)?s2_b:s2_a;count=6;}
    else if(state==3){table=s3;count=3;}
    else if(state==4){table=s4;count=2;}
    if(r->phase>=count){r->state_word=0;return;} /* selector remains for this render; state0 restores next update */
    load_phase(r,table);
}
void wwf_rope_step(wwf_rope_quartet_t *q){unsigned i;for(i=0;i<4;i++)wwf_rope_step_record(&q->r[i]);}
