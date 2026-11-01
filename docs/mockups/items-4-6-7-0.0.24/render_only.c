/* SPDX-License-Identifier: GPL-3.0-only
 * Docs-only, reproducible 0.0.24 mockup harness.
 * All displays start from production firmware host/UI/SMC framebuffers.
 * After-state proposal glyphs are drawn with production canvas and atlas.
 * No input routing, engine, saved project or firmware code is changed.
 */
#define SMC_UI_MAIN smc_ui_regressions
#include "../../../tests/host/smc_ui_test.c"
#undef SMC_UI_MAIN

static void mini_connector(int x,int y,uint16_t c)
{
    cv_line(x,y,x+15,y,c);
    cv_rect(x+3,y-1,2,3,c);
    cv_rect(x+11,y-1,2,3,c);
}

static void item4_sound_recall_proposal(void)
{
    const uint16_t mint=RGB(149,255,185), edge=RGB(49,151,102);
    const uint16_t gold=RGB(255,190,77);
    const uint16_t face=RGB(12,31,32);
    /* Transient native canvas overlay ON the existing HOME view. */
    cv_begin(224,39,face);
    cv_rect(0,0,224,39,RGB(8,23,26));
    cv_rect(0,0,224,1,edge);
    cv_rect(0,38,224,1,edge);
    cv_rect(0,0,1,39,edge);
    cv_rect(223,0,1,39,edge);
    cv_rect(5,4,33,31,RGB(26,68,58));
    ui_pictogram(13,10,UI_G_SOUND,2,mint);
    mini_connector(42,20,edge);
    ui_pictogram(68,9,UI_G_CYCLE,2,gold);
    mini_connector(96,20,edge);
    cv_rect(117,4,34,31,RGB(18,67,48));
    ui_pictogram(125,10,UI_G_CHECK,2,mint);
    /* The right-hand glyph chord shows HOME+REC for undo. */
    cv_rect(157,4,62,31,RGB(19,35,36));
    ui_pictogram(162,14,UI_G_HOME,1,mint);
    cv_rect(175,18,5,1,edge);
    cv_rect(177,16,1,5,edge);
    ui_pictogram(185,14,UI_G_RECORD,1,gold);
    ui_pictogram(204,14,UI_G_PREV,1,mint);
    cv_blit(8,108);
}

static void paint_fx_cell(int strip,int row,int glyph,uint16_t ink,int selected)
{
    int x=strip*30+8;
    int y=MAIN_Y+62+row*12;
    cv_begin(22,11,C_BLACK);
    cv_rect(0,0,22,11,selected?RGB(25,78,79):RGB(17,28,33));
    cv_rect(0,0,22,1,selected?ink:RGB(46,66,68));
    cv_rect(0,10,22,1,selected?ink:RGB(40,55,58));
    ui_pictogram(7,1,glyph,1,ink);
    cv_blit(x,y);
}

