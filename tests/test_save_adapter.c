/* Execute the real save transaction with a deterministic backup device.
 * Only BIOS I/O is substituted; wire format, CRC, cache and failure policy
 * are the production implementation. There is deliberately no format API. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "wf_jo_save.h"

typedef struct {
    uint8_t file[WF_R18_SAVE_IMAGE_BYTES];
    size_t length;
    unsigned exists, opened, opens, closes, reads, writes;
    int unavailable, bad_dir, bad_read, full, bad_write, bad_verify;
} disk_t;
static _Alignas(4) uint8_t workspace[WF_JO_SAVE_WORKSPACE_BYTES];
static int open_disk(void *user,void *area,size_t bytes){
    disk_t*d=user;assert(!d->opened);++d->opens;
    if(d->unavailable||!area||bytes<sizeof workspace||((uintptr_t)area&3u))return 0;
    d->opened=1;memset(area,0xa5,bytes);return 1;
}
static void close_disk(void *user){disk_t*d=user;assert(d->opened);d->opened=0;++d->closes;}
static int find_file(void *user,const char*name,size_t *bytes){
    disk_t*d=user;assert(d->opened&&!strcmp(name,"WFESTSAT001"));
    if(d->bad_dir)return -1;
    if(!d->exists)return 0;
    *bytes=d->length;return 1;
}
static int read_file(void*user,const char*name,void*out,size_t bytes){
    disk_t*d=user;(void)name;assert(d->opened);++d->reads;
    if(d->bad_read||!d->exists||bytes!=d->length)return 0;
    memcpy(out,d->file,bytes);return 1;
}
static int write_file(void*user,const char*name,const void*data,size_t bytes){
    disk_t*d=user;(void)name;assert(d->opened);++d->writes;
    if(d->full||d->bad_write)return 0;
    assert(bytes==sizeof d->file);memcpy(d->file,data,bytes);d->length=bytes;d->exists=1;return 1;
}
static int verify_file(void*user,const char*name,const void*data,size_t bytes){
    disk_t*d=user;(void)name;assert(d->opened);return !d->bad_verify&&d->length==bytes&&!memcmp(data,d->file,bytes);
}
static const wf_jo_save_backend_t io={open_disk,close_disk,find_file,read_file,write_file,verify_file};
static wf_jo_save_status_t load(wf_jo_save_t*s,uint8_t*r){return wf_jo_save_load(s,r,workspace,sizeof workspace);}
static wf_jo_save_status_t save(wf_jo_save_t*s,uint8_t*r){return wf_jo_save_store(s,r,workspace,sizeof workspace);}
int main(void){
    disk_t disk={0};wf_jo_save_t state;uint8_t original[132],ranks[132];
    for(unsigned i=0;i<sizeof original;++i)original[i]=(uint8_t)i;
    memcpy(ranks,original,sizeof ranks);wf_jo_save_init(&state,&io,&disk);
    assert(!wf_jo_save_needs_store(&state,ranks));
    assert(save(&state,ranks)==WF_JO_SAVE_UNAVAILABLE&&disk.opens==0);
    assert(wf_jo_save_load(&state,ranks,workspace,sizeof workspace-1u)==WF_JO_SAVE_UNAVAILABLE&&disk.opens==0);
    assert(wf_jo_save_load(&state,ranks,workspace+1,sizeof workspace)==WF_JO_SAVE_UNAVAILABLE&&disk.opens==0);
    assert(wf_jo_save_load(&state,NULL,workspace,sizeof workspace)==WF_JO_SAVE_UNAVAILABLE&&disk.opens==0);
    assert(load(&state,ranks)==WF_JO_SAVE_DEFAULTS&&!disk.exists&&!disk.writes);
    assert(!wf_jo_save_needs_store(&state,ranks));
    assert(save(&state,ranks)==WF_JO_SAVE_DEFAULTS&&!disk.exists&&disk.opens==1);
    ranks[6]=99;
    assert(wf_jo_save_needs_store(&state,ranks));
    assert(wf_jo_save_store(&state,ranks,NULL,0)==WF_JO_SAVE_UNAVAILABLE&&!disk.exists&&disk.opens==1);
    assert(save(&state,ranks)==WF_JO_SAVE_SAVED&&disk.writes==1);
    assert(!wf_jo_save_needs_store(&state,ranks));
    unsigned opens=disk.opens;assert(save(&state,ranks)==WF_JO_SAVE_SAVED&&disk.opens==opens&&disk.writes==1);
    wf_jo_save_init(&state,&io,&disk);memset(ranks,0,sizeof ranks);
    assert(load(&state,ranks)==WF_JO_SAVE_LOADED&&ranks[6]==99);
    assert(save(&state,ranks)==WF_JO_SAVE_LOADED&&disk.writes==1);
    /* Options from an existing valid WFS1 survive a rankings-only update. */
    wf_r18_save_data_t data;assert(wf_r18_save_decode(disk.file,&data));memset(data.options,0x6b,sizeof data.options);
    assert(wf_r18_save_encode(&data,disk.file));wf_jo_save_init(&state,&io,&disk);assert(load(&state,ranks)==WF_JO_SAVE_LOADED);
    ranks[0]^=1;assert(save(&state,ranks)==WF_JO_SAVE_SAVED);assert(wf_r18_save_decode(disk.file,&data));
    for(unsigned i=0;i<sizeof data.options;++i)assert(data.options[i]==0x6b);
    for(unsigned failure=0;failure<4;++failure){
        disk.full=failure==0;disk.bad_write=failure==1;disk.bad_verify=failure==2;disk.unavailable=failure==3;
        ranks[1]++;unsigned writes=disk.writes;
        assert(save(&state,ranks)==(failure==3?WF_JO_SAVE_UNAVAILABLE:WF_JO_SAVE_WRITE_ERROR));
        assert(wf_jo_save_needs_store(&state,ranks));
        disk.full=disk.bad_write=disk.bad_verify=disk.unavailable=0;
        assert(save(&state,ranks)==WF_JO_SAVE_SAVED&&disk.writes>writes);
    }
    /* Existing corrupt, wrong-size, unreadable and ambiguous records are
     * preserved. The game retains factory defaults and never overwrites. */
    for(unsigned failure=0;failure<4;++failure){
        uint8_t before[160];memcpy(before,disk.file,sizeof before);disk.length=sizeof disk.file;
        disk.file[159]^=failure==0;disk.length-=failure==1;disk.bad_read=failure==2;disk.bad_dir=failure==3;
        wf_jo_save_init(&state,&io,&disk);memcpy(ranks,original,sizeof ranks);unsigned writes=disk.writes;
        assert(load(&state,ranks)==WF_JO_SAVE_CORRUPT&&!memcmp(ranks,original,sizeof ranks));
        assert(!wf_jo_save_needs_store(&state,ranks));
        assert(save(&state,ranks)==WF_JO_SAVE_CORRUPT&&disk.writes==writes);
        memcpy(disk.file,before,sizeof before);disk.length=sizeof disk.file;disk.bad_read=disk.bad_dir=0;
    }
    disk.unavailable=1;wf_jo_save_init(&state,&io,&disk);assert(load(&state,ranks)==WF_JO_SAVE_UNAVAILABLE);
    assert(save(&state,ranks)==WF_JO_SAVE_UNAVAILABLE);disk.unavailable=0;
    assert(!disk.opened&&disk.closes<=disk.opens);
    puts("PASS Jo save: reboot, unchanged writes, options, unavailable/full/write/verify, corrupt preservation");
}
