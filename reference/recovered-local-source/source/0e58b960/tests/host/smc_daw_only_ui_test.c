/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual menu/preference implementation; does not stand in for parser retirement proof. */
#define SMC_UI_MAIN smc_ui_existing_main
#include "smc_ui_test.c"
int main(void)
{
    boot();ui.msg_until=0;held_keys=held_btn=0;
    pref_t old=PR(R_SMC_MAP,0,0,0);
    CHECK(pref_desc(old)->max==0);CHECK(pref_read_only(old));
    for(unsigned raw=0;raw<64;raw++) {
        proj.set.bt=(uint8_t)raw;ui.dirty=0;save_wait=0;
        CHECK(pref_get(old)==0);pref_set(old,1);
        CHECK(proj.set.bt==raw && !ui.dirty && !save_wait);
        pref_set(old,0);CHECK(proj.set.bt==raw && !ui.dirty);
    }
    for(unsigned role=0;role<2;role++) {
        proj.set.bt=(uint8_t)(1u|PROJ_BT_SMC_USER|(role?PROJ_BT_SMC_CENTRAL:0));
        ui.view=V_GLO;ui.overlay=O_NONE;global_section(1);ui.msg_until=0;
        CHECK(ui.list_n==4);
        const unsigned kinds[]={R_BT,R_SMC_ROLE,R_BTSTAT,R_BTNAME};
        for(unsigned i=0;i<4;i++)CHECK(list_rows[i].kind==kinds[i]);
        ui_frame();shot(role?"bluetooth-host-daw-only":"bluetooth-device-daw-only");
        CHECK(ui.list_n==4);CHECK(pref_get(PR(R_SMC_ROLE,0,0,0))==(int)role);
    }
    printf("PASS %u DAW-only menu/stale-preference checks; actual candidate renders\n",checks);
    return 0;
}
