#ifndef WF_NATIVE_DRIVER_H
#define WF_NATIVE_DRIVER_H
#include <stdint.h>
#include <stddef.h>
enum {
    WF_NATIVE_COM_OFFSET=0xc008, WF_NATIVE_MAIL_OFFSET=0xc040,
    WF_NATIVE_ABI=3, WF_NATIVE_INIT=1, WF_NATIVE_READY=2, WF_NATIVE_FAULT=3,
    WF_NATIVE_FM_START=1, WF_NATIVE_FM_STOP=2, WF_NATIVE_RING_PREPARE=3,
    WF_NATIVE_RING_START=4, WF_NATIVE_RING_STOP=5, WF_NATIVE_RING_CA=6,
    WF_NATIVE_ARCADE_WORD=7, WF_NATIVE_SFX_BIND=8, WF_NATIVE_SFX_STOP=9,
    WF_NATIVE_SFX_QUERY=10, WF_NATIVE_OKI_START=11, WF_NATIVE_OKI_STOP=12,
    WF_NATIVE_OK=0, WF_NATIVE_BAD_COMMAND=1, WF_NATIVE_BAD_STATE=2,
    WF_NATIVE_UNIMPLEMENTED=3, WF_NATIVE_PRIORITY_REJECTED=4,
    WF_NATIVE_SFX_PROFILE=0x53f1
};
/* BE 16-bit fields only. SH2 owns request/command/argument; 68000 owns the rest.
 * One outstanding request; request and ack are publication stores, written last.
 * Command 1 plays the fixed observed instrument; no general YM emulation yet. */
typedef struct {
    volatile uint16_t magic_hi, magic_lo, abi, state;
    volatile uint16_t request, ack, command, argument;
    volatile uint16_t result, value, fm_active, ring_prepared;
    volatile uint16_t ring_active, reserved[3];
} wf_native_mail_t;
_Static_assert(sizeof(wf_native_mail_t)==32, "mailbox ABI");
_Static_assert(offsetof(wf_native_mail_t, request)==8, "request ABI");
_Static_assert(offsetof(wf_native_mail_t, ack)==10, "ack ABI");
void wf_native_driver_init(void);
void wf_native_driver_service(void);
void wf_ponesound_main(void);
#endif
