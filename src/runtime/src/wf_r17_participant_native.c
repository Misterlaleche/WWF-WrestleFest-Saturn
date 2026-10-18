/* Native static translation of canonical ROM SHA1 a726c46f350723ea73158bc78055d17d418b14e2.
 * Each C label maps to its ROM instruction; there is no opcode decoder or dynamic CPU dispatch.
 * Bus access is explicit raw actor/global state and staged native FG0 transport. */
#include "wf_r17_participant_helpers.h"
static uint32_t mask(unsigned w){return w==4?0xffffffffu:w==2?65535u:255u;}
static void put(uint32_t *p,uint32_t v,unsigned w){*p=(*p&~mask(w))|(v&mask(w));}
static void flags(wf_participant_machine_t*m,uint32_t v,unsigned w){v&=mask(w);m->z=v==0;m->n=(v>>(w*8-1))&1;m->c=0;}
static void postwrite(wf_participant_machine_t*m,uint32_t*a,uint32_t v,unsigned w){wf_participant_write(m,*a,v,w);*a+=w;}
void wf_participant_7506(wf_participant_machine_t *);
void wf_participant_75ee(wf_participant_machine_t *);
static void wf_participant_7680(wf_participant_machine_t *);
static void wf_participant_76aa(wf_participant_machine_t *);
static void wf_participant_76e8(wf_participant_machine_t *);
void wf_participant_7720(wf_participant_machine_t *);
static void wf_participant_776c(wf_participant_machine_t *);
static void wf_participant_77d8(wf_participant_machine_t *);
void wf_participant_782c(wf_participant_machine_t *);
void wf_participant_1079e(wf_participant_machine_t *);
void wf_participant_9132(wf_participant_machine_t *);
static void wf_participant_9398(wf_participant_machine_t *);
static void wf_participant_94ca(wf_participant_machine_t *);
static void wf_participant_9556(wf_participant_machine_t *);
static void wf_participant_956c(wf_participant_machine_t *);
static void wf_participant_9578(wf_participant_machine_t *);
static void wf_participant_95ae(wf_participant_machine_t *);
static void wf_participant_961c(wf_participant_machine_t *);
static void wf_participant_9658(wf_participant_machine_t *);
static void wf_participant_96a0(wf_participant_machine_t *);
static void wf_participant_96c6(wf_participant_machine_t *);
static void wf_participant_972c(wf_participant_machine_t *);
void wf_participant_7506(wf_participant_machine_t *m){
 uint32_t stack[32];unsigned sp=0; (void)stack;(void)sp;
/* $7506: movem.l d0/a0-a1, -(a7) */
 if(m->status!=WF_OK)return;
 stack[sp++]=m->d[0]; stack[sp++]=m->a[0]; stack[sp++]=m->a[1];
/* $750a: lea.l $1c05b0.l, a0 */
 if(m->status!=WF_OK)return;
 m->a[0]=1836464u;
/* $7510: moveq #$0, d0 */
 if(m->status!=WF_OK)return;
 {uint32_t v=(uint32_t)(int32_t)(int8_t)(0u); put(&m->d[0],v,4); flags(m,v,4);}
L7512: /* $7512: btst.b #$7, $0(a0) */
 if(m->status!=WF_OK)return;
 {uint32_t v=wf_participant_read(m,(m->a[0] + 0),1),b=1u<<(7u%8); m->z=(v&b)==0; }
/* $7518: beq.b $7534 */
 if(m->status!=WF_OK)return;
 if(m->z) goto L7534;
/* $751a: btst.b #$7, $56(a0) */
 if(m->status!=WF_OK)return;
 {uint32_t v=wf_participant_read(m,(m->a[0] + 86),1),b=1u<<(7u%8); m->z=(v&b)==0; }
/* $7520: bne.b $7534 */
 if(m->status!=WF_OK)return;
 if(!m->z) goto L7534;
/* $7522: move.w $66(a0), $6e(a0) */
 if(m->status!=WF_OK)return;
 {uint32_t v=wf_participant_read(m,(m->a[0] + 102),2); wf_participant_write(m,(m->a[0] + 110),v,2); flags(m,v,2);}
/* $7528: bsr.w $75ee */
 if(m->status!=WF_OK)return;
 wf_participant_75ee(m);
/* $752c: bsr.w $782c */
 if(m->status!=WF_OK)return;
 wf_participant_782c(m);
/* $7530: bsr.w $776c */
 if(m->status!=WF_OK)return;
 wf_participant_776c(m);
L7534: /* $7534: adda.l #$10c, a0 */
 if(m->status!=WF_OK)return;
 m->a[0] += 268u;
/* $753a: addq.w #$1, d0 */
 if(m->status!=WF_OK)return;
 {uint32_t s=1u,v=m->d[0],r=(v + s)&mask(2); put(&m->d[0],r,2); flags(m,r,2); m->c=((uint64_t)(v&mask(2))+(s&mask(2)))>mask(2);}
/* $753c: cmpi.w #$4, d0 */
 if(m->status!=WF_OK)return;
 {uint32_t s=4u&mask(2),v=m->d[0]&mask(2);flags(m,v-s,2);m->c=v<s;}
/* $7540: bne.b $7512 */
 if(m->status!=WF_OK)return;
 if(!m->z) goto L7512;
/* $7542: movem.l (a7)+, d0/a0-a1 */
 if(m->status!=WF_OK)return;
 m->a[1]=stack[--sp]; m->a[0]=stack[--sp]; m->d[0]=stack[--sp];
/* $7546: rts  */
 if(m->status!=WF_OK)return;
 return;
}

void wf_participant_75ee(wf_participant_machine_t *m){
 uint32_t stack[32];unsigned sp=0; (void)stack;(void)sp;
/* $75ee: movem.l a0-a5, -(a7) */
 if(m->status!=WF_OK)return;
 stack[sp++]=m->a[0]; stack[sp++]=m->a[1]; stack[sp++]=m->a[2]; stack[sp++]=m->a[3]; stack[sp++]=m->a[4]; stack[sp++]=m->a[5];
/* $75f2: bsr.w $7680 */
 if(m->status!=WF_OK)return;
 wf_participant_7680(m);
/* $75f6: adda.l #$20c, a1 */
 if(m->status!=WF_OK)return;
 m->a[1] += 524u;
/* $75fc: move.w #$5, d2 */
 if(m->status!=WF_OK)return;
 {uint32_t v=5u; put(&m->d[2],v,2); flags(m,v,2);}
L7600: /* $7600: moveq #$0, d1 */
 if(m->status!=WF_OK)return;
 {uint32_t v=(uint32_t)(int32_t)(int8_t)(0u); put(&m->d[1],v,4); flags(m,v,4);}
/* $7602: bsr.w $76aa */
 if(m->status!=WF_OK)return;
 wf_participant_76aa(m);
/* $7606: adda.l #$4, a1 */
 if(m->status!=WF_OK)return;
 m->a[1] += 4u;
/* $760c: dbra d2, $7600 */
 if(m->status!=WF_OK)return;
 put(&m->d[2],m->d[2]-1,2);if((uint16_t)m->d[2]!=65535u)goto L7600;
/* $7610: move.b $6f(a0), d1 */
 if(m->status!=WF_OK)return;
 {uint32_t v=wf_participant_read(m,(m->a[0] + 111),1); put(&m->d[1],v,1); flags(m,v,1);}
/* $7614: andi.l #$ff, d1 */
 if(m->status!=WF_OK)return;
 {uint32_t s=255u,v=m->d[1],r=(v & s)&mask(4); put(&m->d[1],r,4); flags(m,r,4); }
/* $761a: beq.w $767a */
 if(m->status!=WF_OK)return;
 if(m->z) goto L767a;
/* $761e: divu.w #$10, d1 */
 if(m->status!=WF_OK)return;
 {uint32_t s=16u&65535u,v=m->d[1];put(&m->d[1],((v%s)<<16)|(v/s),4); flags(m,v/s,2);}
/* $7622: move.l d1, d2 */
 if(m->status!=WF_OK)return;
 {uint32_t v=m->d[1]; put(&m->d[2],v,4); flags(m,v,4);}
/* $7624: bsr.w $7680 */
 if(m->status!=WF_OK)return;
 wf_participant_7680(m);
/* $7628: adda.l #$20c, a1 */
 if(m->status!=WF_OK)return;
 m->a[1] += 524u;
/* $762e: moveq #$1, d3 */
 if(m->status!=WF_OK)return;
 {uint32_t v=(uint32_t)(int32_t)(int8_t)(1u); put(&m->d[3],v,4); flags(m,v,4);}
/* $7630: andi.l #$ffff, d1 */
 if(m->status!=WF_OK)return;
 {uint32_t s=65535u,v=m->d[1],r=(v & s)&mask(4); put(&m->d[1],r,4); flags(m,r,4); }
/* $7636: beq.b $7664 */
 if(m->status!=WF_OK)return;
 if(m->z) goto L7664;
/* $7638: cmpi.w #$6, d1 */
 if(m->status!=WF_OK)return;
 {uint32_t s=6u&mask(2),v=m->d[1]&mask(2);flags(m,v-s,2);m->c=v<s;}
/* $763c: bcs.b $7644 */
 if(m->status!=WF_OK)return;
 if(m->c) goto L7644;
/* $763e: move.w #$6, d1 */
 if(m->status!=WF_OK)return;
 {uint32_t v=6u; put(&m->d[1],v,2); flags(m,v,2);}
/* $7642: moveq #$0, d3 */
 if(m->status!=WF_OK)return;
 {uint32_t v=(uint32_t)(int32_t)(int8_t)(0u); put(&m->d[3],v,4); flags(m,v,4);}
L7644: /* $7644: subi.w #$1, d1 */
 if(m->status!=WF_OK)return;
 {uint32_t s=1u,v=m->d[1],r=(v - s)&mask(2); put(&m->d[1],r,2); flags(m,r,2); m->c=(v&mask(2))<(s&mask(2));}
L7648: /* $7648: movem.l d1, -(a7) */
 if(m->status!=WF_OK)return;
 stack[sp++]=m->d[1];
/* $764c: moveq #$8, d1 */
 if(m->status!=WF_OK)return;
 {uint32_t v=(uint32_t)(int32_t)(int8_t)(8u); put(&m->d[1],v,4); flags(m,v,4);}
/* $764e: bsr.w $76aa */
 if(m->status!=WF_OK)return;
 wf_participant_76aa(m);
/* $7652: movem.l (a7)+, d1 */
 if(m->status!=WF_OK)return;
 m->d[1]=stack[--sp];
/* $7656: adda.l #$4, a1 */
 if(m->status!=WF_OK)return;
 m->a[1] += 4u;
/* $765c: dbra d1, $7648 */
 if(m->status!=WF_OK)return;
 put(&m->d[1],m->d[1]-1,2);if((uint16_t)m->d[1]!=65535u)goto L7648;
/* $7660: tst.w d3 */
 if(m->status!=WF_OK)return;
 {uint32_t v=m->d[3];  flags(m,v,2);}
/* $7662: beq.b $767a */
 if(m->status!=WF_OK)return;
 if(m->z) goto L767a;
L7664: /* $7664: swap d2 */
 if(m->status!=WF_OK)return;
 {uint32_t v=m->d[2];v=(v<<16)|(v>>16);put(&m->d[2],v,4); flags(m,v,4);}
/* $7666: andi.w #$ff, d2 */
 if(m->status!=WF_OK)return;
 {uint32_t s=255u,v=m->d[2],r=(v & s)&mask(2); put(&m->d[2],r,2); flags(m,r,2); }
/* $766a: beq.b $767a */
 if(m->status!=WF_OK)return;
 if(m->z) goto L767a;
/* $766c: btst.b #$0, d2 */
 if(m->status!=WF_OK)return;
 {uint32_t v=m->d[2],b=1u<<(0u%32); m->z=(v&b)==0; }
/* $7670: beq.b $7674 */
 if(m->status!=WF_OK)return;
 if(m->z) goto L7674;
/* $7672: subq.w #$1, d2 */
 if(m->status!=WF_OK)return;
 {uint32_t s=1u,v=m->d[2],r=(v - s)&mask(2); put(&m->d[2],r,2); flags(m,r,2); m->c=(v&mask(2))<(s&mask(2));}
L7674: /* $7674: lsr.w #$1, d2 */
 if(m->status!=WF_OK)return;
 {unsigned s=1u;uint32_t v=m->d[2]&mask(2),r=v>>s; put(&m->d[2],r,2); flags(m,r,2);m->c=s?((v>>(s-1))&1):0;}
/* $7676: move.w d2, d1 */
 if(m->status!=WF_OK)return;
 {uint32_t v=m->d[2]; put(&m->d[1],v,2); flags(m,v,2);}
/* $7678: bsr.b $76aa */
 if(m->status!=WF_OK)return;
 wf_participant_76aa(m);
L767a: /* $767a: movem.l (a7)+, a0-a5 */
 if(m->status!=WF_OK)return;
 m->a[5]=stack[--sp]; m->a[4]=stack[--sp]; m->a[3]=stack[--sp]; m->a[2]=stack[--sp]; m->a[1]=stack[--sp]; m->a[0]=stack[--sp];
/* $767e: rts  */
 if(m->status!=WF_OK)return;
 return;
}

static void wf_participant_7680(wf_participant_machine_t *m){
 uint32_t stack[32];unsigned sp=0; (void)stack;(void)sp;
/* $7680: movem.l d0, -(a7) */
 if(m->status!=WF_OK)return;
 stack[sp++]=m->d[0];
/* $7684: bsr.w $7720 */
 if(m->status!=WF_OK)return;
 wf_participant_7720(m);
/* $7688: lea.l $76a2.l, a1 */
 if(m->status!=WF_OK)return;
 m->a[1]=30370u;
/* $768e: asl.w #$1, d0 */
 if(m->status!=WF_OK)return;
 {unsigned s=1u;uint32_t v=m->d[0]&mask(2),r=v<<s; put(&m->d[0],r,2); flags(m,r,2);m->c=s?((v>>(16-s))&1):0;}
/* $7690: move.w (a1, d0.w), d0 */
 if(m->status!=WF_OK)return;
 {uint32_t v=wf_participant_read(m,(m->a[1] + 0 + (int16_t)m->d[0]),2); put(&m->d[0],v,2); flags(m,v,2);}
/* $7694: movea.l #$c0000, a1 */
 if(m->status!=WF_OK)return;
 m->a[1]=786432u;
/* $769a: adda.l d0, a1 */
 if(m->status!=WF_OK)return;
 m->a[1] += m->d[0];
/* $769c: movem.l (a7)+, d0 */
 if(m->status!=WF_OK)return;
 m->d[0]=stack[--sp];
/* $76a0: rts  */
 if(m->status!=WF_OK)return;
 return;
}

static void wf_participant_76aa(wf_participant_machine_t *m){
 uint32_t stack[32];unsigned sp=0; (void)stack;(void)sp;
/* $76aa: movem.l d1-d4, -(a7) */
 if(m->status!=WF_OK)return;
 stack[sp++]=m->d[1]; stack[sp++]=m->d[2]; stack[sp++]=m->d[3]; stack[sp++]=m->d[4];
/* $76ae: moveq #$7, d2 */
 if(m->status!=WF_OK)return;
 {uint32_t v=(uint32_t)(int32_t)(int8_t)(7u); put(&m->d[2],v,4); flags(m,v,4);}
/* $76b0: move.w #$b33, d3 */
 if(m->status!=WF_OK)return;
 {uint32_t v=2867u; put(&m->d[3],v,2); flags(m,v,2);}
/* $76b4: tst.w d1 */
 if(m->status!=WF_OK)return;
 {uint32_t v=m->d[1];  flags(m,v,2);}
/* $76b6: beq.b $76ca */
 if(m->status!=WF_OK)return;
 if(m->z) goto L76ca;
/* $76b8: moveq #$b, d2 */
 if(m->status!=WF_OK)return;
 {uint32_t v=(uint32_t)(int32_t)(int8_t)(11u); put(&m->d[2],v,4); flags(m,v,4);}
/* $76ba: move.w #$ad9, d3 */
 if(m->status!=WF_OK)return;
 {uint32_t v=2777u; put(&m->d[3],v,2); flags(m,v,2);}
/* $76be: btst.b #$1, d0 */
 if(m->status!=WF_OK)return;
 {uint32_t v=m->d[0],b=1u<<(1u%32); m->z=(v&b)==0; }
/* $76c2: beq.b $76ca */
 if(m->status!=WF_OK)return;
 if(m->z) goto L76ca;
/* $76c4: moveq #$e, d2 */
 if(m->status!=WF_OK)return;
 {uint32_t v=(uint32_t)(int32_t)(int8_t)(14u); put(&m->d[2],v,4); flags(m,v,4);}
/* $76c6: move.w #$af2, d3 */
 if(m->status!=WF_OK)return;
 {uint32_t v=2802u; put(&m->d[3],v,2); flags(m,v,2);}
L76ca: /* $76ca: add.w d1, d3 */
 if(m->status!=WF_OK)return;
 {uint32_t s=m->d[1],v=m->d[3],r=(v + s)&mask(2); put(&m->d[3],r,2); flags(m,r,2); m->c=((uint64_t)(v&mask(2))+(s&mask(2)))>mask(2);}
/* $76cc: move.w d3, d4 */
 if(m->status!=WF_OK)return;
 {uint32_t v=m->d[3]; put(&m->d[4],v,2); flags(m,v,2);}
/* $76ce: bsr.b $76e8 */
 if(m->status!=WF_OK)return;
 wf_participant_76e8(m);
/* $76d0: adda.l #$100, a1 */
 if(m->status!=WF_OK)return;
 m->a[1] += 256u;
/* $76d6: add.w d2, d3 */
 if(m->status!=WF_OK)return;
 {uint32_t s=m->d[2],v=m->d[3],r=(v + s)&mask(2); put(&m->d[3],r,2); flags(m,r,2); m->c=((uint64_t)(v&mask(2))+(s&mask(2)))>mask(2);}
/* $76d8: move.w d3, d4 */
 if(m->status!=WF_OK)return;
 {uint32_t v=m->d[3]; put(&m->d[4],v,2); flags(m,v,2);}
/* $76da: bsr.b $76e8 */
 if(m->status!=WF_OK)return;
 wf_participant_76e8(m);
/* $76dc: suba.l #$100, a1 */
 if(m->status!=WF_OK)return;
 m->a[1] -= 256u;
/* $76e2: movem.l (a7)+, d1-d4 */
 if(m->status!=WF_OK)return;
 m->d[4]=stack[--sp]; m->d[3]=stack[--sp]; m->d[2]=stack[--sp]; m->d[1]=stack[--sp];
/* $76e6: rts  */
 if(m->status!=WF_OK)return;
 return;
}

