/* Derived from supplied session14 R05 reference/source; original retained read-only.
 * Silent fixed-attract integration, 2026-11-02. See PROVENANCE.md. */
/* Sequential CD transport for the independent WP0 attract player.
 * Commands/register layout follow libyaul and Sega ST-162-062094 sections3
 * and8.2.1. The active path never calls libyaul's blocking command executor.
 * Original implementation; the hidden setup uses the existing libyaul API. */
#include "wp0_cd_stream.h"
#include <string.h>

#define STATUS_PERIODIC 0x20u
#define STATUS_TRANSFER 0x40u
#define STATUS_WAIT 0x80u
#define STATUS_REJECT 0xffu
#define CD_PARTITION_MAX_SECTORS 200u
#define SERVICE_STEPS 12u

static bool fail(wp0_cd_stream_t *s,wp0_cd_stream_error_t error){
    if(!s->error)s->error=error;
    s->state=WP0_CD_STREAM_FAILED;
    return false;
}
static void state_set(wp0_cd_stream_t *s,wp0_cd_stream_state_t state){
    s->state=state;s->wait_start_field=s->last_field;
}
static uint32_t minimum(uint32_t a,uint32_t b){return a<b?a:b;}

bool wp0_cd_stream_init(wp0_cd_stream_t *s,void *ring,uint32_t capacity,
                        uint32_t bytes,const wp0_cd_stream_ops_t *ops){
    if(!s)return false;
    memset(s,0,sizeof *s);
    if(!ring||((uintptr_t)ring&3u)||capacity<WP0_CD_STREAM_SECTOR_BYTES||
       capacity>WP0_CD_STREAM_MAX_CAPACITY||(capacity&(capacity-1u))||
       !bytes||bytes>UINT32_MAX-(WP0_CD_STREAM_SECTOR_BYTES-1u)||
       !ops||!ops->hirq||!ops->clear_hirq||!ops->issue||!ops->response||
       !ops->read_words||!ops->start_hidden)
        return fail(s,WP0_CD_STREAM_BAD_ARGUMENT);
    s->ops=*ops;s->ring=ring;s->capacity=capacity;s->file_bytes=bytes;
    s->sector_count=(bytes+WP0_CD_STREAM_SECTOR_BYTES-1u)/WP0_CD_STREAM_SECTOR_BYTES;
    s->timeout_fields=WP0_CD_STREAM_TIMEOUT_FIELDS;
    s->state=WP0_CD_STREAM_OFF;
    return true;
}

bool wp0_cd_stream_start_hidden(wp0_cd_stream_t *s,uint32_t fad){
    if(!s||s->error)return false;
    if(s->state!=WP0_CD_STREAM_OFF||fad<150u||fad>=0x800000u||
       s->sector_count>0x800000u-fad)
        return fail(s,WP0_CD_STREAM_BAD_ARGUMENT);
    s->start_result=s->ops.start_hidden(s->ops.context,fad,s->sector_count);
    if(s->start_result)return fail(s,WP0_CD_STREAM_START_FAILED);
    state_set(s,WP0_CD_STREAM_QUERY);
    return true;
}

