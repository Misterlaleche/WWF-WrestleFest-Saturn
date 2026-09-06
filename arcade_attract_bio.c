#include "arcade_attract_bio.h"
#include "arcade_attract.h"
#include <string.h>
bool arcade_attract_bio_desc(unsigned state,arcade_attract_bio_desc_t *out){
 if(!out||state<2||state>=ARCADE_ATTRACT_STATE_COUNT)return false;
 const arcade_attract_state_desc_t*d=&arcade_attract_states[state];if(d->featured_character<0)return false;
 unsigned i=state-2;out->character=(uint8_t)d->featured_character;out->composition_set=(uint8_t)(0x30+d->featured_character);
 out->character_palette=(uint8_t)(0x10+d->featured_character);out->common_palette=0x1e;
 out->text_ids[0]=6;out->text_ids[1]=(uint16_t)(7+i);out->text_ids[2]=(uint16_t)(0x40+i);return true;
}
void arcade_attract_bio_anim_begin(arcade_attract_bio_anim_t*a){if(!a)return;memset(a,0,sizeof(*a));a->visible=true;a->countdown=8;}
void arcade_attract_bio_anim_tick(arcade_attract_bio_anim_t*a,uint16_t phase_counter,uint8_t rng_low_nibble){
 if(!a||!a->visible)return;
 a->pose=a->toggle;
 if(phase_counter>=0x0100)a->pose=2;
 if(a->countdown)--a->countdown;
 if(!a->countdown){a->countdown=(uint8_t)((rng_low_nibble&0x0f)+8);a->toggle^=1;}
 if(phase_counter>=0x0200)a->visible=false;
}
