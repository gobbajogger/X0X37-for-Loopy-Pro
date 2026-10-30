/* SPDX-License-Identifier: GPL-3.0-only */
/* The real renderer and engine; no substitute status formatter or fake glyph font. */
#define CHICAGO_PRESENTATION_NO_MAIN
#define bass_publish_test_hook readout_base_publish_hook
#include "chicago_presentation_test.c"
#undef bass_publish_test_hook
static const char *output_dir;
static int capture_publication, captured_publication;
static void check_readout(const char *,const char *,const char *,int,const char *);
void bass_publish_test_hook(int v,int stage)
{
    if(capture_publication && stage==2) {
        CHECK(engine_bass_preset_pending(v)==6);captured_publication++;
        check_readout("WAIT","WAIT FOR UPDATE","PATCH: CANCEL",1,"publication-wait");
    } else readout_base_publish_hook(v,stage);
}
static void expected_glyph(int x,int y,int width,uint16_t c)
{
    if(width==9) {
        cv_line(x,y,x+8,y,c);cv_line(x,y+8,x+8,y+8,c);
        cv_line(x+1,y+1,x+7,y+7,c);cv_line(x+7,y+1,x+1,y+7,c);
    } else if(width==8) {
        cv_line(x,y+7,x+6,y+1,c);cv_line(x+1,y+8,x+7,y+2,c);
    } else if(width==10) {
        cv_line(x,y+4,x+3,y+7,c);cv_line(x+3,y+7,x+9,y+1,c);
    }
}
static void check_readout(const char *status,const char *reason,const char *left,int selected,const char *name)
{
    int snd=row(R_BENGINE,0),r=snd%4,pending=reason!=NULL;
    *page_of(V_PART)=selected?snd:snd-1;
    int night=night_fixture(),top=24+r*(night?67:50);
    int iw=!strcmp(status,"UNKNOWN")?0:pending?9:!strcmp(status,"ACTIVE")?10:8;
    int vy=night?(pending?31:36):(pending?24:29),tx=180-(tw(&FONT_T,status)+iw+(iw?2:0))/2;
    const char *right=UI_CAP_HOME ": BACK";
    sound_t before=proj.sound;int page=cur_page(),dirty=ui.dirty,sw=save_wait;
    shot(output_dir,name);
    CHECK(!memcmp(&before,&proj.sound,sizeof before));CHECK(cur_page()==page && ui.dirty==dirty && save_wait==sw);
    CHECK(pg.r[snd][0].kind==R_BENGINE && pg.r[snd][1].kind==R_BPRESET);
    CHECK(pg.r[snd][2].kind==R_NONE && pg.r[snd][3].kind==R_NONE);
    uint16_t bg=night?(selected?RGB(37,59,45):RGB(16,26,31)):(selected?matrix_col():C_BLACK);
    uint16_t ink=night?(selected?RGB(224,235,214):RGB(222,231,223)):(selected?C_BLACK:C_HI);
    uint16_t label=night?(selected?RGB(224,235,214):RGB(156,170,168)):(selected?C_BLACK:C_GRAY);
    CHECK(tw(&FONT_T,status)+iw+(iw?2:0)<=110);
    CHECK(!reason || tw(&FONT_T,reason)<=110);
    /* Full right-half pixel oracle also rejects the old divider through x181. */
    cv_begin(240,night?66:49,bg);text_c(180,night?14:11,&FONT_T,"STATUS",label);
    cv_text(tx+iw+(iw?2:0),vy,&FONT_T,status,ink);expected_glyph(tx,vy,iw,ink);
    if(reason)text_c(180,night?49:37,&FONT_T,reason,ink);
    for(int y=13;y<(night?64:47);y++)for(int x=125;x<234;x++)CHECK(fb[(top+y)*240+x]==cv_px[y*240+x]);
    /* Measure actual composite ink. Primitive columns are contiguous; spacing
     * is checked against text ink, not a guessed character count. */
    if(iw) {
        int first=999,last=-1,occupied[240]={0};
        for(int y=vy;y<vy+12;y++)for(int x=125;x<234;x++)if(fb[(top+y)*240+x]!=swap16(bg)) {
            occupied[x]=1;if(x<first)first=x;if(x>last)last=x;
        }
        CHECK(first>=125 && last<234 && abs(first+last-360)<=3);
        int end=first;while(occupied[end+1])end++;
        CHECK(end-first+1==iw);int next=end+1;while(!occupied[next] && next<234)next++;
        CHECK(next-end-1>=2 && next-end-1<=3);
    }
    if(selected) {
        int nav=tw(&FONT_T,UI_CAP_ARROWS)+18+tw(&FONT_T,"ROW");
        expected_footer_bg(night);
        uint16_t gray=night?RGB(156,170,168):C_GRAY,hint=night?RGB(196,155,69):C_HI;
        if(pending) {
            CHECK(7+tw(&FONT_T,left)+8+nav<=234);
            cv_text(7,0,&FONT_T,left,gray);expected_row_hint(234-nav,0,hint);
        } else {
            int x=expected_row_hint(7,0,gray);cv_text(x+8,0,&FONT_T,UI_CAP_EDIT ": NEXT",gray);
            CHECK(x+8+tw(&FONT_T,UI_CAP_EDIT ": NEXT")+8+tw(&FONT_T,right)<=234);
            text_r(234,0,&FONT_T,right,hint);
        }
        for(int y=0;y<12;y++)for(int x=0;x<240;x++)CHECK(fb[(228+y)*240+x]==cv_px[y*240+x]);
    }
}
static void cases(int v,int bank)
{
    char name[80];
    fixture(v,bank);snprintf(name,sizeof name,"%s-%c-active",bank?"miami":"chicago",'a'+v);
    check_readout("ACTIVE",NULL,NULL,1,name);
    snprintf(name,sizeof name,"%s-%c-unselected-active",bank?"miami":"chicago",'a'+v);
    check_readout("ACTIVE",NULL,NULL,0,name);
    *page_of(V_PART)=row(R_BENGINE,0);int p=bank?W_CUTOFF:BASS303_CUTOFF;
    pref_set(PR(R_ENG,T_303,v,p),pref_get(PR(R_ENG,T_303,v,p))+1);
    CHECK(engine_bass_preset_edited(v));check_readout("EDITED",NULL,NULL,1,"edited");
    /* Canonical known-engine USER identity, then a quarantined unknown tag. */
    proj.sound.bass[v].bytes[8]=proj.sound.bass[v].bytes[9]=0xff;
    proj.sound.bass[v].bytes[10]=BASS_SOUND_EDITED;
    CHECK(engine_bass_sound_validate(&proj.sound)==0);
    engine_bass_edit_hold(1);engine_bass_base_restore(1u<<v);engine_bass_edit_hold(0);audio();
    CHECK(engine_bass_preset_index(v)<0);check_readout("USER",NULL,NULL,1,"user");
    bass_sound_tag_t known=proj.sound.bass[v];
    memset(proj.sound.bass[v].bytes,0,sizeof proj.sound.bass[v].bytes);build_pages();
    CHECK(pg.n==1);check_readout("UNKNOWN",NULL,NULL,1,"unknown");
    /* Restore the fixture's tag; rendering never applied the unknown identity. */
    proj.sound.bass[v]=known;build_pages();
    for(int cause=1;cause<=6;cause++) {
        fixture(v,bank);request(v,bank^1);
        const char *reason="WAIT FOR AUDIO";
        if(cause==2) {
            /* Give the release-tail positive a nonzero duration; stock presets
             * may legitimately finish within the first host audio block. */
            if(!bank) {
                pref_set(PR(R_ENG,T_303,v,BASS303_AMPMODE),BASS303_AMP_ENV);
                pref_set(PR(R_ENG,T_303,v,BASS303_AATTACK),0);
                pref_set(PR(R_ENG,T_303,v,BASS303_ARELEASE),32);
            } else {
                pref_set(PR(R_ENG,T_303,v,W_ATTACK),0);
                pref_set(PR(R_ENG,T_303,v,W_RELEASE),32);
            }
            engine_bass_on(v,48,100,0);audio();reason="WAIT FOR NOTES/TAIL";
        }
        if(cause==3) {engine_bass_wheel_owners(1u<<v);reason="RETURN WHEELS";}
        if(cause==4) {proj.arr.lane[0].used=1;proj.arr.lane[0].t=T_303;proj.arr.lane[0].v=v;reason="SOUND LOCKS ACTIVE";}
        if(cause==5) {engine_bass_cancel_preset(v);engine_bass_on(v,48,100,0);request(v,bank^1);reason="WAIT FOR INPUT";}
        if(cause==6) {
            capture_publication=1;CHECK(!engine_bass_select_preset(v,bank^1,2,&proj.sound));capture_publication=0;
            continue;
        }
        CHECK(engine_bass_preset_pending(v)==cause);
        snprintf(name,sizeof name,"%s-%c-pending-%d",bank?"miami":"chicago",'a'+v,cause);
        check_readout("WAIT",reason,"PATCH: CANCEL",1,name);
        snprintf(name,sizeof name,"%s-%c-unselected-pending-%d",bank?"miami":"chicago",'a'+v,cause);
        check_readout("WAIT",reason,NULL,0,name);
        *page_of(V_PART)=row(R_BENGINE,0);
        if(cause==2) {
            engine_bass_off(v);audio();CHECK(engine_bass_preset_pending(v)==2);
            snprintf(name,sizeof name,"%s-%c-release-tail",bank?"miami":"chicago",'a'+v);
            check_readout("WAIT","WAIT FOR NOTES/TAIL","PATCH: CANCEL",1,name);
        }
        /* K7/K8 remain inert; only PATCH cancels. */
        for(int k=2;k<4;k++){enc_acc[EN_K1+k]=1;now_ms+=1000;ui_input_only();CHECK(engine_bass_preset_pending(v));}
        patch();CHECK(!engine_bass_preset_pending(v) && ui.view==V_PART);
        if(cause==2) {
            /* End this deliberately lengthened tail by waiting for an actual
             * same-bank preset boundary, not the shared fixture's stock timeout. */
            engine_bass_off(v);request(v,bank);
            for(int i=0;i<5000 && engine_bass_preset_pending(v);i++)audio();
            CHECK(!engine_bass_preset_pending(v));
        }
    }
}
int main(int argc,char **argv)
{
    CHECK(argc==2);output_dir=argv[1];boot();run_ms(50);engine_guard_enable(0);
    for(int v=0;v<2;v++)for(int bank=0;bank<2;bank++)cases(v,bank);
    CHECK(captured_publication==4);
    printf("PASS %u expanded STATUS/readout checks: actual pixels, compact ink gap, actions, all engine reasons and unchanged knob mapping\n",checks);
    return 0;
}