bool wp0_cd_stream_restart_async(wp0_cd_stream_t *s,uint32_t fad,uint32_t bytes){
    if(!s||s->error||s->stop_requested)return false;
    /* Establish completion of the OLD extent before considering or assigning
     * the replacement length. No unread old bytes may be silently discarded. */
    if(s->state!=WP0_CD_STREAM_DONE||s->produced!=s->file_bytes||
       s->consumed!=s->file_bytes||s->sectors_received!=s->sector_count||
       s->pending_sectors||s->pending_bytes||s->pending_valid)
        return fail(s,WP0_CD_STREAM_REWIND_NOT_READY);
    if(!bytes||bytes>UINT32_MAX-(WP0_CD_STREAM_SECTOR_BYTES-1u))
        return fail(s,WP0_CD_STREAM_BAD_ARGUMENT);
    uint32_t sectors=(bytes+WP0_CD_STREAM_SECTOR_BYTES-1u)/WP0_CD_STREAM_SECTOR_BYTES;
    if(fad<150u||fad>=0x800000u||sectors>0x800000u-fad)
        return fail(s,WP0_CD_STREAM_BAD_ARGUMENT);
    /* The exact play range was entirely fetched with Get Then Delete and its
     * last transfer reached EHST. With exclusive CD ownership, partition0 is
     * empty. Keep its connection/sector setup; only request a new bounded PLAY.
     * Do not clear ring RAM: old contents stay unpublished until overwritten. */
    s->file_bytes=bytes;s->sector_count=sectors;
    s->produced=s->consumed=s->write_pos=s->read_pos=s->sectors_received=0;
    s->play_fad=fad;s->empty_query_valid=false;s->data_wait_active=false;
    s->last_end_words=0;
    /* Keep cumulative diagnostics AND the current field's command budget. */
    state_set(s,WP0_CD_STREAM_PLAY);
    return true;
}

bool wp0_cd_stream_rewind_async(wp0_cd_stream_t *s,uint32_t fad){
    return s?wp0_cd_stream_restart_async(s,fad,s->file_bytes):false;
}

static bool issue_registers(wp0_cd_stream_t *s,const wp0_cd_regs_t *r,
                           uint16_t required,uint16_t clear,wp0_cd_stream_state_t next){
    if(s->commands_this_field>=WP0_CD_STREAM_COMMANDS_PER_FIELD)return false;
    if(!s->ops.issue(s->ops.context,r,required,clear))return false;
    s->last_command=r->cr1;++s->commands_issued;++s->commands_this_field;
    state_set(s,next);return true;
}
static bool issue(wp0_cd_stream_t *s,uint16_t command,uint16_t count,
                  uint16_t required,uint16_t clear,wp0_cd_stream_state_t next){
    const wp0_cd_regs_t r={command,0,0,count};
    return issue_registers(s,&r,required,clear,next);
}

/* A completed response is read once, atomically, by the adapter. A periodic
 * report's CR4 is a timestamp/FAD component and is NEVER a sector count. */
static int response(wp0_cd_stream_t *s){
    if(!s->ops.response(s->ops.context,&s->last_response))return 0;
    unsigned status=s->last_response.cr1>>8;
    if(status==STATUS_REJECT){fail(s,WP0_CD_STREAM_REJECT);return -1;}
    if(status&STATUS_WAIT){fail(s,WP0_CD_STREAM_WAIT);return -1;}
    unsigned drive=status&15u;
    if(drive==6u||drive==7u||drive==9u||drive==10u){
        fail(s,WP0_CD_STREAM_DRIVE_ERROR);return -1;
    }
    if(status&STATUS_PERIODIC){
        ++s->periodic_responses;
        if(s->state==WP0_CD_STREAM_QUERY_WAIT){
            s->last_empty_query_field=s->last_field;s->empty_query_valid=true;
            state_set(s,WP0_CD_STREAM_QUERY);return 2;
        }
        fail(s,WP0_CD_STREAM_PERIODIC);return -1;
    }
    return 1;
}

