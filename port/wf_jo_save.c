#include "wf_jo_save.h"
#include <string.h>
_Static_assert(WF_JO_SAVE_WORKSPACE_BYTES >= WF_JO_SAVE_LIBRARY_BYTES + WF_JO_SAVE_BIOS_WORK_BYTES,
               "Sega STN-52 backup work area must include its 120-byte variables");
_Static_assert(WF_JO_SAVE_WORKSPACE_BYTES % 8192u == 0u,
               "Backup lease must cover complete 4 KiB and 8 KiB sprite-cache slots");
#ifdef __sh__
#include <jo/jo.h>

/* Same documented BIOS ABI used by Jo's backup module, with a temporary
 * caller lease and no formatting path. Jo retains reset/SMPC ownership. */
typedef struct {uint16_t unit,partitions;} bios_config_t;
typedef struct {uint32_t bytes,blocks,block_bytes,free_bytes,free_blocks,files;} bios_stat_t;
typedef struct {
    uint8_t filename[12],comment[11],language;
    uint32_t date,bytes;
    uint16_t blocks;
} bios_file_t;
typedef struct {uint8_t year,month,day,hour,minute,week;} bios_date_t;
_Static_assert(sizeof(bios_config_t)==4u,"Saturn BIOS configuration ABI");
_Static_assert(sizeof(bios_stat_t)==24u,"Saturn BIOS status ABI");
_Static_assert(offsetof(bios_file_t,date)==24u,"Saturn BIOS directory ABI");
#define BIOS_ENTRY(offset) (*(volatile uintptr_t *)(*(volatile uintptr_t *)0x06000354u+(offset)))
#define BIOS_FN(offset,type) ((type)BIOS_ENTRY(offset))
static int bios_open(void *unused,void *workspace,size_t bytes){
    (void)unused;
    if(!workspace||bytes<WF_JO_SAVE_WORKSPACE_BYTES||((uintptr_t)workspace&3u))return 0;
    bios_config_t config[3]={{0,0},{0,0},{0,0}};
    bios_stat_t stat;
    jo_core_disable_reset();
#ifdef JO_DEBUG
    /* BUP_Init legitimately reads the BIOS decompressor vector at ROM $5F8
     * (MPR-17933: $7D616). Jo's data-access NULL trap covers $000-$FFF.
     * Suspend only channel A for this BIOS call; preserve its exact mode and
     * every previously recorded fault. Ordinary game NULL checks stay on. */
    volatile uint16_t *const bbra=(volatile uint16_t *)0xffffff48u;
    uint16_t null_trap_mode=*bbra;
    *bbra=0;
    /* SH7604 6.3.7: dummy-read the last UBC register written so its new
     * setting is effective before the following CPU access. */
    (void)*bbra;
#endif
    ((void (*)(void *,void *,bios_config_t *))(*(volatile uintptr_t *)0x06000358u))
        (workspace,(uint8_t *)workspace+WF_JO_SAVE_LIBRARY_BYTES,config);
#ifdef JO_DEBUG
    *bbra=null_trap_mode;
    (void)*bbra;
#endif
    unsigned partition=BIOS_FN(4u,unsigned (*)(unsigned,unsigned short))(0,0);
    int status=partition? -1:BIOS_FN(12u,int (*)(unsigned,unsigned,bios_stat_t *))(0,0,&stat);
    if(status){jo_core_enable_reset();return 0;}
    return 1;
}
static void bios_close(void *unused){(void)unused;jo_core_enable_reset();}
static int bios_find(void *unused,const char *name,size_t *bytes){
    (void)unused;bios_file_t file;memset(&file,0,sizeof file);
    int count=BIOS_FN(28u,int (*)(unsigned,const char *,unsigned short,bios_file_t *))(0,name,1,&file);
    if(!count)return 0;
    /* The game name occupies the full permitted eleven characters, so no
     * longer valid filename can match its prefix. Still check exact bytes. */
    if(count!=1||memcmp(file.filename,name,12u))return -1;
    *bytes=file.bytes;return 1;
}
static int bios_read(void *unused,const char *name,void *out,size_t bytes){
    (void)unused;if(bytes!=WF_R18_SAVE_IMAGE_BYTES)return 0;
    return !BIOS_FN(20u,int (*)(unsigned,const char *,void *))(0,name,out);
}
static int bios_write(void *unused,const char *name,const void *data,size_t bytes){
    (void)unused;if(bytes!=WF_R18_SAVE_IMAGE_BYTES)return 0;
    bios_stat_t stat;
    if(BIOS_FN(12u,int (*)(unsigned,unsigned,bios_stat_t *))(0,(unsigned)bytes,&stat))return 0;
    bios_file_t file;memset(&file,0,sizeof file);
    memcpy(file.filename,name,12u);memcpy(file.comment,"WRESTLEFST",10u);file.language=1;
    jo_datetime now;jo_getdate(&now);
    bios_date_t date={20,1,1,0,0,0};
    if(now.year>=1980u&&now.year<=2100u&&now.month>=1u&&now.month<=12u&&now.day>=1u&&now.day<=31u){
        date.year=(uint8_t)(now.year-1980u);date.month=now.month;date.day=now.day;
        date.hour=now.hour;date.minute=now.minute;date.week=now.week;
    }
    file.date=BIOS_FN(40u,unsigned (*)(bios_date_t *))(&date);file.bytes=(uint32_t)bytes;
    return !BIOS_FN(16u,int (*)(unsigned,bios_file_t *,const void *,unsigned char))(0,&file,data,0);
}
static int bios_verify(void *unused,const char *name,const void *data,size_t bytes){
    (void)unused;if(bytes!=WF_R18_SAVE_IMAGE_BYTES)return 0;
    return !BIOS_FN(32u,int (*)(unsigned,const char *,const void *))(0,name,data);
}
static const wf_jo_save_backend_t internal_backend={bios_open,bios_close,bios_find,bios_read,bios_write,bios_verify};
#endif

