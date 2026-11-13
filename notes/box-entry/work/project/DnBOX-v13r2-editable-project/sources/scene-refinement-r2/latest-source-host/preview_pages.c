/* Private bounded production-frame export; immutable source, no device I/O. */
#define SMC_UI_MAIN unused_full_smc_regression
#include "smc_ui_test.c"
static void capture(const char *name)
{
 ui.msg_until=0;ui.touched=-1;ui.overlay=O_NONE;ui.help=0;ui.dirty=ui.save_failed=save_wait=0;
 memset(blit_hash,0,sizeof blit_hash);now_ms+=120;ui_frame();shot(name);
 printf("FRAME %s BPM=%g playing=%u part=%u view=%u page=%d title=%s pattern=%u\n",name,(double)seq.bpm,seq.playing,ui.part,ui.view,cur_page(),pg.title[cur_page()],seq.ppat[ui.part]);
}
int main(void)
{
 setup(V_HOME,PART_303B);
 capture("HOME-current-state");
 set_view(V_MIX);build_pages();int wave=-1;for(int i=0;i<pg.n;i++)if(!strcmp(pg.title[i],"WAVE"))wave=i;CHECK(wave>=0);*page_of(ui.view)=(uint8_t)wave;
 FILE *f=fopen(getenv("SCOPE_PCM"),"rb");CHECK(f);CHECK(fread(eng_scope,sizeof(int16_t),SCOPE_N,f)==SCOPE_N);fclose(f);eng_scope_w=0;
 capture("MIX-WAVE-current-state");
 ui.part=PART_BRK;set_view(V_PART);build_pages();capture("BREAKS-current-state");
 ui.part=PART_303B;set_view(V_FX);build_pages();capture("FX-current-state");
 printf("FOCUSED_EXPORT %u assertions; no production mutation/full suite/native build\n",checks);return 0;
}
