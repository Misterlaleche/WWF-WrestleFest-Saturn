#ifndef WP01_PAIRS_H
#define WP01_PAIRS_H
/*Random test harness; legal teams follow original TAG selection$5A20.
 *The eight individual choices form28teams; slot8selects bothDemolition.*/
static inline void wp01_pair_from_ticket(unsigned ticket,unsigned ids[2]){
    static const unsigned roster[8]={0,7,2,8,6,9,1,3};
    ticket%=29;
    if(ticket==28){ids[0]=10;ids[1]=11;return;}
    for(unsigned a=0;a<7;a++)for(unsigned b=a+1;b<8;b++){
        if(ticket--==0){ids[0]=roster[a];ids[1]=roster[b];return;}
    }
}
#endif
