/* SPDX-License-Identifier: GPL-3.0-only */
/* Current Chicago/Miami renderer and engine: read-only STATUS is a real cell. */
#define CHICAGO_PRESENTATION_NO_MAIN
#include "chicago_presentation_test.c"
static const char *output_dir;
static void check_readout(const char *status,int selected,const char *name)
{
    int snd=row(R_BENGINE,0),top=24+(snd%4)*50,v=bidx();
    *page_of(V_PART)=selected?snd:snd-1;
    sound_t before=proj.sound;int page=cur_page(),sw=save_wait;
    uint32_t dirty=ui.dirty,serial=engine_bass_choice_serial(),rev=song_structure_revision;
    pref_t refs[4];memcpy(refs,pg.r[snd],sizeof refs);
    shot(output_dir,name);
    CHECK(!memcmp(&before,&proj.sound,sizeof before));
    CHECK(!memcmp(refs,pg.r[snd],sizeof refs));
    CHECK(cur_page()==page && ui.dirty==dirty && save_wait==sw);
    CHECK(engine_bass_choice_serial()==serial && song_structure_revision==rev);
    CHECK(refs[0].kind==R_BENGINE && refs[1].kind==R_BPRESET);
    CHECK(refs[2].kind==R_BSTATUS && refs[2].b==v && refs[3].kind==R_NONE);
    CHECK(pref_read_only(refs[2]));
    char num[16],unit[8];pref_value_of(refs[2],pref_get(refs[2]),num,unit);
    CHECK(!strcmp(num,status) && !unit[0]);
    /* Independent cell-interior oracle: raised label and native text. The
     * STATUS cell has no legacy shared-half icon, reason line or K8 divider. */
    uint16_t bg=selected?SOUND_SELECT:SOUND_FACE;
    uint16_t ink=selected?SOUND_INK:SOUND_WHITE,label=selected?SOUND_INK:SOUND_GRAY;
    cv_begin(240,49,bg);
    int x=151-tw(&FONT_T,"STATUS")/2;
    cv_text(x,12,&FONT_T,"STATUS",UI_N_LEFT);
    cv_text(x,10,&FONT_T,"STATUS",UI_N_SHADOW);
    cv_text(x,11,&FONT_T,"STATUS",label);
    const felucca_font_t *font=!strcmp(status,"FX WAIT")?&FONT_XS:&FONT_B;
    CHECK(tw(font,status)<=54);cv_text(151-tw(font,status)/2,26,font,status,ink);
    for(int y=10;y<44;y++)for(int xx=125;xx<177;xx++)
        CHECK(fb[(top+y)*240+xx]==cv_px[y*240+xx]);
    if(selected) {
        int pending=engine_bass_preset_pending(v),nav=tw(&FONT_T,UI_CAP_ARROWS)+18+tw(&FONT_T,"ROW");
        cv_begin(240,12,SOUND_FACE);
        if(pending) {
            cv_text(7,0,&FONT_T,UI_CAP_HOME ": CANCEL",SOUND_GRAY);
            expected_row_hint(234-nav,0,SOUND_AMBER);
        } else {
            int end=expected_row_hint(7,0,SOUND_GRAY);
            cv_text(end+8,0,&FONT_T,UI_CAP_EDIT ": NEXT",SOUND_GRAY);
            text_r(234,0,&FONT_T,UI_CAP_HOME ": BACK",SOUND_AMBER);
        }
        for(int y=0;y<11;y++)for(int xx=7;xx<234;xx++)
            CHECK(fb[(228+y)*240+xx]==cv_px[y*240+xx]);
    }
}
static void inert_status_knobs(int v)
{
    *page_of(V_PART)=row(R_BENGINE,0);
    for(int k=2;k<4;k++)for(int d=-1;d<=1;d+=2) {
        sound_t before=proj.sound;uint32_t serial=engine_bass_choice_serial(),dirty=ui.dirty;
        int pending=engine_bass_preset_pending(v),sw=save_wait,page=cur_page();
        enc_acc[EN_K1+k]=d;now_ms+=1000;ui_input_only();
        CHECK(!memcmp(&before,&proj.sound,sizeof before));
        CHECK(engine_bass_choice_serial()==serial && engine_bass_preset_pending(v)==pending);
        CHECK(ui.dirty==dirty && save_wait==sw && cur_page()==page);
    }
}
static void cases(int v,int bank)
{
    static const char *const names[]={"READY","WAIT","HELD","WHEEL","LOCKS","INPUT","WAIT"};
    fixture(v,bank);for(int i=0;i<3;i++)audio();check_readout("READY",1,"ready");check_readout("READY",0,"unselected-ready");
    int p=bank?1:ORIGINAL_CHICAGO_CUTOFF;
    pref_t ref=PR(R_ENG,T_303,v,p);int value=pref_get(ref);
    pref_set(ref,value==127?126:value+1);audio();
    CHECK(engine_bass_preset_edited(v));check_readout("READY",1,"edited-ready");
    inert_status_knobs(v);
    /* An unsupported retained tag is quarantined; rendering cannot adopt or
     * repair it silently. It does not invent a legacy standalone USER cell. */
    bass_sound_tag_t known=proj.sound.bass[v];memset(proj.sound.bass[v].bytes,0,sizeof known);
    CHECK(engine_bass_sound_validate(&proj.sound)==-9);build_pages();
    check_readout("READY",1,"invalid-tag-runtime-ready");inert_status_knobs(v);
    proj.sound.bass[v]=known;CHECK(!engine_bass_sound_validate(&proj.sound));build_pages();
    for(int cause=1;cause<=6;cause++) {
        fixture(v,bank);for(int i=0;i<3;i++)audio();
        if(cause==2) {engine_bass_on(v,48,100,0);audio();}
        if(cause==3)engine_bass_wheel_owners(1u<<v);
        if(cause==5)engine_bass_on(v,48,100,0); /* queued input precedes publication */
        request(v,bank^1);
        if(cause==4) {proj.arr.lane[0].used=1;proj.arr.lane[0].t=T_303;proj.arr.lane[0].v=v;}
        if(cause==6)unstable=1; /* Existing fixture injects a torn query into the real UI. */
        CHECK((cause==6?cancel_test_pending(v):engine_bass_preset_pending(v))==cause);
        check_readout(names[cause],1,"pending");check_readout(names[cause],0,"unselected-pending");
        if(cause!=6)inert_status_knobs(v);
        if(cause==2) {
            engine_bass_off(v);audio();CHECK(engine_bass_preset_pending(v)==2);
            check_readout("HELD",1,"release-tail");
        }
        unstable=0;*page_of(V_PART)=row(R_BENGINE,0);patch();
        CHECK(!engine_bass_preset_pending(v) && ui.view==V_PART);
        if(cause==2) {engine_bass_off(v);audio();}
    }
}
int main(int argc,char **argv)
{
    CHECK(argc==2);output_dir=argv[1];boot();run_ms(50);engine_guard_enable(0);
    for(int v=0;v<2;v++)for(int bank=0;bank<2;bank++)cases(v,bank);
    printf("PASS %u current STATUS checks: native pixels, real wait reasons, actions and read-only knob/state invariants\n",checks);
    return 0;
}
