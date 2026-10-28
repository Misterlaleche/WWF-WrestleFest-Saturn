#include "wf_r18_reset_combo.h"
#include <assert.h>
#include <stdio.h>

int main(void)
{
    wf_r18_reset_combo_t combo = {0};
    assert(wf_r18_reset_combo_poll(&combo, 0, 0) == WF_R18_RESET_NONE);
    assert(wf_r18_reset_combo_poll(&combo, 1, 0) == WF_R18_RESET_TITLE);
    assert(wf_r18_reset_combo_poll(&combo, 1, 1) == WF_R18_RESET_NONE);
    assert(wf_r18_reset_combo_poll(&combo, 0, 1) == WF_R18_RESET_NONE);
    assert(wf_r18_reset_combo_poll(&combo, 1, 1) == WF_R18_RESET_BIOS);
    assert(wf_r18_reset_combo_poll(&combo, 1, 1) == WF_R18_RESET_NONE);
    assert(wf_r18_reset_combo_poll(&combo, 0, 1) == WF_R18_RESET_NONE);
    assert(wf_r18_reset_combo_poll(&combo, 1, 0) == WF_R18_RESET_TITLE);
    puts("PASS reset combo edges");
    return 0;
}
