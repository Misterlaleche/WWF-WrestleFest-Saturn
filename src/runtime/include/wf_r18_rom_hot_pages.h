/* Original-ROM 1 KiB pages demanded by complete noinput3008, active-pad3854 and emulated-input3305.
 * This measured profile does not promise residency for historical partial paths. */
#ifndef WF_R18_ROM_HOT_PAGES_H
#define WF_R18_ROM_HOT_PAGES_H
#include <stdint.h>
static const uint16_t wf_r18_rom_hot_pages[]={
10,11,12,15,18,19,38,40,48,129,130,148,151,152,155,156,157,162,163,164,165,166,167,168,169,170,171,172,180,181,182,183,184,185,186,187,188,205,206,207,209,210,227,228,229,230,231,232,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255,256,257,258,259,260,261,262,263,264,265,266,267,268,269,270,271,273,274,275,276,277,279,280,281,282,283,284,285,286,287,288,289,290,291,292,293,294,295,296,297,298,299,300,301,302,303,304,305,307,308,310,311,312,314,315,316,319,322,389,392,396,399,418,419,420,428,429,430,431,432,433,434,435,436,437,438,439,440,441,442,443,444,445,446,447,448,449,450,451,452,453,454,455,456,457,458,459,460,461,462,463,464,465,466,467,468,469,470,471,472,473,474,475,476,477,478,479,480,481,482,483,484,485,486,487,488,489,490,491,492,493,494,495,496,497,498,499,500,501,502,503,504,505,506,507
};
/* Complete controlled and no-input TAG replays: pages first touched by
 * late actions/result presentation after the main LWRAM preload. */
static const uint16_t wf_r18_rom_tag_extra_pages[]={150,272,278,306,309,381};
/* Royal route: repeated LWRAM victims plus absent pages. Existing
 * LWRAM pages are copied into HWRAM; absent pages are read once before play. */
#ifdef WF_R18_DIAG_RATE
/* Keep the hardware-rate test on the validated stage9an cache footprint. */
static const uint16_t wf_r18_rom_royal_extra_pages[]={170,244,250,252,254,318,353,361,373,411};
#else
static const uint16_t wf_r18_rom_royal_extra_pages[]={170,244,245,250,252,254,318,353,361,368,373,395,411,415};
#endif
#endif
