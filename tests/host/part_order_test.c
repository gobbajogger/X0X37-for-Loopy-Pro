/* SPDX-License-Identifier: GPL-3.0-only */
/* Presentation order must never renumber saved lanes, MIDI channels or FX buses. */
#define SMC_UI_MAIN unused_smc_ui_main
#include "smc_ui_test.c"
static struct { uint32_t buttons,frames;uint8_t fader[4]; } fm1_in;
static uint8_t smk_fbank;
enum { SMK_BTN_FADER_BANK = 1 };
#include "smk_faders.inc"
static const int expected[5] = {PART_909, PART_808, PART_BRK, PART_303B, PART_303A};
static void order_navigation(void)
{
    CHECK(PART_909 == 0 && PART_808 == 1 && PART_303A == 2 && PART_303B == 3 && PART_BRK == 4);
    setup(V_HOME, PART_909);
    project_t *before = malloc(sizeof proj); CHECK(before != NULL); memcpy(before, &proj, sizeof proj);
    uint8_t midi[NPARTS]; for(int i=0;i<NPARTS;i++)midi[i]=seq.ch[i];
    for (int pos = 0; pos < NPARTS; pos++) {
        CHECK(part_at(pos) == expected[pos] && part_position(expected[pos]) == pos);
        CHECK(ui.part == expected[pos]);
        enc_acc[EN_ALGO] = 1; ui_input_only();
    }
    CHECK(ui.part == PART_303A); /* last lane clamps, not wraps */
    for (int pos = NPARTS-1; pos >= 0; pos--) {
        CHECK(ui.part == expected[pos]); enc_acc[EN_ALGO] = -1; ui_input_only();
    }
    CHECK(ui.part == PART_909 && !memcmp(before, &proj, sizeof proj));
    for(int i=0;i<NPARTS;i++)CHECK(midi[i]==seq.ch[i]); free(before);
    for (int pos = 0; pos < NPARTS; pos++) {
        setup(V_HOME, expected[pos]);
        smc_button_apply(0x2F, 0); CHECK(ui.part == expected[pos == 4 ? 4 : pos+1]);
        setup(V_HOME, expected[pos]);
        smc_button_apply(0x2E, 0); CHECK(ui.part == expected[pos ? pos-1 : 0]);
    }
    setup(V_SEQ,PART_303A);
    bstep_t *a_step=&proj.pat[0].bass[0].step[0], *b_step=&proj.pat[0].bass[1].step[0];
    a_step->flags=G_NOTE;a_step->note=48;b_step->flags=G_NOTE;b_step->note=55;
    key_event(WHITE_KEY[0],1);CHECK(ui.held_step==0);
    enc_acc[EN_ALGO]=-1;ui_input_only();CHECK(ui.part==PART_303A && ui.held_step==0); /* held-step velocity, not navigation */
    smc_button_apply(27,0);CHECK(ui.part==PART_303B && ui.held_step<0);
    key_event(WHITE_KEY[0],0);CHECK(a_step->flags==G_NOTE && b_step->flags==G_NOTE && b_step->note==55);
    /* Both synth engine choices may change; a physical lane must not move. */
    for (int a=0;a<2;a++) for(int b=0;b<2;b++) {
        engine_bass_select_preset(0,a,0,&proj.sound); engine_bass_select_preset(1,b,0,&proj.sound);
        CHECK(part_at(3)==PART_303B && part_at(4)==PART_303A);
    }
}
static void panel_maps(void)
{
    for (int pos=0;pos<NPARTS;pos++) {
        int p=expected[pos]; setup(V_HOME,PART_909);
        key_event(BLACK_KEY[pos],1);key_event(BLACK_KEY[pos],0);
        CHECK(seq.mute==MUTE_MASK[p]);leds();
        CHECK((dim_keys & (1u<<BLACK_KEY[pos])) && !(lit_keys & (1u<<BLACK_KEY[pos])));
        for(int other=0;other<NPARTS;other++) if(other!=pos)CHECK(lit_keys & (1u<<BLACK_KEY[other]));
        setup(V_MIX,PART_909);*page_of(V_MIX)=0;build_pages();
        key_event(BLACK_KEY[pos],1);key_event(BLACK_KEY[pos],0);
        CHECK(ui.mixsel==p && pg.r[0][0].b==p);leds();
        CHECK(lit_keys & (1u<<BLACK_KEY[pos]));
        ui.btn=1u<<B_HOME;key_event(BLACK_KEY[pos],1);key_event(BLACK_KEY[pos],0);ui.btn=0;
        CHECK(seq.mute==MUTE_MASK[p]);
        setup(V_SONG,PART_909);song_fill(0);
        key_event(BLACK_KEY[pos],1);key_event(BLACK_KEY[pos],0);
        CHECK(song()->bar[0].mute==(1u<<p));leds();CHECK(dim_keys & (1u<<BLACK_KEY[pos]));
    }
    setup(V_HOME,PART_909);
    for(int pos=NPARTS;pos<11;pos++) {key_event(BLACK_KEY[pos],1);key_event(BLACK_KEY[pos],0);CHECK(!seq.mute);}
    setup(V_SONG,PART_909);
    CHECK(pg.r[0][1].a==PART_909 && pg.r[0][2].a==PART_808 && pg.r[0][3].a==PART_BRK);
    CHECK(pg.r[1][1].a==PART_303B && pg.r[1][2].a==PART_303A);
    open_global();int n=0;
    for(int i=0;i<ui.list_n;i++)if(list_rows[i].kind==R_MIDICH)CHECK(list_rows[i].a==expected[n++]);
    CHECK(n==NPARTS);
}
static void surface_maps(void)
{
    for(int pos=0;pos<NPARTS;pos++) {
        int p=expected[pos];setup(V_HOME,PART_909);
        CHECK(smc_same(smc_target(pos),PR(R_ENG,T_MIX,p,MX_LEVEL)));
        CHECK(smc_same(smc_knob_target(pos),PR(R_ENG,T_MIX,p,MX_CUT)));
        int old[NPARTS];for(int i=0;i<NPARTS;i++)old[i]=proj.sound.v[T_MIX][i][MX_LEVEL];
        message(0xE0+pos,0,smc_scaled(smc_target(pos))); message(0xE0+pos,0,0);
        CHECK(proj.sound.v[T_MIX][p][MX_LEVEL]==0);
        for(int i=0;i<NPARTS;i++)if(i!=p)CHECK(proj.sound.v[T_MIX][i][MX_LEVEL]==old[i]);
        smc_button_apply(16+pos,0);CHECK(seq.mute==MUTE_MASK[p] && smc_led(16+pos));
        setup(V_HOME,PART_909);smc_button_apply(pos,0);
        CHECK(ui.part==p && ui.rec && smc_led(pos));
        for(int i=0;i<NPARTS;i++)CHECK(smc_led(i)==(i==pos));
        setup(V_HOME,PART_909);smc_button_apply(24+pos,0);CHECK(ui.part==p && smc_led(24+pos));
        setup(V_HOME,PART_909);smc_button_apply(8+pos,0);
        CHECK(smc.solo==(1u<<p) && smc_led(8+pos));smc_release(8+pos);CHECK(!smc.solo);
        setup(V_FX,PART_909);smc_button_apply(24+pos,0);
        CHECK(smc.fx_down==(1u<<p) && smc.fx_owned==(1u<<p) && smc_led(24+pos));
        set_view(V_HOME);smc_fx_release_note(24+pos);CHECK(!smc.fx_down && !smc.fx_owned);
    }
    setup(V_HOME,PART_909);
    pref_t captured=smc_target(2);CHECK(captured.b==PART_BRK && smc_lock_toggle());
    ui.part=PART_303A;set_view(V_PART);build_pages();
    CHECK(smc_same(smc_target(2),captured)); /* capture stores stable ID, never presentation slot */
    smc_release(0x5D);CHECK(smc_same(smc_target(2),captured));
    CHECK(smc_lock_toggle());
}
static void onboard_faders(void)
{
    setup(V_HOME,PART_909);fm1_in.frames=200;smk_fbank=0;fm1_in.buttons=0;
    memset(fm1_in.fader,0,sizeof fm1_in.fader);smk_faders();
    for(int p=0;p<NPARTS;p++)pref_set(PR(R_ENG,T_MIX,p,MX_LEVEL),80);
    for(int bank=0;bank<2;bank++) {
        if(bank) {
            fm1_in.buttons=1u<<SMK_BTN_FADER_BANK;smk_faders();
            CHECK(strstr(ui.msg,"MIA REV DLY FILTER"));
            fm1_in.buttons=0;smk_faders();
        }
        for(int i=0;i<(bank?1:4);i++) {
            int pos=bank*4+i,p=expected[pos];
            fm1_in.fader[i]=20;smk_faders();CHECK(pref_get(PR(R_ENG,T_MIX,p,MX_LEVEL))==80);
            fm1_in.fader[i]=80;smk_faders();CHECK(pref_get(PR(R_ENG,T_MIX,p,MX_LEVEL))==80);
            fm1_in.fader[i]=79;smk_faders();CHECK(pref_get(PR(R_ENG,T_MIX,p,MX_LEVEL))==79);
            CHECK(strstr(ui.msg,part_s(p)) && strstr(ui.msg,"LEVEL"));
            /* Off/on before another scan must re-arm exactly this physical slot. */
            mute_part(p);CHECK((smk_mute_active & (1u<<i)) && (smk_mute_edit & (1u<<i)));
            mute_part(p);CHECK(!(smk_mute_active & (1u<<i)) && !(smk_mute_edit & (1u<<i)));
            mute_part(p);fm1_in.fader[i]=10;smk_faders();CHECK(pref_get(PR(R_ENG,T_MIX,p,MX_LEVEL))==10);
            mute_part(p);pref_set(PR(R_ENG,T_MIX,p,MX_LEVEL),80);
            fm1_in.fader[i]=11;smk_faders();CHECK(pref_get(PR(R_ENG,T_MIX,p,MX_LEVEL))==80);
        }
        int untouched=bank?PART_BRK:PART_303A;uint8_t active=smk_mute_active,edit=smk_mute_edit;
        mute_part(untouched);CHECK(active==smk_mute_active && edit==smk_mute_edit);mute_part(untouched);
    }
    fm1_in.buttons=1u<<SMK_BTN_FADER_BANK;smk_faders();CHECK(strstr(ui.msg,"909 808 BRK CHI"));
    fm1_in.buttons=0;smk_faders();
}
static void visual_order(void)
{
    setup(V_HOME,PART_303B);
    CHECK(!strcmp(part_s(expected[3]),"CHI") && !strcmp(part_s(expected[4]),"MIA"));
    ui_frame();shot("home-drums-then-synths");
    /* HOME marks require real content; assignment alone must not invent it. */
    for(int p=0;p<NPARTS;p++) {
        seq.ppat[p]=(uint8_t)(p+1);
        pattern_t *pat=&proj.pat[p+1];
        if(p<NKIT)sm_set(pat->drum[p].hit[0],0);
        else if(p==PART_BRK)pat->brk.steps=1;
        else {pat->bass[p-NKIT].step[0].flags=G_NOTE;pat->bass[p-NKIT].step[0].note=48;}
    }
    cv_begin(240,BAND_H,C_BLACK);draw_home(0);
    for(int pos=0;pos<NPARTS;pos++) {
        int p=expected[pos],x=7+(p+1)*14;
        CHECK(cv_px[(4+pos*4)*240+x]==swap16(PART_COL[p]));
    }
    for(int p=0;p<NPARTS;p++)seq.ppat[p]=0;
    cv_begin(240,BAND_H,C_BLACK);draw_home(1);
    uint16_t pixels[240*BAND_H];memcpy(pixels,cv_px,sizeof pixels);
    cv_begin(240,BAND_H,C_BLACK);
    /* The Night rows are20px high; compare all lower-band visible rows in fixed order. */
    for(int pos=1;pos<5;pos++) {
        int p=expected[pos],y=35+pos*20-BAND_H;
        if(p<NKIT) {
            uint32_t bits=0;const dpart_t *d=&pat_of(p)->drum[p];
            for(int v=0;v<NDRUM;v++)bits|=d->hit[v][0];
            lane(y,p,bits&65535u,eng_step[TRK_DRUM+p],d->len,0);
        } else if(p==PART_BRK)lane(y,p,pat_of(p)->brk.steps,eng_step[TRK_BRK],16,0);
        else {const bpart_t *bp=&pat_of(p)->bass[p-NKIT];
            lane(y,p,bass_content_bits16(bp,0),eng_step[TRK_BASS0+p-NKIT],bp->len,0);}
    }
    CHECK(!memcmp(pixels,cv_px,sizeof pixels));
    set_view(V_MIX);*page_of(V_MIX)=0;build_pages();ui.mixsel=PART_BRK;ui_frame();shot("mix-drums-then-synths");
    set_view(V_SONG);song_fill(0);ui_frame();shot("song-drums-then-synths");
    set_view(V_FX);ui_frame();shot("fx-drums-then-synths");
    /* Real current candidate pages for the branding seam; no hand-drawn labels. */
    set_view(V_MIX);*page_of(V_MIX)=4;build_pages();ui_frame();shot("wave-stopped");
    ui.part=PART_BRK;set_view(V_PART);ui_frame();shot("break-stopped");
    ui.part=PART_303B;set_view(V_PART);*page_of(V_PART)=pg.n-1;ui_frame();shot("chicago-sound-user");
}
int main(void)
{
    order_navigation();panel_maps();surface_maps();onboard_faders();visual_order();
    printf("part_order: PASS (%u checks)\n",checks);return 0;
}
