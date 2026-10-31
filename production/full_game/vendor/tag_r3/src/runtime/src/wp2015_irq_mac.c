#include <yaul.h>
#include "wp2015_irq_mac.h"

#define WP2015_SCU_FIRST 0x40u
#define WP2015_SCU_COUNT 14u
/* Assembly indexes this pointer table in four-byte units on SH-2. */
scu_ic_ihr_t volatile wp2015_irq_mac_original[WP2015_SCU_COUNT];
#define DECL(n) extern void wp2015_irq_mac_##n(void)
DECL(40); DECL(41); DECL(42); DECL(43); DECL(44); DECL(45); DECL(46);
DECL(47); DECL(48); DECL(49); DECL(4a); DECL(4b); DECL(4c); DECL(4d);
#undef DECL
#define HANDLER(n) wp2015_irq_mac_##n
static const scu_ic_ihr_t wrappers[WP2015_SCU_COUNT]={
    HANDLER(40),HANDLER(41),HANDLER(42),HANDLER(43),HANDLER(44),HANDLER(45),HANDLER(46),
    HANDLER(47),HANDLER(48),HANDLER(49),HANDLER(4a),HANDLER(4b),HANDLER(4c),HANDLER(4d)
};
#undef HANDLER
static unsigned installed;

int wp2015_irq_mac_verify(void)
{
    if(!installed)return -1;
    for(unsigned i=0;i<WP2015_SCU_COUNT;i++)
        if(scu_ic_ihr_get((scu_ic_interrupt_t)(WP2015_SCU_FIRST+i))!=wrappers[i])return -2;
    return 0;
}

int wp2015_irq_mac_install(void)
{
    if(installed)return wp2015_irq_mac_verify();
    /* Publish every original pointer before any wrapper can run. The public
     * BIOS setter writes one aligned handler pointer; no new masking needed. */
    for(unsigned i=0;i<WP2015_SCU_COUNT;i++){
        scu_ic_ihr_t original=scu_ic_ihr_get((scu_ic_interrupt_t)(WP2015_SCU_FIRST+i));
        if(!original)return -3;
        for(unsigned j=0;j<WP2015_SCU_COUNT;j++)if(original==wrappers[j])return -4;
        wp2015_irq_mac_original[i]=original;
    }
    for(unsigned i=0;i<WP2015_SCU_COUNT;i++)
        scu_ic_ihr_set((scu_ic_interrupt_t)(WP2015_SCU_FIRST+i),wrappers[i]);
    installed=1u;
    return wp2015_irq_mac_verify();
}
