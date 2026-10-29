#ifndef WWF_PONESOUND_MIN_H
#define WWF_PONESOUND_MIN_H
#include <stdint.h>
#include <yaul.h>
#define PCM_PROTECTED (-1)
#define PCM_TYPE_8BIT 1
#define DRV_SYS_END (47 * 1024)
#define SCSP_RAM_ADDR(x) (0x25A00000UL + (x))
typedef struct {
    char loopType; unsigned char bitDepth;
    unsigned short hiAddrBits,loAddrBits,LSA,playsize,pitchword;
    unsigned char pan,volume;
    unsigned short bytes_per_blank,decompression_size;
    unsigned char sh2_permit; char icsr_target;
} wf_pcm_ctrl_t;
typedef struct {
    volatile unsigned int adx_stream_length;
    volatile unsigned short start;
    volatile char adx_buffer_pass[2];
    volatile short drv_adx_coef_1,drv_adx_coef_2;
    volatile wf_pcm_ctrl_t *pcmCtrl;
    volatile unsigned char cdda_left_channel_vol_pan,cdda_right_channel_vol_pan;
} wf_sys_com_t;
void wf_ponesound_load_driver(const uint8_t *driver,int driver_size);
void wf_ponesound_load_8bit_pcm(const void *pcm,int pcm_size,int pcm_number,int load_offset,int sample_rate);
void wf_ponesound_pcm_play(short pcm_number,char ctrl_type,char volume);
void wf_ponesound_pcm_cease(short pcm_number);
void wf_ponesound_copy_to_scsp(const void *pcm,int pcm_size,int load_offset);
void wf_ponesound_configure_8bit_pcm(int pcm_size,int pcm_number,int load_offset,int sample_rate);
void wf_ponesound_vblank(void *work);
#endif
