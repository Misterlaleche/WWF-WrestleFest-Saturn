#include "wf_records_save.h"
#include <string.h>

static void put_be16(uint8_t *p, uint16_t v) { p[0]=(uint8_t)(v>>8); p[1]=(uint8_t)v; }
static void put_be32(uint8_t *p, uint32_t v) { p[0]=(uint8_t)(v>>24); p[1]=(uint8_t)(v>>16); p[2]=(uint8_t)(v>>8); p[3]=(uint8_t)v; }
static uint16_t get_be16(const uint8_t *p) { return (uint16_t)(((uint16_t)p[0]<<8)|p[1]); }
static uint32_t get_be32(const uint8_t *p) { return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3]; }

uint32_t wf_records_crc32(const uint8_t *data, size_t bytes)
{
    uint32_t crc = 0xFFFFFFFFu;
    size_t i;
    unsigned b;
    if (!data && bytes) return 0u;
    for (i = 0; i < bytes; ++i) {
        crc ^= data[i];
        for (b = 0; b < 8u; ++b)
            crc = (crc >> 1) ^ ((crc & 1u) ? 0xEDB88320u : 0u);
    }
    return crc ^ 0xFFFFFFFFu;
}

static void write_entry(uint8_t *p, const wf_record_entry_t *e)
{
    p[0]=e->value; p[1]=e->initials[0]; p[2]=e->initials[1]; p[3]=e->initials[2];
    p[4]=e->wrestler_1; p[5]=e->wrestler_2_or_shared;
}

static void read_entry(wf_record_entry_t *e, const uint8_t *p)
{
    e->value=p[0]; e->initials[0]=p[1]; e->initials[1]=p[2]; e->initials[2]=p[3];
    e->wrestler_1=p[4]; e->wrestler_2_or_shared=p[5];
}

wf_record_save_result_t wf_records_save_encode(uint8_t out[WF_RECORD_SAVE_BYTES],
                                               const wf_record_tables_t *tables,
                                               uint16_t flags)
{
    unsigned i;
    uint8_t *p;
    uint32_t crc;
    if (!out || !tables) return WF_RECORD_SAVE_BAD_ARGUMENT;
    memcpy(out, "WFRK", 4);
    put_be16(out+4, WF_RECORD_SAVE_VERSION);
    put_be16(out+6, WF_RECORD_SAVE_HEADER_BYTES);
    put_be16(out+8, WF_RECORD_SAVE_PAYLOAD_BYTES);
    put_be16(out+10, flags);
    p=out+12;
    for (i=0;i<WF_RECORD_ENTRY_COUNT;++i,p+=6) write_entry(p,&tables->tag[i]);
    for (i=0;i<WF_RECORD_ENTRY_COUNT;++i,p+=6) write_entry(p,&tables->royal[i]);
    crc=wf_records_crc32(out,144u);
    put_be32(out+144,crc);
    return WF_RECORD_SAVE_OK;
}

wf_record_save_result_t wf_records_save_decode(wf_record_tables_t *tables_out,
                                               uint16_t *flags_out,
                                               const uint8_t *data,
                                               size_t bytes)
{
    unsigned i;
    const uint8_t *p;
    if (!tables_out || !data) return WF_RECORD_SAVE_BAD_ARGUMENT;
    if (bytes != WF_RECORD_SAVE_BYTES) return WF_RECORD_SAVE_BAD_SIZE;
    if (memcmp(data,"WFRK",4)!=0) return WF_RECORD_SAVE_BAD_MAGIC;
    if (get_be16(data+4)!=WF_RECORD_SAVE_VERSION) return WF_RECORD_SAVE_BAD_VERSION;
    if (get_be16(data+6)!=WF_RECORD_SAVE_HEADER_BYTES || get_be16(data+8)!=WF_RECORD_SAVE_PAYLOAD_BYTES)
        return WF_RECORD_SAVE_BAD_LAYOUT;
    if (get_be32(data+144)!=wf_records_crc32(data,144u)) return WF_RECORD_SAVE_BAD_CRC;
    p=data+12;
    for (i=0;i<WF_RECORD_ENTRY_COUNT;++i,p+=6) read_entry(&tables_out->tag[i],p);
    for (i=0;i<WF_RECORD_ENTRY_COUNT;++i,p+=6) read_entry(&tables_out->royal[i],p);
    if (flags_out) *flags_out=get_be16(data+10);
    return WF_RECORD_SAVE_OK;
}