bool wp0_cd_stream_service(wp0_cd_stream_t *s,uint32_t field){
    if(!s||s->error)return false;
    ++s->service_calls;
    if(!s->clock_started||s->last_field!=field){
        s->last_field=field;s->commands_this_field=0;
        if(!s->clock_started){s->clock_started=true;s->wait_start_field=field;}
    }
    if(s->state==WP0_CD_STREAM_OFF||s->state==WP0_CD_STREAM_DONE)return true;
    if(s->state!=WP0_CD_STREAM_QUERY&&s->state!=WP0_CD_STREAM_PLAY&&
       field-s->wait_start_field>=s->timeout_fields)
        return fail(s,WP0_CD_STREAM_TIMEOUT);
    /* Empty/periodic queries can complete forever without delivering data.
     * Bound that loss of progress too, but a full consumer ring is permitted
     * to remain full indefinitely and must not be diagnosed as a drive fault. */
    if(s->state==WP0_CD_STREAM_QUERY&&
       s->capacity-(s->produced-s->consumed)<WP0_CD_STREAM_SECTOR_BYTES)
        s->data_wait_active=false;
    else if(!s->data_wait_active){
        s->data_wait_start_field=field;s->data_wait_active=true;
    }else if(field-s->data_wait_start_field>=s->timeout_fields)
        return fail(s,WP0_CD_STREAM_TIMEOUT);
    uint32_t copied=0;
    for(unsigned step=0;step<SERVICE_STEPS;step++){
        s->last_hirq=s->ops.hirq(s->ops.context);
        if(s->last_hirq&WP0_CD_DCHG)return fail(s,WP0_CD_STREAM_DISC_CHANGED);
        if(s->stop_requested&&(s->state==WP0_CD_STREAM_QUERY||s->state==WP0_CD_STREAM_PLAY))return true;
        switch(s->state){
        case WP0_CD_STREAM_PLAY:{
            const wp0_cd_regs_t r={
                (uint16_t)(0x1080u|(s->play_fad>>16)),(uint16_t)s->play_fad,
                (uint16_t)(0x0080u|(s->sector_count>>16)),(uint16_t)s->sector_count};
            if(!issue_registers(s,&r,WP0_CD_CMOK,
                                WP0_CD_CMOK|WP0_CD_PEND|WP0_CD_CSCT,
                                WP0_CD_STREAM_PLAY_WAIT))return true;
            break;
        }
        case WP0_CD_STREAM_PLAY_WAIT:{
            int r=response(s);if(r<=0)return r==0;
            state_set(s,WP0_CD_STREAM_QUERY);break;
        }
        case WP0_CD_STREAM_QUERY:
            if(s->capacity-(s->produced-s->consumed)<WP0_CD_STREAM_SECTOR_BYTES)
                return true;
            if(s->empty_query_valid&&s->last_empty_query_field==field)return true;
            if(!issue(s,0x5100,0,WP0_CD_CMOK,WP0_CD_CMOK,
                      WP0_CD_STREAM_QUERY_WAIT))return true;
            break;
        case WP0_CD_STREAM_QUERY_WAIT:{
            int r=response(s);if(r<=0)return r==0;if(r==2)return true;
            uint32_t count=s->last_response.cr4;
            if(count>CD_PARTITION_MAX_SECTORS)return fail(s,WP0_CD_STREAM_BAD_COUNT);
            if(!count){
                s->last_empty_query_field=field;s->empty_query_valid=true;
                state_set(s,WP0_CD_STREAM_QUERY);return true;
            }
            uint32_t free_sectors=(s->capacity-(s->produced-s->consumed))/WP0_CD_STREAM_SECTOR_BYTES;
            count=minimum(count,free_sectors);
            count=minimum(count,s->sector_count-s->sectors_received);
            count=minimum(count,WP0_CD_STREAM_BURST_BYTES/WP0_CD_STREAM_SECTOR_BYTES);
            if(!count)return fail(s,WP0_CD_STREAM_BAD_COUNT);
            s->pending_sectors=count;s->pending_bytes=count*WP0_CD_STREAM_SECTOR_BYTES;
            s->pending_valid=minimum(s->pending_bytes,s->file_bytes-s->produced);
            state_set(s,WP0_CD_STREAM_GET);
            break;
        }
        case WP0_CD_STREAM_GET:
            if(!issue(s,0x6300,(uint16_t)s->pending_sectors,
                      WP0_CD_CMOK|WP0_CD_EHST,WP0_CD_CMOK|WP0_CD_EHST|WP0_CD_DRDY,
                      WP0_CD_STREAM_GET_WAIT))return true;
            break;
        case WP0_CD_STREAM_GET_WAIT:{
            int r=response(s);if(r<=0)return r==0;
            if(!(s->last_response.cr1&(STATUS_TRANSFER<<8)))
                return fail(s,WP0_CD_STREAM_NO_TRANSFER);
            state_set(s,WP0_CD_STREAM_DATA);break;
        }
        case WP0_CD_STREAM_DATA:{
            if(!(s->last_hirq&WP0_CD_DRDY))return true;
            if(copied+s->pending_bytes>WP0_CD_STREAM_BURST_BYTES)return true;
            s->ops.clear_hirq(s->ops.context,WP0_CD_DRDY);
            uint32_t first=minimum(s->pending_bytes,s->capacity-s->write_pos);
            uint32_t n=s->ops.read_words(s->ops.context,s->ring+s->write_pos,first/4u);
            s->bytes_copied+=n*4u;copied+=n*4u;
            if(n!=first/4u)return fail(s,WP0_CD_STREAM_SHORT_READ);
            uint32_t rest=s->pending_bytes-first;
            if(rest){
                n=s->ops.read_words(s->ops.context,s->ring,rest/4u);
                s->bytes_copied+=n*4u;copied+=n*4u;
                if(n!=rest/4u)return fail(s,WP0_CD_STREAM_SHORT_READ);
            }
            if(copied>s->max_service_bytes)s->max_service_bytes=copied;
            state_set(s,WP0_CD_STREAM_END);break;
        }
        case WP0_CD_STREAM_END:
            if(!issue(s,0x0600,0,WP0_CD_CMOK,WP0_CD_CMOK,
                      WP0_CD_STREAM_END_WAIT))return true;
            break;
        case WP0_CD_STREAM_END_WAIT:{
            int r=response(s);if(r<=0)return r==0;
            s->last_end_words=((uint32_t)(s->last_response.cr1&255u)<<16)|s->last_response.cr2;
            if(s->last_end_words!=s->pending_bytes/2u)
                return fail(s,WP0_CD_STREAM_WORD_COUNT);
            state_set(s,WP0_CD_STREAM_HOST_END);break;
        }
        case WP0_CD_STREAM_HOST_END:
            if(!(s->last_hirq&WP0_CD_EHST))return true;
            s->produced+=s->pending_valid;
            s->write_pos=(s->write_pos+s->pending_bytes)&(s->capacity-1u);
            s->sectors_received+=s->pending_sectors;
            s->pending_sectors=s->pending_bytes=s->pending_valid=0;
            s->data_wait_active=false;
            if(s->sectors_received==s->sector_count){
                state_set(s,WP0_CD_STREAM_DONE);return true;
            }
            state_set(s,WP0_CD_STREAM_QUERY);
            return true;
        case WP0_CD_STREAM_DONE:case WP0_CD_STREAM_OFF:return true;
        default:return fail(s,WP0_CD_STREAM_BAD_ARGUMENT);
        }
    }
    return true;
}

