#ifndef WF_R18_FAULT_PAGES_H
#define WF_R18_FAULT_PAGES_H
#include <stdint.h>
#include <string.h>
#if defined(__GNUC__)
#define WF_FAULT_COLD __attribute__((noinline,cold,optimize("Os")))
#else
#define WF_FAULT_COLD
#endif

/* A failure snapshot is copied before changing the video mode. Hex values
 * remain the original raw values; labels explain what each value means. */
typedef struct wf_fault_actor {
 uint16_t id,state,action,x,y,z,vx,vy,mirror,stamina,anim,timer;
 uint8_t active,role,flags,latch,phase,facing,target;
 uint8_t input_fresh,input_held,input_derived;
} wf_fault_actor_t;
typedef struct wf_fault_snapshot {
 uint32_t owner,dep,frame,vblank,rng,seed,descriptor,callback,tick,action_dep;
 uint32_t cpu_handler,cpu_dep,video_failed,cd_windows,cd_sectors;
 uint32_t cd_refills[6],cd_bulk_sectors[6];
 uint32_t collision_probe[12],collision_first_callback,collision_second_callback;
 uint8_t collision_valid;
 uint16_t pad_pressed,pad_held,cabinet,progression;
 uint8_t slot,mode,trace_action,cpu_state,pad_type,pad_size,pad_connected,input_low;
 wf_fault_actor_t actor[9];
} wf_fault_snapshot_t;

/* Only a pair-dispatch fault writes this probe; reject other $24062 faults
 * so a previous collision cannot be mistaken for the current incident. */
static WF_FAULT_COLD void wf_fault_capture_collision(wf_fault_snapshot_t*s,
 uint32_t owner,uint32_t dep,const volatile uint32_t*probe,
 const uint32_t*first,const uint32_t*second){
 s->collision_valid=0;
 if(owner!=0x24062u||probe[0]>=9u||probe[1]>=9u||probe[2]>=34u||probe[3]>=34u||
    probe[4]>=15u||probe[5]>=13u||first[probe[4]]!=dep)return;
 for(unsigned i=0;i<12u;i++)s->collision_probe[i]=probe[i];
 s->collision_first_callback=first[probe[4]];
 s->collision_second_callback=second[probe[5]];
 s->collision_valid=1;
}

/* Native 5x7 glyphs fit 42 columns within the 320-pixel NTSC viewport.
 * The bitmap has a 512-pixel pitch; that is not the visible screen width. */
