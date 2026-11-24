/* Check the Saturn-only conservative overlap guard without dereferencing
 * synthetic addresses. Only complete canonical high-work-RAM spans may use
 * the fast path. Every hardware mirror/other region falls back to scalar. */
#ifndef WF_JO_BACKEND_SOURCE
#error "Provide the backend source under test"
#endif
#define WF_PLATFORM_SATURN 1
#include WF_JO_BACKEND_SOURCE
#include <assert.h>
#include <stdio.h>

int main(void)
{
    static const unsigned lengths[]={0,1,2,4,31,48,128,256,512};
    static const uintptr_t origins[]={UINT32_C(0x06000000),UINT32_C(0x060a1000),
        UINT32_C(0x060ffff0),UINT32_C(0x061a1000),UINT32_C(0x07fa1000),UINT32_C(0x00210000)};
    unsigned cases=0;
    for(unsigned origin_index=0;origin_index<sizeof origins/sizeof origins[0];origin_index++)
      for(unsigned a=0;a<2;a++)for(unsigned c=0;c<2;c++)
        for(int offset=-520;offset<=(int)sizeof(wf_saturn_yaul_backend_t)+520;offset++)
            for(unsigned n=0;n<sizeof lengths/sizeof lengths[0];n++){
                uintptr_t origin=origins[origin_index];
                uintptr_t b=origin|(a?UINT32_C(0x20000000):0);
                uintptr_t p=(uintptr_t)((intptr_t)origin+offset)|(c?UINT32_C(0x20000000):0);
                unsigned bytes=lengths[n];
                int canonical=b>=UINT32_C(0x06000000)&&
                    b+sizeof(wf_saturn_yaul_backend_t)<=UINT32_C(0x06100000)&&
                    p>=UINT32_C(0x06000000)&&p+bytes<=UINT32_C(0x06100000);
                int expected=bytes&&(!canonical||(p<b+sizeof(wf_saturn_yaul_backend_t)&&b<p+bytes));
                int actual=overlaps_backend((const wf_saturn_yaul_backend_t *)b,(const void *)p,bytes);
                if(actual!=expected){
                    fprintf(stderr,"address alias mismatch backend=%08lx input=%08lx bytes=%u offset=%d\n",
                            (unsigned long)b,(unsigned long)p,bytes,offset);
                    return 1;
                }
                cases++;
            }
    printf("PASS SH2 canonical ranges and conservative mirror fallback: %u cases\n",cases);
    return 0;
}
