/* Wrestlefest-only hardware contract. Included only with WF_JO_ENGINE.
 * No TVSTAT reads: DISP=0 forces VBLANK=1, and reading clears gun latch flags.
 * This game supports 320x224 bootstrap/320x240, noninterlaced gamepads. */
volatile unsigned int __jo_smpc_timing_fault __attribute__((used, externally_visible));
static unsigned short wf_smpc_entry_frc;
static unsigned char wf_smpc_entry_cks;

static unsigned short wf_smpc_frc(void)
{
    unsigned int high=*(volatile unsigned char *)0xfffffe12u;
    unsigned int low=*(volatile unsigned char *)0xfffffe13u;
    return (unsigned short)((high<<8)|low);
}
static unsigned int wf_smpc_minimum_ticks(unsigned int cks)
{
    /* ceil(300us * 28.637MHz / divisor)+1. Constants keep the final
     * beam-sample-to-COMREG path bounded without software division. */
    return cks==0u?1075u:(cks==1u?270u:69u);
}
static unsigned int wf_smpc_timeout_ticks(unsigned int cks)
{
    /* The slowest supported PAL clock bounds the upper wait, not the fastest
     * clock used for the minimum delay. One tick is reserved for phase. */
    return cks==0u?1998u:(cks==1u?498u:123u);
}
void __jo_smpc_vblank_mark(void)
{
    wf_smpc_entry_cks=*(volatile unsigned char *)0xfffffe16u&3u;
    wf_smpc_entry_frc=wf_smpc_frc();
}
static int wf_smpc_beam(unsigned int *height,bool control)
{
    unsigned int tvmd=*(volatile unsigned short *)0x25f80000u;
    unsigned int exten=*(volatile unsigned short *)0x25f80002u;
    if(control ? ((tvmd&4u) || (tvmd&0xc0u)==0x40u || (tvmd&0x30u)==0x30u) :
                 ((tvmd&0x00c7u) || (tvmd&0x0030u)>0x0010u))
    { __jo_smpc_timing_fault|=1u;return -1; }
    if(exten&0x0200u)
    { __jo_smpc_timing_fault|=2u;return -1; }
    *height=224u+((tvmd>>4)&3u)*16u;
    unsigned int line=*(volatile unsigned short *)0x25f8000au&0x3ffu;
    return (control && (tvmd&0xc0u)==0xc0u)?(int)(line>>1):(int)line;
}
static int wf_smpc_control_window(void)
{
    unsigned int height;
    int line=wf_smpc_beam(&height,true);
    if(line<0)return -1;
    return line>=16 && line<=192;
}
static int wf_smpc_blank_window(int line,unsigned int height)
{
    /* Peripheral-only needs COMREG before OUT, not a status+CONT roundtrip.
     * Raw 1FC leaves at least two complete scanlines before OUT at 1FF. */
    return line>=(int)height+6 && line<=0x1fc;
}
static int wf_smpc_wait_intback(void)
{
    unsigned int cks=wf_smpc_entry_cks;
    if(cks==3u){__jo_smpc_timing_fault|=4u;return 0;}
    for(;;)
    {
        unsigned int height;
        int line=wf_smpc_beam(&height,false);
        unsigned int now_cks=*(volatile unsigned char *)0xfffffe16u&3u;
        unsigned short elapsed=(unsigned short)(wf_smpc_frc()-wf_smpc_entry_frc);
        if(line<0)return 0;
        if(now_cks!=cks){__jo_smpc_timing_fault|=8u;return 0;}
        if(line<(int)height || line>=0x1ff)return 0;
        if(elapsed>=wf_smpc_minimum_ticks(cks) && wf_smpc_blank_window(line,height))return 1;
        if(elapsed>wf_smpc_timeout_ticks(cks))return 0;
    }
}
