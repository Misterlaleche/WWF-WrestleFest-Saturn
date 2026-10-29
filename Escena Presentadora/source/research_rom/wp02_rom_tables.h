/* Numeric prematch selector/threshold tables extracted from supplied ROM.
   Main SHA1 a726c46f350723ea73158bc78055d17d418b14e2. */
static const uint16_t actor_windows[12] = {64,128,128,128,128,128,128,128,64,256,64,64};
static const uint8_t identity_voices[12] = {0x6d,0x6e,0x6f,0x74,0x73,0x73,0x71,0x70,0x72,0x75,0x73,0x73};
static const uint8_t program_counts[12] = {11,4,4,2,2,2,2,4,2,6,3,2};
static const uint16_t program_selectors[12][11] = {
    {91,90,89,91,90,89,91,90,89,92,93},
    {89,90,89,90},
    {89,90,91,90},
    {89,90},
    {91,90},
    {91,90},
    {89,90},
    {89,90,91,90},
    {89,90},
    {89,90,89,90,89,90},
    {89,90,91},
    {89,90},
};
static const uint16_t program_thresholds[12][11] = {
    {6,6,6,6,6,6,6,6,6,6,32},
    {16,16,16,16},
    {8,8,8,16},
    {16,32},
    {89,91},
    {89,91},
    {16,32},
    {8,8,8,16},
    {91,8},
    {8,8,8,8,8,16},
    {8,16,16},
    {16,32},
};
