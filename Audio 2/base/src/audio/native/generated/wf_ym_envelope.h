/* Native rate quantization measured against the pinned R04 OPM and MAME EG.
 * Attack matches90% time; decay/release matches~12dB time. Not physical calibration. */
#if defined(__GNUC__)
#define WF_EG_TABLE __attribute__((section(".wf_fm_tables")))
#else
#define WF_EG_TABLE
#endif
static const uint8_t wf_ym_attack_rate[64] WF_EG_TABLE={
 0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13,14,14,15,15,16,16,17,17,18,18,19,19,20,20,21,21,22,22,23,23,24,24,25,25,26,27,27,28,28,29,29,30,30,30,31,31
};
static const uint8_t wf_ym_decay_rate[64] WF_EG_TABLE={
 0,0,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13,14,14,15,15,16,16,17,17,18,18,19,19,20,20,21,21,22,22,23,23,24,24,25,25,26,26,27,27,28,28,29,29,30,30,31,31,31,31,31
};
#undef WF_EG_TABLE