static void wf_participant_76e8(wf_participant_machine_t *m){
 uint32_t stack[32];unsigned sp=0; (void)stack;(void)sp;
/* $76e8: move.b d4, $1(a1) */
 if(m->status!=WF_OK)return;
 {uint32_t v=m->d[4]; wf_participant_write(m,(m->a[1] + 1),v,1); flags(m,v,1);}
/* $76ec: lsr.w #$8, d4 */
 if(m->status!=WF_OK)return;
 {unsigned s=8u;uint32_t v=m->d[4]&mask(2),r=v>>s; put(&m->d[4],r,2); flags(m,r,2);m->c=s?((v>>(s-1))&1):0;}
/* $76ee: ori.w #$10, d4 */
 if(m->status!=WF_OK)return;
 {uint32_t s=16u,v=m->d[4],r=(v | s)&mask(2); put(&m->d[4],r,2); flags(m,r,2); }
/* $76f2: move.b d4, $3(a1) */
 if(m->status!=WF_OK)return;
 {uint32_t v=m->d[4]; wf_participant_write(m,(m->a[1] + 3),v,1); flags(m,v,1);}
/* $76f6: rts  */
 if(m->status!=WF_OK)return;
 return;
}

void wf_participant_7720(wf_participant_machine_t *m){
 uint32_t stack[32];unsigned sp=0; (void)stack;(void)sp;
/* $7720: moveq #$0, d0 */
 if(m->status!=WF_OK)return;
 {uint32_t v=(uint32_t)(int32_t)(int8_t)(0u); put(&m->d[0],v,4); flags(m,v,4);}
/* $7722: lea.l $1c05b0.l, a1 */
 if(m->status!=WF_OK)return;
 m->a[1]=1836464u;
L7728: /* $7728: cmpa.l a1, a0 */
 if(m->status!=WF_OK)return;
 {uint32_t s=m->a[1]&mask(4),v=m->a[0]&mask(4);flags(m,v-s,4);m->c=v<s;}
/* $772a: beq.b $773e */
 if(m->status!=WF_OK)return;
 if(m->z) goto L773e;
/* $772c: adda.l #$10c, a1 */
 if(m->status!=WF_OK)return;
 m->a[1] += 268u;
/* $7732: addi.w #$1, d0 */
 if(m->status!=WF_OK)return;
 {uint32_t s=1u,v=m->d[0],r=(v + s)&mask(2); put(&m->d[0],r,2); flags(m,r,2); m->c=((uint64_t)(v&mask(2))+(s&mask(2)))>mask(2);}
/* $7736: cmpi.w #$4, d0 */
 if(m->status!=WF_OK)return;
 {uint32_t s=4u&mask(2),v=m->d[0]&mask(2);flags(m,v-s,2);m->c=v<s;}
/* $773a: bcs.b $7728 */
 if(m->status!=WF_OK)return;
 if(m->c) goto L7728;
/* $773c: bra.b $7742 */
 if(m->status!=WF_OK)return;
 if(1) goto L7742;
L773e: /* $773e: move.w #$1, ccr */
 if(m->status!=WF_OK)return;
 m->z=0;m->n=0;m->c=1;
L7742: /* $7742: rts  */
 if(m->status!=WF_OK)return;
 return;
}

static void wf_participant_776c(wf_participant_machine_t *m){
 uint32_t stack[32];unsigned sp=0; (void)stack;(void)sp;
/* $776c: movem.l d0-d4/a0-a1, -(a7) */
 if(m->status!=WF_OK)return;
 stack[sp++]=m->d[0]; stack[sp++]=m->d[1]; stack[sp++]=m->d[2]; stack[sp++]=m->d[3]; stack[sp++]=m->d[4]; stack[sp++]=m->a[0]; stack[sp++]=m->a[1];
/* $7770: lea.l $1c05b0.l, a0 */
 if(m->status!=WF_OK)return;
 m->a[0]=1836464u;
/* $7776: moveq #$0, d0 */
 if(m->status!=WF_OK)return;
 {uint32_t v=(uint32_t)(int32_t)(int8_t)(0u); put(&m->d[0],v,4); flags(m,v,4);}
L7778: /* $7778: btst.b #$7, $0(a0) */
 if(m->status!=WF_OK)return;
 {uint32_t v=wf_participant_read(m,(m->a[0] + 0),1),b=1u<<(7u%8); m->z=(v&b)==0; }
/* $777e: beq.b $77bc */
 if(m->status!=WF_OK)return;
 if(m->z) goto L77bc;
/* $7780: btst.b #$7, $56(a0) */
 if(m->status!=WF_OK)return;
 {uint32_t v=wf_participant_read(m,(m->a[0] + 86),1),b=1u<<(7u%8); m->z=(v&b)==0; }
/* $7786: bne.b $77bc */
 if(m->status!=WF_OK)return;
 if(!m->z) goto L77bc;
/* $7788: btst.b #$0, $1c0161.l */
 if(m->status!=WF_OK)return;
 {uint32_t v=wf_participant_read(m,1835361u,1),b=1u<<(0u%8); m->z=(v&b)==0; }
/* $7790: nop  */
 if(m->status!=WF_OK)return;
 ;
/* $7792: move.l $8a(a0), d1 */
 if(m->status!=WF_OK)return;
 {uint32_t v=wf_participant_read(m,(m->a[0] + 138),4); put(&m->d[1],v,4); flags(m,v,4);}
/* $7796: andi.w #$f, d1 */
 if(m->status!=WF_OK)return;
 {uint32_t s=15u,v=m->d[1],r=(v & s)&mask(2); put(&m->d[1],r,2); flags(m,r,2); }
/* $779a: lea.l $77d0.l, a1 */
 if(m->status!=WF_OK)return;
 m->a[1]=30672u;
/* $77a0: move.w (a1, d1.w), d2 */
 if(m->status!=WF_OK)return;
 {uint32_t v=wf_participant_read(m,(m->a[1] + 0 + (int16_t)m->d[1]),2); put(&m->d[2],v,2); flags(m,v,2);}
/* $77a4: bsr.w $7680 */
 if(m->status!=WF_OK)return;
 wf_participant_7680(m);
/* $77a8: moveq #$2, d1 */
 if(m->status!=WF_OK)return;
 {uint32_t v=(uint32_t)(int32_t)(int8_t)(2u); put(&m->d[1],v,4); flags(m,v,4);}
L77aa: /* $77aa: move.w d2, d4 */
 if(m->status!=WF_OK)return;
 {uint32_t v=m->d[2]; put(&m->d[4],v,2); flags(m,v,2);}
/* $77ac: bsr.w $76e8 */
 if(m->status!=WF_OK)return;
 wf_participant_76e8(m);
/* $77b0: adda.l #$4, a1 */
 if(m->status!=WF_OK)return;
 m->a[1] += 4u;
/* $77b6: addq.w #$1, d2 */
 if(m->status!=WF_OK)return;
 {uint32_t s=1u,v=m->d[2],r=(v + s)&mask(2); put(&m->d[2],r,2); flags(m,r,2); m->c=((uint64_t)(v&mask(2))+(s&mask(2)))>mask(2);}
/* $77b8: dbra d1, $77aa */
 if(m->status!=WF_OK)return;
 put(&m->d[1],m->d[1]-1,2);if((uint16_t)m->d[1]!=65535u)goto L77aa;
L77bc: /* $77bc: adda.l #$10c, a0 */
 if(m->status!=WF_OK)return;
 m->a[0] += 268u;
/* $77c2: addq.w #$1, d0 */
 if(m->status!=WF_OK)return;
 {uint32_t s=1u,v=m->d[0],r=(v + s)&mask(2); put(&m->d[0],r,2); flags(m,r,2); m->c=((uint64_t)(v&mask(2))+(s&mask(2)))>mask(2);}
/* $77c4: cmpi.w #$4, d0 */
 if(m->status!=WF_OK)return;
 {uint32_t s=4u&mask(2),v=m->d[0]&mask(2);flags(m,v-s,2);m->c=v<s;}
/* $77c8: bne.b $7778 */
 if(m->status!=WF_OK)return;
 if(!m->z) goto L7778;
/* $77ca: movem.l (a7)+, d0-d4/a0-a1 */
 if(m->status!=WF_OK)return;
 m->a[1]=stack[--sp]; m->a[0]=stack[--sp]; m->d[4]=stack[--sp]; m->d[3]=stack[--sp]; m->d[2]=stack[--sp]; m->d[1]=stack[--sp]; m->d[0]=stack[--sp];
/* $77ce: rts  */
 if(m->status!=WF_OK)return;
 return;
}

static void wf_participant_77d8(wf_participant_machine_t *m){
 uint32_t stack[32];unsigned sp=0; (void)stack;(void)sp;
/* $77d8: movem.l d0-d3/a0-a1, -(a7) */
 if(m->status!=WF_OK)return;
 stack[sp++]=m->d[0]; stack[sp++]=m->d[1]; stack[sp++]=m->d[2]; stack[sp++]=m->d[3]; stack[sp++]=m->a[0]; stack[sp++]=m->a[1];
/* $77dc: lea.l $7814.l, a1 */
 if(m->status!=WF_OK)return;
 m->a[1]=30740u;
/* $77e2: asl.w #$1, d0 */
 if(m->status!=WF_OK)return;
 {unsigned s=1u;uint32_t v=m->d[0]&mask(2),r=v<<s; put(&m->d[0],r,2); flags(m,r,2);m->c=s?((v>>(16-s))&1):0;}
/* $77e4: move.w (a1, d0.w), d0 */
 if(m->status!=WF_OK)return;
 {uint32_t v=wf_participant_read(m,(m->a[1] + 0 + (int16_t)m->d[0]),2); put(&m->d[0],v,2); flags(m,v,2);}
/* $77e8: moveq #$2, d2 */
 if(m->status!=WF_OK)return;
 {uint32_t v=(uint32_t)(int32_t)(int8_t)(2u); put(&m->d[2],v,4); flags(m,v,4);}
L77ea: /* $77ea: moveq #$2, d3 */
 if(m->status!=WF_OK)return;
 {uint32_t v=(uint32_t)(int32_t)(int8_t)(2u); put(&m->d[3],v,4); flags(m,v,4);}
L77ec: /* $77ec: move.b d0, $1(a0) */
 if(m->status!=WF_OK)return;
 {uint32_t v=m->d[0]; wf_participant_write(m,(m->a[0] + 1),v,1); flags(m,v,1);}
/* $77f0: move.w d0, d1 */
 if(m->status!=WF_OK)return;
 {uint32_t v=m->d[0]; put(&m->d[1],v,2); flags(m,v,2);}
/* $77f2: lsr.w #$8, d1 */
 if(m->status!=WF_OK)return;
 {unsigned s=8u;uint32_t v=m->d[1]&mask(2),r=v>>s; put(&m->d[1],r,2); flags(m,r,2);m->c=s?((v>>(s-1))&1):0;}
/* $77f4: move.b d1, $3(a0) */
 if(m->status!=WF_OK)return;
 {uint32_t v=m->d[1]; wf_participant_write(m,(m->a[0] + 3),v,1); flags(m,v,1);}
/* $77f8: addq.w #$1, d0 */
 if(m->status!=WF_OK)return;
 {uint32_t s=1u,v=m->d[0],r=(v + s)&mask(2); put(&m->d[0],r,2); flags(m,r,2); m->c=((uint64_t)(v&mask(2))+(s&mask(2)))>mask(2);}
/* $77fa: adda.l #$4, a0 */
 if(m->status!=WF_OK)return;
 m->a[0] += 4u;
/* $7800: dbra d3, $77ec */
 if(m->status!=WF_OK)return;
 put(&m->d[3],m->d[3]-1,2);if((uint16_t)m->d[3]!=65535u)goto L77ec;
/* $7804: adda.l #$f4, a0 */
 if(m->status!=WF_OK)return;
 m->a[0] += 244u;
/* $780a: dbra d2, $77ea */
 if(m->status!=WF_OK)return;
 put(&m->d[2],m->d[2]-1,2);if((uint16_t)m->d[2]!=65535u)goto L77ea;
/* $780e: movem.l (a7)+, d0-d3/a0-a1 */
 if(m->status!=WF_OK)return;
 m->a[1]=stack[--sp]; m->a[0]=stack[--sp]; m->d[3]=stack[--sp]; m->d[2]=stack[--sp]; m->d[1]=stack[--sp]; m->d[0]=stack[--sp];
/* $7812: rts  */
 if(m->status!=WF_OK)return;
 return;
}

void wf_participant_782c(wf_participant_machine_t *m){
 uint32_t stack[32];unsigned sp=0; (void)stack;(void)sp;
/* $782c: movem.l d0/a0-a1, -(a7) */
 if(m->status!=WF_OK)return;
 stack[sp++]=m->d[0]; stack[sp++]=m->a[0]; stack[sp++]=m->a[1];
/* $7830: tst.w $1c007c.l */
 if(m->status!=WF_OK)return;
 {uint32_t v=wf_participant_read(m,1835132u,2);  flags(m,v,2);}
/* $7836: beq.w $786e */
 if(m->status!=WF_OK)return;
 if(m->z) goto L786e;
/* $783a: btst.b #$7, $0(a0) */
 if(m->status!=WF_OK)return;
 {uint32_t v=wf_participant_read(m,(m->a[0] + 0),1),b=1u<<(7u%8); m->z=(v&b)==0; }
/* $7840: beq.b $786e */
 if(m->status!=WF_OK)return;
 if(m->z) goto L786e;
/* $7842: btst.b #$7, $56(a0) */
 if(m->status!=WF_OK)return;
 {uint32_t v=wf_participant_read(m,(m->a[0] + 86),1),b=1u<<(7u%8); m->z=(v&b)==0; }
/* $7848: bne.b $786e */
 if(m->status!=WF_OK)return;
 if(!m->z) goto L786e;
/* $784a: bsr.w $7720 */
 if(m->status!=WF_OK)return;
 wf_participant_7720(m);
/* $784e: bcc.b $786e */
 if(m->status!=WF_OK)return;
 if(!m->c) goto L786e;
/* $7850: bsr.w $7680 */
 if(m->status!=WF_OK)return;
 wf_participant_7680(m);
/* $7854: adda.l #$100, a1 */
 if(m->status!=WF_OK)return;
 m->a[1] += 256u;
/* $785a: move.w $56(a0), d0 */
 if(m->status!=WF_OK)return;
 {uint32_t v=wf_participant_read(m,(m->a[0] + 86),2); put(&m->d[0],v,2); flags(m,v,2);}
/* $785e: andi.w #$f, d0 */
 if(m->status!=WF_OK)return;
 {uint32_t s=15u,v=m->d[0],r=(v & s)&mask(2); put(&m->d[0],r,2); flags(m,r,2); }
/* $7862: cmpi.w #$5, d0 */
 if(m->status!=WF_OK)return;
 {uint32_t s=5u&mask(2),v=m->d[0]&mask(2);flags(m,v-s,2);m->c=v<s;}
/* $7866: beq.b $786e */
 if(m->status!=WF_OK)return;
 if(m->z) goto L786e;
/* $7868: movea.l a1, a0 */
 if(m->status!=WF_OK)return;
 m->a[0]=m->a[1];
/* $786a: bsr.w $77d8 */
 if(m->status!=WF_OK)return;
 wf_participant_77d8(m);
L786e: /* $786e: movem.l (a7)+, d0/a0-a1 */
 if(m->status!=WF_OK)return;
 m->a[1]=stack[--sp]; m->a[0]=stack[--sp]; m->d[0]=stack[--sp];
/* $7872: rts  */
 if(m->status!=WF_OK)return;
 return;
}

