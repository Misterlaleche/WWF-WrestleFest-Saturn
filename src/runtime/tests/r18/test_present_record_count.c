#include "wf_r18_present.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static wf_r18_video_t video;
static wf_runtime_t runtime;
int main(void){
 size_t capacity;wf_r18_sprite_record_t*records=wf_r18_present_records_buffer(&capacity);
 assert(capacity==512u);memset(records,0,capacity*sizeof *records);
 wf_r18_present_init(NULL,NULL,0,NULL,0);
 assert(wf_r18_present_records_count()==0u);
 /* Disabled records still belong to the original counted prefix; native
  * command count drops them and therefore cannot replace $19B2/$19B4. */
 assert(wf_r18_present_records_commit(&video,3)==WF_OK);
 assert(wf_r18_present_records_count()==3u&&wf_r18_present_commands()==0u);
 assert(wf_r18_present_records_commit(&video,513)==WF_BAD_STATE);
 assert(wf_r18_present_records_count()==3u);
 void *backup=wf_r18_present_backup_workspace_begin(24576u);
 assert(backup!=NULL);
 assert(wf_r18_present_backup_workspace_begin(1u)==NULL);
 assert(wf_r18_present_records_commit(&video,0)==WF_BAD_STATE);
 wf_r18_present_backup_workspace_end();
 assert(wf_r18_present_backup_workspace_begin(32769u)==NULL);
 assert(wf_r18_present_records_commit(&video,3)==WF_OK);
 assert(wf_r18_present_records_commit(NULL,0)==WF_BAD_STATE);
 assert(wf_r18_present_records_count()==3u);
 assert(wf_r18_present_batch(&runtime,NULL,0,&video)==WF_OK);
 assert(wf_r18_present_records_count()==0u);
 assert(wf_r18_present_records_commit(&video,7)==WF_OK);
 wf_r18_present_init(NULL,NULL,0,NULL,0);
 assert(wf_r18_present_records_count()==0u);
 puts("PASS original record count survives command clipping and invalid submissions; reset clears metadata");
 return 0;
}
