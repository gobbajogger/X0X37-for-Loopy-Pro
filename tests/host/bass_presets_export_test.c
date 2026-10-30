/* SPDX-License-Identifier: GPL-3.0-only */
#define X0X_BASS_PRESET_PREVIEW 1
static int dirty_preview;
int engine_bass_preview_changed(void){return dirty_preview;}
#define main original_protocol_main
#include "project_protocol_test.c"
#undef main
int main(void)
{
    memset(&project_stage,0xa5,sizeof project_stage);
    dirty_preview=1;
    op(65,0,0,2);
    assert(!held && !ed_project.mode && !flash_writes && !flash_erases);
    for(unsigned i=0;i<sizeof project_stage;i++)assert(((uint8_t*)&project_stage)[i]==0xa5);
    dirty_preview=0;op(65,0,0,0);assert(held && ed_project.mode==1);
    op(70,0,0,0);assert(!held);
    puts("PASS actual editor EXPORT BEGIN refuses untagged preview before hold/capture/writes; baseline export preserved");
    return 0;
}
