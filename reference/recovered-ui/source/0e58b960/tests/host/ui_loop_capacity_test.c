/* SPDX-License-Identifier: GPL-3.0-only */
#define SMC_UI_MAIN unused_smc_ui_main
#include "smc_ui_test.c"
#ifndef EXPECT_LOOPS
#define EXPECT_LOOPS 40
#endif
#ifndef EXPECT_USERS
#define EXPECT_USERS 32
#endif
int main(void){setup(V_PART,PART_BRK);any_button=1;static uint8_t silence[64];for(int k=0;k<PLAT_NSLOTS;k++){slot_nz[k]=(k==0?EXPECT_USERS-16:16);for(int z=0;z<16;z++){slot_data[k][z]=silence;slot_n[k][z]=128;slot_rate[k][z]=44100;}}engine_brk_loops();CHECK(engine_brk_nslots()==EXPECT_LOOPS);*page_of(V_PART)=0;build_pages();const char*const*names=engine_brk_slot_names();CHECK(!strcmp(names[0],"CRAMEN"));CHECK(!strcmp(names[1],"TASTY"));CHECK(!strcmp(names[2],"HOTPANT"));CHECK(!strcmp(names[EXPECT_LOOPS-EXPECT_USERS],"BR1"));char last_name[8];snprintf(last_name,sizeof last_name,"BR%d",EXPECT_USERS);CHECK(!strcmp(names[EXPECT_LOOPS-1],last_name));int points[]={0,1,EXPECT_LOOPS/2,EXPECT_LOOPS-1,EXPECT_LOOPS-EXPECT_USERS,EXPECT_LOOPS-EXPECT_USERS+8,EXPECT_LOOPS-EXPECT_USERS+9,EXPECT_LOOPS-EXPECT_USERS+15,EXPECT_LOOPS-EXPECT_USERS+16};for(unsigned j=0;j<sizeof points/sizeof points[0];j++){int v=points[j];pref_set(PR(R_BRKSLOT,0,0,0),v);pref_set(PR(R_BRKSLOT,1,0,0),v);ui.msg_until=0;ui_frame();CHECK(pref_get(PR(R_BRKSLOT,0,0,0))==v);CHECK(pref_desc(PR(R_BRKSLOT,0,0,0))->max==EXPECT_LOOPS-1);char shotname[64];snprintf(shotname,sizeof shotname,"count%d-index%d-%s",EXPECT_LOOPS,v,names[v]);shot(shotname);}printf("actual engine selector capacity%d PASS %u checks; synthetic silent bank/slot inputs, no installed-package claim\n",EXPECT_LOOPS,checks);return 0;}
