/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual renderer pixels; no replacement caption formatter. */
#define CHICAGO_PRESENTATION_NO_MAIN
#include "chicago_presentation_test.c"
static const char *out;
static const char *const physical[]={"","KNOB 5","KNOB 6","KNOBS 5-6","KNOB 7","KNOBS 5,7","KNOBS 6-7","KNOBS 5-7","KNOB 8","KNOBS 5,8","KNOBS 6,8","KNOBS 5-6,8","KNOBS 7-8","KNOBS 5,7-8","KNOBS 6-8","KNOBS 5-8"};
static const char *const logical[]={"","KNOB 1","KNOB 2","KNOBS 1-2","KNOB 3","KNOBS 1,3","KNOBS 2-3","KNOBS 1-3","KNOB 4","KNOBS 1,4","KNOBS 2,4","KNOBS 1-2,4","KNOBS 3-4","KNOBS 1,3-4","KNOBS 2-4","KNOBS 1-4"};
static void check_caption(const char *caption,const char *name)
{
    int page=cur_page(),r=page%4,night=night_fixture(),top=24+r*(night?67:50);
    char title[80];put_s(put_s(put_i(title,r+1),"  "),matrix_group(page));
    int sound=pg.r[page][0].kind==R_BENGINE,pending=engine_bass_preset_pending(bidx());
    CHECK(8+tw(&FONT_T,title)+8<=232-tw(&FONT_T,caption));
    sound_t before=proj.sound;int dirty=ui.dirty,sw=save_wait;
    pref_t refs[4];memcpy(refs,pg.r[page],sizeof refs);
    shot(out,name);
    CHECK(!memcmp(&before,&proj.sound,sizeof before));CHECK(!memcmp(refs,pg.r[page],sizeof refs));
    CHECK(cur_page()==page && ui.dirty==dirty && save_wait==sw);
    uint16_t label=night?RGB(224,235,214):C_BLACK;
    cv_begin(240,12,night?RGB(37,59,45):matrix_col());
    if(night)cv_rect(4,0,232,1,RGB(62,92,64));
    cv_text(8,0,&FONT_T,title,label);text_r(232,0,&FONT_T,caption,label);
    for(int y=0;y<11;y++)for(int x=8;x<232;x++)CHECK(fb[(top+y)*240+x]==cv_px[y*240+x]);
    int nav=tw(&FONT_T,UI_CAP_ARROWS)+18+tw(&FONT_T,"ROW");
    char range[12];put_i(put_s(put_i(range,page/4*4+1),"-"),page/4*4+4<pg.n?page/4*4+4:pg.n);
    expected_footer_bg(night);
    uint16_t gray=night?RGB(156,170,168):C_GRAY,hint=night?RGB(196,155,69):C_HI;
    if(sound && pending) {
        CHECK(7+tw(&FONT_T,UI_CAP_HOME ": CANCEL")+8+nav<=234);
        cv_text(7,0,&FONT_T,UI_CAP_HOME ": CANCEL",gray);expected_row_hint(234-nav,0,hint);
    } else {
        const char *right=sound?UI_CAP_HOME ": BACK":range;
        int x=expected_row_hint(7,0,gray);cv_text(x+8,0,&FONT_T,UI_CAP_EDIT ": NEXT",gray);
        CHECK(x+8+tw(&FONT_T,UI_CAP_EDIT ": NEXT")+8+tw(&FONT_T,right)<=234);
        text_r(234,0,&FONT_T,right,hint);
    }
    for(int y=0;y<12;y++)for(int x=0;x<240;x++)CHECK(fb[(228+y)*240+x]==cv_px[y*240+x]);
}
static void check_wrap(void)
{
    for(int v=0;v<2;v++)for(int bank=0;bank<2;bank++) {
        fixture(v,bank);int n=pg.n;CHECK(n==7);
        *page_of(V_PART)=0;
        for(int d=-1;d<=1;d+=2)for(int i=0;i<n*2;i++) {
            int previous=cur_page(),want=(previous+d+n)%n;
            sound_t before=proj.sound;int dirty=ui.dirty,sw=save_wait;
            now_ms++;enc_acc[EN_SELECT]=d;ui_input_only();
            CHECK(cur_page()==want);CHECK(cur_page()/4==want/4);
            CHECK(!memcmp(&before,&proj.sound,sizeof before));CHECK(ui.dirty==dirty && save_wait==sw);
            if(previous/4!=want/4) {
                unsigned mask=0;for(int k=0;k<4;k++)if(pg.r[want][k].kind!=R_NONE)mask|=1u<<k;
                char name[64];snprintf(name,sizeof name,"%s-%c-wrap-%d-%d",bank?"miami":"chicago",'a'+v,d,want+1);
                check_caption((X0X_SMK_CAPTIONS?physical:logical)[mask],name);
            }
        }
        /* Other views keep their previous clamp behavior. */
        set_view(V_SEQ);*page_of(V_SEQ)=0;now_ms++;enc_acc[EN_SELECT]=-1;ui_input_only();CHECK(cur_page()==0);
        *page_of(V_SEQ)=pg.n-1;now_ms++;enc_acc[EN_SELECT]=1;ui_input_only();CHECK(cur_page()==pg.n-1);
    }
}
int main(int argc,char **argv)
{
    CHECK(argc==2);out=argv[1];boot();run_ms(50);engine_guard_enable(0);
    check_wrap();if(getenv("X0X_CAPTION_WRAP_ONLY")){printf("PASS %u row wrap checks\n",checks);return 0;}
    const char *const *names=X0X_SMK_CAPTIONS?physical:logical;
    for(int v=0;v<2;v++)for(int bank=0;bank<2;bank++) {
        fixture(v,bank);
        CHECK(8+tw(&FONT_T,"3  SOUND")+8<=232-tw(&FONT_T,names[3]));
        for(int p=0;p<pg.n;p++) {
            *page_of(V_PART)=p;unsigned mask=0;char name[64];
            for(int k=0;k<4;k++)if(pg.r[p][k].kind!=R_NONE)mask|=1u<<k;
            snprintf(name,sizeof name,"%s-%c-row-%d",bank?"miami":"chicago",'a'+v,p+1);
            check_caption(names[mask],name);
        }
        /* Requested-engine name and pending actions are rendered, not inferred. */
        *page_of(V_PART)=row(R_BENGINE,0);request(v,bank^1);engine_bass_wheel_owners(1u<<v);
        check_caption(names[3],"pending-sound");engine_bass_cancel_preset(v);engine_bass_wheel_owners(0);
    }
    /* Exhaust every possible hole pattern without compacting the four slots. */
    fixture(0,0);*page_of(V_PART)=0;pref_t refs[4];memcpy(refs,pg.r[0],sizeof refs);
    for(int mask=0;mask<16;mask++) {
        for(int k=0;k<4;k++)pg.r[0][k]=(mask&(1<<k))?refs[k]:NONE;
        char name[32];snprintf(name,sizeof name,"mask-%02d",mask);check_caption(names[mask],name);
    }
    puts("Caption and footer tests use exact production framebuffer pixels.");
    printf("PASS %u mapped-knob caption checks; SMK captions=%d\n",checks,X0X_SMK_CAPTIONS);return 0;
}
