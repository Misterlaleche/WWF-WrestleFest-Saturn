#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "wp01_pairs.h"
int main(void){
    unsigned seen[12][12]={{0}};
    for(unsigned t=0;t<29;t++){
        unsigned ids[2]={99,99};wp01_pair_from_ticket(t,ids);
        unsigned a=ids[0],b=ids[1];assert(a!=b&&a<12&&b<12);
        assert(a!=4&&a!=5&&b!=4&&b!=5);
        assert((a>=10||b>=10)?(a==10&&b==11):1);
        if(a>b){unsigned c=a;a=b;b=c;}
        assert(!seen[a][b]);seen[a][b]=1;
    }
    assert(seen[10][11]);puts("PASS:29 unique original-valid TAG teams; Demolition always paired");
}
