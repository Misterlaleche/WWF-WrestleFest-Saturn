#include "wp01_oki.h"
#include "generated/wp01_oki_assets.h"
#include "../research_events/export/entrance_audio_events.h"
#include <assert.h>
#include <stdio.h>

static unsigned reg_writes;
static void wr(void *u,uint16_t address,uint16_t value){
 (void)u;(void)value;
 assert(address>=24u*32u&&address<28u*32u&&!(address&1u));
 reg_writes++;
}

int main(void){
 wp01_oki_t oki;
 wp01_oki_init(&oki,wp01_oki_assets,WP01_OKI_ASSET_COUNT,wr,0);
 unsigned count=sizeof(wp01_audio_events)/sizeof(wp01_audio_events[0]);
 for(unsigned i=0;i<count;i++){
  const wp01_audio_event_t *e=&wp01_audio_events[i];
  wp01_oki_advance(&oki,e->sample);
  if(e->chip==WP01_AUDIO_OKI)wp01_oki_write(&oki,e->value,e->sample);
  if(e->chip==WP01_AUDIO_BANK)assert(wp01_oki_set_bank(&oki,e->value));
 }
 wp01_oki_advance(&oki,WP01_AUDIO_SAMPLES);
 assert(oki.stats.starts==19u); /* 17 kicks, two crowd starts in original tap. */
 assert(oki.stats.natural_ends==16u);
 assert(oki.stats.stops==1u); /* Crowd is explicitly stopped before restart. */
 assert(!oki.stats.ignored_busy&&!oki.stats.missing_asset);
 assert(!oki.stats.invalid_asset&&!oki.stats.unsupported_bank);
 assert(wp01_oki_status(&oki)==0xf5u); /* Voice0 and2 continue at scene boundary. */
 assert(oki.voices[0].sample_id==1u&&oki.voices[2].sample_id==18u);
 assert(oki.voices[0].started_at==311586u&&oki.voices[2].started_at==176216u);
 printf("PASS: 748 original audio writes; OKI starts=%u, natural_ends=%u, explicit_stops=%u, register_writes=%u\n",
        oki.stats.starts,oki.stats.natural_ends,oki.stats.stops,reg_writes);
 return 0;
}
