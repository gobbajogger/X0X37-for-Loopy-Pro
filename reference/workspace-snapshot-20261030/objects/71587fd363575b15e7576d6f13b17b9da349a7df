/* SPDX-License-Identifier: GPL-3.0-only */
/* Host proof: Chicago SEND FX bridge maps Off/Comp onto Mix+fxbus intentions;
 * Crush + Ring + EQ + Cho + Ens + Phaser + SDly + BPM + Hall + GateRev are
 * insert (clear sends only). */
#include <stdio.h>
#include <string.h>
#include "bass303.h"
#include "chicago_fx.h"

static int fails;
#define CHECK(c, ...)                                                                                                  \
    do {                                                                                                               \
        if (!(c)) {                                                                                                    \
            printf("FAIL " __VA_ARGS__);                                                                               \
            printf("\n");                                                                                              \
            fails++;                                                                                                   \
        }                                                                                                              \
    } while (0)

static void test_indices(void)
{
    CHECK(CHI_FX_TYPE == 27 && CHI_FX_EDIT1 == 28 && CHI_FX_EDIT2 == 29, "FX pots at 27..29 after arp");
    CHECK(CHI_FX_END == 30, "CHI_FX_END (aux continues at 30..31)");
    CHECK(CHI_FX_NTYPES == 12, "12 Chicago types");
    CHECK(BASS303_NPARAMS == 23, "bass303 DSP pots stay 0..22");
    CHECK(32 >= CHI_FX_END, "NPARAMS_MAX 32 fits SEND FX + aux");
}

static void test_supported_flags(void)
{
    CHECK(chi_fx_supported(CHI_FX_OFF), "Off supported (inactive)");
    CHECK(chi_fx_supported(CHI_FX_COMP), "Comp supported");
    CHECK(chi_fx_supported(CHI_FX_DECIM), "Crush/Decim supported");
    CHECK(chi_fx_is_insert(CHI_FX_DECIM), "Crush is insert");
    CHECK(chi_fx_supported(CHI_FX_SHORT_DLY), "Short Delay supported (insert)");
    CHECK(chi_fx_is_insert(CHI_FX_SHORT_DLY), "Short Delay is insert");
    CHECK(!chi_fx_is_not_yet(CHI_FX_SHORT_DLY), "Short Delay not NOT_YET");
    CHECK(chi_fx_supported(CHI_FX_BPM_DLY), "BPM Delay supported (insert)");
    CHECK(chi_fx_is_insert(CHI_FX_BPM_DLY), "BPM Delay is insert");
    CHECK(!chi_fx_is_not_yet(CHI_FX_BPM_DLY), "BPM Delay not NOT_YET");
    CHECK(chi_fx_supported(CHI_FX_HALL), "Hall supported (insert)");
    CHECK(chi_fx_is_insert(CHI_FX_HALL), "Hall is insert");
    CHECK(!chi_fx_is_not_yet(CHI_FX_HALL), "Hall not NOT_YET");
    CHECK(chi_fx_supported(CHI_FX_EQ), "EQ supported (insert)");
    CHECK(chi_fx_is_insert(CHI_FX_EQ), "EQ is insert");
    CHECK(!chi_fx_is_not_yet(CHI_FX_EQ), "EQ not NOT_YET");
    CHECK(chi_fx_supported(CHI_FX_RING), "Ring supported (insert)");
    CHECK(chi_fx_is_insert(CHI_FX_RING), "Ring is insert");
    CHECK(!chi_fx_is_not_yet(CHI_FX_RING), "Ring not NOT_YET");
    CHECK(chi_fx_supported(CHI_FX_CHO), "Cho supported (insert)");
    CHECK(chi_fx_is_insert(CHI_FX_CHO), "Cho is insert");
    CHECK(!chi_fx_is_not_yet(CHI_FX_CHO), "Cho not NOT_YET");
    CHECK(chi_fx_supported(CHI_FX_ENS), "Ens supported (insert)");
    CHECK(chi_fx_is_insert(CHI_FX_ENS), "Ens is insert");
    CHECK(!chi_fx_is_not_yet(CHI_FX_ENS), "Ens not NOT_YET");
    CHECK(chi_fx_supported(CHI_FX_PHASER), "Phaser supported (insert)");
    CHECK(chi_fx_is_insert(CHI_FX_PHASER), "Phaser is insert");
    CHECK(!chi_fx_is_not_yet(CHI_FX_PHASER), "Phaser not NOT_YET");
    CHECK(chi_fx_supported(CHI_FX_GATE_REV), "GateRev supported (insert)");
    CHECK(chi_fx_is_insert(CHI_FX_GATE_REV), "GateRev is insert");
    CHECK(!chi_fx_is_not_yet(CHI_FX_GATE_REV), "GateRev not NOT_YET");
}