/* Integration derivative: finish one outstanding GET/END/EHST, then pause the
 * finite extent, disconnect the device and reset its selector. Register
 * encodings verified against the supplied Yaul cmd_disk_seek/connection_set/
 * selector_reset implementation. No blocking SDK executor or CD init here. */
bool wp0_cd_stream_stop_request(wp0_cd_stream_t *s,uint32_t field){
    if(!s||s->error)return false;
    if(!s->stop_requested){s->stop_requested=true;s->stop_start_field=field;s->stop_stage=0;}
    return true;
}
int wp0_cd_stream_stop_service(wp0_cd_stream_t *s,uint32_t field){
    if(!s||!s->stop_requested||s->error)return -1;
    if(s->stop_stage==8)return 1;
    if(field-s->stop_start_field>=WP0_CD_STREAM_TIMEOUT_FIELDS){fail(s,WP0_CD_STREAM_TIMEOUT);return -1;}
    uint16_t h=s->ops.hirq(s->ops.context);
    if(h&WP0_CD_DCHG){fail(s,WP0_CD_STREAM_DISC_CHANGED);return -1;}
    if(!s->stop_stage){
        switch(s->state){
        case WP0_CD_STREAM_QUERY:case WP0_CD_STREAM_DONE:
        case WP0_CD_STREAM_OFF:case WP0_CD_STREAM_PLAY:break;
        default:if(!wp0_cd_stream_service(s,field))return -1;return 0;
        }
        if((h&(WP0_CD_CMOK|WP0_CD_EHST))!=(WP0_CD_CMOK|WP0_CD_EHST))return 0;
        s->stop_stage=1;
    }
    /* At most one command per call/physical field in the native stop loop. */
    wp0_cd_regs_t r={0,0,0,0};uint16_t clear=WP0_CD_CMOK;
    switch(s->stop_stage){
    case 1:r.cr1=0x11ff;r.cr2=0xffff;break; /* SEEK FFFFFF = pause */
    case 3:r.cr1=0x3000;r.cr3=0xff00;clear|=0x0040;break;
    case 5:
        if(!(h&0x0040))return 0;
        r.cr1=0x4800;clear|=0x0240;break;
    case 2:case 4:case 6:{
        int got=response(s);if(got<0)return -1;if(!got)return 0;
        ++s->stop_stage;return 0;
    }
    case 7:
        if((h&(WP0_CD_CMOK|WP0_CD_EHST|0x0040))!=(WP0_CD_CMOK|WP0_CD_EHST|0x0040))return 0;
        s->state=WP0_CD_STREAM_OFF;s->stop_stage=8;return 1;
    default:fail(s,WP0_CD_STREAM_BAD_ARGUMENT);return -1;
    }
    if(s->ops.issue(s->ops.context,&r,WP0_CD_CMOK|WP0_CD_EHST,clear)){
        s->last_command=r.cr1;++s->commands_issued;++s->stop_stage;
    }
    return 0;
}

