#ifndef WF_R18_PAD_ROUTE_H
#define WF_R18_PAD_ROUTE_H

/* Cabinet inputs $140020..$140026 map to four logical players. One pad on
 * each physical Saturn socket receives P1/P2; multitap children fill P3/P4.
 * A single multitap supplies all four players in its own socket order. */
typedef struct wf_r18_pad_location {
    unsigned port; /* 0 absent, otherwise physical Saturn connector 1 or 2 */
    unsigned slot; /* zero-based peripheral position within that connector */
} wf_r18_pad_location_t;

static inline void wf_r18_pad_route(unsigned first, unsigned second,
                                     wf_r18_pad_location_t out[4])
{
    unsigned count=0, used_first=0, used_second=0;
    if(first){out[count++]=(wf_r18_pad_location_t){1u,used_first++};}
    if(second){out[count++]=(wf_r18_pad_location_t){2u,used_second++};}
    else if(first>used_first){out[count++]=(wf_r18_pad_location_t){1u,used_first++};}
    while(count<4u && used_first<first)
        out[count++]=(wf_r18_pad_location_t){1u,used_first++};
    while(count<4u && used_second<second)
        out[count++]=(wf_r18_pad_location_t){2u,used_second++};
    while(count<4u)out[count++]=(wf_r18_pad_location_t){0u,0u};
}
#endif
