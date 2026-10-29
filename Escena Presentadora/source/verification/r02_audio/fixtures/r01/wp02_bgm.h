#ifndef WP02_BGM_H
#define WP02_BGM_H
#include <stdbool.h>
#include <stdint.h>

enum {
    WP02_BGM_RING_OFFSET = 0x6C800,
    WP02_BGM_RING_BYTES = 0x10000,
    WP02_BGM_HALF_BYTES = 0x8000,
    WP02_BGM_LOGICAL_VOICE = 3,
    WP02_BGM_LOW_BYTES = 0x100000,
    WP02_BGM_HIGH_BYTES = 704 * 1024
};
bool wp02_bgm_load(uint32_t fad, uint32_t bytes);
void wp02_bgm_reset(void);
bool wp02_bgm_start(void);
bool wp02_bgm_service(uint64_t now_us);
void wp02_bgm_stop(void);
unsigned wp02_bgm_refills(void);
unsigned wp02_bgm_failures(void);
bool wp02_bgm_is_running(void);
#endif
