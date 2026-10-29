#include <stdio.h>
#include <stdint.h>
#include "wp01_fm.h"
#include "wp01_oki.h"
#include "wp01_oki_assets.h"
#include "entrance_audio_events.h"
static unsigned initial=1;
static uint32_t now;
static void record(void *unused,uint16_t offset,uint16_t value){
    (void)unused;
    printf("%u,%u,%u,%u\n",initial,now,offset,value);
}
int main(void){
    wp01_oki_t oki;
    puts("initial,sample,offset,value");
    wp01_fm_init(record,0);wp01_fm_load_registers(wp01_audio_initial_ym);
    wp01_oki_init(&oki,wp01_oki_assets,WP01_OKI_ASSET_COUNT,record,0);
    initial=0;unsigned event=0;
    for(now=0;now<WP01_AUDIO_SAMPLES;now++){
        wp01_oki_advance(&oki,now);
        while(event<sizeof(wp01_audio_events)/sizeof(wp01_audio_events[0])&&wp01_audio_events[event].sample==now){
            const wp01_audio_event_t *e=&wp01_audio_events[event++];
            if(e->chip==WP01_AUDIO_YM)wp01_fm_write(e->reg,e->value);
            else if(e->chip==WP01_AUDIO_OKI)wp01_oki_write(&oki,e->value,now);
            else if(e->chip==WP01_AUDIO_BANK&&!wp01_oki_set_bank(&oki,e->value))return 3;
        }
    }
    unsigned bad=wp01_fm_unsupported()|oki.stats.missing_asset|oki.stats.invalid_asset|oki.stats.unsupported_bank;
    fprintf(stderr,"events=%u FM_unsupported=%u OKI_starts=%u OKI_stops=%u OKI_natural_ends=%u errors=%u\n",
        event,wp01_fm_unsupported(),oki.stats.starts,oki.stats.stops,oki.stats.natural_ends,bad);
    return bad?2:0;
}