uint32_t wp0_cd_stream_available(const wp0_cd_stream_t *s){
    return s&&!s->error?s->produced-s->consumed:0;
}
const uint8_t *wp0_cd_stream_peek(const wp0_cd_stream_t *s,uint32_t *bytes){
    uint32_t n=wp0_cd_stream_available(s);
    if(n)n=minimum(n,s->capacity-s->read_pos);
    if(bytes)*bytes=n;
    return n?s->ring+s->read_pos:NULL;
}
bool wp0_cd_stream_copy(const wp0_cd_stream_t *s,uint32_t offset,void *dest,uint32_t bytes){
    if(!s||s->error||(!dest&&bytes))return false;
    uint32_t n=wp0_cd_stream_available(s);
    if(offset>n||bytes>n-offset)return false;
    if(!bytes)return true;
    uint32_t pos=(s->read_pos+offset)&(s->capacity-1u);
    uint32_t first=minimum(bytes,s->capacity-pos);
    memcpy(dest,s->ring+pos,first);
    if(bytes>first)memcpy((uint8_t *)dest+first,s->ring,bytes-first);
    return true;
}
bool wp0_cd_stream_consume(wp0_cd_stream_t *s,uint32_t bytes){
    if(!s||s->error||bytes>wp0_cd_stream_available(s))return false;
    s->consumed+=bytes;s->read_pos=(s->read_pos+bytes)&(s->capacity-1u);
    return true;
}
bool wp0_cd_stream_eof(const wp0_cd_stream_t *s){
    return s&&!s->error&&s->state==WP0_CD_STREAM_DONE;
}

#ifdef WP0_CD_STREAM_SATURN
#include <yaul.h>
#define CD_REG16(offset) (*(volatile uint16_t *)(0x25890000UL+(offset)))
#define CD_FIFO32 (*(volatile uint32_t *)0x25818000UL)
#define HIRQ_OFFSET 0x08u
#define CR1_OFFSET 0x18u
#define CR2_OFFSET 0x1cu
#define CR3_OFFSET 0x20u
#define CR4_OFFSET 0x24u
#define HIRQ_VALID 0x3fffu
#define ESEL_FLAG 0x0040u

