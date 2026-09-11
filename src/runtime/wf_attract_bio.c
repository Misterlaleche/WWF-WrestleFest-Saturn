#include "wf_attract_bio.h"
#include <string.h>

static const int8_t state_to_fighter[14]={
 -1,-1,0,1,2,3,6,7,8,9,10,11,4,5
};

static uint16_t be16(const uint8_t *p){return (uint16_t)(((uint16_t)p[0]<<8)|p[1]);}
static uint32_t be32(const uint8_t *p){return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3];}

bool wf_attract_bio_fighter_for_state(unsigned attract_state,unsigned *fighter){
 if(attract_state>=14U||state_to_fighter[attract_state]<0)return false;
 if(fighter)*fighter=(unsigned)state_to_fighter[attract_state];
 return true;
}

void wf_attract_bio_begin(wf_attract_bio_t *bio,unsigned attract_state){
 if(!bio)return;
 memset(bio,0,sizeof(*bio));
 bio->attract_state=(uint8_t)attract_state;
 unsigned fighter=0;
 if(wf_attract_bio_fighter_for_state(attract_state,&fighter))bio->fighter=(uint8_t)fighter;
 else bio->cleaned=true;
}

unsigned wf_attract_bio_dwell_from_nibble(uint8_t rng_nibble){return (unsigned)(rng_nibble&0x0FU)+8U;}

void wf_attract_bio_step(wf_attract_bio_t *bio,uint16_t tick,uint8_t rng_nibble){
 if(!bio)return;
 bio->draw_portrait=false;bio->draw_text=false;
 if(bio->cleaned)return;
 if(!bio->initialized){
  bio->initialized=true;bio->dwell=WF_ATTRACT_BIO_INITIAL_DWELL;bio->toggle=0;bio->frame=0;
 }
 bio->frame=bio->toggle;
 if(tick>=WF_ATTRACT_BIO_ANIM_END_TICK)bio->frame=2;
 if(bio->dwell){
  --bio->dwell;
  if(!bio->dwell){bio->dwell=(uint8_t)wf_attract_bio_dwell_from_nibble(rng_nibble);bio->toggle^=1U;}
 }
 bio->draw_portrait=true;
 bio->draw_text=tick<WF_ATTRACT_BIO_CLEANUP_TICK;
 if(tick>=WF_ATTRACT_BIO_CLEANUP_TICK)bio->cleaned=true;
}

unsigned wf_attract_bio_frame(const wf_attract_bio_t *bio){return bio?bio->frame:0U;}
bool wf_attract_bio_portrait_visible(const wf_attract_bio_t *bio){return bio&&bio->draw_portrait;}
bool wf_attract_bio_text_visible(const wf_attract_bio_t *bio){return bio&&bio->draw_text;}

uint32_t wf_attract_bio_fnv1a(const uint8_t *data,size_t bytes){
 uint32_t h=2166136261UL;
 if(!data)return 0;
 while(bytes--)h=(h^*data++)*16777619UL;
 return h;
}

bool wf_attract_bio_pack_open(wf_attract_bio_pack_t *out,const uint8_t *data,size_t bytes,unsigned expected_fighter){
 static const uint8_t magic[8]={'W','F','B','I','O','0','1',0};
 if(!out||!data||bytes!=WF_ATTRACT_BIO_PACK_BYTES||memcmp(data,magic,8)!=0)return false;
 if(data[8]!=1U||data[9]>=WF_ATTRACT_BIO_FIGHTERS||data[9]!=expected_fighter)return false;
 if(be16(data+10)!=WF_ATTRACT_BIO_PACK_HEADER_BYTES)return false;
 if(be32(data+12)!=(uint32_t)(WF_ATTRACT_BIO_TEXTURE_BYTES+WF_ATTRACT_BIO_PALETTE_BYTES))return false;
 if(be16(data+20)!=WF_ATTRACT_BIO_PORTRAIT_W||be16(data+22)!=WF_ATTRACT_BIO_PORTRAIT_H)return false;
 if(be16(data+24)!=WF_ATTRACT_BIO_TEXT_W||be16(data+26)!=WF_ATTRACT_BIO_TEXT_H)return false;
 if(be16(data+28)!=256U||data[30]!=WF_ATTRACT_BIO_FRAMES)return false;
 uint32_t po=be32(data+32),pb=be32(data+36),to=be32(data+40),tb=be32(data+44),palo=be32(data+48),palb=be32(data+52);
 if(po!=WF_ATTRACT_BIO_PACK_HEADER_BYTES||pb!=WF_ATTRACT_BIO_PORTRAIT_FRAME_BYTES)return false;
 if(to!=WF_ATTRACT_BIO_PACK_HEADER_BYTES+WF_ATTRACT_BIO_PORTRAIT_BYTES||tb!=WF_ATTRACT_BIO_TEXT_BYTES)return false;
 if(palo!=WF_ATTRACT_BIO_PACK_HEADER_BYTES+WF_ATTRACT_BIO_TEXTURE_BYTES||palb!=WF_ATTRACT_BIO_PALETTE_BYTES)return false;
 if(palo+palb!=bytes)return false;
 uint32_t stored=be32(data+16),actual=wf_attract_bio_fnv1a(data+WF_ATTRACT_BIO_PACK_HEADER_BYTES,bytes-WF_ATTRACT_BIO_PACK_HEADER_BYTES);
 if(stored!=actual)return false;
 memset(out,0,sizeof(*out));out->fighter=data[9];out->payload_hash=actual;
 for(unsigned i=0;i<WF_ATTRACT_BIO_FRAMES;i++)out->portrait[i]=data+po+i*pb;
 out->text=data+to;out->palette_be=data+palo;
 return true;
}
