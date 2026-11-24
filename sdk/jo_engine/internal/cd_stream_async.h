/* Prepared music CD startup. Included only by fs.c, after CD definitions.
 * Jo retains sole drive ownership. One poll issues at most one command and
 * never spins on CMOK/ESEL. Normal file APIs explicitly drain this state.
 */
#ifndef JO_INTERNAL_CD_STREAM_ASYNC_H
#define JO_INTERNAL_CD_STREAM_ASYNC_H

static struct {
    unsigned int fad, count, phase;
    unsigned short wait_flags;
    bool active, restart, inflight;
} __jo_cd_async;
static unsigned int __jo_cd_async_errors;

static int jo_cd_stream_async_begin(unsigned int fad,unsigned int count)
{
    if (!__jo_cd_initialized || !count || fad >= 0x800000u || count > 0x800000u-fad)
        return -1;
    __jo_cd_async.fad=fad;
    __jo_cd_async.count=count;
    __jo_cd_async.active=__jo_cd_async.restart=true;
    /* This includes the pending startup. TAKE polls until it is ready. */
    __jo_cd_stream_owner=true;
    return 0;
}

static void jo_cd_stream_async_stop(void)
{
    __jo_cd_async.count=0;
    __jo_cd_async.active=__jo_cd_async.restart=true;
    __jo_cd_stream_owner=false;
}

/* 1: job complete, 0: command still pending, -1: explicit device rejection.
 * A superseding cue waits for the outstanding response before touching CR1-4.
 */
static int jo_cd_stream_async_poll(void)
{
    unsigned short c1,c2=0,c3=0,c4=0,flags=0;
    if (__jo_cd_async.inflight)
    {
        if (!(JO_CD_HIRQ & JO_CD_HIRQ_CMOK)) return 0;
        if ((JO_CD_CR1 >> 8) == JO_CD_STATUS_REJECT)
        {
            __jo_cd_async.active=__jo_cd_async.restart=__jo_cd_async.inflight=false;
            __jo_cd_stream_owner=false;
            ++__jo_cd_async_errors;
            return -1;
        }
        if ((JO_CD_HIRQ & __jo_cd_async.wait_flags) != __jo_cd_async.wait_flags) return 0;
        __jo_cd_async.inflight=false;
    }
    if (!__jo_cd_async.active) return 1;
    if (__jo_cd_async.restart)
    {
        __jo_cd_async.phase=0;
        __jo_cd_async.restart=false;
    }
    if (__jo_cd_async.phase==6 || (__jo_cd_async.phase==2 && __jo_cd_async.count==0))
    {
        __jo_cd_play_next_fad=__jo_cd_async.count ? __jo_cd_async.fad : 0;
        __jo_cd_play_end_fad=__jo_cd_async.count ? __jo_cd_async.fad+__jo_cd_async.count : 0;
        __jo_cd_stream_owner=__jo_cd_async.count!=0;
        __jo_cd_async.active=false;
        return 1;
    }
    if (!(JO_CD_HIRQ & JO_CD_HIRQ_CMOK)) return 0;
    switch (__jo_cd_async.phase)
    {
        case 0:
            c1=JO_CD_CR1_VALUE(JO_CD_COMMAND_SEEK,JO_CD_POSITION_HIGH(JO_CD_POSITION_PAUSE));
            c2=JO_CD_POSITION_LOW(JO_CD_POSITION_PAUSE);
            break;
        case 1:
            c1=JO_CD_CR1_VALUE(JO_CD_COMMAND_RESET_SELECTOR,0);
            c3=JO_CD_PARTITION<<8;flags=JO_CD_HIRQ_ESEL;
            break;
        case 2:
            c1=JO_CD_CR1_VALUE(JO_CD_COMMAND_SET_FILTER_RANGE,JO_CD_POSITION_HIGH(__jo_cd_async.fad));
            c2=JO_CD_POSITION_LOW(__jo_cd_async.fad);
            c3=(JO_CD_FILTER<<8)|JO_CD_POSITION_HIGH(__jo_cd_async.count);
            c4=JO_CD_POSITION_LOW(__jo_cd_async.count);flags=JO_CD_HIRQ_ESEL;
            break;
        case 3:
            c1=JO_CD_CR1_VALUE(JO_CD_COMMAND_SET_FILTER_CONNECTION,JO_CD_FILTER_TRUE_CONNECTION);
            c2=JO_CD_PARTITION<<8;c3=JO_CD_FILTER<<8;flags=JO_CD_HIRQ_ESEL;
            break;
        case 4:
            c1=JO_CD_CR1_VALUE(JO_CD_COMMAND_SET_DEVICE_CONNECTION,0);
            c3=JO_CD_FILTER<<8;flags=JO_CD_HIRQ_ESEL;
            break;
        default:
            c1=JO_CD_CR1_VALUE(JO_CD_COMMAND_PLAY,JO_CD_POSITION_HIGH(JO_CD_POSITION_FAD(__jo_cd_async.fad)));
            c2=JO_CD_POSITION_LOW(__jo_cd_async.fad);
            c3=JO_CD_POSITION_HIGH(JO_CD_POSITION_FAD(__jo_cd_async.count));
            c4=JO_CD_POSITION_LOW(__jo_cd_async.count);
            break;
    }
    JO_CD_HIRQ=~(JO_CD_HIRQ_CMOK|flags);
    JO_CD_CR1=c1;JO_CD_CR2=c2;JO_CD_CR3=c3;JO_CD_CR4=c4;
    __jo_cd_async.wait_flags=flags;
    __jo_cd_async.inflight=true;
    ++__jo_cd_async.phase;
    return 0;
}

/* Ordinary synchronous file operations may wait; prepared-music callbacks
 * never call this function. Drain the job before sharing command registers.
 */
static int jo_cd_stream_async_flush(void)
{
    unsigned int tries=0;
    while (__jo_cd_async.active || __jo_cd_async.inflight)
    {
        if (jo_cd_stream_async_poll()<0) return -1;
        if (++tries>JO_CD_TIMEOUT)
        {
            ++__jo_cd_async_errors;
            return -1;
        }
    }
    return 0;
}
#endif
