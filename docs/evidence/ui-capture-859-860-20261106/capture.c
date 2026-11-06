/* Evidence-only host harness; production source remains unchanged. */
#define SMC_UI_MAIN smc_ui_fixture_main
#include "smc_ui_test.c"
static void capture(const char *name) {
    ui_frame(); shot(name);
    printf("FRAME %s view=%u overlay=%u playing=%u start=%u stop=%u undo=%d/%d msg=%s\n", name, ui.view, ui.overlay, seq.playing, seq.req_start, seq.req_stop, undo.cur, undo.n, ui.msg);
}
static void badges(int theme) {
    setup(V_HOME, PART_909); palette_set(theme);
    seq.playing=seq.song_on=seq.chain_a=seq.chain_b=0;
    for(int p=NPAT-2;p<NPAT;p++) pattern_init(&proj.pat[p],1,2);
    proj.set.accent_q7=127;
    sm_set(proj.pat[NPAT-1].drum[PART_909].hit[0],0);
    sm_set(proj.pat[NPAT-1].drum[PART_909].hit[0],8);
    sm_set(proj.pat[NPAT-2].drum[PART_909].hit[0],2);
    seq.ppat[PART_909]=NPAT-1; seq.pcue[PART_909]=NPAT-2;
    ui.msg_until=0;
    static project_t before; before=proj;
    capture(theme ? "02-badges-light" : "01-badges-dark");
    for(int i=0;i<8;i++) for(int y=8;y<=10;y++) {
        CHECK(fb[y*240+133+i]==swap16((0x11u&(1u<<i))?dim(PART_COL[PART_909],16):SOUND_FACE));
        CHECK(fb[y*240+165+i]==swap16((0x02u&(1u<<i))?dim(PART_COL[PART_909],9):SOUND_FACE));
    }
    CHECK(!memcmp(&before,&proj,sizeof proj));
    puts("PASS badge actual framebuffer: current {0,8}, queued {2}, project unchanged");
}
static void question(int controller) {
    setup(V_HOME,PART_909); palette_set(0);
    seq.playing=seq.req_start=seq.req_stop=seq.req_cont=0;
    static project_t before; before=proj;
    ask(ACT_CLEAR_PAT,0,"CLEAR THIS PATTERN?","ALL FIVE PARTS");
    capture(controller?"05-smc-question":"03-panel-question");
    if(controller) message(0x90,0x5E,127); else button_tap(B_PLAY);
    CHECK(ui.overlay==O_NONE); CHECK(seq.req_start&&!seq.req_stop);
    CHECK(!strcmp(ui.msg,"CANCELLED")); CHECK(!memcmp(&before,&proj,sizeof proj));
    capture(controller?"06-smc-play-cancelled":"04-panel-play-cancelled");
    if(controller) {message(0x90,0x5E,127); CHECK(seq.req_start&&!seq.req_stop); message(0x90,0x5E,0);}
    puts("PASS question PLAY: cancel, one start request, project unchanged");
}
static void open_reset(void) {
    message(0x90,6,127); message(0x90,24+part_position(PART_909),127);
    message(0x90,24+part_position(PART_909),0); message(0x90,6,0);
}
static void reset(void) {
    setup(V_HOME,PART_909); palette_set(0);
    pref_set(PR(R_ENG,T_909,0,0),20);
    int32_t block[512]; for(int i=0;i<20;i++) engine_render(block,256);
    undo_start(); proj.sound.v[T_MIX][PART_909][MX_LEVEL]=32;
    proj.sound.v[T_MIX][PART_909][MX_PAN]=55;
    static project_t before,expected; before=proj;
    sound_t factory; engine_sound_defaults(&factory);
    capture("07-before-reset"); open_reset(); CHECK(ui.overlay==O_ASK);
    capture("08-reset-confirmation"); message(0x90,6,127);
    CHECK(ui.overlay==O_NONE); CHECK(!memcmp(&before,&proj,sizeof proj));
    capture("09-reset-cancelled"); message(0x90,6,0);
    open_reset(); CHECK(ui.overlay==O_ASK); message(0x90,5,127);
    CHECK(ui.overlay==O_NONE);
    expected=before;
    for(int i=0;i<NVOICES_MAX;i++) memcpy(expected.sound.v[T_909][i],factory.v[T_909][i],NPARAMS_MAX);
    CHECK(!memcmp(&expected,&proj,sizeof proj)); capture("10-reset-applied");
    do_undo(0); CHECK(!memcmp(&before,&proj,sizeof proj)); capture("11-reset-undo");
    do_undo(1); CHECK(!memcmp(&expected,&proj,sizeof proj)); capture("12-reset-redo");
    puts("PASS reset: cancel preserves project; confirm changes only selected sound; undo/redo exact; mixer preserved");
}
int main(void) {badges(0);badges(5);question(0);question(1);reset();printf("CAPTURE PASS checks=%u\n",checks);return 0;}