static void test_off_inactive(void)
{
    chicago_fx_out_t o;
    chicago_fx_map(CHI_FX_OFF, 100, 100, 0, &o);
    CHECK(o.touch == 0, "Off does not touch Mix/fxbus (preserve factory Mix on load)");
}

static void test_not_yet_clear(void)
{
    chicago_fx_out_t o;
    chicago_fx_map(CHI_FX_RING, 80, 80, 0, &o);
    CHECK(o.touch == CHI_FX_W_CLEAR_SENDS, "Ring insert clears sends (no Mix theft)");
    CHECK(!(o.touch & CHI_FX_W_COMP), "Ring does not set Comp");
    CHECK(!(o.touch & CHI_FX_W_DIST), "Ring does not set Dist/Crush");
    chicago_fx_map(CHI_FX_DECIM, 70, 100, 0, &o);
    CHECK(o.touch == CHI_FX_W_CLEAR_SENDS, "Crush insert clears sends (no Mix theft)");
    CHECK(!(o.touch & CHI_FX_W_DIST), "Crush insert does not set Dist");
    CHECK(!(o.touch & CHI_FX_W_DRIVE), "Crush insert does not set Drive");
    chicago_fx_map(CHI_FX_EQ, 0, 127, 0, &o);
    CHECK(o.touch == CHI_FX_W_CLEAR_SENDS, "EQ insert clears sends (no Mix theft)");
    CHECK(!(o.touch & CHI_FX_W_COMP), "EQ does not set Comp");
    chicago_fx_map(CHI_FX_SHORT_DLY, 50, 60, 0, &o);
    CHECK(o.touch == CHI_FX_W_CLEAR_SENDS, "SDly insert clears sends (no Mix theft)");
    CHECK(!(o.touch & CHI_FX_W_MX_DLY), "SDly insert does not set Mix delay");
    chicago_fx_map(CHI_FX_BPM_DLY, 80, 60, 0, &o);
    CHECK(o.touch == CHI_FX_W_CLEAR_SENDS, "BPM insert clears sends (no Mix theft)");
    CHECK(!(o.touch & CHI_FX_W_MX_DLY), "BPM insert does not set Mix delay");
    chicago_fx_map(CHI_FX_HALL, 64, 100, 0, &o);
    CHECK(o.touch == CHI_FX_W_CLEAR_SENDS, "Hall insert clears sends (no Mix theft)");
    CHECK(!(o.touch & CHI_FX_W_MX_REV), "Hall insert does not set Mix reverb");
    chicago_fx_map(CHI_FX_CHO, 47, 16, 0, &o);
    CHECK(o.touch == CHI_FX_W_CLEAR_SENDS, "Cho insert clears sends (no Mix theft)");
    CHECK(!(o.touch & CHI_FX_W_MX_DLY), "Cho insert does not set Mix delay");
    chicago_fx_map(CHI_FX_GATE_REV, 28, 31, 0, &o);
    CHECK(o.touch == CHI_FX_W_CLEAR_SENDS, "GateRev insert clears sends (no Mix theft)");
    CHECK(!(o.touch & CHI_FX_W_MX_REV), "GateRev insert does not set Mix reverb");
    chicago_fx_map(CHI_FX_ENS, 20, 80, 0, &o);
    CHECK(o.touch == CHI_FX_W_CLEAR_SENDS, "Ens insert clears sends (no Mix theft)");
    CHECK(!(o.touch & CHI_FX_W_MX_DLY), "Ens insert does not set Mix delay");
    chicago_fx_map(CHI_FX_PHASER, 95, 64, 0, &o);
    CHECK(o.touch == CHI_FX_W_CLEAR_SENDS, "Phaser insert clears sends (no Mix theft)");
    CHECK(!(o.touch & CHI_FX_W_MX_DLY), "Phaser insert does not set Mix delay");
}

static void test_hall(void)
{
    chicago_fx_out_t o;
    chicago_fx_map(CHI_FX_HALL, 64, 100, 0, &o);
    CHECK(o.touch == CHI_FX_W_CLEAR_SENDS, "Hall is insert (clear sends)");
    CHECK(o.mx_rev == 0 && o.fx_rv_decay == 0 && o.fx_rv_level == 0, "Hall no Mix/fxbus reverb fields");
    chicago_fx_map(CHI_FX_HALL, 25, 17, 0, &o);
    CHECK(o.touch == CHI_FX_W_CLEAR_SENDS, "Hall Snail-ish edits still insert");
    CHECK(!(o.touch & CHI_FX_W_MX_REV), "Hall insert does not set Mix reverb");
}