static uint16_t hardware_hirq(void *unused){(void)unused;return CD_REG16(HIRQ_OFFSET);}
static void hardware_clear(void *unused,uint16_t bits){
    (void)unused;
    /* HIRQ is write-zero-to-clear. Ones preserve other events, avoiding a
     * read/modify/write race with asynchronous CD-block event assertions. */
    CD_REG16(HIRQ_OFFSET)=(uint16_t)(HIRQ_VALID&~bits);
}
static bool hardware_issue(void *unused,const wp0_cd_regs_t *r,uint16_t required,uint16_t clear){
    (void)unused;
    uint32_t saved=cpu_intc_mask_get();cpu_intc_mask_set(15);
    if((CD_REG16(HIRQ_OFFSET)&required)!=required){cpu_intc_mask_set(saved);return false;}
    CD_REG16(HIRQ_OFFSET)=(uint16_t)(HIRQ_VALID&~clear);
    CD_REG16(CR1_OFFSET)=r->cr1;CD_REG16(CR2_OFFSET)=r->cr2;
    CD_REG16(CR3_OFFSET)=r->cr3;CD_REG16(CR4_OFFSET)=r->cr4;
    __asm__ volatile("" ::: "memory");cpu_intc_mask_set(saved);return true;
}
static bool hardware_response(void *unused,wp0_cd_regs_t *r){
    (void)unused;
    uint32_t saved=cpu_intc_mask_get();cpu_intc_mask_set(15);
    if(!(CD_REG16(HIRQ_OFFSET)&WP0_CD_CMOK)){cpu_intc_mask_set(saved);return false;}
    r->cr1=CD_REG16(CR1_OFFSET);r->cr2=CD_REG16(CR2_OFFSET);
    r->cr3=CD_REG16(CR3_OFFSET);r->cr4=CD_REG16(CR4_OFFSET);
    __asm__ volatile("" ::: "memory");cpu_intc_mask_set(saved);return true;
}
static uint32_t hardware_read(void *unused,void *output,uint32_t words){
    (void)unused;uint32_t *dst=output;
    /* Aligned32-bit stores on big-endian SH-2 preserve the FIFO bytes.
     * This bounded loop contains no readiness polling and no IRQ masking. */
    for(uint32_t i=0;i<words;i++)dst[i]=CD_FIFO32;
    return words;
}
static int hidden_wait_esel(void){
    for(volatile uint32_t i=0;i<0x240000u;i++)if(CD_REG16(HIRQ_OFFSET)&ESEL_FLAG)return 0;
    return 0xaau;
}
static int hardware_start(void *unused,uint32_t fad,uint32_t sectors){
    (void)unused;int r;
    /* Hidden startup only. These SDK calls are unreachable from service. */
    if((r=cd_block_cmd_data_transfer_end()))return r;
    if((r=cd_block_cmd_sector_length_set(SECTOR_LENGTH_2048)))return r;
    if((r=hidden_wait_esel()))return r;
    if((r=cd_block_cmd_selector_reset(0,0)))return r;
    if((r=hidden_wait_esel()))return r;
    if((r=cd_block_cmd_cd_dev_connection_set(0)))return r;
    if((r=hidden_wait_esel()))return r;
    hardware_clear(NULL,WP0_CD_PEND|WP0_CD_CSCT|WP0_CD_DCHG|WP0_CD_DRDY);
    return cd_block_cmd_disk_play(0,fad,(int32_t)sectors);
}
wp0_cd_stream_ops_t wp0_cd_stream_saturn_ops(void){
    const wp0_cd_stream_ops_t ops={NULL,hardware_hirq,hardware_clear,
        hardware_issue,hardware_response,hardware_read,hardware_start};
    return ops;
}
#endif
