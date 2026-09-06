#include "campaign_presentation.h"
#include <string.h>

void campaign_presentation_reset(campaign_presentation_t *p){
 memset(p,0,sizeof(*p));
 p->screen=CAMPAIGN_PRESENT_NONE;
}

void campaign_presentation_begin_progress(campaign_presentation_t *p){
 campaign_presentation_reset(p);
 p->screen=CAMPAIGN_PRESENT_PROGRESS;
 p->duration=300;
}

void campaign_presentation_begin_records(campaign_presentation_t *p){
 campaign_presentation_reset(p);
 p->screen=CAMPAIGN_PRESENT_RECORDS;
 p->duration=480;
}

bool campaign_presentation_tick(campaign_presentation_t *p,unsigned intervals,bool skip){
 if(p->screen==CAMPAIGN_PRESENT_NONE)return true;
 if(skip)return true;
 unsigned next=(unsigned)p->tick+intervals;
 p->tick=(uint16_t)(next>0xffffu?0xffffu:next);
 return p->tick>=p->duration;
}
