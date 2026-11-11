/* SPDX-License-Identifier: GPL-3.0-only */
static char bt_name_fixture[21];
#define X0X_HOST_BLE_NAME bt_name_fixture
#define SMC_UI_MAIN unused_smc_ui_main
#include "smc_ui_test.c"
static void enter_group(int g){ui.msg_until=0;global_index();ui.list_sel=(uint8_t)g;list_enter();CHECK(ui.glo_section==g);}
int main(void){
 setup(V_HOME,PART_909);pref_set(PR(R_ENG,T_MST,0,MST_PUMP),0);audio_due_ms=now_ms;run_ms(20);button_tap(B_GLO);CHECK(ui.glo_section==-1&&ui.list_n==6);ui_frame();shot("01-global-sections");
 project_t *keep=malloc(sizeof proj);CHECK(keep);memcpy(keep,&proj,sizeof proj);turn_ref(list_rows[0],1,EN_ALGO);CHECK(!memcmp(keep,&proj,sizeof proj));
 pref_t seen[40];int count=0;const int counts[]={9,5,2,3,5,4};
 for(int g=0;g<6;g++){
  enter_group(g);CHECK(ui.list_n==counts[g]);for(int i=0;i<ui.list_n;i++){pref_t r=list_rows[i];CHECK(r.kind!=R_HDR&&r.kind!=R_SECTION);for(int j=0;j<count;j++)CHECK(memcmp(&r,&seen[j],sizeof r));seen[count++]=r;}
  ui_frame();char name[40];snprintf(name,sizeof name,"group-%d",g);shot(name);
  button_tap(B_HOME);CHECK(ui.glo_section==-1&&ui.list_sel==g);
 }
 CHECK(count==28);CHECK(!memcmp(keep,&proj,sizeof proj));
 enter_group(0);for(int i=0;i<5;i++)CHECK(list_rows[3+i].a==part_at(i));list_move(8);CHECK(ui.list_sel==8&&ui.list_top==3);ui_frame();shot("midi-scroll");button_tap(B_HOME);list_enter();CHECK(ui.list_sel==8&&ui.list_top==3);
 button_tap(B_GLO);CHECK(ui.view!=V_GLO);button_tap(B_GLO);CHECK(ui.glo_section==-1&&ui.list_sel==0);
 /* Physical > entry consumes the held repeat until release, including clip mode. */
 clip_grid_mode=clip_on=1;memset(&host_arw,0,sizeof host_arw);arrows_held=2;host_arrows();CHECK(ui.glo_section==0);int value=pref_get(list_rows[0]);
 for(int t=0;t<20;t++){now_ms+=80;host_arrows();CHECK(pref_get(list_rows[0])==value);}arrows_held=0;host_arrows();arrows_held=2;host_arrows();CHECK(pref_get(list_rows[0])==value+1);arrows_held=0;host_arrows();
 enter_group(5);CHECK(ui.list_n==3);clip_grid_mode=clip_on=0;
 enter_group(4);ui.list_sel=4;list_enter();CHECK(ui.overlay==O_ASK&&ui.ask_act==ACT_RESET);ui_frame();shot("reset-confirmation");button_tap(B_HOME);CHECK(ui.overlay==O_LIST&&ui.glo_section==4&&ui.list_sel==4);
 /* Connected-name bounds and marquee are covered by ui_peer_names_test.c. */
 enter_group(2);ui.list_sel=1;pref_set(PR(R_USBFULL,0,0,0),0);ui_frame();shot("audio-long-enum");
 free(keep);printf("settings sections PASS %u checks\n",checks);return 0;
}
