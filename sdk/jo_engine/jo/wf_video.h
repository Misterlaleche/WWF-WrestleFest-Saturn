/* Local WrestleFest extension to Jo Engine 2026.3. Register writes remain
 * inside Jo's VDP2 shadow/commit implementation; game code never owns them. */
#ifndef JO_WF_VIDEO_H
#define JO_WF_VIDEO_H
#define JO_WF_VIDEO_BANKS 4u
void jo_wf_vdp2_init(void);
void jo_wf_vdp2_present(unsigned int map_group,
                       const unsigned short scroll[4],
                       unsigned char priority, unsigned int layers,
                       unsigned int opaque,unsigned int sprites);
void jo_wf_vdp2_blank(void);
void jo_wf_vdp2_gate(void);
void jo_wf_vdp2_diagnostics(void);
#endif