static void test_bpm_delay(void)
{
    chicago_fx_out_t o;
    chicago_fx_map(CHI_FX_BPM_DLY, 80, 0, 0, &o);
    CHECK(o.touch == CHI_FX_W_CLEAR_SENDS, "BPM Delay is insert (clear sends)");
    CHECK(o.mx_dly == 0 && o.fx_dl_time == 0 && o.fx_dl_level == 0, "BPM Delay no Mix/fxbus delay fields");
    chicago_fx_map(CHI_FX_BPM_DLY, 80, 127, 0, &o);
    CHECK(o.touch == CHI_FX_W_CLEAR_SENDS, "BPM Delay max Edit2 still insert");
    CHECK(!(o.touch & CHI_FX_W_MX_DLY), "BPM insert does not set Mix delay");
}

static void test_short_delay(void)
{
    chicago_fx_out_t o;
    chicago_fx_map(CHI_FX_SHORT_DLY, 50, 60, 0, &o);
    CHECK(o.touch == CHI_FX_W_CLEAR_SENDS, "Short Delay is insert (clear sends)");
    CHECK(o.mx_dly == 0 && o.fx_dl_time == 0 && o.fx_dl_level == 0, "Short Delay no Mix/fxbus delay fields");
}

static void test_comp(void)
{
    chicago_fx_out_t o;
    chicago_fx_map(CHI_FX_COMP, 90, 0, 0, &o);
    CHECK(o.fx_comp == 90, "Comp Edit1 → FX_COMP (global)");
    CHECK(o.touch & CHI_FX_W_CLEAR_SENDS, "Comp clears part sends");
}

static void test_factory_decode(void)
{
    uint8_t in[3] = {0, 128, 255}, out[3];
    chi_fx_decode_factory(in, out);
    CHECK(out[0] == CHI_FX_OFF, "factory type 0 → Off");
    CHECK(out[1] == 64, "edit mid");
    CHECK(out[2] == 127, "edit max");
    in[0] = 255;
    chi_fx_decode_factory(in, out);
    CHECK(out[0] == CHI_FX_GATE_REV, "factory type max → GateRev");
}

static void test_labels_honest(void)
{
    CHECK(strcmp(CHI_FX_TYPE_N[CHI_FX_RING], "Ring") == 0, "Ring plain insert name");
    CHECK(strcmp(CHI_FX_TYPE_N[CHI_FX_DECIM], "Crush") == 0, "Crush plain insert name");
    CHECK(strcmp(CHI_FX_TYPE_N[CHI_FX_EQ], "EQ") == 0, "EQ plain insert name");
    CHECK(strcmp(CHI_FX_TYPE_N[CHI_FX_SHORT_DLY], "SDly") == 0, "SDly plain insert name");
    CHECK(strcmp(CHI_FX_TYPE_N[CHI_FX_BPM_DLY], "BPM") == 0, "BPM plain insert name");
    CHECK(strcmp(CHI_FX_TYPE_N[CHI_FX_HALL], "Hall") == 0, "Hall plain insert name");
    CHECK(strcmp(CHI_FX_TYPE_N[CHI_FX_CHO], "Cho") == 0, "Cho plain insert name");
    CHECK(strcmp(CHI_FX_TYPE_N[CHI_FX_ENS], "Ens") == 0, "Ens plain insert name");
    CHECK(strcmp(CHI_FX_TYPE_N[CHI_FX_PHASER], "Phas") == 0, "Phaser plain insert name");
    CHECK(strcmp(CHI_FX_TYPE_N[CHI_FX_GATE_REV], "Gate") == 0, "Gate plain insert name");
    CHECK(strcmp(CHI_FX_PARAMS[0].name, "FxTyp") == 0, "param name FxTyp");
}

int main(void)
{
    test_indices();
    test_supported_flags();
    test_off_inactive();
    test_not_yet_clear();
    test_hall();
    test_bpm_delay();
    test_short_delay();
    test_comp();
    test_factory_decode();
    test_labels_honest();
    if (fails) {
        printf("%d FAIL(s)\n", fails);
        return 1;
    }
    printf("PASS chicago_fx_bridge\n");
    return 0;
}
