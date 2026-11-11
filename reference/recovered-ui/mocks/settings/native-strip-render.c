#define SMC_UI_MAIN unused_smc_ui_main
#include "/workspace/scratch/f88caed124c4/ui-glyph-semantic-20261027/tests/host/smc_ui_test.c"
int main(void) {
 setup(V_GLO,PART_909);audio_due_ms=now_ms;run_ms(20);build_pages();
 seq.bpm=125.0f;pref_set(pg.r[cur_page()][1],0);
 pref_set(pg.r[cur_page()][2],64);pref_set(pg.r[cur_page()][3],0);
 run_ms(20);open_global();ui_frame();shot("actual-global-approved-glyphs");
 for(int i=0;i<4;i++){char v[32],u[16];pref_value(pg.r[cur_page()][i],v,u);printf("slot%d %s=%s%s\n",i,pref_name(pg.r[cur_page()][i]),v,u);}
 return 0;
}
