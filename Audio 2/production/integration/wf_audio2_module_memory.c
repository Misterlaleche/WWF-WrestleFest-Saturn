#include <stddef.h>
typedef struct FILE FILE;
FILE *stderr;
extern void wf_music_unknown_opcode(void);
int fprintf(FILE *f,const char *format,...){(void)f;(void)format;wf_music_unknown_opcode();return 0;}
int printf(const char *format,...){(void)format;wf_music_unknown_opcode();return 0;}
void *memset(void *p,int c,size_t n){unsigned char*d=p;while(n--)*d++=(unsigned char)c;return p;}
void *memcpy(void *d,const void *s,size_t n){unsigned char*x=d;const unsigned char*y=s;while(n--)*x++=*y++;return d;}
int memcmp(const void *a,const void *b,size_t n){const unsigned char*x=a,*y=b;while(n--){if(*x!=*y)return *x-*y;++x;++y;}return 0;}
