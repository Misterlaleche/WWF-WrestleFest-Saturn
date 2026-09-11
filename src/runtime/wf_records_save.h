#ifndef WF_RECORDS_SAVE_H
#define WF_RECORDS_SAVE_H

#include <stddef.h>
#include <stdint.h>
#include "wf_records.h"

#define WF_RECORD_SAVE_BYTES 148u
#define WF_RECORD_SAVE_HEADER_BYTES 12u
#define WF_RECORD_SAVE_PAYLOAD_BYTES 132u
#define WF_RECORD_SAVE_VERSION 1u

typedef enum wf_record_save_result {
    WF_RECORD_SAVE_OK = 0,
    WF_RECORD_SAVE_BAD_ARGUMENT,
    WF_RECORD_SAVE_BAD_SIZE,
    WF_RECORD_SAVE_BAD_MAGIC,
    WF_RECORD_SAVE_BAD_VERSION,
    WF_RECORD_SAVE_BAD_LAYOUT,
    WF_RECORD_SAVE_BAD_CRC
} wf_record_save_result_t;

uint32_t wf_records_crc32(const uint8_t *data, size_t bytes);
wf_record_save_result_t wf_records_save_encode(uint8_t out[WF_RECORD_SAVE_BYTES],
                                               const wf_record_tables_t *tables,
                                               uint16_t flags);
wf_record_save_result_t wf_records_save_decode(wf_record_tables_t *tables_out,
                                               uint16_t *flags_out,
                                               const uint8_t *data,
                                               size_t bytes);
#endif
