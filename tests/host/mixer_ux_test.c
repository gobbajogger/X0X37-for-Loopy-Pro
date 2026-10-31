/* SPDX-License-Identifier: GPL-3.0-only */
/* Scoped port of #688@7b2d1726: existing navigation, unmute and beat tests.
 * Tests the shipping engines and mixer through their production input queue.
 * Older FX mapping/gesture cases remain with the current dedicated FX suites. */
#define SMC_UI_MAIN old_smc_ui_main
#include "smc_ui_test.c"
#undef SMC_UI_MAIN
#ifndef SMK_BTN_FADER_BANK
#define SMK_BTN_FADER_BANK 7
#endif
static uint8_t smk_fbank;
static struct { uint32_t buttons, frames; uint8_t fader[4]; } fm1_in;
#include "keyboard_faders.inc"

static void press(int note) { message(0x90, note, 127); }
static void release(int note) { message(0x80, note, 0); }
static void tap(int note) { press(note); release(note); }
static void render_audio(unsigned n)
{
    int32_t out[512];
    do { unsigned k = n > 256 ? 256 : n; engine_render(out, k); n -= k; } while (n);
}
static void item31(void)
{
    setup(V_HOME, PART_BRK);
    int strip = part_position(PART_909);
    tap(24 + strip); CHECK(ui.part == PART_909 && ui.view == V_PART);
    tap(24 + strip); CHECK(ui.view == V_MIX && cur_page() == 1);
    tap(24); CHECK(ui.view == V_MIX && cur_page() == 0);
    tap(0x5E); CHECK(seq.req_start); render_audio(1); CHECK(seq.playing);
    tap(0x5E); render_audio(1); CHECK(!seq.playing);
    tap(0x5D); CHECK(smc.locked);
    pref_t captured = smc_target(0);
    tap(24 + part_position(PART_303B)); CHECK(ui.part == PART_303B);
    CHECK(smc_same(captured, smc_target(0)));
    tap(0); CHECK(ui.rec);
    puts("PASS item31: deliberate selection, repeat mode, transport and usable locked actions");
}
static void item37_39(void)
{
    setup(V_HOME, PART_303B);
    uint8_t view = ui.view, page = cur_page();
    pref_t old = smc_knob_target(0);
    for (int bank = 1; bank <= 4; bank++) {
        tap(0x61); CHECK(ui.view == view && cur_page() == page);
        CHECK(smc.knob_bank == bank % 4);
        for (int n = 0; n < 8; n++) for (int f = 0; f < 8; f++)
            CHECK(smc_knob_target(n).kind == R_NONE || !smc_same(smc_knob_target(n), smc_target(f)));
    }
    CHECK(smc_same(old, smc_knob_target(0)));
    tap(7); CHECK(ui.view == V_SONG); tap(7); CHECK(ui.view == V_HOME);
    tap(15); CHECK(ui.view == V_PART); tap(15); CHECK(ui.view == V_SEQ);
    tap(15); CHECK(ui.view == V_GEN); tap(15); CHECK(ui.view == V_PART);
    tap(31); CHECK(ui.view == V_MIX && cur_page() == 0);
    tap(31); CHECK(cur_page() == 1);
    setup(V_MIX, PART_909); *page_of(V_MIX) = 1; build_pages();
    tap(0x5D); pref_t first = smc_target(0);
    press(8); tap(0x2F); CHECK(smc.lock_bank == 8);
    CHECK(!smc_same(first, smc_target(0))); CHECK(smc_target(3).kind == R_NONE);
    CHECK(smc.solo_target[0] == NPARTS + 1);
    release(8); CHECK(!smc.solo);
    fader(0, 0); ui_frame(); shot("mixer-ux-drum-locked-bank2");
    setup(V_PART, PART_303B); tap(0x5D); CHECK(smc.locked);
    uint8_t original_page = smc.lock_page;
    set_view(V_HOME); build_pages(); tap(0x61);
    CHECK(smc.knob_bank == 1 && ui.view == V_HOME && smc.lock_page == original_page);
    setup(V_MIX,PART_909); *page_of(V_MIX)=1; build_pages();tap(0x5D);
    *page_of(V_MIX)=3;build_pages();tap(0x2F);
    CHECK(smc_target(0).a == T_909 && smc_target(0).b == 8);
    setup(V_MIX,PART_909);*page_of(V_MIX)=1;build_pages();tap(0x5D);
    ui.part=PART_808;*page_of(V_MIX)=2;build_pages();tap(0);
    CHECK(ui.part==PART_909 && drum_mix_page()==PART_909 && ui.rec);
    CHECK(smc_target(0).a==T_909);
    setup(V_HOME,PART_303B);clip_grid_mode=1;ck.pg=0;
    tap(7);CHECK((ck.pg&3u)==1);tap(7);CHECK((ck.pg&3u)==2);
    tap(15);CHECK((ck.pg&3u)==1);tap(31);CHECK((ck.pg&3u)==2);
    uint8_t stage=ck.pg;tap(0x61);CHECK(ck.pg==stage);
    puts("PASS items37/39: independent banks, eighth-strip cycling, unique knobs and locked recapture");
}
static void item45(void)
{
    setup(V_HOME, PART_303B); pref_t level = smc_target(0);
    pref_set(level, 100); fader(0, 20); tap(16); CHECK(part_muted(PART_909));
    tap(16); CHECK(!part_muted(PART_909)); CHECK(pref_get(level) == 20); CHECK(smc.picked & 1u);
    fader(0,21); CHECK(pref_get(level)==21);
    setup(V_HOME, PART_303B); level=smc_target(0); pref_set(level,99);
    tap(16); tap(16); CHECK(pref_get(level)==99); CHECK(!(smc.picked & 1u));
    setup(V_HOME, PART_303B); fader(7,0); int filter=pref_get(smc_target(7));
    tap(23); tap(23); CHECK(pref_get(smc_target(7))==filter);
    CHECK(pref_get(PR(R_ENG,T_FX,0,FX_VOLUME))>0);
    setup(V_MIX, PART_909); *page_of(V_MIX)=1; build_pages();
    level=smc_target(0); pref_set(level,100); fader(0,35); tap(16); tap(16);
    CHECK(pref_get(level)==35 && (smc.picked&1u));
    for(int which=0;which<2;which++) for(int raw=0;raw<=20;raw+=20) {
        setup(V_HOME,PART_303B); int strip=5+which;
        level=smc_target(strip); const x0x_param_t *d=pref_desc(level);
        pref_set(level,d->max); fader(strip,raw);
        /* Leave the mapped FX-page peek: its four rows own momentary FX. */
        smc_peek_cancel();set_view(V_HOME);build_pages();
        tap(16+strip);tap(16+strip);
        CHECK(pref_get(level)==(raw*d->max+63)/127 && (smc.picked&(1u<<strip)));
        fader(strip,raw+1);CHECK(pref_get(level)==((raw+1)*d->max+63)/127);
    }
    /* Actual keyboard fader scan: moving while muted adopts one value and
     * the first movement after Unmute must not need another pickup. */
    setup(V_HOME,PART_303B);smk_fbank=0;fm1_in.frames=300;fm1_in.buttons=0;
    for(int i=0;i<4;i++) {pref_set(PR(R_ENG,T_MIX,part_at(i),MX_LEVEL),100);fm1_in.fader[i]=20;}
    smk_faders();fm1_in.fader[0]=100;smk_faders();
    mute_part(PART_909);fm1_in.fader[0]=30;smk_faders();
    CHECK(pref_get(PR(R_ENG,T_MIX,PART_909,MX_LEVEL))==30);
    mute_part(PART_909);fm1_in.fader[0]=31;smk_faders();
    CHECK(pref_get(PR(R_ENG,T_MIX,PART_909,MX_LEVEL))==31);
    puts("PASS item45: exact matching physical level adopted once; unknown and filter positions protected");
}
static void item46_20(void)
{
    setup(V_HOME,PART_303B); seq.bpm=120; seq.clk_src=SEQ_CLK_INT; seq.send_clock=0;
    render_audio(1); CHECK(!seq.metro_view);
    fader(0,0); smc_ui_draw_peer(); shot("mixer-ux-stopped-pickup");
    seq_start(&seq);render_audio(1);CHECK(seq.metro_view==1); CHECK(smc_led(0x5D));
    smc_ui_draw_peer();  shot("mixer-ux-beat-pickup-lit"); uint16_t lit[240*240];memcpy(lit,fb,sizeof fb);
    render_audio(FS/4);CHECK(seq.metro_view>=16); smc_ui_draw_peer();shot("mixer-ux-beat-pickup-dim");
    CHECK(memcmp(lit,fb,sizeof fb));
    unsigned phase=seq.metro_view; seq_stop(&seq);render_audio(1);CHECK(!seq.metro_view);
    render_audio(FS); seq_continue(&seq);render_audio(1);CHECK(seq.metro_view==phase);
    seq_stop(&seq);render_audio(1);seq.clk_src=SEQ_CLK_EXT;seq_start(&seq);render_audio(1);CHECK(!seq.metro_view);
    seq_midi_rt(&seq,0xF8);render_audio(1);CHECK(seq.metro_view==1);
    render_audio(FS);CHECK(!seq.metro_view);
    /* Header must not read a per-track playhead. */
    smc_ui_cancel();ui.msg_until=0;eng_step[TRK_DRUM]=0;draw_header();memcpy(lit,fb,sizeof fb);
    eng_step[TRK_DRUM]=3;draw_header();CHECK(!memcmp(lit,fb,sizeof fb));
    puts("PASS items46/20: sample-clock pickup and header, Start/Continue, STOP/WAIT and accepted external ticks");
}

