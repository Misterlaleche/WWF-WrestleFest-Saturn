#ifndef WWF_CAMPAIGN_PRESENTATION_H
#define WWF_CAMPAIGN_PRESENTATION_H
#include <stdbool.h>
#include <stdint.h>
#include "campaign.h"

typedef enum {
 CAMPAIGN_PRESENT_NONE=0,
 CAMPAIGN_PRESENT_PROGRESS,
 CAMPAIGN_PRESENT_RECORDS
} campaign_present_screen_t;

typedef struct {
 campaign_present_screen_t screen;
 uint16_t tick;
 uint16_t duration;
} campaign_presentation_t;

void campaign_presentation_reset(campaign_presentation_t *p);
void campaign_presentation_begin_progress(campaign_presentation_t *p);
void campaign_presentation_begin_records(campaign_presentation_t *p);
bool campaign_presentation_tick(campaign_presentation_t *p,unsigned intervals,bool skip);
unsigned campaign_matches_remaining(const campaign_t *c);
#endif
