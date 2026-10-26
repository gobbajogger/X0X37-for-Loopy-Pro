/* SPDX-License-Identifier: GPL-3.0-only */
/* Replay every physical DAW strip button through real dispatch, not captions alone. */
#include <stdint.h>
static uint8_t fx_led[128];
int plat_ble_smc_send(const uint8_t *m, uint8_t n)
{
    if (n == 3 && m[0] == 0x90) fx_led[m[1]] = m[2];
    return 1;
}
#define SMC_UI_SEND_IMPLEMENTED 1
#define SMC_UI_MAIN smc_ui_existing_main
#include "smc_ui_test.c"
#undef SMC_UI_MAIN

static void check_wire(int note)
{
    smc_frame(); CHECK(fx_led[note] == (smc_led(note) ? 127 : 0));
}
static void expect_readout(const char *name, const char *value)
{
    uint16_t actual[240 * 48];
    ui_frame(); shot("readout-actual"); memcpy(actual, fb + KNOB_Y * 240, sizeof actual);
    cv_begin(240, KNOB_H, C_BLACK); cv_rect(0, 0, 240, 1, C_LINE);
    text_c(120, 4, &FONT_S, caps_of(name), C_WHITE);
    text_c(120, 22, &FONT_M, value, C_HI); cv_commit(3, 0, KNOB_Y);
    if(memcmp(actual, fb + KNOB_Y * 240, sizeof actual)) {
        fprintf(stderr,"READOUT expected %s / %s; kind=%u action=%u view=%u\n",name,value,smc_ui.kind,smc_ui.action,ui.view);
        shot("readout-expected");
    }
    CHECK(!memcmp(actual, fb + KNOB_Y * 240, sizeof actual));
}
static void direct_drum_readouts(void)
{
    for(int kit=0;kit<2;kit++) for(int strip=5;strip<8;strip++) {
        setup(V_MIX,kit); *page_of(V_MIX)=(uint8_t)(kit+1); ui.sel[kit]=0; build_pages();
        message(0x90,24+strip,127); CHECK(ui.sel[kit]==strip && drum_mix_page()==kit);
        expect_readout("SELECT",engine_voice_name(kit?T_808:T_909,strip));
        if(!kit && strip==5) { ui_frame(); shot("direct-drum-select6"); }
        message(0x80,24+strip,0);
        /* The selected sound's repeated SELECT intentionally returns to main MIX. */
        message(0x90,24+strip,127); CHECK(ui.view==V_MIX && cur_page()==0);
        expect_readout("PAGE","PARTS"); message(0x80,24+strip,0);
    }
    setup(V_SEQ,PART_909); message(0x90,7,127); CHECK(ui.rec && ui.sel[0]==7);
    char label[24]; smc_ui_label(smc_target(7),label); expect_readout(label,"RECORD ON");
    message(0x80,7,0); message(0x90,15,127); CHECK(smc.drum_solo[0]&(1u<<7));
    expect_readout(label,"SOLO ON"); message(0x80,15,0);
    puts("PASS rendered direct-drum SELECT6/7/8 sound names, repeated return PAGE, and drum8 Record/Solo ownership");
}

