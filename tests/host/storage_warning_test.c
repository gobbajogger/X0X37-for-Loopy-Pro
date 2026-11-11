/* SPDX-License-Identifier: GPL-3.0-only */
#define SMC_UI_MAIN storage_warning_unused_ui_main
#include "smc_ui_test.c"
int main(void)
{
    setup(V_HOME,PART_303A);
    proj.set.bt|=PROJ_BT_CLK_IN;
    CHECK(project_save()==0);
    store[OBJ_PAT0][8]^=1u; /* invalid raw CRC; no previous complete project */
    ui.save_failed=ui.dirty=save_wait=0;
    CHECK(ui_project_load()<0);
    CHECK(project_storage_unsupported()==2);
    CHECK(save_state()==SAVE_FAILED); /* runtime LOAD also exposes the glyph */
    save_wait=1;CHECK(save_state()==SAVE_FAILED); /* queued SAVE cannot hide it */
    unsigned writes=store_writes;
    seq.req_start=1;button_tap(B_SAVE); /* real SAVE dispatch, engine busy */
    CHECK(!save_wait);CHECK(save_state()==SAVE_FAILED);seq.req_start=0;
    ui.msg[0]=0;ui.msg_until=0;
    mark_dirty();now_ms+=40000;autosave();
    CHECK(store_writes==writes);CHECK(ui.dirty);
    CHECK(!ui.msg[0] && !ui.msg_until); /* no recurring long warning */
    project_defaults();CHECK(project_save()==-9);CHECK(store_writes==writes);
    CHECK(ui_project_reset()==0);
    CHECK(!project_storage_unsupported());CHECK(save_state()!=SAVE_FAILED);
    printf("storage warning: %u checks; failed LOAD, queued SAVE and explicit reset\n",checks);
    return 0;
}