void wf_participant_1079e(wf_participant_machine_t *m){
 uint32_t stack[32];unsigned sp=0; (void)stack;(void)sp;
/* $1079e: movem.l d4/a2, -(a7) */
 if(m->status!=WF_OK)return;
 stack[sp++]=m->d[4]; stack[sp++]=m->a[2];
/* $107a2: btst.b #$7, $56(a0) */
 if(m->status!=WF_OK)return;
 {uint32_t v=wf_participant_read(m,(m->a[0] + 86),1),b=1u<<(7u%8); m->z=(v&b)==0; }
/* $107a8: bne.b $107c8 */
 if(m->status!=WF_OK)return;
 if(!m->z) goto L107c8;
/* $107aa: lea.l $10830.l, a2 */
 if(m->status!=WF_OK)return;
 m->a[2]=67632u;
/* $107b0: move.w $56(a0), d4 */
 if(m->status!=WF_OK)return;
 {uint32_t v=wf_participant_read(m,(m->a[0] + 86),2); put(&m->d[4],v,2); flags(m,v,2);}
/* $107b4: andi.w #$f, d4 */
 if(m->status!=WF_OK)return;
 {uint32_t s=15u,v=m->d[4],r=(v & s)&mask(2); put(&m->d[4],r,2); flags(m,r,2); }
/* $107b8: lsl.w #$1, d4 */
 if(m->status!=WF_OK)return;
 {unsigned s=1u;uint32_t v=m->d[4]&mask(2),r=v<<s; put(&m->d[4],r,2); flags(m,r,2);m->c=s?((v>>(16-s))&1):0;}
/* $107ba: move.w (a2, d4.w), $66(a0) */
 if(m->status!=WF_OK)return;
 {uint32_t v=wf_participant_read(m,(m->a[2] + 0 + (int16_t)m->d[4]),2); wf_participant_write(m,(m->a[0] + 102),v,2); flags(m,v,2);}
/* $107c0: move.w (a2, d4.w), $72(a0) */
 if(m->status!=WF_OK)return;
 {uint32_t v=wf_participant_read(m,(m->a[2] + 0 + (int16_t)m->d[4]),2); wf_participant_write(m,(m->a[0] + 114),v,2); flags(m,v,2);}
/* $107c6: bra.b $10826 */
 if(m->status!=WF_OK)return;
 if(1) goto L10826;
L107c8: /* $107c8: btst.b #$0, $1c0161.l */
 if(m->status!=WF_OK)return;
 {uint32_t v=wf_participant_read(m,1835361u,1),b=1u<<(0u%8); m->z=(v&b)==0; }
/* $107d0: bne.b $1081a */
 if(m->status!=WF_OK)return;
 if(!m->z) goto L1081a;
/* $107d2: lea.l $10848.l, a2 */
 if(m->status!=WF_OK)return;
 m->a[2]=67656u;
/* $107d8: move.w $1c0162.l, d4 */
 if(m->status!=WF_OK)return;
 {uint32_t v=wf_participant_read(m,1835362u,2); put(&m->d[4],v,2); flags(m,v,2);}
/* $107de: lsl.w #$1, d4 */
 if(m->status!=WF_OK)return;
 {unsigned s=1u;uint32_t v=m->d[4]&mask(2),r=v<<s; put(&m->d[4],r,2); flags(m,r,2);m->c=s?((v>>(16-s))&1):0;}
/* $107e0: move.w (a2, d4.w), d0 */
 if(m->status!=WF_OK)return;
 {uint32_t v=wf_participant_read(m,(m->a[2] + 0 + (int16_t)m->d[4]),2); put(&m->d[0],v,2); flags(m,v,2);}
/* $107e4: cmpa.l #$1c09e0, a0 */
 if(m->status!=WF_OK)return;
 {uint32_t s=1837536u&mask(4),v=m->a[0]&mask(4);flags(m,v-s,4);m->c=v<s;}
/* $107ea: beq.b $107f0 */
 if(m->status!=WF_OK)return;
 if(m->z) goto L107f0;
/* $107ec: subi.w #$f, d0 */
 if(m->status!=WF_OK)return;
 {uint32_t s=15u,v=m->d[0],r=(v - s)&mask(2); put(&m->d[0],r,2); flags(m,r,2); m->c=(v&mask(2))<(s&mask(2));}
L107f0: /* $107f0: move.w d0, $66(a0) */
 if(m->status!=WF_OK)return;
 {uint32_t v=m->d[0]; wf_participant_write(m,(m->a[0] + 102),v,2); flags(m,v,2);}
/* $107f4: move.w d0, $72(a0) */
 if(m->status!=WF_OK)return;
 {uint32_t v=m->d[0]; wf_participant_write(m,(m->a[0] + 114),v,2); flags(m,v,2);}
/* $107f8: move.w $1c0066.l, d4 */
 if(m->status!=WF_OK)return;
 {uint32_t v=wf_participant_read(m,1835110u,2); put(&m->d[4],v,2); flags(m,v,2);}
/* $107fe: andi.w #$300, d4 */
 if(m->status!=WF_OK)return;
 {uint32_t s=768u,v=m->d[4],r=(v & s)&mask(2); put(&m->d[4],r,2); flags(m,r,2); }
/* $10802: lsr.w #$8, d4 */
 if(m->status!=WF_OK)return;
 {unsigned s=8u;uint32_t v=m->d[4]&mask(2),r=v>>s; put(&m->d[4],r,2); flags(m,r,2);m->c=s?((v>>(s-1))&1):0;}
/* $10804: asl.w #$1, d4 */
 if(m->status!=WF_OK)return;
 {unsigned s=1u;uint32_t v=m->d[4]&mask(2),r=v<<s; put(&m->d[4],r,2); flags(m,r,2);m->c=s?((v>>(16-s))&1):0;}
/* $10806: lea.l $1085c.l, a2 */
 if(m->status!=WF_OK)return;
 m->a[2]=67676u;
/* $1080c: move.w (a2, d4.w), d4 */
 if(m->status!=WF_OK)return;
 {uint32_t v=wf_participant_read(m,(m->a[2] + 0 + (int16_t)m->d[4]),2); put(&m->d[4],v,2); flags(m,v,2);}
/* $10810: add.w d4, $66(a0) */
 if(m->status!=WF_OK)return;
 {uint32_t s=m->d[4],v=wf_participant_read(m,(m->a[0] + 102),2),r=(v + s)&mask(2); wf_participant_write(m,(m->a[0] + 102),r,2); flags(m,r,2); m->c=((uint64_t)(v&mask(2))+(s&mask(2)))>mask(2);}
/* $10814: add.w d4, $72(a0) */
 if(m->status!=WF_OK)return;
 {uint32_t s=m->d[4],v=wf_participant_read(m,(m->a[0] + 114),2),r=(v + s)&mask(2); wf_participant_write(m,(m->a[0] + 114),r,2); flags(m,r,2); m->c=((uint64_t)(v&mask(2))+(s&mask(2)))>mask(2);}
/* $10818: bra.b $10826 */
 if(m->status!=WF_OK)return;
 if(1) goto L10826;
L1081a: /* $1081a: move.w #$87, $66(a0) */
 if(m->status!=WF_OK)return;
 {uint32_t v=135u; wf_participant_write(m,(m->a[0] + 102),v,2); flags(m,v,2);}
/* $10820: move.w $66(a0), $72(a0) */
 if(m->status!=WF_OK)return;
 {uint32_t v=wf_participant_read(m,(m->a[0] + 102),2); wf_participant_write(m,(m->a[0] + 114),v,2); flags(m,v,2);}
L10826: /* $10826: clr.w $70(a0) */
 if(m->status!=WF_OK)return;
 {uint32_t v=0; wf_participant_write(m,(m->a[0] + 112),v,2); flags(m,v,2);}
/* $1082a: movem.l (a7)+, d4/a2 */
 if(m->status!=WF_OK)return;
 m->a[2]=stack[--sp]; m->d[4]=stack[--sp];
/* $1082e: rts  */
 if(m->status!=WF_OK)return;
 return;
}