static const uint8_t wf_fault_font[36][7]={
 {14,17,19,21,25,17,14},{4,12,4,4,4,4,14},{14,17,1,2,4,8,31},
 {30,1,1,14,1,1,30},{2,6,10,18,31,2,2},{31,16,16,30,1,1,30},
 {14,16,16,30,17,17,14},{31,1,2,4,8,8,8},{14,17,17,14,17,17,14},
 {14,17,17,15,1,1,14},{14,17,17,31,17,17,17},{30,17,17,30,17,17,30},
 {15,16,16,16,16,16,15},{30,17,17,17,17,17,30},{31,16,16,30,16,16,31},
 {31,16,16,30,16,16,16},{15,16,16,19,17,17,15},{17,17,17,31,17,17,17},
 {14,4,4,4,4,4,14},{1,1,1,1,17,17,14},{17,18,20,24,20,18,17},
 {16,16,16,16,16,16,31},{17,27,21,21,17,17,17},{17,25,21,19,17,17,17},
 {14,17,17,17,17,17,14},{30,17,17,30,16,16,16},{14,17,17,17,21,18,13},
 {30,17,17,30,20,18,17},{15,16,16,14,1,1,30},{31,4,4,4,4,4,4},
 {17,17,17,17,17,17,14},{17,17,17,17,17,10,4},{17,17,17,21,21,21,10},
 {17,17,10,4,10,17,17},{17,17,10,4,4,4,4},{31,1,2,4,8,16,31}
};
static WF_FAULT_COLD const uint8_t *wf_fault_glyph(char c){
 static const uint8_t colon[7]={0,4,4,0,4,4,0},dash[7]={0,0,0,31,0,0,0};
 static const uint8_t slash[7]={1,1,2,4,8,16,16},arrow[7]={0,16,8,31,8,16,0};
 if(c>='0'&&c<='9')return wf_fault_font[c-'0'];
 if(c>='A'&&c<='Z')return wf_fault_font[c-'A'+10];
 if(c==':')return colon;
 if(c=='-')return dash;
 if(c=='/')return slash;
 if(c=='>')return arrow;
 return 0;
}
typedef struct wf_fault_line {char b[43];unsigned n;} wf_fault_line_t;
static void wf_fault_reset(wf_fault_line_t*l){l->n=0;}
static WF_FAULT_COLD void wf_fault_text(wf_fault_line_t*l,const char*s){while(*s&&l->n<42u)l->b[l->n++]=*s++;}
static WF_FAULT_COLD void wf_fault_hex(wf_fault_line_t*l,uint32_t v,unsigned digits){
 static const char h[]="0123456789ABCDEF";
 for(unsigned i=0;i<digits&&l->n<42u;i++)l->b[l->n++]=h[(v>>((digits-1u-i)*4u))&15u];
}
static WF_FAULT_COLD void wf_fault_row(volatile uint16_t*bitmap,unsigned row,unsigned color,const wf_fault_line_t*l){
 if(row>=14u)return;
 for(unsigned col=0;col<l->n&&col<42u;col++){
  const uint8_t*g=wf_fault_glyph(l->b[col]);if(!g)continue;
  unsigned x0=24u+col*6u,y0=18u+row*14u;
  for(unsigned y=0;y<7u;y++)for(unsigned x=0;x<5u;x++)if(g[y]&(16u>>x)){
    unsigned px=x0+x,at=(y0+y)*256u+px/2u;
    uint16_t old=bitmap[at];
    bitmap[at]=(px&1u)?(uint16_t)((old&0xff00u)|color):
      (uint16_t)((old&0x00ffu)|(color<<8));
  }
 }
}
static WF_FAULT_COLD const char*wf_fault_name(unsigned id){
 static const char*const n[12]={"HOGAN","WARRIOR","SNAKE","BOSSMAN",
  "HAWK","ANIMAL","SLAUGHTER","PERFECT","EARTHQUAKE","DIBIASE","SMASH","CRUSH"};
 return id<12u?n[id]:"UNKNOWN";
}
static WF_FAULT_COLD void wf_fault_title(volatile uint16_t*b,unsigned page){
 wf_fault_line_t l;wf_fault_reset(&l);wf_fault_text(&l,"WRESTLEFEST FAULT R05  ");
  wf_fault_hex(&l,page+1u,1);wf_fault_text(&l,"/5");wf_fault_row(b,0,4,&l);
 wf_fault_reset(&l);wf_fault_text(&l,"LEFT/RIGHT PAGE  -  GAME FROZEN");wf_fault_row(b,1,5,&l);
}
#define WF_FL(row,color) do { \
 wf_fault_row(bitmap,(row),(color),&l); \
 wf_fault_reset(&l); \
} while(0)
static WF_FAULT_COLD void wf_fault_page0(volatile uint16_t*bitmap,const wf_fault_snapshot_t*s){
 wf_fault_line_t l;wf_fault_reset(&l);wf_fault_title(bitmap,0);
 wf_fault_text(&l,s->dep==0x12614u?"UNIMPLEMENTED RAW ACTION":"RUNTIME OPERATION BLOCKED");WF_FL(2,4);
 wf_fault_text(&l,"OWNER ");wf_fault_hex(&l,s->owner,6);wf_fault_text(&l," DEP ");wf_fault_hex(&l,s->dep,6);WF_FL(3,3);
 wf_fault_text(&l,"FRAME ");wf_fault_hex(&l,s->frame,8);wf_fault_text(&l," VBL ");wf_fault_hex(&l,s->vblank,8);WF_FL(4,3);
 wf_fault_text(&l,"MODE ");wf_fault_text(&l,(s->mode&1u)?"ROYAL":"TAG");wf_fault_text(&l," PROG ");wf_fault_hex(&l,s->progression,4);wf_fault_text(&l," SLOT ");wf_fault_hex(&l,s->slot,2);WF_FL(5,3);
 wf_fault_text(&l,"ACTOR ");wf_fault_hex(&l,s->slot,2);
 if(s->slot<9u){const wf_fault_actor_t*a=&s->actor[s->slot];wf_fault_text(&l," ");wf_fault_text(&l,wf_fault_name(a->id));wf_fault_text(&l," > ");wf_fault_hex(&l,a->target,2);
  if(a->target<9u){wf_fault_text(&l," ");wf_fault_text(&l,wf_fault_name(s->actor[a->target].id));}}
 else wf_fault_text(&l," UNKNOWN");
 WF_FL(6,4);
 wf_fault_text(&l,"TRACE ACT ");wf_fault_hex(&l,s->trace_action,2);
 if(s->slot<9u){wf_fault_text(&l," RAW ");wf_fault_hex(&l,s->actor[s->slot].action,4);wf_fault_text(&l," ST ");wf_fault_hex(&l,s->actor[s->slot].state,4);}WF_FL(7,3);
 wf_fault_text(&l,"DESCR ");wf_fault_hex(&l,s->descriptor,6);wf_fault_text(&l," CALLBACK ");wf_fault_hex(&l,s->callback,6);WF_FL(8,3);
 wf_fault_text(&l,"TRACE DEP ");wf_fault_hex(&l,s->action_dep,6);wf_fault_text(&l," TICK ");wf_fault_hex(&l,s->tick,6);WF_FL(9,3);
 wf_fault_text(&l,"CPU ");wf_fault_hex(&l,s->cpu_handler,6);wf_fault_text(&l," CPU DEP ");wf_fault_hex(&l,s->cpu_dep,6);WF_FL(10,3);
 wf_fault_text(&l,"PAD ");wf_fault_hex(&l,s->pad_pressed,4);wf_fault_text(&l,"/");wf_fault_hex(&l,s->pad_held,4);wf_fault_text(&l," INPUT ");wf_fault_hex(&l,s->input_low,2);WF_FL(11,3);
  wf_fault_text(&l,"PHOTO ALL FIVE PAGES TO REPORT FAULT");WF_FL(12,5);
}
static WF_FAULT_COLD void wf_fault_page1(volatile uint16_t*bitmap,const wf_fault_snapshot_t*s){
 wf_fault_line_t l;wf_fault_reset(&l);wf_fault_title(bitmap,1);
 if(s->collision_valid){
  const uint32_t*p=s->collision_probe;
  wf_fault_text(&l,"COLLISION PAIR AND HANDLERS");WF_FL(2,4);
  wf_fault_text(&l,"SOURCE SLOT ");wf_fault_hex(&l,p[0],2);wf_fault_text(&l," TARGET SLOT ");wf_fault_hex(&l,p[1],2);WF_FL(3,3);
  wf_fault_text(&l,"PROFILE ");wf_fault_hex(&l,p[2],2);wf_fault_text(&l,"/");wf_fault_hex(&l,p[3],2);wf_fault_text(&l," SELECT ");wf_fault_hex(&l,p[4],2);wf_fault_text(&l,"/");wf_fault_hex(&l,p[5],2);WF_FL(4,3);
  wf_fault_text(&l,"FIRST ");wf_fault_hex(&l,s->collision_first_callback,6);wf_fault_text(&l," SECOND ");wf_fault_hex(&l,s->collision_second_callback,6);WF_FL(5,3);
  wf_fault_text(&l,"RAW PROFILES ");wf_fault_hex(&l,p[6],4);wf_fault_text(&l,"/");wf_fault_hex(&l,p[7],4);WF_FL(6,3);
  wf_fault_text(&l,"USED LINK ");wf_fault_hex(&l,p[8],8);WF_FL(7,3);
  wf_fault_text(&l,"SUBTYPE ");wf_fault_hex(&l,p[9],4);wf_fault_text(&l," TIMER52 ");wf_fault_hex(&l,p[10],4);WF_FL(8,3);
  wf_fault_text(&l,"FACING ");wf_fault_hex(&l,p[11]>>24,2);wf_fault_text(&l,"/");wf_fault_hex(&l,p[11]>>16,2);wf_fault_text(&l," FLAGS33 ");wf_fault_hex(&l,p[11],2);WF_FL(9,3);
  wf_fault_text(&l,"SEE PAGE 3 FOR ACTOR STATES");WF_FL(11,5);return;
 }
 if(s->slot>=9u){wf_fault_text(&l,"NO PARTICIPANT MATCHED THIS FAULT");WF_FL(3,4);return;}
 const wf_fault_actor_t*a=&s->actor[s->slot],*t=a->target<9u?&s->actor[a->target]:0;
 wf_fault_text(&l,"ACTOR ");wf_fault_hex(&l,s->slot,2);wf_fault_text(&l," ");wf_fault_text(&l,wf_fault_name(a->id));WF_FL(2,4);
 wf_fault_text(&l,"ID ");wf_fault_hex(&l,a->id,4);wf_fault_text(&l," ACTIVE ");wf_fault_hex(&l,a->active,2);wf_fault_text(&l," ROLE ");wf_fault_hex(&l,a->role,2);wf_fault_text(&l," FLAGS ");wf_fault_hex(&l,a->flags,2);WF_FL(3,3);
 wf_fault_text(&l,"STATE ");wf_fault_hex(&l,a->state,4);wf_fault_text(&l," LATCH ");wf_fault_hex(&l,a->latch,2);wf_fault_text(&l," ACT ");wf_fault_hex(&l,a->action,4);wf_fault_text(&l," PH ");wf_fault_hex(&l,a->phase,2);WF_FL(4,3);
 wf_fault_text(&l,"X ");wf_fault_hex(&l,a->x,4);wf_fault_text(&l," Y ");wf_fault_hex(&l,a->y,4);wf_fault_text(&l," Z ");wf_fault_hex(&l,a->z,4);WF_FL(5,3);
 wf_fault_text(&l,"VX ");wf_fault_hex(&l,a->vx,4);wf_fault_text(&l," VY ");wf_fault_hex(&l,a->vy,4);wf_fault_text(&l," FACE ");wf_fault_hex(&l,a->facing,2);wf_fault_text(&l," MIR ");wf_fault_hex(&l,a->mirror,4);WF_FL(6,3);
 wf_fault_text(&l,"HP ");wf_fault_hex(&l,a->stamina,4);wf_fault_text(&l," ANIM ");wf_fault_hex(&l,a->anim,4);wf_fault_text(&l," TIMER ");wf_fault_hex(&l,a->timer,4);WF_FL(7,3);
 wf_fault_text(&l,"INPUT FRESH ");wf_fault_hex(&l,a->input_fresh,2);wf_fault_text(&l," HELD ");wf_fault_hex(&l,a->input_held,2);wf_fault_text(&l," DER ");wf_fault_hex(&l,a->input_derived,2);WF_FL(8,3);
 wf_fault_text(&l,"TARGET ");wf_fault_hex(&l,a->target,2);if(t){wf_fault_text(&l," ");wf_fault_text(&l,wf_fault_name(t->id));}else wf_fault_text(&l," UNKNOWN");WF_FL(9,4);
 if(t){wf_fault_text(&l,"T STATE ");wf_fault_hex(&l,t->state,4);wf_fault_text(&l," ACT ");wf_fault_hex(&l,t->action,4);wf_fault_text(&l," FACE ");wf_fault_hex(&l,t->facing,2);WF_FL(10,3);
  wf_fault_text(&l,"T X ");wf_fault_hex(&l,t->x,4);wf_fault_text(&l," Y ");wf_fault_hex(&l,t->y,4);wf_fault_text(&l," Z ");wf_fault_hex(&l,t->z,4);WF_FL(11,3);
  wf_fault_text(&l,"T FLAGS ");wf_fault_hex(&l,t->flags,2);wf_fault_text(&l," HP ");wf_fault_hex(&l,t->stamina,4);wf_fault_text(&l," MIR ");wf_fault_hex(&l,t->mirror,4);WF_FL(12,3);}
}
static WF_FAULT_COLD void wf_fault_page2(volatile uint16_t*bitmap,const wf_fault_snapshot_t*s){
 wf_fault_line_t l;wf_fault_reset(&l);wf_fault_title(bitmap,2);
 wf_fault_text(&l,"S ID AC STATE ACT  X    Y   TG FACE");WF_FL(2,4);
 for(unsigned i=0;i<9u;i++){const wf_fault_actor_t*a=&s->actor[i];
  wf_fault_hex(&l,i,1);wf_fault_text(&l," ");wf_fault_hex(&l,a->id,2);
  wf_fault_text(&l," ");wf_fault_hex(&l,a->active,2);
  wf_fault_text(&l," ");wf_fault_hex(&l,a->state,4);
  wf_fault_text(&l," ");wf_fault_hex(&l,a->action,2);wf_fault_text(&l,"   ");wf_fault_hex(&l,a->x,4);
  wf_fault_text(&l," ");wf_fault_hex(&l,a->y,4);wf_fault_text(&l,"   ");wf_fault_hex(&l,a->target,2);
  wf_fault_text(&l,"  ");wf_fault_hex(&l,a->facing,2);WF_FL(3u+i,i==s->slot?4u:3u);
 }
 wf_fault_text(&l,"HIGHLIGHT BLOCKED SLOT  FF NO TARGET");WF_FL(12,5);
}
static WF_FAULT_COLD void wf_fault_page3(volatile uint16_t*bitmap,const wf_fault_snapshot_t*s){
 wf_fault_line_t l;wf_fault_reset(&l);wf_fault_title(bitmap,3);
 wf_fault_text(&l,"SYSTEM AND INPUT SNAPSHOT");WF_FL(2,4);
 wf_fault_text(&l,"FRAME ");wf_fault_hex(&l,s->frame,8);wf_fault_text(&l," VBL ");wf_fault_hex(&l,s->vblank,8);WF_FL(3,3);
 wf_fault_text(&l,"RNG ");wf_fault_hex(&l,s->rng,8);wf_fault_text(&l," BOOT ");wf_fault_hex(&l,s->seed,8);WF_FL(4,3);
 wf_fault_text(&l,"PAD TYPE ");wf_fault_hex(&l,s->pad_type,2);wf_fault_text(&l," SIZE ");wf_fault_hex(&l,s->pad_size,2);wf_fault_text(&l," CONNECT ");wf_fault_hex(&l,s->pad_connected,2);WF_FL(5,3);
 wf_fault_text(&l,"PRESSED ");wf_fault_hex(&l,s->pad_pressed,4);wf_fault_text(&l," HELD ");wf_fault_hex(&l,s->pad_held,4);WF_FL(6,3);
 wf_fault_text(&l,"INPUT LOW ");wf_fault_hex(&l,s->input_low,2);wf_fault_text(&l," CABINET ");wf_fault_hex(&l,s->cabinet,4);WF_FL(7,3);
 wf_fault_text(&l,"CPU STATE ");wf_fault_hex(&l,s->cpu_state,2);wf_fault_text(&l," HANDLER ");wf_fault_hex(&l,s->cpu_handler,6);WF_FL(8,3);
 wf_fault_text(&l,"CPU DEP ");wf_fault_hex(&l,s->cpu_dep,6);wf_fault_text(&l," VIDEO ");wf_fault_hex(&l,s->video_failed,6);WF_FL(9,3);
 wf_fault_text(&l,"OWNER ");wf_fault_hex(&l,s->owner,6);wf_fault_text(&l," DEP ");wf_fault_hex(&l,s->dep,6);WF_FL(10,3);
 wf_fault_text(&l,"ACTION TRACE ");wf_fault_hex(&l,s->trace_action,2);wf_fault_text(&l," DEP ");wf_fault_hex(&l,s->action_dep,6);WF_FL(11,3);
 wf_fault_text(&l,"CD WINDOWS ");wf_fault_hex(&l,s->cd_windows,6);wf_fault_text(&l," SECTORS ");wf_fault_hex(&l,s->cd_sectors,6);WF_FL(12,3);
  wf_fault_text(&l,"COPY ALL FIVE PAGES BEFORE RESET");WF_FL(13,5);
}
static WF_FAULT_COLD void wf_fault_page4(volatile uint16_t*bitmap,const wf_fault_snapshot_t*s){
 wf_fault_line_t l;wf_fault_reset(&l);wf_fault_title(bitmap,4);
 static const char*const source[6]={"ROM68K","RAW SPRITES","ATLAS PACK","SCENE PACK","MUSIC","OTHER"};
 wf_fault_text(&l,"CD CACHE REFILLS BY SOURCE");WF_FL(2,4);
 for(unsigned i=0;i<6u;i++){
  wf_fault_text(&l,source[i]);wf_fault_text(&l," ");wf_fault_hex(&l,s->cd_refills[i],6);
  wf_fault_text(&l," BULK ");wf_fault_hex(&l,s->cd_bulk_sectors[i],6);
  WF_FL(3u+i,3);
 }
 wf_fault_text(&l,"REFILLS PAUSE PLAY; BULK COUNTS SECTORS");WF_FL(10,5);
 wf_fault_text(&l,"COMPARE ROM/SPRITES WITH VIDEO FPS");WF_FL(11,5);
}
static WF_FAULT_COLD void wf_fault_draw(volatile uint16_t*bitmap,const wf_fault_snapshot_t*s,unsigned page){
 memset((void*)bitmap,1,512u*240u);
 switch(page%5u){case 0:wf_fault_page0(bitmap,s);break;
  case 1:wf_fault_page1(bitmap,s);break;
  case 2:wf_fault_page2(bitmap,s);break;
   case 3:wf_fault_page3(bitmap,s);break;
   default:wf_fault_page4(bitmap,s);break;}
}
#undef WF_FL
#undef WF_FAULT_COLD
#endif