static void item6_fx_proposal(int fill_active)
{
    const uint16_t amber=RGB(255,190,77), bright=RGB(145,239,246);
    const uint16_t green=RGB(152,247,160), dim=RGB(45,70,72);
    /* Item 6 changes only the existing eight-strip mixer window. A dedicated
       BRK strip REC button (SMC note 2) is proposed as a drum-fill gesture.
       FX button held on RVB SELECT remains momentary; arrows step its effect. */
    for(int n=0;n<8;n++) {
        int bus=smc_fx_bus_now((uint8_t)(24+n));
        if(bus<0) continue;
        unsigned fx=smcfx_strip_get(&smc.fx_strip,smc.fx_page,bus,SMCFX_SELECT);
        int symbol=smc_fx_symbol(fx);
        if(symbol==UI_G_NONE)symbol=UI_G_FX_EMPTY;
        paint_fx_cell(n,3,symbol,n==5?bright:C_GRAY,n==5);
    }
    /* Dedicated fill button has an unambiguous roll/step glyph, never CYC. */
    paint_fx_cell(2,2,UI_G_FX_STUTTER,fill_active?C_WHITE:amber,1);
    /* Two compact feedback islands in the existing 82px mixer footer. */
    cv_begin(240,KNOB_H,C_BLACK);
    cv_rect(0,0,240,1,C_LINE);
    cv_rect(5,7,108,67,RGB(14,27,32));
    cv_rect(5,7,108,1,fill_active?C_WHITE:amber);
    cv_rect(5,73,108,1,RGB(68,76,64));
    cv_rect(125,7,110,67,RGB(15,32,37));
    cv_rect(125,7,110,1,bright);
    cv_rect(125,73,110,1,dim);
    ui_pictogram(15,15,UI_G_PATTERN,2,amber);
    ui_pictogram(49,14,UI_G_FX_STUTTER,2,fill_active?C_WHITE:amber);
    ui_pictogram(84,19,fill_active?UI_G_PLAY:UI_G_HOLD,1,fill_active?green:C_GRAY);
    for(int k=0;k<8;k++) {
        int h=(k%3==0?12:k%3==1?7:16);
        int x=12+k*12;
        uint16_t c=fill_active && (k==1||k==3||k==5||k==6)?green:(k<5?amber:dim);
        cv_rect(x,64-h,7,h,c);
        cv_rect(x,65,7,2,dim);
    }
    ui_pictogram(137,25,UI_G_PREV,2,bright);
    ui_pictogram(166,21,UI_G_FX_REVERB,3,fill_active?C_GRAY:C_WHITE);
    ui_pictogram(207,25,UI_G_NEXT,2,bright);
    ui_pictogram(175,54,UI_G_HOLD,1,C_GRAY);
    cv_rect(157,65,55,2,dim);
    cv_rect(173,65,24,2,fill_active?amber:bright);
    cv_blit(0,KNOB_Y);
}

static void item7_part_toggles_proposal(void)
{
    static const int parts[5]={PART_909,PART_808,PART_BRK,PART_303A,PART_303B};
    static const char *const names[5]={"909","808","BREAK","CHICAGO","MIAMI"};
    static const uint8_t enabled[5]={1,1,1,1,0};
    cv_begin(240,2*BAND_H,C_BLACK);
    cv_rect(0,0,240,2*BAND_H,C_BLACK);
    cv_rect(2,1,236,136,RGB(10,22,25));
    for(int i=0;i<5;i++) {
        int y=5+i*26;
        uint16_t ink=PART_COL[parts[i]];
        uint16_t light=enabled[i]?C_WHITE:C_DIM;
        cv_rect(5,y,230,23,i==0?RGB(29,62,46):RGB(19,31,35));
        cv_rect(5,y,2,23,ink);
        cv_rect(8,y+22,224,1,dim(ink,13));
        int g=i==2?UI_G_FX_STUTTER:i<2?UI_G_PATTERN:UI_G_SOUND;
        ui_pictogram(16,y+7,g,1,ink);
        cv_text(43,vc(&FONT_B,y+2,23),&FONT_B,names[i],light);
        ui_pictogram(157,y+7,enabled[i]?UI_G_CHECK:UI_G_MUTE,1,enabled[i]?ink:C_DIM);
        ui_state_toggle(185,y+7,enabled[i],enabled[i]?ink:C_GRAY);
        cv_rect(215,y+9,10,5,enabled[i]?ink:C_LINE);
    }
    cv_blit(0,MAIN_Y);
}

int main(void)
{
    setup(V_HOME,PART_303A);
    shot("item4-before");
    item4_sound_recall_proposal();
    shot("item4-after");

    setup(V_FX,PART_303A);
    smc.home_down=1;
    smc_ui_begin(6,1);
    ui_frame();
    shot("item6-before");
    item6_fx_proposal(0);
    shot("item6-after-effect");
    item6_fx_proposal(1);
    shot("item6-after-fill");

    setup(V_GLO,PART_303A);
    open_global();
    ui_frame();
    shot("item7-before");
    item7_part_toggles_proposal();
    shot("item7-after");
    return 0;
}
