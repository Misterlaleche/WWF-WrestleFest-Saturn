// Host-audio validation adapter only. Save/load is unused; DSP/audio unchanged.
#pragma once
struct StateMem {};
struct SFORMAT { int dummy; };
#define SFPTR16N(...) {0}
#define SFPTR8N(...) {0}
#define SFPTR32N(...) {0}
#define SFPTR64N(...) {0}
#define SFVARN(...) {0}
#define SFEND {0}
inline void MDFNSS_StateAction(StateMem*,unsigned,bool,SFORMAT*,const char*,bool){}