static void all_rows(void)
{
    const int notes[4][8] = {
        {0,1,2,3,4,5,6,7}, {8,9,10,11,12,13,14,15},
        {16,17,18,19,20,21,22,23}, {24,25,26,27,28,29,30,31}
    };
    for (int row = 0; row < 4; row++) for (int strip = 0; strip < 8; strip++) {
        setup(V_FX, PART_303A);
        int note = notes[row][strip];
        const char *caption = smc_button_caption(note);
        if (row == 3) CHECK(!strcmp(caption, SMCFX_SHORT[SMCFX_PAGE[0][strip]]));
        else if (strip < 5 || row == 2) CHECK(!strcmp(caption, row == 0 ? "R" : row == 1 ? "S" : "M"));
        int muted = strip < 5 ? part_muted(strip) : 0;
        pref_t tail = PR(R_ENG, T_FX, 0, strip == 5 ? FX_RV_LEVEL : strip == 6 ? FX_DL_LEVEL : FX_VOLUME);
        if (row == 2 && strip >= 5) pref_set(tail, 100);
        message(0x90, note, 127);
        if (row == 0) {
            if (strip < 5) { CHECK(ui.part == strip); CHECK(ui.rec); CHECK(ui.view == V_FX); }
            else if (strip == 5) CHECK(ui.view == V_FX && ui.overlay == O_LIST);
            else CHECK(ui.view == (strip == 6 ? V_FX : V_HOME));
        } else if (row == 1) {
            if (strip < 5) CHECK(smc.solo & (1u << strip));
            else CHECK(ui.view == (strip == 5 ? V_MIX : strip == 6 ? V_GLO : V_PART));
        } else if (row == 2) {
            if (strip < 5) CHECK(part_muted(strip) != muted);
            else CHECK(pref_get(tail) == 0);
        } else {
            if (strip < 5) CHECK(smc.fx_down == (1u << strip));
            else { CHECK(!smc.fx_down); CHECK(smc.fx_main_wait == (1u << (strip - 5))); }
            now_ms += 180; smc_poll();
            CHECK(smc.fx_down == (1u << strip)); CHECK(ui.view == V_FX && ui.part == PART_303A);
            CHECK(smc.fx_effect[strip] == SMCFX_PAGE[0][strip]);
            if (strip < 5) CHECK(smc_led(note));
            message(0x90, note, 127); CHECK(smc.fx_down == (1u << strip));
        }
        check_wire(note);
        int rec = ui.rec, part = ui.part, view = ui.view;
        message(0x80, note, 0);
        CHECK(ui.rec == rec && ui.part == part);
        CHECK(ui.view == (row == 0 && strip == 6 ? V_HOME : view));
        if (row == 1 && strip < 5) CHECK(!smc.solo);
        CHECK(!smc.fx_down && !smc.fx_main_wait); check_wire(note);
    }
    puts("PASS FX rows: all32 DAW actions, actual target/transport/UI changes, release, caption and wire feedback");
}
static void guards_and_main_tap(void)
{
    for (int strip = 0; strip < 8; strip++) {
        setup(V_FX, PART_303A); open_list_of_pages();
        message(0x90, 24 + strip, 127); CHECK(!smc.fx_down && !smc.fx_main_wait);
        message(0x80, 24 + strip, 0); CHECK(!smc.fx_down);
        setup(V_FX, PART_303A); clip_grid_mode = 1;
        message(0x90, 24 + strip, 127); CHECK(!smc.fx_down && !smc.fx_main_wait);
        message(0x80, 24 + strip, 0); CHECK(!smc.fx_down);
        setup(V_SEQ, PART_909); CHECK(smc_lock_toggle());
        set_view(V_FX); build_pages();
        pref_t frozen = smc_target(0);
        message(0x90, 24 + strip, 127); now_ms += 180; smc_poll();
        CHECK(smc.fx_down == (1u << strip)); CHECK(smc.locked && ui.view == V_FX);
        CHECK(!strcmp(smc_button_caption(24 + strip), SMCFX_SHORT[SMCFX_PAGE[0][strip]]));
        CHECK(smc_same(frozen, smc_target(0))); check_wire(24 + strip);
        message(0x80, 24 + strip, 0); CHECK(!smc.fx_down);
    }
    setup(V_FX, PART_303A);
    message(0x90,31,127); CHECK(ui.view == V_FX && !smc.fx_down);
    message(0x90,31,127); CHECK(ui.view == V_FX && !smc.fx_down);
    message(0x80,31,0); CHECK(ui.view == V_MIX && cur_page() == 3);
    CHECK(!smc.fx_owned && !smc.fx_main_wait && !smc.fx_down);
    setup(V_FX, PART_303A); message(0x90,24,127); CHECK(smc.fx_down);
    set_view(V_HOME); build_pages(); smc_poll(); CHECK(!smc.fx_down);
    message(0x80,24,0); CHECK(ui.view == V_HOME && !smc.fx_owned);
    puts("PASS FX ownership: overlays/clip guarded; drum lock permits FX with frozen faders, main tap release once, navigation clears owned holds");
}
static void decode(void)
{
    setup(V_FX, PART_303A);
    message(0x91,24,127); CHECK(!smc.fx_down);
    message(0xB0,24,127); CHECK(!smc.fx_down); /* no button CC mapping */
    CHECK(!smc_midi_host(9u | 0x90u << 8 | 24u << 16 | 127u << 24));
    proj.set.bt |= PROJ_BT_SMC_USER;
    CHECK(smc_midi_host(9u | 0x9Fu << 8 | 24u << 16 | 127u << 24)); smc_poll();
    CHECK(smc.fx_down == 1u);
    CHECK(smc_midi_host(8u | 0x8Fu << 8 | 24u << 16)); smc_poll(); CHECK(!smc.fx_down);
    puts("PASS FX decode: DAW channel1, opt-in hosted channel16, CC rejection and release routing");
}
static void modifier(void)
{
    setup(V_FX,PART_303A);
    message(0x90,6,127); CHECK(ui.view==V_FX && smc_ui_active()); ui_frame(); shot("fx-home-assignment");
    message(0x90,24,127); CHECK(smc.fx_effect[0]==SMCFX_DLY && !smc.fx_down);
    message(0x80,24,0); CHECK(ui.view==V_FX && !smc.fx_down);
    message(0x90,0x62,127); CHECK(smc.fx_page==2 && ui.view==V_FX); expect_readout("FX PAGE","ECHO");
    message(0x80,0x62,0); message(0x80,6,0); CHECK(ui.view==V_FX && !smc.fx_down && !smc.fx_owned);
    /* Modifier + Down remains a mapping-bank gesture, never a preset reset or mode change. */
    message(0x90,6,127); message(0x90,0x61,127); CHECK(ui.view==V_FX);
    message(0x80,0x61,0); message(0x80,6,0); CHECK(ui.view==V_FX);
    message(0x90,6,127); CHECK(ui.view==V_FX);
    message(0x90,6,127); CHECK(ui.view==V_FX);
    message(0x80,6,0); CHECK(ui.view==V_HOME); expect_readout("PAGE","PERFORM");
    setup(V_FX,PART_303A); ui.help=1;
    message(0x90,6,127); CHECK(!ui.help && ui.view==V_FX);
    message(0x80,6,0); CHECK(ui.view==V_FX);
    setup(V_FX,PART_303A); message(0x90,6,127); ui.help=1;
    message(0x80,6,0); CHECK(ui.help && ui.view==V_FX); ui.help=0;
    setup(V_HOME,PART_303A); message(0x90,24,127); CHECK(ui.view==V_PART && ui.part==PART_909);
    ui_frame(); shot("ordinary-source-select"); message(0x80,24,0);
    puts("PASS FX Home modifier: saved SELECT assignment/page gesture stays FX; plain tap navigates once on release");
}

