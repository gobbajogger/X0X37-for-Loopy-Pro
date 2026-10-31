/* SPDX-License-Identifier: GPL-3.0-only */
static void peer_fixture_copy(void *out);
static int peer_fixture_state(void);
#define X0X_HOST_BLE_PEERS(out) peer_fixture_copy(out)
#define X0X_HOST_BLE_STATE peer_fixture_state()
#define SMC_UI_MAIN unused_smc_ui_main
#include "smc_ui_test.c"
static plat_ble_peers_t fixture;
static void peer_fixture_copy(void *out){memcpy(out,&fixture,sizeof fixture);}
static int peer_fixture_state(void){return fixture.state;}
static void peers(unsigned count,const char*a,const char*b){memset(&fixture,0,sizeof fixture);fixture.count=count;fixture.state=count?PLAT_BLE_CONNECTED:PLAT_BLE_READY;fixture.central=1;snprintf(fixture.name[0],21,"%s",a);snprintf(fixture.name[1],21,"%s",b);}
static void peer_frame(const char *name){ui.msg_until=0;ui_frame();shot(name);}
static unsigned name_row;
int main(void){
 setup(V_HOME,PART_909);any_button=1;pref_set(PR(R_BT,0,0,0),1);pref_set(PR(R_ENG,T_MST,0,MST_PUMP),0);audio_due_ms=now_ms;run_ms(20);open_global();ui.overlay=O_LIST;ui.help=0;for(unsigned i=0;i<ui.list_n;i++)if(list_rows[i].kind==R_BTNAME)name_row=i;CHECK(name_row);ui.list_top=name_row>=3?name_row-3:0;ui.list_sel=name_row-1;
 peers(0,"","");peer_frame("00-none");char joined[PEER_TEXT_CAP];peer_join(joined,&fixture);CHECK(!strcmp(joined,"-"));
 peers(1,"SMC-Mixer","");peer_frame("01-one");peer_join(joined,&fixture);CHECK(!strcmp(joined,"SMC-Mixer"));
 peers(2,"SMC-Mixer","SMC-Mixer");peer_frame("02-duplicates");peer_join(joined,&fixture);CHECK(!strcmp(joined,"SMC-Mixer, SMC-Mixer"));
 peers(2,"SMC-Mixer Left","SMC-Mixer Right");peer_frame("03-start");uint32_t start=peer_scroll.start;peer_join(joined,&fixture);int travel=tw(&FONT_S,joined)-(226-(8+tw(&FONT_S,"NAME")+8));CHECK(travel>0);CHECK(peer_offset(joined,&fixture,travel,start)==0);CHECK(peer_offset(joined,&fixture,travel,start+699)==0);
 now_ms=start+700+(uint32_t)travel*1000/48;peer_frame("04-middle");CHECK(peer_offset(joined,&fixture,travel,now_ms)>0);CHECK(peer_offset(joined,&fixture,travel,now_ms)<travel);
 now_ms=start+700+((uint32_t)travel*1000+23)/24;peer_frame("05-end");CHECK(peer_offset(joined,&fixture,travel,now_ms)==travel);
 /* NAME is selectable, only edits read-only. */
 ui.list_sel=name_row;peer_scroll.active=0;peer_frame("06-selected-start");now_ms+=10000;peer_frame("07-selected-motion");
 peers(1,"SMC-Mixer Left","");peer_frame("08-disconnect-one");CHECK(peer_scroll.start==now_ms);CHECK(!strcmp(peer_scroll.text,"SMC-Mixer Left"));
 peers(0,"","");fixture.central=0;pref_set(PR(R_SMC_ROLE,0,0,0),0);peer_frame("09-device");CHECK(!strcmp(peer_scroll.text,"-"));fixture.state=PLAT_BLE_OFF;pref_set(PR(R_BT,0,0,0),0);peer_frame("10-off");CHECK(peer_scroll.state==PLAT_BLE_OFF);
 /* Reachable protocol20-byte cap, both slots, bounded sanitation and duplicate retention. */
 for(unsigned n=0;n<=20;n++) {peers(2,"","");memset(fixture.name[0],'W',n);memset(fixture.name[1],'X',n);struct{unsigned a;char v[PEER_TEXT_CAP];unsigned b;} q={0x12345678,{0},0x87654321};peer_join(q.v,&fixture);CHECK(q.a==0x12345678&&q.b==0x87654321);CHECK(strlen(q.v)==2*n+2);}
 memset(fixture.name,'W',sizeof fixture.name);fixture.count=2;peer_join(joined,&fixture);CHECK(strlen(joined)==42);
 CHECK(peer_offset(joined,&fixture,20,0xfffffff0u)==0);CHECK(peer_offset(joined,&fixture,20,0x000002acu)==0); /*700ms wrap-safe*/
 peers(1,"SMC-Mixer","");peer_frame("11-restored");CHECK(!strcmp(peer_scroll.text,"SMC-Mixer"));
 /* Leaving the list stops the scroll clock. */
 ui.overlay=0;run_ms(20);CHECK(!peer_scroll.active);
 printf("peer names PASS %u checks\n",checks);return 0;
}