void wf_participant_9132(wf_participant_machine_t *m){
 uint32_t stack[32];unsigned sp=0; (void)stack;(void)sp;
/* $9132: movem.l d0-d7/a0-a6, -(a7) */
 if(m->status!=WF_OK)return;
 stack[sp++]=m->d[0]; stack[sp++]=m->d[1]; stack[sp++]=m->d[2]; stack[sp++]=m->d[3]; stack[sp++]=m->d[4]; stack[sp++]=m->d[5]; stack[sp++]=m->d[6]; stack[sp++]=m->d[7]; stack[sp++]=m->a[0]; stack[sp++]=m->a[1]; stack[sp++]=m->a[2]; stack[sp++]=m->a[3]; stack[sp++]=m->a[4]; stack[sp++]=m->a[5]; stack[sp++]=m->a[6];
/* $9136: bset.b #$1, $1c167b.l */
 if(m->status!=WF_OK)return;
 {uint32_t v=wf_participant_read(m,1840763u,1),b=1u<<(1u%8); m->z=(v&b)==0; wf_participant_write(m,1840763u,v|b,1);}
/* $913e: bsr.w $9658 */
 if(m->status!=WF_OK)return;
 wf_participant_9658(m);
/* $9142: clr.w $1c15ce.l */
 if(m->status!=WF_OK)return;
 {uint32_t v=0; wf_participant_write(m,1840590u,v,2); flags(m,v,2);}
/* $9148: clr.w d1 */
 if(m->status!=WF_OK)return;
 {uint32_t v=0; put(&m->d[1],v,2); flags(m,v,2);}
/* $914a: move.w #$8, d0 */
 if(m->status!=WF_OK)return;
 {uint32_t v=8u; put(&m->d[0],v,2); flags(m,v,2);}
/* $914e: lea.l $1c05b0.l, a0 */
 if(m->status!=WF_OK)return;
 m->a[0]=1836464u;
L9154: /* $9154: btst.b #$7, $0(a0) */
 if(m->status!=WF_OK)return;
 {uint32_t v=wf_participant_read(m,(m->a[0] + 0),1),b=1u<<(7u%8); m->z=(v&b)==0; }
/* $915a: beq.b $9168 */
 if(m->status!=WF_OK)return;
 if(m->z) goto L9168;
/* $915c: btst.b #$4, $32(a0) */
 if(m->status!=WF_OK)return;
 {uint32_t v=wf_participant_read(m,(m->a[0] + 50),1),b=1u<<(4u%8); m->z=(v&b)==0; }
/* $9162: bne.b $9168 */
 if(m->status!=WF_OK)return;
 if(!m->z) goto L9168;
/* $9164: addi.w #$1, d1 */
 if(m->status!=WF_OK)return;
 {uint32_t s=1u,v=m->d[1],r=(v + s)&mask(2); put(&m->d[1],r,2); flags(m,r,2); m->c=((uint64_t)(v&mask(2))+(s&mask(2)))>mask(2);}
L9168: /* $9168: adda.l #$10c, a0 */
 if(m->status!=WF_OK)return;
 m->a[0] += 268u;
/* $916e: dbra d0, $9154 */
 if(m->status!=WF_OK)return;
 put(&m->d[0],m->d[0]-1,2);if((uint16_t)m->d[0]!=65535u)goto L9154;
/* $9172: cmpi.w #$6, d1 */
 if(m->status!=WF_OK)return;
 {uint32_t s=6u&mask(2),v=m->d[1]&mask(2);flags(m,v-s,2);m->c=v<s;}
/* $9176: bcs.b $9180 */
 if(m->status!=WF_OK)return;
 if(m->c) goto L9180;
/* $9178: move.w #$8000, $1c15ce.l */
 if(m->status!=WF_OK)return;
 {uint32_t v=32768u; wf_participant_write(m,1840590u,v,2); flags(m,v,2);}
L9180: /* $9180: clr.w $1c15b4.l */
 if(m->status!=WF_OK)return;
 {uint32_t v=0; wf_participant_write(m,1840564u,v,2); flags(m,v,2);}
/* $9186: move.w #$7, d0 */
 if(m->status!=WF_OK)return;
 {uint32_t v=7u; put(&m->d[0],v,2); flags(m,v,2);}
/* $918a: lea.l $1c1576.l, a0 */
 if(m->status!=WF_OK)return;
 m->a[0]=1840502u;
L9190: /* $9190: clr.w (a0)+ */
 if(m->status!=WF_OK)return;
 {uint32_t v=0; postwrite(m,&m->a[0],v,2); flags(m,v,2);}
/* $9192: dbra d0, $9190 */
 if(m->status!=WF_OK)return;
 put(&m->d[0],m->d[0]-1,2);if((uint16_t)m->d[0]!=65535u)goto L9190;
/* $9196: move.w #$8, d0 */
 if(m->status!=WF_OK)return;
 {uint32_t v=8u; put(&m->d[0],v,2); flags(m,v,2);}
/* $919a: lea.l $1c15a0.l, a0 */
 if(m->status!=WF_OK)return;
 m->a[0]=1840544u;
L91a0: /* $91a0: clr.w (a0)+ */
 if(m->status!=WF_OK)return;
 {uint32_t v=0; postwrite(m,&m->a[0],v,2); flags(m,v,2);}
/* $91a2: dbra d0, $91a0 */
 if(m->status!=WF_OK)return;
 put(&m->d[0],m->d[0]-1,2);if((uint16_t)m->d[0]!=65535u)goto L91a0;
/* $91a6: lea.l $1c05b0.l, a0 */
 if(m->status!=WF_OK)return;
 m->a[0]=1836464u;
/* $91ac: lea.l $1c1576.l, a1 */
 if(m->status!=WF_OK)return;
 m->a[1]=1840502u;
/* $91b2: move.w #$8, d0 */
 if(m->status!=WF_OK)return;
 {uint32_t v=8u; put(&m->d[0],v,2); flags(m,v,2);}
L91b6: /* $91b6: btst.b #$7, $0(a0) */
 if(m->status!=WF_OK)return;
 {uint32_t v=wf_participant_read(m,(m->a[0] + 0),1),b=1u<<(7u%8); m->z=(v&b)==0; }
/* $91bc: beq.b $91cc */
 if(m->status!=WF_OK)return;
 if(m->z) goto L91cc;
/* $91be: move.w $56(a0), d1 */
 if(m->status!=WF_OK)return;
 {uint32_t v=wf_participant_read(m,(m->a[0] + 86),2); put(&m->d[1],v,2); flags(m,v,2);}
/* $91c2: andi.w #$ff, d1 */
 if(m->status!=WF_OK)return;
 {uint32_t s=255u,v=m->d[1],r=(v & s)&mask(2); put(&m->d[1],r,2); flags(m,r,2); }
/* $91c6: ori.w #$8000, d1 */
 if(m->status!=WF_OK)return;
 {uint32_t s=32768u,v=m->d[1],r=(v | s)&mask(2); put(&m->d[1],r,2); flags(m,r,2); }
/* $91ca: move.w d1, (a1)+ */
 if(m->status!=WF_OK)return;
 {uint32_t v=m->d[1]; postwrite(m,&m->a[1],v,2); flags(m,v,2);}
L91cc: /* $91cc: adda.l #$10c, a0 */
 if(m->status!=WF_OK)return;
 m->a[0] += 268u;
/* $91d2: dbra d0, $91b6 */
 if(m->status!=WF_OK)return;
 put(&m->d[0],m->d[0]-1,2);if((uint16_t)m->d[0]!=65535u)goto L91b6;
/* $91d6: move.w #$8004, (a1)+ */
 if(m->status!=WF_OK)return;
 {uint32_t v=32772u; postwrite(m,&m->a[1],v,2); flags(m,v,2);}
/* $91da: move.w #$8005, (a1)+ */
 if(m->status!=WF_OK)return;
 {uint32_t v=32773u; postwrite(m,&m->a[1],v,2); flags(m,v,2);}
/* $91de: tst.w $1c15ce.l */
 if(m->status!=WF_OK)return;
 {uint32_t v=wf_participant_read(m,1840590u,2);  flags(m,v,2);}
/* $91e4: bne.w $9282 */
 if(m->status!=WF_OK)return;
 if(!m->z) goto L9282;
/* $91e8: lea.l $c1b30.l, a0 */
 if(m->status!=WF_OK)return;
 m->a[0]=793392u;
/* $91ee: lea.l $1c15a0.l, a1 */
 if(m->status!=WF_OK)return;
 m->a[1]=1840544u;
/* $91f4: move.w #$8000, d0 */
 if(m->status!=WF_OK)return;
 {uint32_t v=32768u; put(&m->d[0],v,2); flags(m,v,2);}
L91f8: /* $91f8: lea.l $1c1576.l, a2 */
 if(m->status!=WF_OK)return;
 m->a[2]=1840502u;
/* $91fe: move.w #$0, d1 */
 if(m->status!=WF_OK)return;
 {uint32_t v=0u; put(&m->d[1],v,2); flags(m,v,2);}
L9202: /* $9202: move.w d1, d2 */
 if(m->status!=WF_OK)return;
 {uint32_t v=m->d[1]; put(&m->d[2],v,2); flags(m,v,2);}
/* $9204: asl.w #$1, d2 */
 if(m->status!=WF_OK)return;
 {unsigned s=1u;uint32_t v=m->d[2]&mask(2),r=v<<s; put(&m->d[2],r,2); flags(m,r,2);m->c=s?((v>>(16-s))&1):0;}
/* $9206: move.w (a2, d2.w), d2 */
 if(m->status!=WF_OK)return;
 {uint32_t v=wf_participant_read(m,(m->a[2] + 0 + (int16_t)m->d[2]),2); put(&m->d[2],v,2); flags(m,v,2);}
/* $920a: cmp.w d0, d2 */
 if(m->status!=WF_OK)return;
 {uint32_t s=m->d[0]&mask(2),v=m->d[2]&mask(2);flags(m,v-s,2);m->c=v<s;}
/* $920c: beq.w $9268 */
 if(m->status!=WF_OK)return;
 if(m->z) goto L9268;
/* $9210: addi.w #$1, d1 */
 if(m->status!=WF_OK)return;
 {uint32_t s=1u,v=m->d[1],r=(v + s)&mask(2); put(&m->d[1],r,2); flags(m,r,2); m->c=((uint64_t)(v&mask(2))+(s&mask(2)))>mask(2);}
/* $9214: cmpi.w #$6, d1 */
 if(m->status!=WF_OK)return;
 {uint32_t s=6u&mask(2),v=m->d[1]&mask(2);flags(m,v-s,2);m->c=v<s;}
/* $9218: bcs.b $9202 */
 if(m->status!=WF_OK)return;
 if(m->c) goto L9202;
/* $921a: move.w d0, (a1)+ */
 if(m->status!=WF_OK)return;
 {uint32_t v=m->d[0]; postwrite(m,&m->a[1],v,2); flags(m,v,2);}
/* $921c: move.w d0, d2 */
 if(m->status!=WF_OK)return;
 {uint32_t v=m->d[0]; put(&m->d[2],v,2); flags(m,v,2);}
/* $921e: andi.w #$ff, d2 */
 if(m->status!=WF_OK)return;
 {uint32_t s=255u,v=m->d[2],r=(v & s)&mask(2); put(&m->d[2],r,2); flags(m,r,2); }
/* $9222: asl.w #$1, d2 */
 if(m->status!=WF_OK)return;
 {unsigned s=1u;uint32_t v=m->d[2]&mask(2),r=v<<s; put(&m->d[2],r,2); flags(m,r,2);m->c=s?((v>>(16-s))&1):0;}
/* $9224: lea.l $9714.l, a3 */
 if(m->status!=WF_OK)return;
 m->a[3]=38676u;
/* $922a: move.w (a3, d2.w), d2 */
 if(m->status!=WF_OK)return;
 {uint32_t v=wf_participant_read(m,(m->a[3] + 0 + (int16_t)m->d[2]),2); put(&m->d[2],v,2); flags(m,v,2);}
/* $922e: move.w #$2, d3 */
 if(m->status!=WF_OK)return;
 {uint32_t v=2u; put(&m->d[3],v,2); flags(m,v,2);}
L9232: /* $9232: move.w #$2, d4 */
 if(m->status!=WF_OK)return;
 {uint32_t v=2u; put(&m->d[4],v,2); flags(m,v,2);}
L9236: /* $9236: move.b d2, $1(a0) */
 if(m->status!=WF_OK)return;
 {uint32_t v=m->d[2]; wf_participant_write(m,(m->a[0] + 1),v,1); flags(m,v,1);}
/* $923a: move.w d2, d5 */
 if(m->status!=WF_OK)return;
 {uint32_t v=m->d[2]; put(&m->d[5],v,2); flags(m,v,2);}
/* $923c: asr.w #$8, d5 */
 if(m->status!=WF_OK)return;
 {unsigned s=8u;uint32_t v=m->d[5]&mask(2),r=(uint32_t)((int16_t)v>>s); put(&m->d[5],r,2); flags(m,r,2);m->c=s?((v>>(s-1))&1):0;}
/* $923e: move.b d5, $3(a0) */
 if(m->status!=WF_OK)return;
 {uint32_t v=m->d[5]; wf_participant_write(m,(m->a[0] + 3),v,1); flags(m,v,1);}
/* $9242: adda.l #$4, a0 */
 if(m->status!=WF_OK)return;
 m->a[0] += 4u;
/* $9248: addi.b #$1, d2 */
 if(m->status!=WF_OK)return;
 {uint32_t s=1u,v=m->d[2],r=(v + s)&mask(1); put(&m->d[2],r,1); flags(m,r,1); m->c=((uint64_t)(v&mask(1))+(s&mask(1)))>mask(1);}
/* $924c: dbra d4, $9236 */
 if(m->status!=WF_OK)return;
 put(&m->d[4],m->d[4]-1,2);if((uint16_t)m->d[4]!=65535u)goto L9236;
/* $9250: adda.l #$f4, a0 */
 if(m->status!=WF_OK)return;
 m->a[0] += 244u;
/* $9256: dbra d3, $9232 */
 if(m->status!=WF_OK)return;
 put(&m->d[3],m->d[3]-1,2);if((uint16_t)m->d[3]!=65535u)goto L9232;
/* $925a: addi.w #$1, $1c15b4.l */
 if(m->status!=WF_OK)return;
 {uint32_t s=1u,v=wf_participant_read(m,1840564u,2),r=(v + s)&mask(2); wf_participant_write(m,1840564u,r,2); flags(m,r,2); m->c=((uint64_t)(v&mask(2))+(s&mask(2)))>mask(2);}
/* $9262: suba.l #$2f4, a0 */
 if(m->status!=WF_OK)return;
 m->a[0] -= 756u;
L9268: /* $9268: addi.w #$1, d0 */
 if(m->status!=WF_OK)return;
 {uint32_t s=1u,v=m->d[0],r=(v + s)&mask(2); put(&m->d[0],r,2); flags(m,r,2); m->c=((uint64_t)(v&mask(2))+(s&mask(2)))>mask(2);}
/* $926c: cmpi.b #$4, d0 */
 if(m->status!=WF_OK)return;
 {uint32_t s=4u&mask(1),v=m->d[0]&mask(1);flags(m,v-s,1);m->c=v<s;}
/* $9270: bne.b $9276 */
 if(m->status!=WF_OK)return;
 if(!m->z) goto L9276;
/* $9272: move.b #$6, d0 */
 if(m->status!=WF_OK)return;
 {uint32_t v=6u; put(&m->d[0],v,1); flags(m,v,1);}
L9276: /* $9276: cmpi.b #$c, d0 */
 if(m->status!=WF_OK)return;
 {uint32_t s=12u&mask(1),v=m->d[0]&mask(1);flags(m,v-s,1);m->c=v<s;}
/* $927a: bcs.w $91f8 */
 if(m->status!=WF_OK)return;
 if(m->c) goto L91f8;
/* $927e: bra.w $930c */
 if(m->status!=WF_OK)return;
 if(1) goto L930c;
L9282: /* $9282: lea.l $c1b30.l, a0 */
 if(m->status!=WF_OK)return;
 m->a[0]=793392u;
/* $9288: lea.l $1c15a0.l, a1 */
 if(m->status!=WF_OK)return;
 m->a[1]=1840544u;
/* $928e: lea.l $1c09e0.l, a2 */
 if(m->status!=WF_OK)return;
 m->a[2]=1837536u;
/* $9294: move.w #$4, d1 */
 if(m->status!=WF_OK)return;
 {uint32_t v=4u; put(&m->d[1],v,2); flags(m,v,2);}
L9298: /* $9298: btst.b #$7, $0(a2) */
 if(m->status!=WF_OK)return;
 {uint32_t v=wf_participant_read(m,(m->a[2] + 0),1),b=1u<<(7u%8); m->z=(v&b)==0; }
/* $929e: beq.b $9302 */
 if(m->status!=WF_OK)return;
 if(m->z) goto L9302;
/* $92a0: move.w $56(a2), d0 */
 if(m->status!=WF_OK)return;
 {uint32_t v=wf_participant_read(m,(m->a[2] + 86),2); put(&m->d[0],v,2); flags(m,v,2);}
/* $92a4: andi.w #$ff, d0 */
 if(m->status!=WF_OK)return;
 {uint32_t s=255u,v=m->d[0],r=(v & s)&mask(2); put(&m->d[0],r,2); flags(m,r,2); }
/* $92a8: cmpi.b #$4, d0 */
 if(m->status!=WF_OK)return;
 {uint32_t s=4u&mask(1),v=m->d[0]&mask(1);flags(m,v-s,1);m->c=v<s;}
/* $92ac: beq.b $9302 */
 if(m->status!=WF_OK)return;
 if(m->z) goto L9302;
/* $92ae: cmpi.b #$5, d0 */
 if(m->status!=WF_OK)return;
 {uint32_t s=5u&mask(1),v=m->d[0]&mask(1);flags(m,v-s,1);m->c=v<s;}
/* $92b2: beq.b $9302 */
 if(m->status!=WF_OK)return;
 if(m->z) goto L9302;
/* $92b4: move.w d0, (a1)+ */
 if(m->status!=WF_OK)return;
 {uint32_t v=m->d[0]; postwrite(m,&m->a[1],v,2); flags(m,v,2);}
/* $92b6: move.w d0, d2 */
 if(m->status!=WF_OK)return;
 {uint32_t v=m->d[0]; put(&m->d[2],v,2); flags(m,v,2);}
/* $92b8: andi.w #$ff, d2 */
 if(m->status!=WF_OK)return;
 {uint32_t s=255u,v=m->d[2],r=(v & s)&mask(2); put(&m->d[2],r,2); flags(m,r,2); }
/* $92bc: asl.w #$1, d2 */
 if(m->status!=WF_OK)return;
 {unsigned s=1u;uint32_t v=m->d[2]&mask(2),r=v<<s; put(&m->d[2],r,2); flags(m,r,2);m->c=s?((v>>(16-s))&1):0;}
/* $92be: lea.l $9714.l, a3 */
 if(m->status!=WF_OK)return;
 m->a[3]=38676u;
/* $92c4: move.w (a3, d2.w), d2 */
 if(m->status!=WF_OK)return;
 {uint32_t v=wf_participant_read(m,(m->a[3] + 0 + (int16_t)m->d[2]),2); put(&m->d[2],v,2); flags(m,v,2);}
/* $92c8: move.w #$2, d3 */
 if(m->status!=WF_OK)return;
 {uint32_t v=2u; put(&m->d[3],v,2); flags(m,v,2);}
L92cc: /* $92cc: move.w #$2, d4 */
 if(m->status!=WF_OK)return;
 {uint32_t v=2u; put(&m->d[4],v,2); flags(m,v,2);}
L92d0: /* $92d0: move.b d2, $1(a0) */
 if(m->status!=WF_OK)return;
 {uint32_t v=m->d[2]; wf_participant_write(m,(m->a[0] + 1),v,1); flags(m,v,1);}
/* $92d4: move.w d2, d5 */
 if(m->status!=WF_OK)return;
 {uint32_t v=m->d[2]; put(&m->d[5],v,2); flags(m,v,2);}
/* $92d6: asr.w #$8, d5 */
 if(m->status!=WF_OK)return;
 {unsigned s=8u;uint32_t v=m->d[5]&mask(2),r=(uint32_t)((int16_t)v>>s); put(&m->d[5],r,2); flags(m,r,2);m->c=s?((v>>(s-1))&1):0;}
/* $92d8: move.b d5, $3(a0) */
 if(m->status!=WF_OK)return;
 {uint32_t v=m->d[5]; wf_participant_write(m,(m->a[0] + 3),v,1); flags(m,v,1);}
/* $92dc: adda.l #$4, a0 */
 if(m->status!=WF_OK)return;
 m->a[0] += 4u;
/* $92e2: addi.b #$1, d2 */
 if(m->status!=WF_OK)return;
 {uint32_t s=1u,v=m->d[2],r=(v + s)&mask(1); put(&m->d[2],r,1); flags(m,r,1); m->c=((uint64_t)(v&mask(1))+(s&mask(1)))>mask(1);}
/* $92e6: dbra d4, $92d0 */
 if(m->status!=WF_OK)return;
 put(&m->d[4],m->d[4]-1,2);if((uint16_t)m->d[4]!=65535u)goto L92d0;
/* $92ea: adda.l #$f4, a0 */
 if(m->status!=WF_OK)return;
 m->a[0] += 244u;
/* $92f0: dbra d3, $92cc */
 if(m->status!=WF_OK)return;
 put(&m->d[3],m->d[3]-1,2);if((uint16_t)m->d[3]!=65535u)goto L92cc;
/* $92f4: addi.w #$1, $1c15b4.l */
 if(m->status!=WF_OK)return;
 {uint32_t s=1u,v=wf_participant_read(m,1840564u,2),r=(v + s)&mask(2); wf_participant_write(m,1840564u,r,2); flags(m,r,2); m->c=((uint64_t)(v&mask(2))+(s&mask(2)))>mask(2);}
/* $92fc: suba.l #$2f4, a0 */
 if(m->status!=WF_OK)return;
 m->a[0] -= 756u;
L9302: /* $9302: adda.l #$10c, a2 */
 if(m->status!=WF_OK)return;
 m->a[2] += 268u;
/* $9308: dbra d1, $9298 */
 if(m->status!=WF_OK)return;
 put(&m->d[1],m->d[1]-1,2);if((uint16_t)m->d[1]!=65535u)goto L9298;
L930c: /* $930c: moveq #$0, d0 */
 if(m->status!=WF_OK)return;
 {uint32_t v=(uint32_t)(int32_t)(int8_t)(0u); put(&m->d[0],v,4); flags(m,v,4);}
/* $930e: lea.l $1c1586.l, a1 */
 if(m->status!=WF_OK)return;
 m->a[1]=1840518u;
L9314: /* $9314: btst.b #$7, $0(a1) */
 if(m->status!=WF_OK)return;
 {uint32_t v=wf_participant_read(m,(m->a[1] + 0),1),b=1u<<(7u%8); m->z=(v&b)==0; }
/* $931a: beq.b $9356 */
 if(m->status!=WF_OK)return;
 if(m->z) goto L9356;
/* $931c: move.w d0, d1 */
 if(m->status!=WF_OK)return;
 {uint32_t v=m->d[0]; put(&m->d[1],v,2); flags(m,v,2);}
/* $931e: asl.w #$1, d1 */
 if(m->status!=WF_OK)return;
 {unsigned s=1u;uint32_t v=m->d[1]&mask(2),r=v<<s; put(&m->d[1],r,2); flags(m,r,2);m->c=s?((v>>(16-s))&1):0;}
/* $9320: lea.l $1c1598.l, a0 */
 if(m->status!=WF_OK)return;
 m->a[0]=1840536u;
/* $9326: btst.b #$7, (a0, d1.w) */
 if(m->status!=WF_OK)return;
 {uint32_t v=wf_participant_read(m,(m->a[0] + 0 + (int16_t)m->d[1]),1),b=1u<<(7u%8); m->z=(v&b)==0; }
/* $932c: bne.b $9356 */
 if(m->status!=WF_OK)return;
 if(!m->z) goto L9356;
/* $932e: moveq #$0, d2 */
 if(m->status!=WF_OK)return;
 {uint32_t v=(uint32_t)(int32_t)(int8_t)(0u); put(&m->d[2],v,4); flags(m,v,4);}
L9330: /* $9330: bsr.w $9578 */
 if(m->status!=WF_OK)return;
 wf_participant_9578(m);
/* $9334: bcc.b $9346 */
 if(m->status!=WF_OK)return;
 if(!m->c) goto L9346;
/* $9336: addi.w #$1, d2 */
 if(m->status!=WF_OK)return;
 {uint32_t s=1u,v=m->d[2],r=(v + s)&mask(2); put(&m->d[2],r,2); flags(m,r,2); m->c=((uint64_t)(v&mask(2))+(s&mask(2)))>mask(2);}
/* $933a: cmp.w $1c15b4.l, d2 */
 if(m->status!=WF_OK)return;
 {uint32_t s=wf_participant_read(m,1840564u,2)&mask(2),v=m->d[2]&mask(2);flags(m,v-s,2);m->c=v<s;}
/* $9340: bcs.b $9330 */
 if(m->status!=WF_OK)return;
 if(m->c) goto L9330;
/* $9342: moveq #$0, d2 */
 if(m->status!=WF_OK)return;
 {uint32_t v=(uint32_t)(int32_t)(int8_t)(0u); put(&m->d[2],v,4); flags(m,v,4);}
/* $9344: bra.b $9330 */
 if(m->status!=WF_OK)return;
 if(1) goto L9330;
L9346: /* $9346: bset.b #$f, d2 */
 if(m->status!=WF_OK)return;
 {uint32_t v=m->d[2],b=1u<<(15u%32); m->z=(v&b)==0; put(&m->d[2],v|b,4);}
/* $934a: move.w d2, (a0, d1.w) */
 if(m->status!=WF_OK)return;
 {uint32_t v=m->d[2]; wf_participant_write(m,(m->a[0] + 0 + (int16_t)m->d[1]),v,2); flags(m,v,2);}
/* $934e: move.w #$140, $1c15d0.l */
 if(m->status!=WF_OK)return;
 {uint32_t v=320u; wf_participant_write(m,1840592u,v,2); flags(m,v,2);}
L9356: /* $9356: adda.l #$2, a1 */
 if(m->status!=WF_OK)return;
 m->a[1] += 2u;
/* $935c: addi.w #$1, d0 */
 if(m->status!=WF_OK)return;
 {uint32_t s=1u,v=m->d[0],r=(v + s)&mask(2); put(&m->d[0],r,2); flags(m,r,2); m->c=((uint64_t)(v&mask(2))+(s&mask(2)))>mask(2);}
/* $9360: cmpi.w #$4, d0 */
 if(m->status!=WF_OK)return;
 {uint32_t s=4u&mask(2),v=m->d[0]&mask(2);flags(m,v-s,2);m->c=v<s;}
/* $9364: bne.b $9314 */
 if(m->status!=WF_OK)return;
 if(!m->z) goto L9314;
/* $9366: bsr.w $96c6 */
 if(m->status!=WF_OK)return;
 wf_participant_96c6(m);
/* $936a: moveq #$0, d0 */
 if(m->status!=WF_OK)return;
 {uint32_t v=(uint32_t)(int32_t)(int8_t)(0u); put(&m->d[0],v,4); flags(m,v,4);}
/* $936c: lea.l $1c1586.l, a0 */
 if(m->status!=WF_OK)return;
 m->a[0]=1840518u;
L9372: /* $9372: btst.b #$7, $0(a0) */
 if(m->status!=WF_OK)return;
 {uint32_t v=wf_participant_read(m,(m->a[0] + 0),1),b=1u<<(7u%8); m->z=(v&b)==0; }
/* $9378: beq.b $9380 */
 if(m->status!=WF_OK)return;
 if(m->z) goto L9380;
/* $937a: bsr.b $9398 */
 if(m->status!=WF_OK)return;
 wf_participant_9398(m);
/* $937c: bsr.w $94ca */
 if(m->status!=WF_OK)return;
 wf_participant_94ca(m);
L9380: /* $9380: adda.l #$2, a0 */
 if(m->status!=WF_OK)return;
 m->a[0] += 2u;
/* $9386: addq.w #$1, d0 */
 if(m->status!=WF_OK)return;
 {uint32_t s=1u,v=m->d[0],r=(v + s)&mask(2); put(&m->d[0],r,2); flags(m,r,2); m->c=((uint64_t)(v&mask(2))+(s&mask(2)))>mask(2);}
/* $9388: cmpi.w #$4, d0 */
 if(m->status!=WF_OK)return;
 {uint32_t s=4u&mask(2),v=m->d[0]&mask(2);flags(m,v-s,2);m->c=v<s;}
/* $938c: bne.b $9372 */
 if(m->status!=WF_OK)return;
 if(!m->z) goto L9372;
/* $938e: bsr.w $95ae */
 if(m->status!=WF_OK)return;
 wf_participant_95ae(m);
/* $9392: movem.l (a7)+, d0-d7/a0-a6 */
 if(m->status!=WF_OK)return;
 m->a[6]=stack[--sp]; m->a[5]=stack[--sp]; m->a[4]=stack[--sp]; m->a[3]=stack[--sp]; m->a[2]=stack[--sp]; m->a[1]=stack[--sp]; m->a[0]=stack[--sp]; m->d[7]=stack[--sp]; m->d[6]=stack[--sp]; m->d[5]=stack[--sp]; m->d[4]=stack[--sp]; m->d[3]=stack[--sp]; m->d[2]=stack[--sp]; m->d[1]=stack[--sp]; m->d[0]=stack[--sp];
/* $9396: rts  */
 if(m->status!=WF_OK)return;
 return;
}