/* Target-branch conflict coverage: both header paths and page-derived lock kits. */
static void port_boundaries(void)
{
    setup(V_HOME, PART_303B); clip_grid_mode=1;
    CHECK(!strcmp(smc_button_caption(7), "NEXT"));
    CHECK(!strcmp(smc_button_caption(15), "PREV"));
    CHECK(!strcmp(smc_button_caption(31), "NEXT"));
    CHECK(!strcmp(smc_button_caption(23), "M"));
    smc_ui_draw_peer(); shot("mixer-ux-clip-prev-next");
    uint8_t stage=ck.pg, bank=smc.knob_bank;
    tap(0x61); CHECK(smc.knob_bank != bank && ck.pg == stage);
    smc_ui_draw_peer(); shot("mixer-ux-clip-knob-bank2");
    tap(0x60); CHECK(smc.knob_bank == bank && ck.pg == stage);
    ui.overlay=O_ASK; tap(7); CHECK(ck.pg==stage);
    ui.overlay=O_NONE; clip_grid_mode=0; tap(0x5D); CHECK(smc.locked);
    clip_grid_mode=1; tap(7); CHECK(ck.pg==stage);
    CHECK(strcmp(smc_button_caption(7), "NEXT"));
    for(int kit=0;kit<NKIT;kit++) {
        setup(V_MIX, PART_909); *page_of(V_MIX)=0;build_pages();tap(0x5D);
        smc_page_set(kit+1); CHECK(smc_drum_mix()==kit);
        set_view(V_HOME);ui.part=PART_BRK;build_pages();
        pref_t captured=smc_target(0); tap(0);
        CHECK(ui.part==kit && ui.view==V_MIX && drum_mix_page()==kit);
        CHECK(smc.locked && smc.lock_page==kit+1 && smc_same(captured,smc_target(0)));
        smc_ui_cancel();ui.msg_until=0;ui_frame();shot(kit?"mixer-ux-locked-808":"mixer-ux-locked-909");
        set_view(V_HOME);ui.part=PART_BRK;build_pages();tap(25);
        CHECK(ui.part==kit && ui.view==V_MIX && drum_mix_page()==kit && ui.sel[kit]==1);
    }
    const uint8_t phases[]={0,1,4,5,16,17,32};
    for(int v=0;v<2;v++) {
        setup(v ? V_PART : V_HOME, PART_303B);
        seq.playing=1; ui.msg_until=0;
        for(unsigned i=0;i<sizeof phases;i++) {
            seq.metro_view=phases[i]; ui_frame();
            uint16_t colour=!phases[i]?C_DIM:phases[i]<=4?C_WHITE:C_HI;
            CHECK(fb[9*240+(v?176:218)] == (uint16_t)((colour<<8)|(colour>>8)));
            if(phases[i]) CHECK(smc_led(0x5D)==(phases[i]<=16));
            if(phases[i]==1 || phases[i]==17) shot(v ? (phases[i]==1?"mixer-ux-part-beat-lit":"mixer-ux-part-beat-dim") : (phases[i]==1?"mixer-ux-header-beat-lit":"mixer-ux-header-beat-dim"));
        }
    }
    puts("PASS port boundaries: clip captions/banks/guards, active locked kits and both header beat paths");
}

int main(void)
{
    CHECK(X0X_ORIGINAL_SYNTH==1 && PROJ_SND_REV==13);
    port_boundaries();item31();item37_39();item45();item46_20();
    printf("mixer-ux: PASS (%u checks); item41 mapping remains partial\n",checks);
    return 0;
}
