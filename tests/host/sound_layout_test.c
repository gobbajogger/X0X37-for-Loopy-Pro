/* SPDX-License-Identifier: GPL-3.0-only */
/* Accepted SOUND layout: real production input/render/history, not a mock UI. */
#define CHICAGO_PRESENTATION_NO_MAIN
#include "chicago_presentation_test.c"
static const char *out;
static void flatten(char *to,const char *from)
{
    while(*from) {char c=*from++;if(c!=' ')*to++=c>='a' && c<='z'?(char)(c-32):c;}
    *to=0;
}
static void name_bounds(void)
{
    for(int tall=0;tall<2;tall++)for(int bank=0;bank<2;bank++)
        for(int i=0;i<factory_bank(bank)->npresets;i++) {
            preset_text_t text;char actual[64],expected[64],joined[64];
            const char *name=factory_preset_name(bank,i);preset_text(&text,name,tall);
            CHECK(text.lines>=1 && text.lines<=2);
            for(int line=0;line<text.lines;line++)CHECK(tw(text.font,text.line[line])<=54);
            put_s(put_s(joined,text.line[0]),text.line[1]);flatten(actual,joined);flatten(expected,name);
            CHECK(!strcmp(actual,expected)); /* all shipping factory names are complete, not clipped */
            CHECK(!strstr(joined,".."));
            if(!tall && text.lines==2)CHECK(text.font==&FONT_T);
        }
    preset_text_t text;
    preset_text(&text,"Acid Lab 1",1);
    CHECK(text.font==&FONT_B && text.lines==2);
    CHECK(!strcmp(text.line[0],"ACID") && !strcmp(text.line[1],"LAB 1"));
    preset_text(&text,"WWWWWWWWWWWWWWWWWWWWWWW",1);
    CHECK(text.lines==2 && strstr(text.line[1],".."));
    CHECK(tw(text.font,text.line[0])<=54 && tw(text.font,text.line[1])<=54);
    preset_text(&text,NULL,1);CHECK(!strcmp(text.line[0],"UNKNOWN"));
    preset_text(&text,"",0);CHECK(!strcmp(text.line[0],"UNKNOWN"));
}
static void factory_name_pixels(int selected)
{
    fixture(0,0);int sound_page=row(R_BENGINE,0);
    CHECK(sound_page==7);*page_of(V_PART)=selected?sound_page:sound_page-1;
    sound_t before=proj.sound;unsigned writes=store_writes;
    shot(out,selected?"accepted-chicago-sound":"unselected-preset-name");
    CHECK(!memcmp(&before,&proj.sound,sizeof before) && store_writes==writes);
    /* The current four-row matrix uses the compact one-line 48px factory name.
     * SOUND is row 4 of the current bank, starting at canvas y=154.
     * Its name is in the real knob-strip blit, below screen y=158. */
    uint16_t bg=selected?RGB(37,59,45):RGB(16,26,31),ink=selected?RGB(224,235,214):RGB(222,231,223);
    CHECK(!matrix_three_rows());
    CHECK(!strcmp(matrix_preset_name(0),"Bite Bass"));
    CHECK(tw(&FONT_XS,"BITE BASS")==48);
    cv_begin(240,49,bg);text_c(92,26,&FONT_XS,"BITE BASS",ink);
    for(int y=25;y<43;y++)for(int x=64;x<122;x++)
        CHECK(fb[(MAIN_Y+154+y)*240+x]==cv_px[y*240+x]);
    CHECK(fb[(MAIN_Y+154+43)*240+181]==swap16(bg)); /* right-half STATUS has no fourth-cell divider */
}
static void physical_and_history(int voice)
{
    fixture(voice,0);int envelope_page=row(R_ENG,10);
    CHECK(envelope_page==3);*page_of(V_PART)=envelope_page;
    CHECK(UI_FIRST_KNOB+3==8); /* panel.c's physical K8 is logical EN_K4 */
    /* Native runtime damper is outside patch editing. The current K8 patch
     * control is envelope RELEASE (10); K6 is the separate DECAY (8). */
    CHECK(pg.r[envelope_page][3].kind==R_ENG && pg.r[envelope_page][3].c==10 && pg.r[envelope_page][3].b==voice);
    CHECK(pg.r[envelope_page][1].c==8);
    pref_set(pg.r[envelope_page][3],63);pref_set(pg.r[envelope_page][1],20);audio();
    now_ms+=1000;enc_acc[EN_K2]=1;ui_input_only();
    CHECK(pref_get(pg.r[envelope_page][1])==21 && pref_get(pg.r[envelope_page][3])==63);
    undo_start();sound_t before=proj.sound;
    now_ms+=1000;enc_acc[EN_K4]=1;ui_input_only();audio();sound_t after=proj.sound;
    CHECK(pref_get(pg.r[envelope_page][3])==64 && pref_get(pg.r[envelope_page][1])==21);
    CHECK(!memcmp(before.v[T_303][voice^1],after.v[T_303][voice^1],NPARAMS_MAX));
    for(int i=0;i<engine_nparams(T_303,voice);i++)if(i!=10)
        CHECK(before.v[T_303][voice][i]==after.v[T_303][voice][i]);
    do_undo(0);CHECK(!undo_wait && !memcmp(&before,&proj.sound,sizeof before));
    do_undo(1);CHECK(!undo_wait && !memcmp(&after,&proj.sound,sizeof after));
    *page_of(V_PART)=row(R_BENGINE,0);int page=cur_page(),dirty=ui.dirty;unsigned writes=store_writes;
    for(int k=2;k<4;k++) {now_ms+=1000;enc_acc[EN_K1+k]=1;ui_input_only();}
    CHECK(!memcmp(&after,&proj.sound,sizeof after) && page==cur_page() && dirty==ui.dirty && writes==store_writes);
    fixture(voice,1);CHECK(pg.n==6);
    CHECK(!strcmp(pg.title[4],"OUTPUT") && pg.r[4][0].c==4 &&
          pg.r[4][1].c==15 && pg.r[4][2].c==14 && pg.r[4][3].kind==R_NONE);
}
static void identities(int voice,int bank)
{
    fixture(voice,bank);*page_of(V_PART)=row(R_BENGINE,0);
    CHECK(!strcmp(pg.title[cur_page()],"SOUND"));
    CHECK(!strcmp(matrix_preset_name(voice),factory_preset_name(bank,0)));
    engine_bass_wheel_owners(1u<<voice);
    CHECK(!engine_bass_select_preset(voice,bank,9,&proj.sound));
    CHECK(engine_bass_preset_index(voice)==0 && engine_bass_preset_pending(voice)==3);
    CHECK(!strcmp(matrix_preset_name(voice),factory_preset_name(bank,9)));
    char name[64];snprintf(name,sizeof name,"%s-%d-requested-name",bank?"miami":"chicago",voice);shot(out,name);
    sound_t before=proj.sound;patch();CHECK(!engine_bass_preset_pending(voice));
    CHECK(!memcmp(&before,&proj.sound,sizeof before));CHECK(!strcmp(matrix_preset_name(voice),factory_preset_name(bank,0)));
    engine_bass_wheel_owners(0);
    unstable=1;CHECK(!strcmp(matrix_preset_name(voice),"UPDATING"));shot(out,"publication-name-updating");unstable=0;
    /* Original edited cells retain their immutable factory source identity;
     * the edit mask and tag must remain coherent after an actual UI edit. */
    pref_t cutoff=PR(R_ENG,T_303,voice,bank?9:ORIGINAL_CHICAGO_CUTOFF);
    int value=pref_get(cutoff);pref_set(cutoff,value?value-1:1);audio();
    CHECK(!engine_bass_sound_validate(&proj.sound));
    CHECK(engine_bass_preset_edited(voice) && engine_bass_preset_index(voice)==0);
    CHECK(!strcmp(matrix_preset_name(voice),factory_preset_name(bank,0)));
    shot(out,"edited-preset-name");
    int known_pages=pg.n;
    bass_sound_tag_t known=proj.sound.bass[voice];
    memset(proj.sound.bass[voice].bytes,0,sizeof proj.sound.bass[voice].bytes);build_pages();
    CHECK(pg.n==known_pages && !strcmp(matrix_preset_name(voice),"UNKNOWN"));shot(out,"unknown-preset-name");
    proj.sound.bass[voice]=known;build_pages();
}
/* Combined Save/SOUND boundary: a pending sound and a queued explicit save
 * are separate requests. PATCH cancels only SOUND; neither cuts the MIDI note. */