static void wf_participant_9398(wf_participant_machine_t *m){
 uint32_t stack[32];unsigned sp=0; (void)stack;(void)sp;
/* $9398: movem.l d0/a0, -(a7) */
 if(m->status!=WF_OK)return;
 stack[sp++]=m->d[0]; stack[sp++]=m->a[0];
/* $939c: subi.w #$1, $1c15d0.l */
 if(m->status!=WF_OK)return;
 {uint32_t s=1u,v=wf_participant_read(m,1840592u,2),r=(v - s)&mask(2); wf_participant_write(m,1840592u,r,2); flags(m,r,2); m->c=(v&mask(2))<(s&mask(2));}
/* $93a4: tst.w $1c15d0.l */
 if(m->status!=WF_OK)return;
 {uint32_t v=wf_participant_read(m,1840592u,2);  flags(m,v,2);}
/* $93aa: beq.b $93c8 */
 if(m->status!=WF_OK)return;
 if(m->z) goto L93c8;
/* $93ac: lea.l $1c15be.l, a1 */
 if(m->status!=WF_OK)return;
 m->a[1]=1840574u;
/* $93b2: move.w d0, d1 */
 if(m->status!=WF_OK)return;
 {uint32_t v=m->d[0]; put(&m->d[1],v,2); flags(m,v,2);}
/* $93b4: asl.w #$1, d1 */
 if(m->status!=WF_OK)return;
 {unsigned s=1u;uint32_t v=m->d[1]&mask(2),r=v<<s; put(&m->d[1],r,2); flags(m,r,2);m->c=s?((v>>(16-s))&1):0;}
/* $93b6: andi.l #$ffff, d1 */
 if(m->status!=WF_OK)return;
 {uint32_t s=65535u,v=m->d[1],r=(v & s)&mask(4); put(&m->d[1],r,4); flags(m,r,4); }
/* $93bc: move.w (a1, d1.w), d2 */
 if(m->status!=WF_OK)return;
 {uint32_t v=wf_participant_read(m,(m->a[1] + 0 + (int16_t)m->d[1]),2); put(&m->d[2],v,2); flags(m,v,2);}
/* $93c0: andi.w #$ff, d2 */
 if(m->status!=WF_OK)return;
 {uint32_t s=255u,v=m->d[2],r=(v & s)&mask(2); put(&m->d[2],r,2); flags(m,r,2); }
/* $93c4: beq.w $94c4 */
 if(m->status!=WF_OK)return;
 if(m->z) goto L94c4;
L93c8: /* $93c8: tst.w $1c15ce.l */
 if(m->status!=WF_OK)return;
 {uint32_t v=wf_participant_read(m,1840590u,2);  flags(m,v,2);}
/* $93ce: bne.b $93d6 */
 if(m->status!=WF_OK)return;
 if(!m->z) goto L93d6;
/* $93d0: move.w #$4000, $0(a0) */
 if(m->status!=WF_OK)return;
 {uint32_t v=16384u; wf_participant_write(m,(m->a[0] + 0),v,2); flags(m,v,2);}
L93d6: /* $93d6: move.w #$8000, $1c169e.l */
 if(m->status!=WF_OK)return;
 {uint32_t v=32768u; wf_participant_write(m,1840798u,v,2); flags(m,v,2);}
/* $93de: bclr.b #$1, $1c167b.l */
 if(m->status!=WF_OK)return;
 {uint32_t v=wf_participant_read(m,1840763u,1),b=1u<<(1u%8); m->z=(v&b)==0; wf_participant_write(m,1840763u,v&~b,1);}
/* $93e6: lea.l $1c1598.l, a0 */
 if(m->status!=WF_OK)return;
 m->a[0]=1840536u;
/* $93ec: move.w (a0, d1.w), d2 */
 if(m->status!=WF_OK)return;
 {uint32_t v=wf_participant_read(m,(m->a[0] + 0 + (int16_t)m->d[1]),2); put(&m->d[2],v,2); flags(m,v,2);}
/* $93f0: andi.w #$f, d2 */
 if(m->status!=WF_OK)return;
 {uint32_t s=15u,v=m->d[2],r=(v & s)&mask(2); put(&m->d[2],r,2); flags(m,r,2); }
/* $93f4: clr.w (a0, d1.w) */
 if(m->status!=WF_OK)return;
 {uint32_t v=0; wf_participant_write(m,(m->a[0] + 0 + (int16_t)m->d[1]),v,2); flags(m,v,2);}
/* $93f8: lea.l $1c15a0.l, a1 */
 if(m->status!=WF_OK)return;
 m->a[1]=1840544u;
/* $93fe: asl.w #$1, d2 */
 if(m->status!=WF_OK)return;
 {unsigned s=1u;uint32_t v=m->d[2]&mask(2),r=v<<s; put(&m->d[2],r,2); flags(m,r,2);m->c=s?((v>>(16-s))&1):0;}
/* $9400: move.w (a1, d2.w), d3 */
 if(m->status!=WF_OK)return;
 {uint32_t v=wf_participant_read(m,(m->a[1] + 0 + (int16_t)m->d[2]),2); put(&m->d[3],v,2); flags(m,v,2);}
/* $9404: andi.w #$f, d3 */
 if(m->status!=WF_OK)return;
 {uint32_t s=15u,v=m->d[3],r=(v & s)&mask(2); put(&m->d[3],r,2); flags(m,r,2); }
/* $9408: tst.w $1c15ce.l */
 if(m->status!=WF_OK)return;
 {uint32_t v=wf_participant_read(m,1840590u,2);  flags(m,v,2);}
/* $940e: beq.b $941e */
 if(m->status!=WF_OK)return;
 if(m->z) goto L941e;
/* $9410: move.b d3, $1c15cf.l */
 if(m->status!=WF_OK)return;
 {uint32_t v=m->d[3]; wf_participant_write(m,1840591u,v,1); flags(m,v,1);}
/* $9416: bsr.w $972c */
 if(m->status!=WF_OK)return;
 wf_participant_972c(m);
/* $941a: bra.w $94c4 */
 if(m->status!=WF_OK)return;
 if(1) goto L94c4;
L941e: /* $941e: lea.l $1c05b0.l, a0 */
 if(m->status!=WF_OK)return;
 m->a[0]=1836464u;
/* $9424: move.w d0, d2 */
 if(m->status!=WF_OK)return;
 {uint32_t v=m->d[0]; put(&m->d[2],v,2); flags(m,v,2);}
L9426: /* $9426: tst.w d2 */
 if(m->status!=WF_OK)return;
 {uint32_t v=m->d[2];  flags(m,v,2);}
/* $9428: beq.b $9434 */
 if(m->status!=WF_OK)return;
 if(m->z) goto L9434;
/* $942a: adda.l #$10c, a0 */
 if(m->status!=WF_OK)return;
 m->a[0] += 268u;
/* $9430: subq.w #$1, d2 */
 if(m->status!=WF_OK)return;
 {uint32_t s=1u,v=m->d[2],r=(v - s)&mask(2); put(&m->d[2],r,2); flags(m,r,2); m->c=(v&mask(2))<(s&mask(2));}
/* $9432: bra.b $9426 */
 if(m->status!=WF_OK)return;
 if(1) goto L9426;
L9434: /* $9434: bset.b #$7, $0(a0) */
 if(m->status!=WF_OK)return;
 {uint32_t v=wf_participant_read(m,(m->a[0] + 0),1),b=1u<<(7u%8); m->z=(v&b)==0; wf_participant_write(m,(m->a[0] + 0),v|b,1);}
/* $943a: move.w d3, $56(a0) */
 if(m->status!=WF_OK)return;
 {uint32_t v=m->d[3]; wf_participant_write(m,(m->a[0] + 86),v,2); flags(m,v,2);}
/* $943e: addi.w #$10, d3 */
 if(m->status!=WF_OK)return;
 {uint32_t s=16u,v=m->d[3],r=(v + s)&mask(2); put(&m->d[3],r,2); flags(m,r,2); m->c=((uint64_t)(v&mask(2))+(s&mask(2)))>mask(2);}
/* $9442: move.w d3, $2(a0) */
 if(m->status!=WF_OK)return;
 {uint32_t v=m->d[3]; wf_participant_write(m,(m->a[0] + 2),v,2); flags(m,v,2);}
/* $9446: move.w #$8003, $32(a0) */
 if(m->status!=WF_OK)return;
 {uint32_t v=32771u; wf_participant_write(m,(m->a[0] + 50),v,2); flags(m,v,2);}
/* $944c: move.w #$0, $1c(a0) */
 if(m->status!=WF_OK)return;
 {uint32_t v=0u; wf_participant_write(m,(m->a[0] + 28),v,2); flags(m,v,2);}
/* $9452: move.b #$1, $a1(a0) */
 if(m->status!=WF_OK)return;
 {uint32_t v=1u; wf_participant_write(m,(m->a[0] + 161),v,1); flags(m,v,1);}
/* $9458: lea.l $1c16a8.l, a1 */
 if(m->status!=WF_OK)return;
 m->a[1]=1840808u;
/* $945e: move.w $56(a0), d3 */
 if(m->status!=WF_OK)return;
 {uint32_t v=wf_participant_read(m,(m->a[0] + 86),2); put(&m->d[3],v,2); flags(m,v,2);}
/* $9462: asl.w #$1, d3 */
 if(m->status!=WF_OK)return;
 {unsigned s=1u;uint32_t v=m->d[3]&mask(2),r=v<<s; put(&m->d[3],r,2); flags(m,r,2);m->c=s?((v>>(16-s))&1):0;}
/* $9464: move.w #$1, (a1, d3.w) */
 if(m->status!=WF_OK)return;
 {uint32_t v=1u; wf_participant_write(m,(m->a[1] + 0 + (int16_t)m->d[3]),v,2); flags(m,v,2);}
/* $946a: movea.l #$140020, a1 */
 if(m->status!=WF_OK)return;
 m->a[1]=1310752u;
/* $9470: adda.l d1, a1 */
 if(m->status!=WF_OK)return;
 m->a[1] += m->d[1];
/* $9472: move.l a1, $8a(a0) */
 if(m->status!=WF_OK)return;
 {uint32_t v=m->a[1]; wf_participant_write(m,(m->a[0] + 138),v,4); flags(m,v,4);}
/* $9476: jsr $1079e.l */
 if(m->status!=WF_OK)return;
 wf_participant_1079e(m);
/* $947c: bsr.w $782c */
 if(m->status!=WF_OK)return;
 wf_participant_782c(m);
/* $9480: bsr.w $7506 */
 if(m->status!=WF_OK)return;
 wf_participant_7506(m);
/* $9484: moveq #$3, d3 */
 if(m->status!=WF_OK)return;
 {uint32_t v=(uint32_t)(int32_t)(int8_t)(3u); put(&m->d[3],v,4); flags(m,v,4);}
L9486: /* $9486: move.w d3, d5 */
 if(m->status!=WF_OK)return;
 {uint32_t v=m->d[3]; put(&m->d[5],v,2); flags(m,v,2);}
/* $9488: asl.w #$1, d5 */
 if(m->status!=WF_OK)return;
 {unsigned s=1u;uint32_t v=m->d[5]&mask(2),r=v<<s; put(&m->d[5],r,2); flags(m,r,2);m->c=s?((v>>(16-s))&1):0;}
/* $948a: move.w (a0, d5.w), d2 */
 if(m->status!=WF_OK)return;
 {uint32_t v=wf_participant_read(m,(m->a[0] + 0 + (int16_t)m->d[5]),2); put(&m->d[2],v,2); flags(m,v,2);}
/* $948e: btst.b #$f, d2 */
 if(m->status!=WF_OK)return;
 {uint32_t v=m->d[2],b=1u<<(15u%32); m->z=(v&b)==0; }
/* $9492: beq.b $94bc */
 if(m->status!=WF_OK)return;
 if(m->z) goto L94bc;
/* $9494: andi.w #$f, d2 */
 if(m->status!=WF_OK)return;
 {uint32_t s=15u,v=m->d[2],r=(v & s)&mask(2); put(&m->d[2],r,2); flags(m,r,2); }
/* $9498: move.w $1c15b4.l, d6 */
 if(m->status!=WF_OK)return;
 {uint32_t v=wf_participant_read(m,1840564u,2); put(&m->d[6],v,2); flags(m,v,2);}
/* $949e: subq.w #$1, d6 */
 if(m->status!=WF_OK)return;
 {uint32_t s=1u,v=m->d[6],r=(v - s)&mask(2); put(&m->d[6],r,2); flags(m,r,2); m->c=(v&mask(2))<(s&mask(2));}
/* $94a0: tst.w d6 */
 if(m->status!=WF_OK)return;
 {uint32_t v=m->d[6];  flags(m,v,2);}
/* $94a2: beq.b $94c4 */
 if(m->status!=WF_OK)return;
 if(m->z) goto L94c4;
/* $94a4: cmp.w d6, d2 */
 if(m->status!=WF_OK)return;
 {uint32_t s=m->d[6]&mask(2),v=m->d[2]&mask(2);flags(m,v-s,2);m->c=v<s;}
/* $94a6: bcs.b $94bc */
 if(m->status!=WF_OK)return;
 if(m->c) goto L94bc;
L94a8: /* $94a8: subq.w #$1, d2 */
 if(m->status!=WF_OK)return;
 {uint32_t s=1u,v=m->d[2],r=(v - s)&mask(2); put(&m->d[2],r,2); flags(m,r,2); m->c=(v&mask(2))<(s&mask(2));}
/* $94aa: bpl.b $94ae */
 if(m->status!=WF_OK)return;
 if(!m->n) goto L94ae;
/* $94ac: move.w d6, d2 */
 if(m->status!=WF_OK)return;
 {uint32_t v=m->d[6]; put(&m->d[2],v,2); flags(m,v,2);}
L94ae: /* $94ae: bsr.w $9578 */
 if(m->status!=WF_OK)return;
 wf_participant_9578(m);
/* $94b2: bcs.b $94a8 */
 if(m->status!=WF_OK)return;
 if(m->c) goto L94a8;
/* $94b4: bset.b #$f, d2 */
 if(m->status!=WF_OK)return;
 {uint32_t v=m->d[2],b=1u<<(15u%32); m->z=(v&b)==0; put(&m->d[2],v|b,4);}
/* $94b8: move.w d2, (a0, d5.w) */
 if(m->status!=WF_OK)return;
 {uint32_t v=m->d[2]; wf_participant_write(m,(m->a[0] + 0 + (int16_t)m->d[5]),v,2); flags(m,v,2);}
L94bc: /* $94bc: dbra d3, $9486 */
 if(m->status!=WF_OK)return;
 put(&m->d[3],m->d[3]-1,2);if((uint16_t)m->d[3]!=65535u)goto L9486;
/* $94c0: bsr.w $96a0 */
 if(m->status!=WF_OK)return;
 wf_participant_96a0(m);
L94c4: /* $94c4: movem.l (a7)+, d0/a0 */
 if(m->status!=WF_OK)return;
 m->a[0]=stack[--sp]; m->d[0]=stack[--sp];
/* $94c8: rts  */
 if(m->status!=WF_OK)return;
 return;
}

