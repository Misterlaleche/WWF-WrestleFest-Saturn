#include "wf_roster_identity.h"
#include <stddef.h>
bool wf_roster_raw_identity(uint16_t slot,uint8_t *raw){
 static const uint8_t identities[10]={0,7,2,8,6,9,1,3,10,11};
 if(!raw||slot>=10)return false;
 *raw=identities[slot];return true;
}
