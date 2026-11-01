/* SPDX-License-Identifier: GPL-3.0-only
 * Docs-only, reproducible 0.0.24 mockup harness.
 * All displays start from production firmware host/UI/SMC framebuffers.
 * After-state proposal glyphs are drawn with production canvas and atlas.
 * No input routing, engine, saved project or firmware code is changed.
 */
#define SMC_UI_MAIN smc_ui_regressions
#include "../../../tests/host/smc_ui_test.c"
#undef SMC_UI_MAIN


static void item4_sound_recall_proposal(void)
{
    const uint16_t mint=RGB(149,255,185), edge=RGB(49,151,102);
    const uint16_t gold=RGB(255,190,77), ghost=RGB(112,144,148);
    /* Review-only transient notice over the real HOME framebuffer.
     * Glyphs disambiguate the SMC HOME + strip SELECT shortcut from keyboard
     * HOME + black key (mute). The latter is never shown in this proposal.
     * Engine-only reset, with mixer state invariant; missing saved sound => no-op.
     */
    cv_begin(224,74,RGB(10,25,30));
    cv_rect(0,0,224,74,RGB(9,23,28));
    cv_rect(0,0,224,2,edge);
    cv_rect(0,72,224,2,edge);
    cv_rect(0,0,2,74,edge);
    cv_rect(222,0,2,74,edge);
    cv_rect(5,5,214,19,RGB(21,48,46));
    ui_pictogram(10,9,UI_G_HOME,1,mint);
    ui_pictogram(24,9,UI_G_SELECT,1,mint);
    text_c(132,8,&FONT_T,"HOME + SELECT: RESET SOUND",C_WHITE);
    cv_rect(5,27,214,27,RGB(15,41,41));
    ui_pictogram(12,31,UI_G_SOUND,2,mint);
    text_c(117,28,&FONT_M,"RESET SOUND",C_WHITE);
    ui_pictogram(196,31,UI_G_CHECK,2,mint);
    cv_rect(6,56,212,1,edge);
    ui_pictogram(10,60,UI_G_RECORD,1,gold);
    cv_text(23,59,&FONT_T,"UNDO HOME+REC",C_GRAY);
    ui_pictogram(121,60,UI_G_FX_EMPTY,1,ghost);
    cv_text(135,59,&FONT_T,"NO SAVE: NO OP",C_DIM);
    cv_blit(8,84);
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


/* The audio effect's source name is still governed by the production
 * SMCFX table. This fixture visualizes the REVERB effect on the Chicago
 * SELECT FX button (index 3, page 0), rather than inventing a value. */
static void item6_fx_proposal(int fill_active)
{
    const uint16_t amber=RGB(255,190,77), bright=RGB(145,239,246);
    const uint16_t green=RGB(152,247,160), dim=RGB(45,70,72);
    for(int n=0;n<8;n++) {
        int bus=smc_fx_bus_now((uint8_t)(24+n));
        if(bus<0) continue;
        unsigned fx=smcfx_strip_get(&smc.fx_strip,smc.fx_page,bus,SMCFX_SELECT);
        int symbol=smc_fx_symbol(fx);
        if(symbol==UI_G_NONE)symbol=UI_G_FX_EMPTY;
        int highlighted=fill_active ? (n==5) : (n==3);
        paint_fx_cell(n,3,symbol,highlighted?bright:C_GRAY,highlighted);
    }
    /* Verify the highlighted CHICAGO SELECT button really holds REVERB
     * in the unchanged pinned base, rather than labeling an invented FX. */
    if(!fill_active) {
        int held_bus=smc_fx_bus_now(27u);
        CHECK(held_bus==3);
        CHECK(smcfx_strip_get(&smc.fx_strip,smc.fx_page,
                              held_bus,SMCFX_SELECT)==SMCFX_REV);
        CHECK(tw(&FONT_T,"HOME + SELECT: RESET SOUND")<185);
        CHECK(tw(&FONT_M,"REVERB")<174);
    }
    /* BRK REC (note 2) is the proposed fill button, replacing one FX throw. */
    paint_fx_cell(2,2,UI_G_FX_STUTTER,fill_active?C_WHITE:amber,fill_active);

    cv_begin(240,KNOB_H,C_BLACK);
    cv_rect(0,0,240,1,C_LINE);
    if(!fill_active) {
        /* One single footer, no confusing mini-grid. Both icon and text
         * identify the HOLD + arrows instruction while the current effect
         * name dominates. All primitives belong to the real firmware UI. */
        cv_rect(5,5,230,72,RGB(13,31,36));
        cv_rect(5,5,230,1,bright);
        cv_rect(5,76,230,1,dim);
        cv_rect(5,5,1,72,dim);
        cv_rect(234,5,1,72,dim);
        cv_rect(11,9,51,18,RGB(22,61,64));
        ui_pictogram(15,10,UI_G_HOLD,1,amber);
        ui_pictogram(33,10,UI_G_FX_REVERB,1,bright);
        text_c(146,10,&FONT_B,"FX + < >",C_WHITE);
        cv_rect(12,30,216,1,dim);
        ui_pictogram(16,42,UI_G_PREV,2,bright);
        ui_pictogram(206,42,UI_G_NEXT,2,bright);
        text_c(120,34,&FONT_M,"REVERB",C_WHITE);
        cv_rect(83,69,74,2,dim);
        cv_rect(100,69,40,2,bright);
    } else {
        /* The fill screenshot is intentionally unchanged from v1 so the
         * separate owner's fill-placement proposal stays reviewable. */
        cv_rect(5,7,108,67,RGB(14,27,32));
        cv_rect(5,7,108,1,C_WHITE);
        cv_rect(5,73,108,1,RGB(68,76,64));
        cv_rect(125,7,110,67,RGB(15,32,37));
        cv_rect(125,7,110,1,bright);
        cv_rect(125,73,110,1,dim);
        ui_pictogram(15,15,UI_G_PATTERN,2,amber);
        ui_pictogram(49,14,UI_G_FX_STUTTER,2,C_WHITE);
        ui_pictogram(84,19,UI_G_PLAY,1,green);
        for(int k=0;k<8;k++) {
            int h=(k%3==0?12:k%3==1?7:16);
            int x=12+k*12;
            uint16_t c=(k==1||k==3||k==5||k==6)?green:(k<5?amber:dim);
            cv_rect(x,64-h,7,h,c);
            cv_rect(x,65,7,2,dim);
        }
        ui_pictogram(137,25,UI_G_PREV,2,bright);
        ui_pictogram(166,21,UI_G_FX_REVERB,3,C_GRAY);
        ui_pictogram(207,25,UI_G_NEXT,2,bright);
        ui_pictogram(175,54,UI_G_HOLD,1,C_GRAY);
        cv_rect(157,65,55,2,dim);
        cv_rect(173,65,24,2,amber);
    }
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
