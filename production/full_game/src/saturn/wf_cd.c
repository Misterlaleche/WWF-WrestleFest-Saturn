#include <yaul.h>
#include "wf_cd.h"
#include "wf_r18_cdfs.h"
volatile uint32_t wf_cd_stage,wf_cd_error,wf_cd_windows,wf_cd_sectors;
void wf_cd_snapshot(void){uint32_t windows,sectors;wf_r18_cdfs_stats(&windows,&sectors);wf_cd_windows=windows;wf_cd_sectors=sectors;}
bool wf_cd_init(void){
 wf_cd_stage=1;
 int rc=cd_block_init();if(rc){wf_cd_error=(uint32_t)rc;return false;}
 wf_cd_stage=2;
 if(wf_r18_cdfs_init()){wf_cd_error=0x100;return false;}
 wf_cd_stage=3;wf_cd_snapshot();return true;
}
static int size_of(void*u __unused,const char*name,uint32_t*size){return wf_r18_file_size(name,size);}
static int read_data(void*u __unused,const char*name,uint32_t offset,void*data,size_t bytes){
 int rc=wf_r18_file_read(name,offset,data,bytes);wf_cd_snapshot();if(rc)wf_cd_error=0x101;return rc;
}
const wf_asset_io_t wf_cd_asset_io={size_of,read_data,NULL};