static void wf_participant_94ca(wf_participant_machine_t *m){
 uint32_t stack[32];unsigned sp=0; (void)stack;(void)sp;
/* $94ca: move.w d0, d1 */
 if(m->status!=WF_OK)return;
 {uint32_t v=m->d[0]; put(&m->d[1],v,2); flags(m,v,2);}
/* $94cc: asl.w #$1, d1 */
 if(m->status!=WF_OK)return;
 {unsigned s=1u;uint32_t v=m->d[1]&mask(2),r=v<<s; put(&m->d[1],r,2); flags(m,r,2);m->c=s?((v>>(16-s))&1):0;}
/* $94ce: lea.l $1c1598.l, a1 */
 if(m->status!=WF_OK)return;
 m->a[1]=1840536u;
/* $94d4: move.w (a1, d1.w), d2 */
 if(m->status!=WF_OK)return;
 {uint32_t v=wf_participant_read(m,(m->a[1] + 0 + (int16_t)m->d[1]),2); put(&m->d[2],v,2); flags(m,v,2);}
/* $94d8: btst.b #$f, d2 */
 if(m->status!=WF_OK)return;
 {uint32_t v=m->d[2],b=1u<<(15u%32); m->z=(v&b)==0; }
/* $94dc: beq.w $9554 */
 if(m->status!=WF_OK)return;
 if(m->z) goto L9554;
/* $94e0: andi.w #$f, d2 */
 if(m->status!=WF_OK)return;
 {uint32_t s=15u,v=m->d[2],r=(v & s)&mask(2); put(&m->d[2],r,2); flags(m,r,2); }
/* $94e4: movea.l #$140020, a2 */
 if(m->status!=WF_OK)return;
 m->a[2]=1310752u;
/* $94ea: move.w (a2, d1.w), d3 */
 if(m->status!=WF_OK)return;
 {uint32_t v=wf_participant_read(m,(m->a[2] + 0 + (int16_t)m->d[1]),2); put(&m->d[3],v,2); flags(m,v,2);}
/* $94ee: not.w d3 */
 if(m->status!=WF_OK)return;
 {uint32_t v=~(m->d[3]); put(&m->d[3],v,2); flags(m,v,2);}
/* $94f0: andi.w #$f, d3 */
 if(m->status!=WF_OK)return;
 {uint32_t s=15u,v=m->d[3],r=(v & s)&mask(2); put(&m->d[3],r,2); flags(m,r,2); }
/* $94f4: btst.b #$0, d3 */
 if(m->status!=WF_OK)return;
 {uint32_t v=m->d[3],b=1u<<(0u%32); m->z=(v&b)==0; }
/* $94f8: bne.b $952c */
 if(m->status!=WF_OK)return;
 if(!m->z) goto L952c;
/* $94fa: btst.b #$1, d3 */
 if(m->status!=WF_OK)return;
 {uint32_t v=m->d[3],b=1u<<(1u%32); m->z=(v&b)==0; }
/* $94fe: bne.b $950e */
 if(m->status!=WF_OK)return;
 if(!m->z) goto L950e;
/* $9500: lea.l $1c15b6.l, a2 */
 if(m->status!=WF_OK)return;
 m->a[2]=1840566u;
/* $9506: move.w #$5, (a2, d1.w) */
 if(m->status!=WF_OK)return;
 {uint32_t v=5u; wf_participant_write(m,(m->a[2] + 0 + (int16_t)m->d[1]),v,2); flags(m,v,2);}
/* $950c: bra.b $954c */
 if(m->status!=WF_OK)return;
 if(1) goto L954c;
L950e: /* $950e: bsr.b $9556 */
 if(m->status!=WF_OK)return;
 wf_participant_9556(m);
/* $9510: bcs.b $954c */
 if(m->status!=WF_OK)return;
 if(m->c) goto L954c;
/* $9512: move.w $1c15b4.l, d3 */
 if(m->status!=WF_OK)return;
 {uint32_t v=wf_participant_read(m,1840564u,2); put(&m->d[3],v,2); flags(m,v,2);}
/* $9518: cmpi.w #$1, d3 */
 if(m->status!=WF_OK)return;
 {uint32_t s=1u&mask(2),v=m->d[3]&mask(2);flags(m,v-s,2);m->c=v<s;}
/* $951c: beq.b $9554 */
 if(m->status!=WF_OK)return;
 if(m->z) goto L9554;
/* $951e: subq.w #$1, d3 */
 if(m->status!=WF_OK)return;
 {uint32_t s=1u,v=m->d[3],r=(v - s)&mask(2); put(&m->d[3],r,2); flags(m,r,2); m->c=(v&mask(2))<(s&mask(2));}
L9520: /* $9520: subq.w #$1, d2 */
 if(m->status!=WF_OK)return;
 {uint32_t s=1u,v=m->d[2],r=(v - s)&mask(2); put(&m->d[2],r,2); flags(m,r,2); m->c=(v&mask(2))<(s&mask(2));}
/* $9522: bpl.b $9526 */
 if(m->status!=WF_OK)return;
 if(!m->n) goto L9526;
/* $9524: move.w d3, d2 */
 if(m->status!=WF_OK)return;
 {uint32_t v=m->d[3]; put(&m->d[2],v,2); flags(m,v,2);}
L9526: /* $9526: bsr.b $9578 */
 if(m->status!=WF_OK)return;
 wf_participant_9578(m);
/* $9528: bcc.b $954a */
 if(m->status!=WF_OK)return;
 if(!m->c) goto L954a;
/* $952a: bra.b $9520 */
 if(m->status!=WF_OK)return;
 if(1) goto L9520;
L952c: /* $952c: bsr.b $9556 */
 if(m->status!=WF_OK)return;
 wf_participant_9556(m);
/* $952e: bcs.b $954c */
 if(m->status!=WF_OK)return;
 if(m->c) goto L954c;
/* $9530: move.w $1c15b4.l, d3 */
 if(m->status!=WF_OK)return;
 {uint32_t v=wf_participant_read(m,1840564u,2); put(&m->d[3],v,2); flags(m,v,2);}
/* $9536: cmpi.w #$1, d3 */
 if(m->status!=WF_OK)return;
 {uint32_t s=1u&mask(2),v=m->d[3]&mask(2);flags(m,v-s,2);m->c=v<s;}
/* $953a: beq.b $9554 */
 if(m->status!=WF_OK)return;
 if(m->z) goto L9554;
L953c: /* $953c: addq.w #$1, d2 */
 if(m->status!=WF_OK)return;
 {uint32_t s=1u,v=m->d[2],r=(v + s)&mask(2); put(&m->d[2],r,2); flags(m,r,2); m->c=((uint64_t)(v&mask(2))+(s&mask(2)))>mask(2);}
/* $953e: cmp.w d3, d2 */
 if(m->status!=WF_OK)return;
 {uint32_t s=m->d[3]&mask(2),v=m->d[2]&mask(2);flags(m,v-s,2);m->c=v<s;}
/* $9540: bcs.b $9544 */
 if(m->status!=WF_OK)return;
 if(m->c) goto L9544;
/* $9542: moveq #$0, d2 */
 if(m->status!=WF_OK)return;
 {uint32_t v=(uint32_t)(int32_t)(int8_t)(0u); put(&m->d[2],v,4); flags(m,v,4);}
L9544: /* $9544: bsr.b $9578 */
 if(m->status!=WF_OK)return;
 wf_participant_9578(m);
/* $9546: bcc.b $954a */
 if(m->status!=WF_OK)return;
 if(!m->c) goto L954a;
/* $9548: bra.b $953c */
 if(m->status!=WF_OK)return;
 if(1) goto L953c;
L954a: /* $954a: bsr.b $956c */
 if(m->status!=WF_OK)return;
 wf_participant_956c(m);
L954c: /* $954c: bset.b #$f, d2 */
 if(m->status!=WF_OK)return;
 {uint32_t v=m->d[2],b=1u<<(15u%32); m->z=(v&b)==0; put(&m->d[2],v|b,4);}
/* $9550: move.w d2, (a1, d1.w) */
 if(m->status!=WF_OK)return;
 {uint32_t v=m->d[2]; wf_participant_write(m,(m->a[1] + 0 + (int16_t)m->d[1]),v,2); flags(m,v,2);}
L9554: /* $9554: rts  */
 if(m->status!=WF_OK)return;
 return;
}

static void wf_participant_9556(wf_participant_machine_t *m){
 uint32_t stack[32];unsigned sp=0; (void)stack;(void)sp;
/* $9556: lea.l $1c15b6.l, a2 */
 if(m->status!=WF_OK)return;
 m->a[2]=1840566u;
/* $955c: move.w (a2, d1.w), d3 */
 if(m->status!=WF_OK)return;
 {uint32_t v=wf_participant_read(m,(m->a[2] + 0 + (int16_t)m->d[1]),2); put(&m->d[3],v,2); flags(m,v,2);}
/* $9560: addq.w #$1, d3 */
 if(m->status!=WF_OK)return;
 {uint32_t s=1u,v=m->d[3],r=(v + s)&mask(2); put(&m->d[3],r,2); flags(m,r,2); m->c=((uint64_t)(v&mask(2))+(s&mask(2)))>mask(2);}
/* $9562: move.w d3, (a2, d1.w) */
 if(m->status!=WF_OK)return;
 {uint32_t v=m->d[3]; wf_participant_write(m,(m->a[2] + 0 + (int16_t)m->d[1]),v,2); flags(m,v,2);}
/* $9566: cmpi.w #$6, d3 */
 if(m->status!=WF_OK)return;
 {uint32_t s=6u&mask(2),v=m->d[3]&mask(2);flags(m,v-s,2);m->c=v<s;}
/* $956a: rts  */
 if(m->status!=WF_OK)return;
 return;
}

static void wf_participant_956c(wf_participant_machine_t *m){
 uint32_t stack[32];unsigned sp=0; (void)stack;(void)sp;
/* $956c: lea.l $1c15b6.l, a2 */
 if(m->status!=WF_OK)return;
 m->a[2]=1840566u;
/* $9572: clr.w (a2, d1.w) */
 if(m->status!=WF_OK)return;
 {uint32_t v=0; wf_participant_write(m,(m->a[2] + 0 + (int16_t)m->d[1]),v,2); flags(m,v,2);}
/* $9576: rts  */
 if(m->status!=WF_OK)return;
 return;
}

static void wf_participant_9578(wf_participant_machine_t *m){
 uint32_t stack[32];unsigned sp=0; (void)stack;(void)sp;
/* $9578: movem.l d0-d1/a0, -(a7) */
 if(m->status!=WF_OK)return;
 stack[sp++]=m->d[0]; stack[sp++]=m->d[1]; stack[sp++]=m->a[0];
/* $957c: moveq #$3, d0 */
 if(m->status!=WF_OK)return;
 {uint32_t v=(uint32_t)(int32_t)(int8_t)(3u); put(&m->d[0],v,4); flags(m,v,4);}
/* $957e: lea.l $1c1598.l, a0 */
 if(m->status!=WF_OK)return;
 m->a[0]=1840536u;
L9584: /* $9584: move.w d0, d1 */
 if(m->status!=WF_OK)return;
 {uint32_t v=m->d[0]; put(&m->d[1],v,2); flags(m,v,2);}
/* $9586: asl.w #$1, d1 */
 if(m->status!=WF_OK)return;
 {unsigned s=1u;uint32_t v=m->d[1]&mask(2),r=v<<s; put(&m->d[1],r,2); flags(m,r,2);m->c=s?((v>>(16-s))&1):0;}
/* $9588: move.w (a0, d1.w), d1 */
 if(m->status!=WF_OK)return;
 {uint32_t v=wf_participant_read(m,(m->a[0] + 0 + (int16_t)m->d[1]),2); put(&m->d[1],v,2); flags(m,v,2);}
/* $958c: btst.b #$f, d1 */
 if(m->status!=WF_OK)return;
 {uint32_t v=m->d[1],b=1u<<(15u%32); m->z=(v&b)==0; }
/* $9590: beq.b $959a */
 if(m->status!=WF_OK)return;
 if(m->z) goto L959a;
/* $9592: andi.w #$ff, d1 */
 if(m->status!=WF_OK)return;
 {uint32_t s=255u,v=m->d[1],r=(v & s)&mask(2); put(&m->d[1],r,2); flags(m,r,2); }
/* $9596: cmp.w d2, d1 */
 if(m->status!=WF_OK)return;
 {uint32_t s=m->d[2]&mask(2),v=m->d[1]&mask(2);flags(m,v-s,2);m->c=v<s;}
/* $9598: beq.b $95a4 */
 if(m->status!=WF_OK)return;
 if(m->z) goto L95a4;
L959a: /* $959a: dbra d0, $9584 */
 if(m->status!=WF_OK)return;
 put(&m->d[0],m->d[0]-1,2);if((uint16_t)m->d[0]!=65535u)goto L9584;
/* $959e: move.w #$0, ccr */
 if(m->status!=WF_OK)return;
 m->z=0;m->n=0;m->c=0;
/* $95a2: bra.b $95a8 */
 if(m->status!=WF_OK)return;
 if(1) goto L95a8;
L95a4: /* $95a4: move.w #$1, ccr */
 if(m->status!=WF_OK)return;
 m->z=0;m->n=0;m->c=1;
L95a8: /* $95a8: movem.l (a7)+, d0-d1/a0 */
 if(m->status!=WF_OK)return;
 m->a[0]=stack[--sp]; m->d[1]=stack[--sp]; m->d[0]=stack[--sp];
/* $95ac: rts  */
 if(m->status!=WF_OK)return;
 return;
}

static void wf_participant_95ae(wf_participant_machine_t *m){
 uint32_t stack[32];unsigned sp=0; (void)stack;(void)sp;
/* $95ae: move.w #$1a, d0 */
 if(m->status!=WF_OK)return;
 {uint32_t v=26u; put(&m->d[0],v,2); flags(m,v,2);}
/* $95b2: movea.l #$c1930, a0 */
 if(m->status!=WF_OK)return;
 m->a[0]=792880u;
L95b8: /* $95b8: move.l #$0, $0(a0) */
 if(m->status!=WF_OK)return;
 {uint32_t v=0u; wf_participant_write(m,(m->a[0] + 0),v,4); flags(m,v,4);}
/* $95c0: movea.l a0, a1 */
 if(m->status!=WF_OK)return;
 m->a[1]=m->a[0];
/* $95c2: adda.l #$100, a1 */
 if(m->status!=WF_OK)return;
 m->a[1] += 256u;
/* $95c8: move.l #$0, $0(a1) */
 if(m->status!=WF_OK)return;
 {uint32_t v=0u; wf_participant_write(m,(m->a[1] + 0),v,4); flags(m,v,4);}
/* $95d0: adda.l #$4, a0 */
 if(m->status!=WF_OK)return;
 m->a[0] += 4u;
/* $95d6: dbra d0, $95b8 */
 if(m->status!=WF_OK)return;
 put(&m->d[0],m->d[0]-1,2);if((uint16_t)m->d[0]!=65535u)goto L95b8;
/* $95da: moveq #$0, d0 */
 if(m->status!=WF_OK)return;
 {uint32_t v=(uint32_t)(int32_t)(int8_t)(0u); put(&m->d[0],v,4); flags(m,v,4);}
/* $95dc: lea.l $1c1598.l, a0 */
 if(m->status!=WF_OK)return;
 m->a[0]=1840536u;
L95e2: /* $95e2: addq.w #$1, d0 */
 if(m->status!=WF_OK)return;
 {uint32_t s=1u,v=m->d[0],r=(v + s)&mask(2); put(&m->d[0],r,2); flags(m,r,2); m->c=((uint64_t)(v&mask(2))+(s&mask(2)))>mask(2);}
/* $95e4: cmpi.w #$5, d0 */
 if(m->status!=WF_OK)return;
 {uint32_t s=5u&mask(2),v=m->d[0]&mask(2);flags(m,v-s,2);m->c=v<s;}
/* $95e8: beq.w $961a */
 if(m->status!=WF_OK)return;
 if(m->z) goto L961a;
/* $95ec: btst.b #$7, $0(a0) */
 if(m->status!=WF_OK)return;
 {uint32_t v=wf_participant_read(m,(m->a[0] + 0),1),b=1u<<(7u%8); m->z=(v&b)==0; }
/* $95f2: bne.b $95fc */
 if(m->status!=WF_OK)return;
 if(!m->z) goto L95fc;
/* $95f4: adda.l #$2, a0 */
 if(m->status!=WF_OK)return;
 m->a[0] += 2u;
/* $95fa: bra.b $95e2 */
 if(m->status!=WF_OK)return;
 if(1) goto L95e2;
L95fc: /* $95fc: move.b $1(a0), d1 */
 if(m->status!=WF_OK)return;
 {uint32_t v=wf_participant_read(m,(m->a[0] + 1),1); put(&m->d[1],v,1); flags(m,v,1);}
/* $9600: andi.w #$ff, d1 */
 if(m->status!=WF_OK)return;
 {uint32_t s=255u,v=m->d[1],r=(v & s)&mask(2); put(&m->d[1],r,2); flags(m,r,2); }
/* $9604: mulu.w #$c, d1 */
 if(m->status!=WF_OK)return;
 {uint32_t v=(12u&65535u)*(m->d[1]&65535u);put(&m->d[1],v,4); flags(m,v,4);}
/* $9608: movea.l #$c1930, a1 */
 if(m->status!=WF_OK)return;
 m->a[1]=792880u;
/* $960e: adda.l d1, a1 */
 if(m->status!=WF_OK)return;
 m->a[1] += m->d[1];
/* $9610: bsr.b $961c */
 if(m->status!=WF_OK)return;
 wf_participant_961c(m);
/* $9612: adda.l #$2, a0 */
 if(m->status!=WF_OK)return;
 m->a[0] += 2u;
/* $9618: bra.b $95e2 */
 if(m->status!=WF_OK)return;
 if(1) goto L95e2;
L961a: /* $961a: rts  */
 if(m->status!=WF_OK)return;
 return;
}

