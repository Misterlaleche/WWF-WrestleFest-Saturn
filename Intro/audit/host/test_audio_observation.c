#include "wp0_audio_player.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static unsigned observed,reads,stops;
static unsigned read_region(void *unused){(void)unused;++reads;return observed;}
static void key_off(void *unused){(void)unused;++stops;}
static void setup(wp0_audio_player_t *p){
 memset(p,0,sizeof *p);p->state=WP0_AUDIO_PLAYING;
 p->ops.read_region=read_region;p->ops.key_off=key_off;
 p->queued_samples=WP0_AUDIO_RING_SAMPLES;p->stream.total_samples=1000000;
 observed=0;reads=0;stops=0;
}
int main(void){
 wp0_audio_player_t p;setup(&p);
 assert(wp0_audio_service(&p,10000));
 assert(p.last_region==0&&p.last_advance==0&&p.last_service_us==10000&&p.max_service_us==10000);
 assert(wp0_audio_service(&p,20000));assert(wp0_audio_service(&p,15000));
 assert(p.last_service_us==15000&&p.max_service_us==20000&&reads==3&&stops==0);
 observed=8;assert(!wp0_audio_service(&p,16000));
 assert(p.error==WP0_AUDIO_BAD_PLATFORM&&p.last_region==8&&p.last_advance==UINT32_MAX&&stops==1);
 assert(!wp0_audio_service(&p,1));assert(p.last_service_us==16000&&p.max_service_us==20000);
 setup(&p);assert(!wp0_audio_service(&p,800000));
 assert(p.error==WP0_AUDIO_SERVICE_GAP&&p.last_service_us==800000&&p.max_service_us==800000);
 assert(p.last_region==UINT32_MAX&&p.last_advance==UINT32_MAX&&reads==0&&stops==1);
 setup(&p);observed=2;assert(!wp0_audio_service(&p,16683));
 assert(p.error==WP0_AUDIO_SERVICE_GAP&&p.last_region==2&&p.last_advance==2&&reads==1&&stops==1);
 setup(&p);p.previous_region=1;p.consumed_lower_bound=4096;p.queued_samples=8192;observed=2;
 assert(!wp0_audio_service(&p,16683));
 assert(p.error==WP0_AUDIO_UNDERRUN&&p.last_region==2&&p.last_advance==1&&reads==1&&stops==1);
 puts("observation PASS: normal/max timing, CA8, elapsed gap, CA jump, underrun, failure snapshot retained; no extra monitor reads");
}
