#include "wf_sdk_assert.h"
#ifdef WF_COMBAT_TAG_STARTUP_STREAM
/* Same allocation-free terminal as ordinary failures. Its implementation
 * preserves wf_error, captures live actors, and does not wait for IRQs. */
extern void wf_platform_fail(uint32_t);
volatile wf_sdk_assert_record_t wf_sdk_assert_record;
uint32_t wf_sdk_assert_line_number(const char *line){
 if(!line)return UINT32_MAX;
 uint32_t value=0;
 for(unsigned i=0;i<=10u;i++){
  unsigned c=(unsigned char)line[i];
  if(!c)return i?value:UINT32_MAX;
  if(i==10u||c<'0'||c>'9')return UINT32_MAX;
  unsigned digit=c-'0';
  if(value>(UINT32_MAX-digit)/10u)return UINT32_MAX;
  value=value*10u+digit;
 }
 return UINT32_MAX;
}
__attribute__((cold,noinline,noreturn)) void _assert(const char * __restrict file,
 const char * __restrict line,const char * __restrict func,const char * __restrict failed_expr){
 /* No strings are copied/formatted. Publish code last and retain the first
  * original pointers even if a terminal dependency asserts again. */
 if(!wf_sdk_assert_record.code){
  wf_sdk_assert_record.file=file;wf_sdk_assert_record.line=line;
  wf_sdk_assert_record.function=func;wf_sdk_assert_record.expression=failed_expr;
  wf_sdk_assert_record.line_number=wf_sdk_assert_line_number(line);
  wf_sdk_assert_record.code=WF_SDK_ASSERT_ERROR;
 }
 wf_platform_fail(WF_SDK_ASSERT_ERROR);
 /* Defensive terminal fallback if that contract is ever broken: no IRQ,
  * SDK, allocator, formatter, file/CD access or frame wait. */
 for(;;)__asm__ volatile("nop");
}
#endif

#ifdef WF_COMBAT_TAG_EXCEPTION_SHIM
volatile wf_sdk_exception_record_t wf_sdk_exception_record;
__attribute__((cold,noinline,noreturn)) void wf_sdk_exception_handler(
 const cpu_registers_t *registers,const char *message){
 if(!wf_sdk_exception_record.code){
  /* Byte views preserve all23words exactly, independent of host endianness,
   * packed-member aliasing or compiler memcpy lowering. Volatile writes keep
   * this snapshot observable before the terminal can reuse the source stack. */
  const unsigned char *source=(const unsigned char *)registers;
  volatile unsigned char *dest=(volatile unsigned char *)&wf_sdk_exception_record.registers;
  for(unsigned i=0;i<sizeof(cpu_registers_t);i++)dest[i]=registers?source[i]:0xffu;
  wf_sdk_exception_record.message=message;
  wf_sdk_exception_record.code=WF_SDK_CPU_EXCEPTION_ERROR;
 }
 wf_platform_fail(WF_SDK_CPU_EXCEPTION_ERROR);
 for(;;)__asm__ volatile("nop");
}
#endif
