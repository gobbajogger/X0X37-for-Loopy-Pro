/* Panel COPY/PASTE uses the five live part patterns selected with the target key.
 * The replacement question must keep those source slots even if playback cues a new bar. */
#define main x0x_host_main
#include "../../host/x0x_host.c"
#undef main

static int fails;
#define CHECK(c, msg) do { if (!(c)) { fails++; printf("FAIL: %s\n", msg); } } while (0)

static void tick(uint32_t n) { while (n--) run_ms(1); }
static void btn(int b, int down)
{
    if (down) held_btn |= 1u << b;
    else held_btn &= ~(1u << b);
    tick(1);
}
static void key(int k, int down)
{
    if (down) held_keys |= 1u << k;
    else held_keys &= ~(1u << k);
    tick(1);
}

int main(void)
{
    int p;
    boot();
    tick(300);
    for (p = 0; p < 5; p++)
        pattern_init(&proj.pat[p], 1, 2);
    sm_set(proj.pat[0].drum[0].hit[0], 0);
    sm_set(proj.pat[2].drum[0].hit[0], 7);
    sm_set(proj.pat[3].drum[1].hit[0], 9);
    sm_set(proj.pat[4].drum[0].hit[0], 12); /* destination requires confirmation */
    for (p = 0; p < NPARTS; p++)
        seq.ppat[p] = 0;

    btn(B_SAVE, 1);
    tick(300);
    CHECK(strcmp(ui.msg, "CHOOSE PASTE TARGET") == 0, "holding SAVE must not claim a snapshot was copied");
    seq.ppat[PART_909] = 2;
    seq.ppat[PART_808] = 3;
    key(WHITE_KEY[1], 1); /* empty P2: copy the now-live, mixed sources */
    key(WHITE_KEY[1], 0);
    CHECK(sm_get(proj.pat[1].drum[0].hit[0], 7), "P2 gets P3's 909 after source changed during hold");
    CHECK(!sm_get(proj.pat[1].drum[0].hit[0], 0), "P2 does not get the old P1 909");
    CHECK(sm_get(proj.pat[1].drum[1].hit[0], 9), "P2 gets P4's 808 from a different source slot");
    CHECK(strcmp(ui.msg, "PARTS PASTED TO P2") == 0, "paste feedback must not imply every part came from P1");

    key(WHITE_KEY[4], 1); /* P5 contains notes: ask before replacing */
    key(WHITE_KEY[4], 0);
    CHECK(ui.overlay == O_ASK && ui.ask_act == ACT_COPY, "occupied target asks before paste");
    seq.ppat[PART_909] = 0;
    seq.ppat[PART_808] = 0; /* a song cue while the question is open */
    btn(B_SAVE, 0);
    btn(B_SEL, 1);
    btn(B_SEL, 0);
    CHECK(sm_get(proj.pat[4].drum[0].hit[0], 7), "confirmation uses the selected P3 909 source");
    CHECK(!sm_get(proj.pat[4].drum[0].hit[0], 0), "confirmation does not switch to newly cued P1");
    CHECK(sm_get(proj.pat[4].drum[1].hit[0], 9), "confirmation keeps the selected P4 808 source");
    CHECK(!sm_get(proj.pat[4].drum[0].hit[0], 12), "confirmation replaces old destination notes");

    printf("copy_semantics: %s\n", fails ? "FAIL" : "ok");
    return fails != 0;
}