static void user_mmc(void)
{
    for(int strip=0;strip<8;strip++) {
        setup(V_FX,PART_303A); proj.set.bt|=PROJ_BT_SMC_USER;
        uint8_t cmd[]={0xF0,0x7F,0x7F,6,(uint8_t)(4+strip*4),0xF7};
        CHECK(smc_mmc_note(cmd[4])==24+strip);
        smc_ble_midi(cmd,6); smc_poll();
        if(strip<5) {
            CHECK(smc.fx_down==(1u<<strip));
            smc_ble_midi(cmd,6); smc_poll(); CHECK(!smc.fx_down);
        } else {
            CHECK(ui.view==(strip==7?V_MIX:V_FX));
            CHECK(cur_page()==(strip==5?0:strip==6?1:3));
            CHECK(!smc.fx_down && !smc.fx_main_wait && !smc.fx_owned);
            if(strip==5) { ui_frame(); shot("user-mmc-reverb-page"); }
        }
    }
    puts("PASS stock User MMC: source FX latch toggles; release-less main buttons execute existing tap navigation");
}

int main(void)
{
    const char *only=getenv("SMC_FX_CASE");
    if(!only) { all_rows(); guards_and_main_tap(); decode(); }
    if(!only||!strcmp(only,"mmc")) user_mmc();
    if(!only||!strcmp(only,"modifier")) modifier();
    if(!only||!strcmp(only,"drum-readout")) direct_drum_readouts();
    printf("PASS full SMC FX binding: %u checks\n", checks);
    return 0;
}