static void wf_participant_961c(wf_participant_machine_t *m){
 uint32_t stack[32];unsigned sp=0; (void)stack;(void)sp;
/* $961c: move.w d0, d1 */
 if(m->status!=WF_OK)return;
 {uint32_t v=m->d[0]; put(&m->d[1],v,2); flags(m,v,2);}
/* $961e: subq.w #$1, d1 */
 if(m->status!=WF_OK)return;
 {uint32_t s=1u,v=m->d[1],r=(v - s)&mask(2); put(&m->d[1],r,2); flags(m,r,2); m->c=(v&mask(2))<(s&mask(2));}
/* $9620: asl.w #$1, d1 */
 if(m->status!=WF_OK)return;
 {unsigned s=1u;uint32_t v=m->d[1]&mask(2),r=v<<s; put(&m->d[1],r,2); flags(m,r,2);m->c=s?((v>>(16-s))&1):0;}
/* $9622: lea.l $970c.l, a2 */
 if(m->status!=WF_OK)return;
 m->a[2]=38668u;
/* $9628: move.w (a2, d1.w), d2 */
 if(m->status!=WF_OK)return;
 {uint32_t v=wf_participant_read(m,(m->a[2] + 0 + (int16_t)m->d[1]),2); put(&m->d[2],v,2); flags(m,v,2);}
/* $962c: moveq #$1, d5 */
 if(m->status!=WF_OK)return;
 {uint32_t v=(uint32_t)(int32_t)(int8_t)(1u); put(&m->d[5],v,4); flags(m,v,4);}
/* $962e: movea.l a1, a2 */
 if(m->status!=WF_OK)return;
 m->a[2]=m->a[1];
L9630: /* $9630: moveq #$2, d4 */
 if(m->status!=WF_OK)return;
 {uint32_t v=(uint32_t)(int32_t)(int8_t)(2u); put(&m->d[4],v,4); flags(m,v,4);}
L9632: /* $9632: move.b d2, $1(a1) */
 if(m->status!=WF_OK)return;
 {uint32_t v=m->d[2]; wf_participant_write(m,(m->a[1] + 1),v,1); flags(m,v,1);}
/* $9636: move.w d2, d3 */
 if(m->status!=WF_OK)return;
 {uint32_t v=m->d[2]; put(&m->d[3],v,2); flags(m,v,2);}
/* $9638: lsr.w #$8, d3 */
 if(m->status!=WF_OK)return;
 {unsigned s=8u;uint32_t v=m->d[3]&mask(2),r=v>>s; put(&m->d[3],r,2); flags(m,r,2);m->c=s?((v>>(s-1))&1):0;}
/* $963a: move.b d3, $3(a1) */
 if(m->status!=WF_OK)return;
 {uint32_t v=m->d[3]; wf_participant_write(m,(m->a[1] + 3),v,1); flags(m,v,1);}
/* $963e: addq.w #$1, d2 */
 if(m->status!=WF_OK)return;
 {uint32_t s=1u,v=m->d[2],r=(v + s)&mask(2); put(&m->d[2],r,2); flags(m,r,2); m->c=((uint64_t)(v&mask(2))+(s&mask(2)))>mask(2);}
/* $9640: adda.l #$4, a1 */
 if(m->status!=WF_OK)return;
 m->a[1] += 4u;
/* $9646: dbra d4, $9632 */
 if(m->status!=WF_OK)return;
 put(&m->d[4],m->d[4]-1,2);if((uint16_t)m->d[4]!=65535u)goto L9632;
/* $964a: movea.l a2, a1 */
 if(m->status!=WF_OK)return;
 m->a[1]=m->a[2];
/* $964c: adda.l #$100, a1 */
 if(m->status!=WF_OK)return;
 m->a[1] += 256u;
/* $9652: dbra d5, $9630 */
 if(m->status!=WF_OK)return;
 put(&m->d[5],m->d[5]-1,2);if((uint16_t)m->d[5]!=65535u)goto L9630;
/* $9656: rts  */
 if(m->status!=WF_OK)return;
 return;
}

static void wf_participant_9658(wf_participant_machine_t *m){
 uint32_t stack[32];unsigned sp=0; (void)stack;(void)sp;
/* $9658: movem.l d0-d3/a0-a1, -(a7) */
 if(m->status!=WF_OK)return;
 stack[sp++]=m->d[0]; stack[sp++]=m->d[1]; stack[sp++]=m->d[2]; stack[sp++]=m->d[3]; stack[sp++]=m->a[0]; stack[sp++]=m->a[1];
/* $965c: movea.l #$1730, a0 */
 if(m->status!=WF_OK)return;
 m->a[0]=5936u;
/* $9662: adda.l #$c0000, a0 */
 if(m->status!=WF_OK)return;
 m->a[0] += 786432u;
/* $9668: move.w #$507, d2 */
 if(m->status!=WF_OK)return;
 {uint32_t v=1287u; put(&m->d[2],v,2); flags(m,v,2);}
/* $966c: moveq #$1, d0 */
 if(m->status!=WF_OK)return;
 {uint32_t v=(uint32_t)(int32_t)(int8_t)(1u); put(&m->d[0],v,4); flags(m,v,4);}
L966e: /* $966e: move.w #$10, d1 */
 if(m->status!=WF_OK)return;
 {uint32_t v=16u; put(&m->d[1],v,2); flags(m,v,2);}
/* $9672: movea.l a0, a1 */
 if(m->status!=WF_OK)return;
 m->a[1]=m->a[0];
L9674: /* $9674: move.b d2, $1(a1) */
 if(m->status!=WF_OK)return;
 {uint32_t v=m->d[2]; wf_participant_write(m,(m->a[1] + 1),v,1); flags(m,v,1);}
/* $9678: move.w d2, d3 */
 if(m->status!=WF_OK)return;
 {uint32_t v=m->d[2]; put(&m->d[3],v,2); flags(m,v,2);}
/* $967a: lsr.w #$8, d3 */
 if(m->status!=WF_OK)return;
 {unsigned s=8u;uint32_t v=m->d[3]&mask(2),r=v>>s; put(&m->d[3],r,2); flags(m,r,2);m->c=s?((v>>(s-1))&1):0;}
/* $967c: ori.w #$10, d3 */
 if(m->status!=WF_OK)return;
 {uint32_t s=16u,v=m->d[3],r=(v | s)&mask(2); put(&m->d[3],r,2); flags(m,r,2); }
/* $9680: move.b d3, $3(a1) */
 if(m->status!=WF_OK)return;
 {uint32_t v=m->d[3]; wf_participant_write(m,(m->a[1] + 3),v,1); flags(m,v,1);}
/* $9684: adda.l #$4, a1 */
 if(m->status!=WF_OK)return;
 m->a[1] += 4u;
/* $968a: addq.w #$1, d2 */
 if(m->status!=WF_OK)return;
 {uint32_t s=1u,v=m->d[2],r=(v + s)&mask(2); put(&m->d[2],r,2); flags(m,r,2); m->c=((uint64_t)(v&mask(2))+(s&mask(2)))>mask(2);}
/* $968c: dbra d1, $9674 */
 if(m->status!=WF_OK)return;
 put(&m->d[1],m->d[1]-1,2);if((uint16_t)m->d[1]!=65535u)goto L9674;
/* $9690: adda.l #$100, a0 */
 if(m->status!=WF_OK)return;
 m->a[0] += 256u;
/* $9696: dbra d0, $966e */
 if(m->status!=WF_OK)return;
 put(&m->d[0],m->d[0]-1,2);if((uint16_t)m->d[0]!=65535u)goto L966e;
/* $969a: movem.l (a7)+, d0-d3/a0-a1 */
 if(m->status!=WF_OK)return;
 m->a[1]=stack[--sp]; m->a[0]=stack[--sp]; m->d[3]=stack[--sp]; m->d[2]=stack[--sp]; m->d[1]=stack[--sp]; m->d[0]=stack[--sp];
/* $969e: rts  */
 if(m->status!=WF_OK)return;
 return;
}

static void wf_participant_96a0(wf_participant_machine_t *m){
 uint32_t stack[32];unsigned sp=0; (void)stack;(void)sp;
/* $96a0: movem.l d0-d1/a0, -(a7) */
 if(m->status!=WF_OK)return;
 stack[sp++]=m->d[0]; stack[sp++]=m->d[1]; stack[sp++]=m->a[0];
/* $96a4: lea.l $c1730.l, a0 */
 if(m->status!=WF_OK)return;
 m->a[0]=792368u;
/* $96aa: moveq #$6, d0 */
 if(m->status!=WF_OK)return;
 {uint32_t v=(uint32_t)(int32_t)(int8_t)(6u); put(&m->d[0],v,4); flags(m,v,4);}
L96ac: /* $96ac: move.w #$1a, d1 */
 if(m->status!=WF_OK)return;
 {uint32_t v=26u; put(&m->d[1],v,2); flags(m,v,2);}
L96b0: /* $96b0: clr.l (a0)+ */
 if(m->status!=WF_OK)return;
 {uint32_t v=0; postwrite(m,&m->a[0],v,4); flags(m,v,4);}
/* $96b2: dbra d1, $96b0 */
 if(m->status!=WF_OK)return;
 put(&m->d[1],m->d[1]-1,2);if((uint16_t)m->d[1]!=65535u)goto L96b0;
/* $96b6: adda.l #$94, a0 */
 if(m->status!=WF_OK)return;
 m->a[0] += 148u;
/* $96bc: dbra d0, $96ac */
 if(m->status!=WF_OK)return;
 put(&m->d[0],m->d[0]-1,2);if((uint16_t)m->d[0]!=65535u)goto L96ac;
/* $96c0: movem.l (a7)+, d0-d1/a0 */
 if(m->status!=WF_OK)return;
 m->a[0]=stack[--sp]; m->d[1]=stack[--sp]; m->d[0]=stack[--sp];
/* $96c4: rts  */
 if(m->status!=WF_OK)return;
 return;
}

static void wf_participant_96c6(wf_participant_machine_t *m){
 uint32_t stack[32];unsigned sp=0; (void)stack;(void)sp;
/* $96c6: movem.l d0-d7/a0-a6, -(a7) */
 if(m->status!=WF_OK)return;
 stack[sp++]=m->d[0]; stack[sp++]=m->d[1]; stack[sp++]=m->d[2]; stack[sp++]=m->d[3]; stack[sp++]=m->d[4]; stack[sp++]=m->d[5]; stack[sp++]=m->d[6]; stack[sp++]=m->d[7]; stack[sp++]=m->a[0]; stack[sp++]=m->a[1]; stack[sp++]=m->a[2]; stack[sp++]=m->a[3]; stack[sp++]=m->a[4]; stack[sp++]=m->a[5]; stack[sp++]=m->a[6];
/* $96ca: lea.l $140020.l, a5 */
 if(m->status!=WF_OK)return;
 m->a[5]=1310752u;
/* $96d0: lea.l $1c15be.l, a4 */
 if(m->status!=WF_OK)return;
 m->a[4]=1840574u;
/* $96d6: lea.l $1c15c6.l, a3 */
 if(m->status!=WF_OK)return;
 m->a[3]=1840582u;
/* $96dc: clr.w d0 */
 if(m->status!=WF_OK)return;
 {uint32_t v=0; put(&m->d[0],v,2); flags(m,v,2);}
L96de: /* $96de: move.w (a5, d0.w), d1 */
 if(m->status!=WF_OK)return;
 {uint32_t v=wf_participant_read(m,(m->a[5] + 0 + (int16_t)m->d[0]),2); put(&m->d[1],v,2); flags(m,v,2);}
/* $96e2: not.w d1 */
 if(m->status!=WF_OK)return;
 {uint32_t v=~(m->d[1]); put(&m->d[1],v,2); flags(m,v,2);}
/* $96e4: andi.w #$30, d1 */
 if(m->status!=WF_OK)return;
 {uint32_t s=48u,v=m->d[1],r=(v & s)&mask(2); put(&m->d[1],r,2); flags(m,r,2); }
/* $96e8: lsr.w #$4, d1 */
 if(m->status!=WF_OK)return;
 {unsigned s=4u;uint32_t v=m->d[1]&mask(2),r=v>>s; put(&m->d[1],r,2); flags(m,r,2);m->c=s?((v>>(s-1))&1):0;}
/* $96ea: and.w d1, (a3, d0.w) */
 if(m->status!=WF_OK)return;
 {uint32_t s=m->d[1],v=wf_participant_read(m,(m->a[3] + 0 + (int16_t)m->d[0]),2),r=(v & s)&mask(2); wf_participant_write(m,(m->a[3] + 0 + (int16_t)m->d[0]),r,2); flags(m,r,2); }
/* $96ee: eor.w d1, (a3, d0.w) */
 if(m->status!=WF_OK)return;
 {uint32_t s=m->d[1],v=wf_participant_read(m,(m->a[3] + 0 + (int16_t)m->d[0]),2),r=(v ^ s)&mask(2); wf_participant_write(m,(m->a[3] + 0 + (int16_t)m->d[0]),r,2); flags(m,r,2); }
/* $96f2: move.w (a3, d0.w), (a4, d0.w) */
 if(m->status!=WF_OK)return;
 {uint32_t v=wf_participant_read(m,(m->a[3] + 0 + (int16_t)m->d[0]),2); wf_participant_write(m,(m->a[4] + 0 + (int16_t)m->d[0]),v,2); flags(m,v,2);}
/* $96f8: move.w d1, (a3, d0.w) */
 if(m->status!=WF_OK)return;
 {uint32_t v=m->d[1]; wf_participant_write(m,(m->a[3] + 0 + (int16_t)m->d[0]),v,2); flags(m,v,2);}
/* $96fc: addi.w #$2, d0 */
 if(m->status!=WF_OK)return;
 {uint32_t s=2u,v=m->d[0],r=(v + s)&mask(2); put(&m->d[0],r,2); flags(m,r,2); m->c=((uint64_t)(v&mask(2))+(s&mask(2)))>mask(2);}
/* $9700: cmpi.w #$8, d0 */
 if(m->status!=WF_OK)return;
 {uint32_t s=8u&mask(2),v=m->d[0]&mask(2);flags(m,v-s,2);m->c=v<s;}
/* $9704: bcs.b $96de */
 if(m->status!=WF_OK)return;
 if(m->c) goto L96de;
/* $9706: movem.l (a7)+, d0-d7/a0-a6 */
 if(m->status!=WF_OK)return;
 m->a[6]=stack[--sp]; m->a[5]=stack[--sp]; m->a[4]=stack[--sp]; m->a[3]=stack[--sp]; m->a[2]=stack[--sp]; m->a[1]=stack[--sp]; m->a[0]=stack[--sp]; m->d[7]=stack[--sp]; m->d[6]=stack[--sp]; m->d[5]=stack[--sp]; m->d[4]=stack[--sp]; m->d[3]=stack[--sp]; m->d[2]=stack[--sp]; m->d[1]=stack[--sp]; m->d[0]=stack[--sp];
/* $970a: rts  */
 if(m->status!=WF_OK)return;
 return;
}

