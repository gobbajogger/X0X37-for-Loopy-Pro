/* Exercise the real fader scan from main_fm1.c with saved targets on both banks. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

enum { R_ENG, T_MIX, T_FX, T_MST, MX_LEVEL, FX_RV_LEVEL = 1, FX_DL_LEVEL, MST_DJF = 3, PART_BRK = 4,
       SMK_BTN_FADER_BANK = 1 };
enum { PART_909=0, PART_808=1, PART_303A=2, PART_303B=3, NPARTS=5 };
#include "part_order.inc"
static const char *part_s(int p) { static const char *const n[5]={"909","808","MIA","CHI","BRK"};return n[p]; }
typedef struct { uint8_t kind, a, b, c; } pref_t;
#define PR(k, a, b, c) ((pref_t){(k), (a), (b), (c)})
static struct { uint32_t buttons, frames; uint8_t fader[4]; } fm1_in;
static struct { uint32_t act_t; } ui;
static uint8_t smk_fbank, val[2][4], project_busy;
static unsigned writes[2][4], touches;
static char message[64];
static int ui_project_busy(void) { return project_busy; }
static uint32_t plat_ms(void) { return 1000u; }
static char *put_s(char *d, const char *s) { while (*s) *d++ = *s++; *d = 0; return d; }
static char *put_i(char *d, int n) { return d + sprintf(d, "%d", n); }
static void say(const char *a, const char *b) { snprintf(message, sizeof message, "%s%s", a, b ? b : ""); }
static unsigned slot(pref_t r)
{
    if (r.a == T_MIX) { static const unsigned position[5]={0,1,4,3,2};return position[r.b]; }
    if (r.a == T_FX) return r.c == FX_RV_LEVEL ? 5u : 6u;
    return 7u;
}
static int pref_get(pref_t r) { unsigned s = slot(r); return val[s / 4u][s % 4u]; }
static void pref_set(pref_t r, int n)
{
    unsigned s = slot(r);
    val[s / 4u][s % 4u] = (uint8_t)n;
    writes[s / 4u][s % 4u]++;
}
static void motion_touch(pref_t r) { (void)r; touches++; }
static void pref_value(pref_t r, char *num, char *unit) { sprintf(num, "%d", pref_get(r)); *unit = 0; }
/* smk_faders.inc shares mute-pickup state with ui.c; this fixture owns fader
 * pickup / bank / project-hold behaviour, not mute edges — seed zeros and a
 * never-muted volume_muted so -Werror compiles (same stub/seed class as
 * #453/#494 harness leftovers, different harness). */
static uint8_t smk_mute_intended[4], smk_mute_known, smk_mute_active, smk_mute_edit, smk_mute_bank;
static int volume_muted(pref_t r) { (void)r; return 0; }
#include "smk_faders.inc"

static int failed;
static void check(const char *name, int ok)
{
    printf("%s %s\n", ok ? "PASS" : "FAIL", name);
    failed += !ok;
}
static void bank(void)
{
    fm1_in.buttons = 1u << SMK_BTN_FADER_BANK;
    smk_faders();
    fm1_in.buttons = 0u;
    smk_faders();
}
int main(void)
{
    val[0][0] = 100;              /* 909 level: distant from physical 10 */
    val[1][0] = 80;               /* Miami A level, fifth displayed part */
    val[1][1] = 50;               /* REV return */
    val[1][3] = 64;               /* FILTER off at the centre */
    fm1_in.fader[0] = 10;
    fm1_in.fader[3] = 100;
    fm1_in.frames = 100;
    smk_faders();                 /* settling samples never take over */
    fm1_in.frames = 200;
    fm1_in.fader[0] = 11;
    smk_faders();
    check("distant 909 target stays saved after a one-count movement", val[0][0] == 100 && !writes[0][0]);
    check("pickup shows direction", strcmp(message, "PICK F1 UP") == 0);
    fm1_in.fader[0] = 98;
    smk_faders();
    check("within two counts takes over", val[0][0] == 98 && writes[0][0] == 1);
    fm1_in.fader[0] = 97;
    smk_faders();
    check("after pickup, normal absolute movement works", val[0][0] == 97 && writes[0][0] == 2);
    bank();                       /* bank B starts from the current positions */
    fm1_in.fader[0] = 96;
    smk_faders();
    check("bank B distant target waits", val[1][0] == 80 && !writes[1][0]);
    check("bank B hints downward movement", strcmp(message, "PICK F1 DOWN") == 0);
    fm1_in.fader[0] = 79;
    smk_faders();
    check("crossing fifth displayed part target takes over", val[1][0] == 79 && writes[1][0] == 1);
    fm1_in.fader[1] = 48;
    smk_faders();
    check("equal-value tolerance takes over REV return", val[1][1] == 48 && writes[1][1] == 1);
    fm1_in.fader[3] = 99;
    smk_faders();
    check("bipolar filter remains off before centre crossing", val[1][3] == 64 && !writes[1][3]);
    fm1_in.fader[3] = 63;
    smk_faders();
    check("bipolar filter takes over across centre", val[1][3] == 63 && writes[1][3] == 1);
    val[1][3] = 20;              /* another knob changes the active target */
    fm1_in.fader[3] = 62;
    smk_faders();
    check("knob target change re-arms pickup", val[1][3] == 20 && writes[1][3] == 1);
    fm1_in.fader[3] = 20;
    smk_faders();
    check("re-armed filter takes over at new target", val[1][3] == 20 && writes[1][3] == 2);
    check("accepted moves touched automation", touches == 6u);
    {
        uint8_t saved[2][4];
        unsigned before[2][4];
        memcpy(saved, val, sizeof saved);
        memcpy(before, writes, sizeof before);
        project_busy = 1;
        fm1_in.buttons = 1u << SMK_BTN_FADER_BANK;
        fm1_in.fader[0] = 120;
        smk_faders();
        check("project hold consumes faders and bank button without edits",
              smk_fbank == 1 && !memcmp(saved, val, sizeof saved) &&
              !memcmp(before, writes, sizeof before) && touches == 6u);
        project_busy = 0;
        smk_faders();
        check("held bank button and fader position do not replay after release",
              smk_fbank == 1 && !memcmp(before, writes, sizeof before));
        fm1_in.buttons = 0;
        fm1_in.fader[0] = 119;
        smk_faders();
        check("project hold re-arms pickup at the saved target", val[1][0] == 79 && writes[1][0] == before[1][0]);
        fm1_in.fader[0] = 79;
        smk_faders();
        check("fresh target crossing resumes fader writes", writes[1][0] == before[1][0] + 1 && touches == 7u);
    }
    return failed ? 1 : 0;
}