static const char filename[12]="WFESTSAT001";
static int ready(const wf_jo_save_t *s){
    const wf_jo_save_backend_t*b=s?s->backend:NULL;
    return b&&b->open&&b->close&&b->find&&b->read&&b->write&&b->verify;
}
void wf_jo_save_init(wf_jo_save_t *s,const wf_jo_save_backend_t *backend,void *user){
    if(!s)return;
    memset(s,0,sizeof *s);s->backend=backend;s->user=user;
#ifdef __sh__
    if(!backend)s->backend=&internal_backend;
#endif
}
wf_jo_save_status_t wf_jo_save_load(wf_jo_save_t *s,uint8_t *rankings,void *workspace,size_t bytes){
    if(!s)return WF_JO_SAVE_UNAVAILABLE;
    s->writable=s->cached=0;s->status=WF_JO_SAVE_UNAVAILABLE;
    if(!rankings||!ready(s))return s->status;
    wf_r18_save_defaults(&s->data,rankings);
    if(!workspace||bytes<WF_JO_SAVE_WORKSPACE_BYTES||((uintptr_t)workspace&3u)||
       !s->backend->open(s->user,workspace,bytes))return s->status;
    size_t file_bytes=0;int found=s->backend->find(s->user,filename,&file_bytes);
    if(!found){s->writable=s->cached=1;s->status=WF_JO_SAVE_DEFAULTS;}
    else{
        uint8_t image[WF_R18_SAVE_IMAGE_BYTES];wf_r18_save_data_t decoded;
        if(found!=1||file_bytes!=sizeof image||!s->backend->read(s->user,filename,image,sizeof image)||
           !wf_r18_save_decode(image,&decoded))s->status=WF_JO_SAVE_CORRUPT;
        else{
            s->data=decoded;memcpy(rankings,decoded.rankings,WF_R18_SAVE_RANKING_BYTES);
            s->writable=s->cached=1;s->status=WF_JO_SAVE_LOADED;
        }
    }
    s->backend->close(s->user);return s->status;
}
int wf_jo_save_needs_store(const wf_jo_save_t *s,const uint8_t *rankings){
    return s&&s->writable&&rankings&&ready(s)&&
        (!s->cached||memcmp(rankings,s->data.rankings,WF_R18_SAVE_RANKING_BYTES)!=0);
}
wf_jo_save_status_t wf_jo_save_store(wf_jo_save_t *s,const uint8_t *rankings,void *workspace,size_t bytes){
    if(!s)return WF_JO_SAVE_UNAVAILABLE;
    if(!wf_jo_save_needs_store(s,rankings))return s->status;
    wf_r18_save_data_t next=s->data;uint8_t image[WF_R18_SAVE_IMAGE_BYTES];
    memcpy(next.rankings,rankings,WF_R18_SAVE_RANKING_BYTES);
    if(!wf_r18_save_encode(&next,image))return s->status=WF_JO_SAVE_WRITE_ERROR;
    if(!workspace||bytes<WF_JO_SAVE_WORKSPACE_BYTES||((uintptr_t)workspace&3u)||
       !s->backend->open(s->user,workspace,bytes))return s->status=WF_JO_SAVE_UNAVAILABLE;
    int success=s->backend->write(s->user,filename,image,sizeof image)&&
                s->backend->verify(s->user,filename,image,sizeof image);
    s->backend->close(s->user);
    if(success){s->data=next;s->cached=1;s->status=WF_JO_SAVE_SAVED;}
    else s->status=WF_JO_SAVE_WRITE_ERROR;
    return s->status;
}
