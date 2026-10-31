/* One initial query can consume sectors already buffered. An empty response
 * is followed by register polling for a fresh CSCT, never a stream of0x51.
 * Acknowledge CSCT BEFORE the query so an arrival during it is not lost.
 */
#ifndef WP2015_CD_CSCT_READ
#define WP2015_CD_CSCT_READ() (HIRQ)
#endif
#ifndef WP2015_CD_CSCT_ACK
#define WP2015_CD_CSCT_ACK(flags) (HIRQ=(uint16_t)~(flags))
#endif
static int sector_count_when_ready(response_t *response,uint32_t *available){
    for(uint32_t notices=0;notices<EMPTY_COMMAND_LIMIT;notices++){
        WP2015_CD_CSCT_ACK(CSCT);
        int rc=command(0x5100u,0,0,0,0,response);
        if(rc)return rc;
        uint32_t count=response->cr4;
        if(count>200u)return WP1979_CD_BUFFER_COUNT;
        if(count){*available=count;return 0;}
        uint32_t polls;
        for(polls=0;polls<POLL_LIMIT;polls++){
            uint16_t flags=WP2015_CD_CSCT_READ();
            if(flags&CSCT)break;
            if(flags&PEND)return WP1979_CD_SECTOR_TIMEOUT;
        }
        if(polls==POLL_LIMIT)return WP1979_CD_SECTOR_TIMEOUT;
    }
    return WP1979_CD_SECTOR_TIMEOUT;
}
