#ifndef WF_SDK_ASSERT_H
#define WF_SDK_ASSERT_H
#include <stdint.h>
enum {WF_SDK_ASSERT_ERROR=0x41535354u};
typedef struct {
 const char *file,*line,*function,*expression;
 uint32_t line_number,code;
} wf_sdk_assert_record_t;
extern volatile wf_sdk_assert_record_t wf_sdk_assert_record;
/* SDK ABI: all four arguments are strings; line is NOT an integer. */
void _assert(const char * __restrict file,const char * __restrict line,
 const char * __restrict func,const char * __restrict failed_expr) __attribute__((noreturn));
/* At most eleven byte reads: up to ten decimal digits plus terminator.
 * Null/empty/nondecimal/overlong/overflow returns UINT32_MAX. */
uint32_t wf_sdk_assert_line_number(const char *);
#ifdef WF_COMBAT_TAG_EXCEPTION_SHIM
#ifndef WF_COMBAT_TAG_STARTUP_STREAM
#error CPU exception shim requires the explicit TAG startup-stream experiment
#endif
#include <stddef.h>
#include <cpu/registers.h>
_Static_assert(sizeof(cpu_registers_t)==23u*sizeof(uint32_t),"SDK exception frame is23words");
_Static_assert(offsetof(cpu_registers_t,sp)==0u&&offsetof(cpu_registers_t,r)==4u&&
 offsetof(cpu_registers_t,macl)==64u&&offsetof(cpu_registers_t,mach)==68u&&
 offsetof(cpu_registers_t,vbr)==72u&&offsetof(cpu_registers_t,gbr)==76u&&
 offsetof(cpu_registers_t,pr)==80u&&offsetof(cpu_registers_t,pc)==84u&&
 offsetof(cpu_registers_t,sr)==88u,"Exact SDK exception register order");
enum {WF_SDK_CPU_EXCEPTION_ERROR=0x43505545u};
typedef struct {
 cpu_registers_t registers;
 const char *message;
 uint32_t code;
} wf_sdk_exception_record_t;
extern volatile wf_sdk_exception_record_t wf_sdk_exception_record;
/* Native SDK trampolines pass frame in r4 and message in r5. GNU ld wraps
 * ELF ___exception_assert; an explicit assembler name avoids C-prefix guesses. */
void wf_sdk_exception_handler(const cpu_registers_t *,const char *)
 __asm__("__wrap____exception_assert") __attribute__((noreturn));
#endif
#endif
