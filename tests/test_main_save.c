/* Real RESET -> selections -> live Royal combat -> native loss/ranking.
 * Only backup storage is replaced; this observer never writes actors, RNG,
 * results or progress. Loaded low records are an explicit backup fixture. */
#include "wf_jo_save.h"
#include "wf_jo_scene_prepare.h"
#define main first_entry_main
#define __wrap_wf_jo_frontend_run first_entry_wrapper
#define __wrap_wf_jo_frontend_after_combat_at_level first_terminal_wrapper
#include "native/jo_main_probe.c"
#undef main
#undef __wrap_wf_jo_frontend_run
#undef __wrap_wf_jo_frontend_after_combat_at_level

static unsigned save_inits,save_opens,save_closes,save_reads,save_writes,save_opened,save_post_seen;
static unsigned save_fault,save_drains;
static uint8_t save_disk[WF_R18_SAVE_IMAGE_BYTES],save_factory[WF_R18_SAVE_RANKING_BYTES];
static wf_jo_save_t *save_context;
static size_t save_cache_bytes;
void __real_wf_jo_save_init(wf_jo_save_t *,const wf_jo_save_backend_t *,void *);
wf_status_t __real_wf_jo_frontend_after_combat_at_level(wf_jo_frontend_t *,uint32_t,unsigned);
wf_status_t __real_wf_jo_video_drain_overlap(wf_status_t (*)(void *),void *);
wf_status_t __wrap_wf_jo_video_drain_overlap(wf_status_t (*service)(void *),void *user){
    ++save_drains;return __real_wf_jo_video_drain_overlap(service,user);
}
static int disk_open(void *user,void *area,size_t bytes){
    (void)user;assert(!save_opened&&area&&bytes==WF_JO_SAVE_WORKSPACE_BYTES&&!((uintptr_t)area&3u));
    assert(wf_jo_assets_sprite_cache_active_bytes()==save_cache_bytes-WF_JO_SAVE_WORKSPACE_BYTES);
    if(!save_opens)assert(!memcmp(frontend.driver.ram+0xc2u,save_factory,sizeof save_factory));
    ++save_opens;
    if(save_fault==2u)return 0;
    save_opened=1;memset(area,0x5a,bytes);return 1;
}
static void disk_close(void *user){(void)user;assert(save_opened);save_opened=0;++save_closes;}
static int disk_find(void *user,const char *name,size_t *bytes){
    (void)user;assert(save_opened&&!strcmp(name,"WFESTSAT001"));*bytes=sizeof save_disk;return 1;
}
static int disk_read(void *user,const char *name,void *data,size_t bytes){
    (void)user;(void)name;assert(save_opened&&bytes==sizeof save_disk);memcpy(data,save_disk,bytes);++save_reads;return 1;
}
static int disk_write(void *user,const char *name,const void *data,size_t bytes){
    (void)user;(void)name;assert(save_opened&&bytes==sizeof save_disk&&save_post_seen);
    ++save_writes;if(save_fault==1u)return 0;memcpy(save_disk,data,bytes);return 1;
}
static int disk_verify(void *user,const char *name,const void *data,size_t bytes){
    (void)user;(void)name;return bytes==sizeof save_disk&&!memcmp(data,save_disk,bytes);
}
static const wf_jo_save_backend_t disk_io={disk_open,disk_close,disk_find,disk_read,disk_write,disk_verify};
void __wrap_wf_jo_save_init(wf_jo_save_t *s,const wf_jo_save_backend_t *backend,void *user){
    assert(!backend&&!user&&!save_inits);++save_inits;save_context=s;
    save_cache_bytes=wf_jo_assets_sprite_cache_active_bytes();
    __real_wf_jo_save_init(s,&disk_io,NULL);
}
wf_status_t __wrap_wf_jo_frontend_run(wf_jo_frontend_t *f){
    wf_status_t status=__real_wf_jo_frontend_run(f);
    if(status==WF_OK&&f->handed_off){
        assert(save_inits==1u&&save_opens==1u&&save_writes==0u);
        assert(save_reads==(save_fault==2u?0u:1u));
        assert(f->driver.ram[0xc3u]==(save_fault==2u?save_factory[1]:0x51u));
        assert(wf_jo_assets_sprite_cache_active_bytes()==save_cache_bytes);
        host_entry_seen=1;
    }
    return status;
}
wf_status_t __wrap_wf_jo_frontend_after_combat_at_level(wf_jo_frontend_t *f,uint32_t owner,unsigned high){
    assert(f==&frontend&&high==1u&&owner==0x1afcu&&wf_r18_royal_game_over(&rt));
    assert(!save_post_seen);save_post_seen=1;host_terminal_owner=owner;host_terminal_seen=1;
    assert(__real_wf_jo_frontend_after_combat_at_level(f,owner,high)==WF_OK);
    assert(f->driver.phase==WF_R18_DRIVER_BOOT&&f->driver.pc==0x6fcu);
    assert(!save_opened&&wf_jo_assets_sprite_cache_active_bytes()==save_cache_bytes);
    assert(save_writes==(save_fault==2u?0u:1u));
    assert(save_context->status==(save_fault==2u?WF_JO_SAVE_UNAVAILABLE:
        save_fault==1u?WF_JO_SAVE_WRITE_ERROR:WF_JO_SAVE_SAVED));
    if(!save_fault){
        wf_r18_save_data_t saved;assert(wf_r18_save_decode(save_disk,&saved));
        assert(!memcmp(saved.rankings,f->driver.ram+0xc2u,sizeof saved.rankings));
        /* An unchanged notification must not drain video, cancel a prepared
         * scene, open the BIOS, borrow a second lease or write the file. */
        assert(wf_jo_scene_prepare_begin("R18MAP4.BIN")==0);
        unsigned opens=save_opens,drains=save_drains;size_t cache=wf_jo_assets_sprite_cache_active_bytes();
        assert(f->config.records_ready&&f->config.records_ready(f->config.user,f->driver.ram,f->driver.ram_bytes)==WF_OK);
        assert(save_opens==opens&&save_writes==1u&&save_drains==drains);
        assert(wf_jo_scene_prepare_active("R18MAP4.BIN")&&wf_jo_assets_sprite_cache_active_bytes()==cache);
        wf_jo_scene_prepare_cancel();
    }
    longjmp(host_terminal_exit,1);
}
int main(int argc,char **argv){
    assert(argc==3);save_fault=(unsigned)strtoul(argv[2],NULL,0);assert(save_fault<=2u);
    FILE *rom=fopen("ROM68K.BIN","rb");assert(rom&&!fseek(rom,0x6db0,SEEK_SET));
    assert(fread(save_factory,1,sizeof save_factory,rom)==sizeof save_factory);assert(!fclose(rom));
    wf_r18_save_data_t data;wf_r18_save_defaults(&data,save_factory);data.rankings[1]=0x51u;
    /* Valid persisted Royal table with zero scores makes this real result
     * eligible without altering any gameplay RAM or outcome. */
    for(unsigned i=0;i<11u;i++)data.rankings[66u+i*6u]=0;
    assert(wf_r18_save_encode(&data,save_disk));
    char *args[]={argv[0],"1","40000",argv[1],NULL};
    int result=first_entry_main(4,args);assert(!result&&!failed&&save_post_seen&&!save_opened);
    printf("PASS actual main save: fault=%u inits=%u opens=%u closes=%u reads=%u writes=%u native_ticks=%u\n",
        save_fault,save_inits,save_opens,save_closes,save_reads,save_writes,rt.g.frame_counter);
    return 0;
}
