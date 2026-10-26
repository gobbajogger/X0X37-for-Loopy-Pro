/* SPDX-License-Identifier: GPL-3.0-only */
#define main simulator_main
#include "../../host/x0x_host.c"
#undef main
#include "../../firmware/src/app/factory_banks.c"
#include "../../firmware/src/app/bass_presets.h"
static unsigned checks;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"FAIL line%d: %s\n",__LINE__,#x);exit(1);}}while(0)
static void picture(const char *dir,const char *name,int voice)
{
    char path[512]; ui.part=PART_303A+voice;ui.view=V_PART;ui.overlay=O_NONE;ui.held_step=-1;
    build_pages();*page_of(V_PART)=0;ui.msg_until=0;ui.touched=-1;
    memset(blit_hash,0,sizeof blit_hash);draw_header();draw_main();draw_knobs();
    snprintf(path,sizeof path,"%s/%s-%d.png",dir,name,X0X_DEV_WOBBLE);png_write(path);
}
int main(int argc,char **argv)
{
    uint8_t expected[32], old[32];unsigned writes;int32_t audio[512];
    CHECK(X0X_BASS_PRESET_PREVIEW==1);boot();run_ms(50);
    CHECK(engine_bass_select_preset(0,0,0,&proj.sound)==0);
    CHECK(engine_bass_bank(0)==0);CHECK(engine_nparams(T_303,0)==32);
    CHECK(bass_preset_decode(0,0,engine_bass_bank_param,expected)==0);
    CHECK(!memcmp(expected,proj.sound.v[T_303][0],32));CHECK(!engine_bass_preset_edited(0));
    CHECK(engine_bass_preset_index(0)==0);CHECK(engine_bass_preview_changed());
    if(argc>1)picture(argv[1],"chicago-acid-lab-1",0);
    writes=store_writes;CHECK(project_save()==-9);CHECK(store_writes==writes);
    memcpy(old,proj.sound.v[T_303][0],32);engine_bass_wheel_owners(1);
    CHECK(engine_bass_select_preset(0,0,1,&proj.sound)==-3);CHECK(!memcmp(old,proj.sound.v[T_303][0],32));engine_bass_wheel_owners(0);
    engine_bass_on(0,48,100,0);CHECK(engine_bass_select_preset(0,0,1,&proj.sound)==-3);
    { int audible=0; for(int k=0;k<10;k++){engine_render(audio,256);for(int j=0;j<512;j++)if(audio[j])audible=1;} CHECK(audible); }
    engine_bass_off(0);run_ms(1500);
    CHECK(engine_bass_select_preset(0,0,1,&proj.sound)==0);CHECK(engine_bass_preset_index(0)==1);
    run_ms(20);
#if X0X_DEV_WOBBLE
    ui.part=PART_303B;ui.view=V_PART;build_pages();*page_of(V_PART)=0;
    pref_set(PR(R_BENGINE,0,1,0),1);
    CHECK(engine_bass_preset_index(1)==0);
    CHECK(engine_bass_bank(1)==1);CHECK(engine_nparams(T_303,1)==23);
    CHECK(bass_preset_decode(1,0,engine_bass_bank_param,expected)==0);
    CHECK(!memcmp(expected,proj.sound.v[T_303][1],32));CHECK(!engine_bass_preset_edited(1));
    if(argc>1)picture(argv[1],"miami-16th-cutz",1);
    engine_bass_on(1,48,100,0);
    { int audible=0;for(int k=0;k<10;k++){engine_render(audio,256);for(int j=0;j<512;j++)if(audio[j])audible=1;}CHECK(audible); }
    engine_bass_off(1);run_ms(2000);
    CHECK(!engine_bass_bank_available(0,1));CHECK(engine_bass_select_preset(0,1,0,&proj.sound)==-2);
    if(argc>1)picture(argv[1],"miami-occupied",0);
    proj.arr.lane[0].used=1;proj.arr.lane[0].t=T_303;proj.arr.lane[0].v=1;
    CHECK(engine_bass_select_preset(1,0,0,&proj.sound)==-4);CHECK(engine_bass_bank(1)==1);
    proj.arr.lane[0].used=0;
    pref_set(PR(R_ENG,T_303,1,W_CARRIER_LEVEL),50);run_ms(20);CHECK(engine_bass_preset_edited(1));
    if(argc>1)picture(argv[1],"miami-edited",1);
#else
    CHECK(!engine_bass_bank_available(0,1));CHECK(engine_bass_select_preset(0,1,0,&proj.sound)==-2);
#endif
    printf("PASS %u live backend/preset/UI/save checks WBL%d\n",checks,X0X_DEV_WOBBLE);return 0;
}