static void wf_participant_972c(wf_participant_machine_t *m){
 uint32_t stack[32];unsigned sp=0; (void)stack;(void)sp;
/* $972c: jsr $96a0.l */
 if(m->status!=WF_OK)return;
 wf_participant_96a0(m);
/* $9732: lea.l $1c1586.l, a0 */
 if(m->status!=WF_OK)return;
 m->a[0]=1840518u;
/* $9738: move.w #$3, d0 */
 if(m->status!=WF_OK)return;
 {uint32_t v=3u; put(&m->d[0],v,2); flags(m,v,2);}
L973c: /* $973c: cmpi.b #$80, $0(a0) */
 if(m->status!=WF_OK)return;
 {uint32_t s=128u&mask(1),v=wf_participant_read(m,(m->a[0] + 0),1)&mask(1);flags(m,v-s,1);m->c=v<s;}
/* $9742: bne.b $975e */
 if(m->status!=WF_OK)return;
 if(!m->z) goto L975e;
/* $9744: asl.w #$2, d0 */
 if(m->status!=WF_OK)return;
 {unsigned s=2u;uint32_t v=m->d[0]&mask(2),r=v<<s; put(&m->d[0],r,2); flags(m,r,2);m->c=s?((v>>(16-s))&1):0;}
/* $9746: lea.l $989a.l, a0 */
 if(m->status!=WF_OK)return;
 m->a[0]=39066u;
/* $974c: movea.l (a0, d0.w), a1 */
 if(m->status!=WF_OK)return;
 m->a[1]=wf_participant_read(m,(m->a[0] + 0 + (int16_t)m->d[0]),4);
/* $9750: lea.l $98aa.l, a0 */
 if(m->status!=WF_OK)return;
 m->a[0]=39082u;
/* $9756: movea.l (a0, d0.w), a2 */
 if(m->status!=WF_OK)return;
 m->a[2]=wf_participant_read(m,(m->a[0] + 0 + (int16_t)m->d[0]),4);
/* $975a: movea.l a1, a0 */
 if(m->status!=WF_OK)return;
 m->a[0]=m->a[1];
/* $975c: bra.b $9768 */
 if(m->status!=WF_OK)return;
 if(1) goto L9768;
L975e: /* $975e: adda.l #$2, a0 */
 if(m->status!=WF_OK)return;
 m->a[0] += 2u;
/* $9764: dbra d0, $973c */
 if(m->status!=WF_OK)return;
 put(&m->d[0],m->d[0]-1,2);if((uint16_t)m->d[0]!=65535u)goto L973c;
L9768: /* $9768: lea.l $1c09e0.l, a1 */
 if(m->status!=WF_OK)return;
 m->a[1]=1837536u;
/* $976e: move.w #$4, d0 */
 if(m->status!=WF_OK)return;
 {uint32_t v=4u; put(&m->d[0],v,2); flags(m,v,2);}
L9772: /* $9772: btst.b #$7, $0(a1) */
 if(m->status!=WF_OK)return;
 {uint32_t v=wf_participant_read(m,(m->a[1] + 0),1),b=1u<<(7u%8); m->z=(v&b)==0; }
/* $9778: beq.b $9786 */
 if(m->status!=WF_OK)return;
 if(m->z) goto L9786;
/* $977a: move.w $1c15ce.l, d1 */
 if(m->status!=WF_OK)return;
 {uint32_t v=wf_participant_read(m,1840590u,2); put(&m->d[1],v,2); flags(m,v,2);}
/* $9780: cmp.b $57(a1), d1 */
 if(m->status!=WF_OK)return;
 {uint32_t s=wf_participant_read(m,(m->a[1] + 87),1)&mask(1),v=m->d[1]&mask(1);flags(m,v-s,1);m->c=v<s;}
/* $9784: beq.b $9790 */
 if(m->status!=WF_OK)return;
 if(m->z) goto L9790;
L9786: /* $9786: adda.l #$10c, a1 */
 if(m->status!=WF_OK)return;
 m->a[1] += 268u;
/* $978c: dbra d0, $9772 */
 if(m->status!=WF_OK)return;
 put(&m->d[0],m->d[0]-1,2);if((uint16_t)m->d[0]!=65535u)goto L9772;
L9790: /* $9790: movem.l a0-a1, -(a7) */
 if(m->status!=WF_OK)return;
 stack[sp++]=m->a[0]; stack[sp++]=m->a[1];
/* $9794: move.w #$10b, d0 */
 if(m->status!=WF_OK)return;
 {uint32_t v=267u; put(&m->d[0],v,2); flags(m,v,2);}
L9798: /* $9798: move.b (a1), (a0)+ */
 if(m->status!=WF_OK)return;
 {uint32_t v=wf_participant_read(m,m->a[1],1); postwrite(m,&m->a[0],v,1); flags(m,v,1);}
/* $979a: clr.b (a1)+ */
 if(m->status!=WF_OK)return;
 {uint32_t v=0; postwrite(m,&m->a[1],v,1); flags(m,v,1);}
/* $979c: dbra d0, $9798 */
 if(m->status!=WF_OK)return;
 put(&m->d[0],m->d[0]-1,2);if((uint16_t)m->d[0]!=65535u)goto L9798;
/* $97a0: movem.l (a7)+, a0-a1 */
 if(m->status!=WF_OK)return;
 m->a[1]=stack[--sp]; m->a[0]=stack[--sp];
/* $97a4: clr.w d0 */
 if(m->status!=WF_OK)return;
 {uint32_t v=0; put(&m->d[0],v,2); flags(m,v,2);}
/* $97a6: move.l #$1c05b0, d1 */
 if(m->status!=WF_OK)return;
 {uint32_t v=1836464u; put(&m->d[1],v,4); flags(m,v,4);}
L97ac: /* $97ac: cmpa.l d1, a1 */
 if(m->status!=WF_OK)return;
 {uint32_t s=m->d[1]&mask(4),v=m->a[1]&mask(4);flags(m,v-s,4);m->c=v<s;}
/* $97ae: beq.b $97c0 */
 if(m->status!=WF_OK)return;
 if(m->z) goto L97c0;
/* $97b0: addi.l #$10c, d1 */
 if(m->status!=WF_OK)return;
 {uint32_t s=268u,v=m->d[1],r=(v + s)&mask(4); put(&m->d[1],r,4); flags(m,r,4); m->c=((uint64_t)(v&mask(4))+(s&mask(4)))>mask(4);}
/* $97b6: addi.w #$1, d0 */
 if(m->status!=WF_OK)return;
 {uint32_t s=1u,v=m->d[0],r=(v + s)&mask(2); put(&m->d[0],r,2); flags(m,r,2); m->c=((uint64_t)(v&mask(2))+(s&mask(2)))>mask(2);}
/* $97ba: cmpi.w #$9, d0 */
 if(m->status!=WF_OK)return;
 {uint32_t s=9u&mask(2),v=m->d[0]&mask(2);flags(m,v-s,2);m->c=v<s;}
/* $97be: bcs.b $97ac */
 if(m->status!=WF_OK)return;
 if(m->c) goto L97ac;
L97c0: /* $97c0: asl.w #$1, d0 */
 if(m->status!=WF_OK)return;
 {unsigned s=1u;uint32_t v=m->d[0]&mask(2),r=v<<s; put(&m->d[0],r,2); flags(m,r,2);m->c=s?((v>>(16-s))&1):0;}
/* $97c2: lea.l $1c1586.l, a3 */
 if(m->status!=WF_OK)return;
 m->a[3]=1840518u;
/* $97c8: move.w (a3, d0.w), d1 */
 if(m->status!=WF_OK)return;
 {uint32_t v=wf_participant_read(m,(m->a[3] + 0 + (int16_t)m->d[0]),2); put(&m->d[1],v,2); flags(m,v,2);}
/* $97cc: beq.b $97ec */
 if(m->status!=WF_OK)return;
 if(m->z) goto L97ec;
/* $97ce: clr.w d5 */
 if(m->status!=WF_OK)return;
 {uint32_t v=0; put(&m->d[5],v,2); flags(m,v,2);}
L97d0: /* $97d0: move.w (a3, d5.w), d6 */
 if(m->status!=WF_OK)return;
 {uint32_t v=wf_participant_read(m,(m->a[3] + 0 + (int16_t)m->d[5]),2); put(&m->d[6],v,2); flags(m,v,2);}
/* $97d4: tst.w d6 */
 if(m->status!=WF_OK)return;
 {uint32_t v=m->d[6];  flags(m,v,2);}
/* $97d6: beq.b $97e2 */
 if(m->status!=WF_OK)return;
 if(m->z) goto L97e2;
/* $97d8: move.w d1, (a3, d5.w) */
 if(m->status!=WF_OK)return;
 {uint32_t v=m->d[1]; wf_participant_write(m,(m->a[3] + 0 + (int16_t)m->d[5]),v,2); flags(m,v,2);}
/* $97dc: clr.w (a3, d0.w) */
 if(m->status!=WF_OK)return;
 {uint32_t v=0; wf_participant_write(m,(m->a[3] + 0 + (int16_t)m->d[0]),v,2); flags(m,v,2);}
/* $97e0: bra.b $97ec */
 if(m->status!=WF_OK)return;
 if(1) goto L97ec;
L97e2: /* $97e2: addi.w #$2, d5 */
 if(m->status!=WF_OK)return;
 {uint32_t s=2u,v=m->d[5],r=(v + s)&mask(2); put(&m->d[5],r,2); flags(m,r,2); m->c=((uint64_t)(v&mask(2))+(s&mask(2)))>mask(2);}
/* $97e6: cmpi.w #$8, d5 */
 if(m->status!=WF_OK)return;
 {uint32_t s=8u&mask(2),v=m->d[5]&mask(2);flags(m,v-s,2);m->c=v<s;}
/* $97ea: bne.b $97d0 */
 if(m->status!=WF_OK)return;
 if(!m->z) goto L97d0;
L97ec: /* $97ec: lea.l $1c1586.l, a5 */
 if(m->status!=WF_OK)return;
 m->a[5]=1840518u;
/* $97f2: move.w #$3, d7 */
 if(m->status!=WF_OK)return;
 {uint32_t v=3u; put(&m->d[7],v,2); flags(m,v,2);}
L97f6: /* $97f6: cmpi.b #$80, $0(a5) */
 if(m->status!=WF_OK)return;
 {uint32_t s=128u&mask(1),v=wf_participant_read(m,(m->a[5] + 0),1)&mask(1);flags(m,v-s,1);m->c=v<s;}
/* $97fc: bne.b $9802 */
 if(m->status!=WF_OK)return;
 if(!m->z) goto L9802;
/* $97fe: clr.w $0(a5) */
 if(m->status!=WF_OK)return;
 {uint32_t v=0; wf_participant_write(m,(m->a[5] + 0),v,2); flags(m,v,2);}
L9802: /* $9802: adda.l #$2, a5 */
 if(m->status!=WF_OK)return;
 m->a[5] += 2u;
/* $9808: dbra d7, $97f6 */
 if(m->status!=WF_OK)return;
 put(&m->d[7],m->d[7]-1,2);if((uint16_t)m->d[7]!=65535u)goto L97f6;
/* $980c: move.l a2, $8a(a0) */
 if(m->status!=WF_OK)return;
 {uint32_t v=m->a[2]; wf_participant_write(m,(m->a[0] + 138),v,4); flags(m,v,4);}
/* $9810: andi.b #$7f, $56(a0) */
 if(m->status!=WF_OK)return;
 {uint32_t s=127u,v=wf_participant_read(m,(m->a[0] + 86),1),r=(v & s)&mask(1); wf_participant_write(m,(m->a[0] + 86),r,1); flags(m,r,1); }
/* $9816: cmpi.b #$1, $1d(a0) */
 if(m->status!=WF_OK)return;
 {uint32_t s=1u&mask(1),v=wf_participant_read(m,(m->a[0] + 29),1)&mask(1);flags(m,v-s,1);m->c=v<s;}
/* $981c: bne.b $9826 */
 if(m->status!=WF_OK)return;
 if(!m->z) goto L9826;
/* $981e: cmpi.b #$b, $af(a0) */
 if(m->status!=WF_OK)return;
 {uint32_t s=11u&mask(1),v=wf_participant_read(m,(m->a[0] + 175),1)&mask(1);flags(m,v-s,1);m->c=v<s;}
/* $9824: beq.b $982a */
 if(m->status!=WF_OK)return;
 if(m->z) goto L982a;
L9826: /* $9826: clr.b $56(a0) */
 if(m->status!=WF_OK)return;
 {uint32_t v=0; wf_participant_write(m,(m->a[0] + 86),v,1); flags(m,v,1);}
L982a: /* $982a: move.b #$1, $a1(a0) */
 if(m->status!=WF_OK)return;
 {uint32_t v=1u; wf_participant_write(m,(m->a[0] + 161),v,1); flags(m,v,1);}
/* $9830: jsr $1079e.l */
 if(m->status!=WF_OK)return;
 wf_participant_1079e(m);
/* $9836: move.w $66(a0), $6e(a0) */
 if(m->status!=WF_OK)return;
 {uint32_t v=wf_participant_read(m,(m->a[0] + 102),2); wf_participant_write(m,(m->a[0] + 110),v,2); flags(m,v,2);}
/* $983c: move.w #$1, $aa(a0) */
 if(m->status!=WF_OK)return;
 {uint32_t v=1u; wf_participant_write(m,(m->a[0] + 170),v,2); flags(m,v,2);}
/* $9842: jsr $782c.l */
 if(m->status!=WF_OK)return;
 wf_participant_782c(m);
/* $9848: movem.l d0/a1, -(a7) */
 if(m->status!=WF_OK)return;
 stack[sp++]=m->d[0]; stack[sp++]=m->a[1];
/* $984c: jsr $7720.l */
 if(m->status!=WF_OK)return;
 wf_participant_7720(m);
/* $9852: jsr $75ee.l */
 if(m->status!=WF_OK)return;
 wf_participant_75ee(m);
/* $9858: movem.l (a7)+, d0/a1 */
 if(m->status!=WF_OK)return;
 m->a[1]=stack[--sp]; m->d[0]=stack[--sp];
/* $985c: ori.w #$3, $32(a0) */
 if(m->status!=WF_OK)return;
 {uint32_t s=3u,v=wf_participant_read(m,(m->a[0] + 50),2),r=(v | s)&mask(2); wf_participant_write(m,(m->a[0] + 50),r,2); flags(m,r,2); }
/* $9862: bset.b #$4, $35(a0) */
 if(m->status!=WF_OK)return;
 {uint32_t v=wf_participant_read(m,(m->a[0] + 53),1),b=1u<<(4u%8); m->z=(v&b)==0; wf_participant_write(m,(m->a[0] + 53),v|b,1);}
/* $9868: movea.l $26(a0), a2 */
 if(m->status!=WF_OK)return;
 m->a[2]=wf_participant_read(m,(m->a[0] + 38),4);
/* $986c: movea.l $9c(a0), a3 */
 if(m->status!=WF_OK)return;
 m->a[3]=wf_participant_read(m,(m->a[0] + 156),4);
/* $9870: cmpa.l $26(a2), a1 */
 if(m->status!=WF_OK)return;
 {uint32_t s=wf_participant_read(m,(m->a[2] + 38),4)&mask(4),v=m->a[1]&mask(4);flags(m,v-s,4);m->c=v<s;}
/* $9874: bne.b $987a */
 if(m->status!=WF_OK)return;
 if(!m->z) goto L987a;
/* $9876: move.l a0, $26(a2) */
 if(m->status!=WF_OK)return;
 {uint32_t v=m->a[0]; wf_participant_write(m,(m->a[2] + 38),v,4); flags(m,v,4);}
L987a: /* $987a: cmpa.l $9c(a2), a1 */
 if(m->status!=WF_OK)return;
 {uint32_t s=wf_participant_read(m,(m->a[2] + 156),4)&mask(4),v=m->a[1]&mask(4);flags(m,v-s,4);m->c=v<s;}
/* $987e: bne.b $9884 */
 if(m->status!=WF_OK)return;
 if(!m->z) goto L9884;
/* $9880: move.l a0, $9c(a2) */
 if(m->status!=WF_OK)return;
 {uint32_t v=m->a[0]; wf_participant_write(m,(m->a[2] + 156),v,4); flags(m,v,4);}
L9884: /* $9884: cmpa.l $26(a3), a1 */
 if(m->status!=WF_OK)return;
 {uint32_t s=wf_participant_read(m,(m->a[3] + 38),4)&mask(4),v=m->a[1]&mask(4);flags(m,v-s,4);m->c=v<s;}
/* $9888: bne.b $988e */
 if(m->status!=WF_OK)return;
 if(!m->z) goto L988e;
/* $988a: move.l a0, $26(a3) */
 if(m->status!=WF_OK)return;
 {uint32_t v=m->a[0]; wf_participant_write(m,(m->a[3] + 38),v,4); flags(m,v,4);}
L988e: /* $988e: cmpa.l $9c(a3), a1 */
 if(m->status!=WF_OK)return;
 {uint32_t s=wf_participant_read(m,(m->a[3] + 156),4)&mask(4),v=m->a[1]&mask(4);flags(m,v-s,4);m->c=v<s;}
/* $9892: bne.b $9898 */
 if(m->status!=WF_OK)return;
 if(!m->z) goto L9898;
/* $9894: move.l a0, $9c(a3) */
 if(m->status!=WF_OK)return;
 {uint32_t v=m->a[0]; wf_participant_write(m,(m->a[3] + 156),v,4); flags(m,v,4);}
L9898: /* $9898: rts  */
 if(m->status!=WF_OK)return;
 return;
}
