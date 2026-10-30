#define SMC_UI_SEND_IMPLEMENTED 1
#define SMC_UI_MAIN unused_smc_ui_main
#include "smc_ui_test.c"
int plat_ble_smc_send(const uint8_t *m, uint8_t n) { (void)m; (void)n; return 1; }
static void dump(const char *tag)
{
    printf("%-14s", tag);
    for (int n = 0; n < 8; n++) { pref_t f = smc_target(n), k = smc_knob_target(n);
        printf(" |%d %s / %s", n, f.kind?pref_name(f):"-", k.kind?pref_name(k):"-"); }
    printf("\n");
}
static void tryfader(const char *tag, int n)
{
    pref_t r = smc_target(n); if (r.kind == R_NONE) { printf("%s f%d NONE\n", tag, n); return; }
    pref_set(r, 64); smc_frame();
    fader(n, 0); fader(n, 64); fader(n, 100);
    printf("%s f%d picked=%d val=%d (want 100)\n", tag, n, !!(smc.picked & (1u<<n)), pref_get(r));
}
int main(void)
{
    static const char *nm[] = {"PARTS","909MIX","808MIX","MASTER","WAVE"};
    setup(V_HOME, PART_303A); dump("HOME"); tryfader("HOME",0); tryfader("HOME",7);
    for (int p = 0; p < 5; p++) { setup(V_MIX, PART_303A); *page_of(V_MIX) = p; build_pages(); smc_frame();
        printf("title=%s drum_mix=%d\n", pg.title[cur_page()], smc_drum_mix()); dump(nm[p]); tryfader(nm[p],0); tryfader(nm[p],4); }
    setup(V_SEQ, PART_909); dump("SEQ909"); tryfader("SEQ909",0);
    setup(V_FX, PART_303A); dump("FX"); tryfader("FX",5);
    return 0;
}