static void save_with_pending_sound(int cancel)
{
    fixture(0,0);*page_of(V_PART)=row(R_BENGINE,0);
    CHECK(!project_save());
    pref_set(PR(R_ENG,T_303,0,ORIGINAL_CHICAGO_CUTOFF),pref_get(PR(R_ENG,T_303,0,ORIGINAL_CHICAGO_CUTOFF))+1);
    audio();
    min_q[mi_w++%MQ]=9u|((0x90u|seq.ch[TRK_BASS0])<<8)|(48u<<16)|(100u<<24);
    audio();CHECK(engine_project_notes() && (engine_midi_held()&1));
    now_ms+=1000;enc_acc[EN_K2]=1;ui_input_only();
    CHECK(engine_bass_preset_pending(0)==2 && engine_bass_preset_index(0)==0);
    CHECK(!strcmp(matrix_preset_name(0),factory_preset_name(0,1)));
    unsigned writes=store_writes;
    edges(1u<<B_SAVE,0);edges(0,0);
    CHECK(save_wait && ui.dirty && !ui.save_failed && store_writes==writes);
    now_ms+=AUTOSAVE_QUIET+AUTOSAVE_GAP+1;ui_frame();
    CHECK(save_wait && store_writes==writes && (engine_midi_held()&1));
    shot(out,cancel?"combined-save-before-patch":"combined-save-and-sound-pending");
    if(cancel) {
        patch();CHECK(!engine_bass_preset_pending(0) && save_wait);
        CHECK((engine_midi_held()&1) && !strcmp(matrix_preset_name(0),factory_preset_name(0,0)));
    }
    min_q[mi_w++%MQ]=8u|((0x80u|seq.ch[TRK_BASS0])<<8)|(48u<<16);
    for(int i=0;i<3000 && (engine_project_notes() || engine_bass_preset_pending(0) || mi_r!=mi_w);i++)audio();
    CHECK(!engine_project_notes() && !engine_bass_preset_pending(0));
    CHECK(store_writes==writes);ui_frame();
    CHECK(!save_wait && !ui.dirty && !ui.save_failed && store_writes==writes+1);
    CHECK(engine_bass_preset_index(0)==(cancel?0:1));
    CHECK(!strcmp(matrix_preset_name(0),factory_preset_name(0,cancel?0:1)));
    now_ms+=AUTOSAVE_QUIET+AUTOSAVE_GAP+1;ui_frame();CHECK(store_writes==writes+1);
    shot(out,cancel?"combined-save-patch-complete":"combined-save-and-sound-complete");
}
int main(int argc,char **argv)
{
    CHECK(argc==2);out=argv[1];boot();run_ms(50);engine_guard_enable(0);
    name_bounds();factory_name_pixels(1);factory_name_pixels(0);
    for(int v=0;v<2;v++) {physical_and_history(v);for(int bank=0;bank<2;bank++)identities(v,bank);}
    save_with_pending_sound(0);save_with_pending_sound(1);
    printf("PASS %u accepted SOUND layout/name/physical mapping/history checks; no device claim\n",checks);
    return 0;
}
