#include <assert.h>
#include <string.h>
#include "../../src/runtime/wf_records.h"
#include "../../src/runtime/wf_records_save.h"

static int same_entry(const wf_record_entry_t *a,const wf_record_entry_t *b){return memcmp(a,b,sizeof(*a))==0;}

int main(void)
{
    wf_record_tables_t t, d;
    uint8_t rank=0xff;
    uint8_t save[WF_RECORD_SAVE_BYTES];
    uint16_t flags=0xffff;
    wf_records_reset_defaults(&t);
    assert(sizeof(wf_record_entry_t)==6u);
    assert(t.tag[0].value==8u && memcmp(t.tag[0].initials,"TAK",3)==0 && t.tag[0].wrestler_1==0u && t.tag[0].wrestler_2_or_shared==1u);
    assert(t.royal[0].value==9u);
    assert(wf_records_table_valid(t.tag));
    assert(wf_records_tag_display_value(8u)==9u);
    assert(wf_records_royal_display_value(9u)==9u);

    { wf_record_tables_t before=t; assert(!wf_records_qualify(t.tag,WF_RECORD_MODE_TAG,0u,0u,0u,1u,&rank)); assert(memcmp(&before,&t,sizeof(t))==0); }
    wf_records_reset_defaults(&t);
    assert(wf_records_qualify(t.tag,WF_RECORD_MODE_TAG,9u,0u,8u,9u,&rank));
    assert(rank==0u && t.tag[0].value==9u);
    assert(t.tag[0].initials[0]==WF_RECORD_INITIAL_BLANK && t.tag[0].initials[1]==WF_RECORD_INITIAL_BLANK && t.tag[0].initials[2]==WF_RECORD_INITIAL_BLANK);

    wf_records_reset_defaults(&t);
    assert(wf_records_qualify(t.tag,WF_RECORD_MODE_TAG,4u,0u,8u,9u,&rank));
    assert(rank==4u);

    wf_records_reset_defaults(&t);
    assert(!wf_records_qualify(t.royal,WF_RECORD_MODE_ROYAL,101u,0u,0u,0u,&rank));
    assert(wf_records_qualify(t.royal,WF_RECORD_MODE_ROYAL,100u,0u,0u,0u,&rank));
    assert(rank==0u);

    assert(wf_records_initial_step_right(0x4Au)==0x31u);
    assert(wf_records_initial_step_left(0x31u)==0x4Bu);
    assert(wf_records_initial_step_left(0x4Bu)==0x4Au);

    wf_records_reset_defaults(&t);
    assert(wf_records_qualify(t.tag,WF_RECORD_MODE_TAG,9u,0u,8u,9u,&rank));
    assert(wf_records_save_encode(save,&t,0u)==WF_RECORD_SAVE_OK);
    assert(wf_records_save_decode(&d,&flags,save,sizeof(save))==WF_RECORD_SAVE_OK);
    assert(flags==0u);
    for(unsigned i=0;i<WF_RECORD_ENTRY_COUNT;i++){assert(same_entry(&t.tag[i],&d.tag[i]));assert(same_entry(&t.royal[i],&d.royal[i]));}
    save[20]^=1u;
    assert(wf_records_save_decode(&d,&flags,save,sizeof(save))==WF_RECORD_SAVE_BAD_CRC);

    wf_records_reset_defaults(&t);
    assert(t.tag[0].value==8u && memcmp(t.tag[0].initials,"TAK",3)==0);
    return 0;
}
