#ifndef WP0_CD_STREAM_H
#define WP0_CD_STREAM_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define WP0_CD_STREAM_SECTOR_BYTES 2048u
#define WP0_CD_STREAM_BURST_BYTES 8192u
#define WP0_CD_STREAM_MAX_CAPACITY 1048576u
#define WP0_CD_STREAM_COMMANDS_PER_FIELD 12u
#define WP0_CD_STREAM_TIMEOUT_FIELDS 300u

enum {
    WP0_CD_CMOK=0x0001u, WP0_CD_DRDY=0x0002u, WP0_CD_CSCT=0x0004u,
    WP0_CD_PEND=0x0010u, WP0_CD_DCHG=0x0020u, WP0_CD_EHST=0x0080u
};
typedef struct { uint16_t cr1,cr2,cr3,cr4; } wp0_cd_regs_t;

/* One owner only: callbacks/other threads must not use the CD registers while
 * this stream is active. issue/response perform one atomic register sequence,
 * never a readiness wait. read_words counts 32-bit words, is bounded, and stores
 * original byte order. It must not write beyond word_count*4 output bytes.
 * It returns the count actually copied, allowing a short FIFO read to fail shut.
 * start_hidden may block and is NEVER invoked by service. */
typedef struct {
    void *context;
    uint16_t (*hirq)(void *);
    void (*clear_hirq)(void *,uint16_t clear_bits);
    bool (*issue)(void *,const wp0_cd_regs_t *,uint16_t required,uint16_t clear_bits);
    bool (*response)(void *,wp0_cd_regs_t *);
    uint32_t (*read_words)(void *,void *aligned_output,uint32_t word_count);
    int (*start_hidden)(void *,uint32_t fad,uint32_t sectors);
} wp0_cd_stream_ops_t;

typedef enum {
    WP0_CD_STREAM_OFF, WP0_CD_STREAM_QUERY, WP0_CD_STREAM_QUERY_WAIT,
    WP0_CD_STREAM_GET, WP0_CD_STREAM_GET_WAIT, WP0_CD_STREAM_DATA,
    WP0_CD_STREAM_END, WP0_CD_STREAM_END_WAIT, WP0_CD_STREAM_HOST_END,
    WP0_CD_STREAM_DONE, WP0_CD_STREAM_FAILED,
    WP0_CD_STREAM_PLAY, WP0_CD_STREAM_PLAY_WAIT
} wp0_cd_stream_state_t;
typedef enum {
    WP0_CD_STREAM_OK, WP0_CD_STREAM_BAD_ARGUMENT, WP0_CD_STREAM_START_FAILED,
    WP0_CD_STREAM_REJECT, WP0_CD_STREAM_WAIT, WP0_CD_STREAM_DRIVE_ERROR,
    WP0_CD_STREAM_PERIODIC, WP0_CD_STREAM_BAD_COUNT,
    WP0_CD_STREAM_NO_TRANSFER, WP0_CD_STREAM_SHORT_READ,
    WP0_CD_STREAM_WORD_COUNT, WP0_CD_STREAM_TIMEOUT, WP0_CD_STREAM_DISC_CHANGED,
    WP0_CD_STREAM_REWIND_NOT_READY
} wp0_cd_stream_error_t;

typedef struct {
    wp0_cd_stream_ops_t ops;
    uint8_t *ring;
    uint32_t capacity,file_bytes,sector_count,play_fad;
    uint32_t produced,consumed,write_pos,read_pos;
    uint32_t sectors_received,pending_sectors,pending_bytes,pending_valid;
    uint32_t service_calls,commands_issued,bytes_copied,periodic_responses;
    uint32_t last_field,commands_this_field,wait_start_field,timeout_fields;
    uint32_t data_wait_start_field;
    uint32_t last_empty_query_field,last_end_words,max_service_bytes;
    uint16_t last_hirq,last_command;
    wp0_cd_regs_t last_response;
    wp0_cd_stream_state_t state;
    wp0_cd_stream_error_t error;
    int start_result;
    bool clock_started,empty_query_valid,data_wait_active,stop_requested;
    uint32_t stop_start_field,stop_stage;
} wp0_cd_stream_t;

bool wp0_cd_stream_init(wp0_cd_stream_t *,void *aligned_ring,uint32_t capacity,
                        uint32_t file_bytes,const wp0_cd_stream_ops_t *);
bool wp0_cd_stream_start_hidden(wp0_cd_stream_t *,uint32_t first_fad);
/* Queue a nonblocking replay only after transport EOF and complete consumption.
 * No SDK/setup callback or ring clearing occurs. A premature request is fatal
 * REWIND_NOT_READY; an invalid FAD is BAD_ARGUMENT. Service sends PLAY later. */
bool wp0_cd_stream_rewind_async(wp0_cd_stream_t *,uint32_t first_fad);
/* Restart at a different bounded file extent after the OLD extent reaches
 * clean EOF. The old EOF is checked before either length field is changed. */
bool wp0_cd_stream_restart_async(wp0_cd_stream_t *,uint32_t first_fad,uint32_t new_file_bytes);
/* Main-thread ownership: do not call service/copy/consume concurrently.
 * Nonblocking. One call performs <=8192 data bytes and bounded state work.
 * Call repeatedly in available CPU time; pass the same field counter throughout
 * a field. IRQs remain enabled during all device waiting and FIFO data copies. */
bool wp0_cd_stream_service(wp0_cd_stream_t *,uint32_t vblank_counter);
uint32_t wp0_cd_stream_available(const wp0_cd_stream_t *);
const uint8_t *wp0_cd_stream_peek(const wp0_cd_stream_t *,uint32_t *contiguous_bytes);
bool wp0_cd_stream_copy(const wp0_cd_stream_t *,uint32_t relative_offset,
                        void *destination,uint32_t bytes);
/* copy's destination must not overlap the ring. Both APIs reject unavailable
 * bytes. eof means transport complete; available bytes may remain unconsumed. */
bool wp0_cd_stream_consume(wp0_cd_stream_t *,uint32_t bytes);
bool wp0_cd_stream_eof(const wp0_cd_stream_t *);

/* Stop is cooperative: retain ring/ops until +1; -1 is fatal, never CDFS-safe.
 * Zero means pending. The single global 300-field deadline includes cleanup. */
bool wp0_cd_stream_stop_request(wp0_cd_stream_t *,uint32_t field);
int wp0_cd_stream_stop_service(wp0_cd_stream_t *,uint32_t field);

#ifdef WP0_CD_STREAM_SATURN
wp0_cd_stream_ops_t wp0_cd_stream_saturn_ops(void);
#endif
#endif
