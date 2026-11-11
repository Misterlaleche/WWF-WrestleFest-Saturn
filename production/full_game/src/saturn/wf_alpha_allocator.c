/* Alpha-only SDK boundary. prepare_alpha_allocator.py redirects the SDK's
 * original memory-manager object explicitly: non-plugin LTO loses --wrap.
 * Link that derived object before libyaul, without any linker --wrap flag.
 * SH2's C symbol prefix is included in the assembler names below. */
#include <stddef.h>
#include <stdint.h>

#if defined(WF_ALPHA_ALLOCATOR_HOST)
extern uintptr_t wf_alpha_allocator_program_end,wf_alpha_allocator_ceiling;
#define PROGRAM_END wf_alpha_allocator_program_end
#define USER_CEILING wf_alpha_allocator_ceiling
#elif defined(WF_ALPHA_RETURN)
extern uint8_t __end[];
#define PROGRAM_END ((uintptr_t)__end)
#define USER_CEILING ((uintptr_t)0x060fffc0u)
#else
#error This SDK guard belongs only to the presentation alpha.
#endif

/* Supplied libyaul TLSF: control0xC74 + pool overhead8 + minimum block12.
 * pool_create writes the entire control before pool_add checks its extent. */
enum { PRIVATE_BYTES=0xa000u,MINIMUM_BYTES=0xc88u };
static void *alpha_private_pool;
extern void *wf_alpha_allocator_real_create(void *,size_t)
#if defined(WF_ALPHA_ALLOCATOR_HOST)
 __asm__("__real__tlsf_pool_create");
#else
 __asm__("_tlsf_pool_create");
#endif
void *wf_alpha_allocator_create(void *,size_t)
 __asm__("__wrap__tlsf_pool_create");

__attribute__((cold,noinline,used))
void *wf_alpha_allocator_create(void *memory,size_t bytes){
 uintptr_t at=(uintptr_t)memory,end=PROGRAM_END,ceiling=USER_CEILING;
 if(!at||(at&3u)||end>ceiling)return NULL;
 if(at!=end){
  /* __mm_init creates its static40KiB private arena first. Other valid
   * explicit arenas retain the original constructor and cannot replace it. */
  if(at>end||bytes>end-at||bytes<MINIMUM_BYTES)return NULL;
  void *pool=wf_alpha_allocator_real_create(memory,bytes);
  if(bytes==PRIVATE_BYTES&&!alpha_private_pool)alpha_private_pool=pool;
  return pool;
 }
 if(!alpha_private_pool)return NULL; /* Reject an unexpected SDK init order. */
 size_t available=(size_t)(ceiling-end);
 if(bytes>available)bytes=available;
 bytes&=~(size_t)3u;
 /* Both SDK handles use the same TLSF API; neither has a pool destructor.
  * Sharing its existing arena avoids a NULL handle and any extra allocation. */
 if(bytes<MINIMUM_BYTES)return alpha_private_pool;
 return wf_alpha_allocator_real_create(memory,bytes);
}
