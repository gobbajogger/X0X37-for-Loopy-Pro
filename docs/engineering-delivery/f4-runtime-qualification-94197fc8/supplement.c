/* Isolated F4 acceptance extension. Production queue/dispatcher/UI/engine.
 * The included fixture substitutes hardware and clip storage/grid painting.
 * No production source or registered assertion is changed. */
#define F4_CAPTURE_DRIVER 1
#include "/workspace/f4-qualification/source/gobbajogger-X0X37-for-Loopy-Pro-94197fc/tests/host/smc_remainder_test.c"

static int route;
static void begin_route(int view, int part, int r)
{
    setup(view,part); route=r;
    if(r) proj.set.bt|=PROJ_BT_SMC_USER;
    if(r==2) smc_ble_link(0);
    smc_poll(); CHECK(SMC_PEERS==1);
}
static void send_note(int note, int down)
{
    if(route==2) {
        uint32_t st=down?0x9f:0x8f;
        CHECK(smc_midi_host((down?9u:8u)|(st<<8)|((uint32_t)note<<16)|(down?127u<<24:0)));
        smc_poll();
    } else message((down?0x90:0x80)|(route?15:0),note,down?127:0);
}
static void route_tap(int note) { send_note(note,1);send_note(note,0); }
static void route_fader(int n, int v)
{
    if(route==2) { CHECK(smc_midi_host(11u|0xbfu<<8|(uint32_t)(40+n)<<16|(uint32_t)v<<24));smc_poll(); }
    else if(route) message(0xbf,40+n,v);
    else fader(n,v);
}
static void selected_kits(void)
{
    for(int r=0;r<3;r++) for(int kit=0;kit<NKIT;kit++) {
        begin_route(V_HOME,PART_BRK,r);
        screen_state_t keyboard=screen_state(); int key=24+part_position(kit);
        send_note(key,1); CHECK(smc.part==kit && smc.view==V_PART);
        send_note(key,1); CHECK(smc.part==kit && smc.view==V_PART);
        send_note(key,0);
        keyboard_unchanged(keyboard);
        route_fader(4,0); CHECK(smc.part==kit && smc.view==V_PART);
        /* Current #737 mapped-page peek temporarily borrows the keyboard view.
         * Match its actual restoration contract before another navigation edge. */
        now_ms+=SMC_PEEK_MS;smc_poll();keyboard_unchanged(keyboard);
        route_tap(key); CHECK(smc.view==V_MIX && smc_drum_mix()==kit);
        route_tap(25); CHECK(smc.sel[kit]==1 && smc_drum_mix()==kit);
        route_tap(25); CHECK(smc.view==V_MIX && smc_page_now()==0);
        keyboard_unchanged(keyboard);
        printf("PASS #31/#40 route=%d kit=%d first/held-repeat/repress/sound-return; keyboard invariant\n",r,kit);
    }
    /* MMC is an action stream; it has no physical release or held-FX chord. */
    for(int src=0;src<3;src++) for(int kit=0;kit<NKIT;kit++) {
        begin_route(V_HOME,PART_BRK,1);
        uint8_t cmd=(uint8_t)(4+4*part_position(kit));
        if(src==0) { uint8_t m[]={0xf0,0x7f,0x7f,6,cmd,0xf7};smc_ble_midi(m,sizeof m); }
        else smc_mmc_host(cmd,(uint8_t)(src-1));
        smc_poll(); CHECK(smc.view==V_PART && smc.part==kit);
        if(src==0) { uint8_t m[]={0xf0,0x7f,0x7f,6,cmd,0xf7};smc_ble_midi(m,sizeof m); }
        else smc_mmc_host(cmd,(uint8_t)(src-1));
        smc_poll(); CHECK(smc.view==V_MIX && smc_drum_mix()==kit);
        CHECK(!smc_hold_any(smc.held));
    }
    puts("PASS #40 User MMC central/USB/peripheral action compatibility, both kits");
}
static void popup_boundaries(void)
{
    for(int r=0;r<3;r++) for(int wrap=0;wrap<2;wrap++) {
        begin_route(V_HOME,PART_BRK,r);
        if(wrap) now_ms=UINT32_MAX-600u;
        route_fader(0,0);send_note(16,1);CHECK(smc_ui_active());
        now_ms+=4u*SMC_UI_MS;CHECK(smc_ui_active());
        send_note(16,0); CHECK(!smc_hold_any(smc.held));
        now_ms+=SMC_UI_MS-1u;CHECK(smc_ui_active());
        now_ms++;CHECK(!smc_ui_active());
        printf("PASS #67 route=%d wrap=%d held 4800ms, release+1199 active/+1200 expired\n",r,wrap);
    }
    for(int r=0;r<3;r++) for(int guard=0;guard<3;guard++) {
        begin_route(V_HOME,PART_BRK,r);route_fader(0,0);send_note(16,1);CHECK(smc_ui_active());
        if(guard==0) ui.help=1;
        if(guard==1) ask(ACT_SONG_CLR,0,"QUALIFICATION",0);
        if(guard==2) { held_btn=1u<<B_FX;input();held_btn=0;input(); }
        CHECK(!smc_ui_active());send_note(16,0);
        now_ms+=SMC_UI_MS+1;CHECK(!smc_ui_active());
    }
    puts("PASS #67 held popup yields to help/confirmation/keyboard navigation on all three input routes");
}
static void safe_frames(void)
{
    CHECK(NPAT==32 && sizeof proj.pat/sizeof proj.pat[0]==32);
    /* Do not call the unchanged capture_scenes(): it writes proj.pat[63]. */
    for(int kit=0;kit<NKIT;kit++) {
        char name[64];begin_route(V_HOME,PART_BRK,0);
        route_tap(24+part_position(kit));route_fader(0,0);ui_frame();
        snprintf(name,sizeof name,"kit%d-first-select",kit);shot(name);
        route_tap(24+part_position(kit));route_fader(0,0);ui_frame();
        snprintf(name,sizeof name,"kit%d-repeat-select",kit);shot(name);
    }
    begin_route(V_HOME,PART_BRK,0);clip_grid_mode=1;route_fader(0,0);ui_frame();shot("clip-strip8");
    route_tap(0x61);ui_frame();shot("clip-knob-bank");
    begin_route(V_HOME,PART_303B,0);pref_set(smc_target(0),100);route_fader(0,0);
    pref_set(PR(R_CLKSRC,0,0,0),SEQ_CLK_INT);seq.clk_src=SEQ_CLK_INT;seq.bpm=120;
    seq_start(&seq);render_audio(1);CHECK(seq.metro_view==1);
    ui_frame();shot("pickup-clock-lit");
    CHECK(!(smc.picked&1));uint8_t original_last=smc.last[0];
    render_audio(FS/4);CHECK(seq.metro_view>16);CHECK(!(smc.picked&1) && smc.last[0]==original_last);
    ui_frame();shot("pickup-clock-dim");
    route_tap(0x5d);CHECK(smc.locked);ui_frame();shot("locked-clock-dim");
    seq_stop(&seq);render_audio(1);CHECK(seq.metro_view==0);CHECK(smc_led(0x5d));
    ui_frame();shot("locked-stopped");
    begin_route(V_HOME,PART_303B,0);pref_set(PR(R_CLKSRC,0,0,0),SEQ_CLK_EXT);seq.clk_src=SEQ_CLK_EXT;
    pref_set(smc_target(0),100);route_fader(0,0);seq_start(&seq);render_audio(1);
    CHECK(seq.metro_view==0);render_audio(FS/2+1); /* existing half-second WAIT deadline */
    CHECK(seq_clock_waiting(&seq));ui_frame();shot("pickup-external-wait");
    CHECK(seq_midi_rt(&seq,0xf8));render_audio(1);CHECK(seq.metro_view==1);
    ui_frame();shot("pickup-external-tick");
    render_audio(FS);CHECK(seq_clock_waiting(&seq) && seq.metro_view==0);CHECK(!(smc.picked&1));
    ui_frame();shot("pickup-external-loss");
    begin_route(V_FX,PART_303B,0);send_note(0,1);ui_frame();shot("fx-held-before-arrow");
    route_tap(0x63);CHECK(smc.fx_strip.map[0][0][0]==SMCFX_DLY);ui_frame();shot("fx-held-after-arrow");
    send_note(0,0);ui_frame();shot("fx-release");
    puts("PASS #46 actual audio-clock pickup scenes and loss, no synthetic physical touch; safe 240x240 production renders");
}
int main(void)
{
    setvbuf(stdout,0,_IONBF,0);
    selected_kits();popup_boundaries();safe_frames();
    printf("f4-supplement: %u checks PASS\n",checks);return 0;
}
