/* SPDX-License-Identifier: GPL-3.0-only */
/* Host proof: Chicago SEND FX bridge maps Off/Comp/Crush/Delay/Hall onto Mix+fxbus
 * intentions; NOT_YET types never invent Ring/Cho/Phaser/Ensemble/Gate/EQ DSP. */
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
    CHECK(CHI_FX_TYPE == 23 && CHI_FX_EDIT1 == 24 && CHI_FX_EDIT2 == 25, "FX pots at 23..25");
    CHECK(CHI_FX_END == 26, "CHI_FX_END");
    CHECK(CHI_FX_NTYPES == 12, "12 Chicago types");
    CHECK(BASS303_NPARAMS == 23, "bass303 DSP pots stay 0..22");
    CHECK(28 >= CHI_FX_END, "NPARAMS_MAX 28 fits SEND FX (engine.h)");
}

static void test_supported_flags(void)
{
    CHECK(chi_fx_supported(CHI_FX_OFF), "Off supported (inactive)");
    CHECK(chi_fx_supported(CHI_FX_COMP), "Comp supported");
    CHECK(chi_fx_supported(CHI_FX_DECIM), "Crush/Decim supported");
    CHECK(chi_fx_supported(CHI_FX_SHORT_DLY), "Short Delay supported");
    CHECK(chi_fx_supported(CHI_FX_BPM_DLY), "BPM Delay supported");
    CHECK(chi_fx_supported(CHI_FX_HALL), "Hall supported");
    CHECK(chi_fx_is_not_yet(CHI_FX_EQ), "EQ NOT_YET");
    CHECK(chi_fx_is_not_yet(CHI_FX_RING), "Ring NOT_YET");
    CHECK(chi_fx_is_not_yet(CHI_FX_CHO), "Cho NOT_YET");
    CHECK(chi_fx_is_not_yet(CHI_FX_ENS), "Ens NOT_YET");
    CHECK(chi_fx_is_not_yet(CHI_FX_PHASER), "Phaser NOT_YET");
    CHECK(chi_fx_is_not_yet(CHI_FX_GATE_REV), "GateRev NOT_YET");
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
    CHECK(o.touch == 0, "Ring without clear_ny writes nothing");
    chicago_fx_map(CHI_FX_RING, 80, 80, 1, &o);
    CHECK(o.touch == CHI_FX_W_CLEAR_SENDS, "Ring with clear_ny only clears sends");
    CHECK(!(o.touch & CHI_FX_W_COMP), "Ring does not set Comp");
    CHECK(!(o.touch & CHI_FX_W_DIST), "Ring does not set Dist/Crush");
    chicago_fx_map(CHI_FX_CHO, 90, 90, 1, &o);
    CHECK(o.touch == CHI_FX_W_CLEAR_SENDS, "Cho clear_ny only clears sends");
    chicago_fx_map(CHI_FX_GATE_REV, 90, 90, 1, &o);
    CHECK(o.touch == CHI_FX_W_CLEAR_SENDS, "GateRev does not fake hall writes");
}

static void test_hall(void)
{
    chicago_fx_out_t o;
    chicago_fx_map(CHI_FX_HALL, 64, 100, 0, &o);
    CHECK(o.touch & CHI_FX_W_MX_REV, "Hall touches MX_REV");
    CHECK(o.mx_rev == 64, "Hall Edit1 → MX_REV");
    CHECK(o.mx_dly == 0, "Hall clears DLY send");
    CHECK(o.touch & CHI_FX_W_RV_DEC, "Hall touches RvDec");
    CHECK(o.fx_rv_decay == 100, "Hall Edit2 → RvDec");
}

static void test_bpm_delay(void)
{
    chicago_fx_out_t o;
    chicago_fx_map(CHI_FX_BPM_DLY, 80, 0, 0, &o);
    CHECK(o.mx_dly == 80 && o.mx_rev == 0, "BPM Delay Edit1 → MX_DLY");
    CHECK(o.fx_dl_time == 0, "BPM Edit2 0 → div 0");
    chicago_fx_map(CHI_FX_BPM_DLY, 80, 127, 0, &o);
    CHECK(o.fx_dl_time == 12, "BPM Edit2 max → div 12");
}

static void test_short_delay(void)
{
    chicago_fx_out_t o;
    chicago_fx_map(CHI_FX_SHORT_DLY, 50, 60, 0, &o);
    CHECK(o.fx_dl_time == 0, "Short Delay forces 1/32");
    CHECK(o.mx_dly == 50, "Short Delay send");
}

static void test_comp_crush(void)
{
    chicago_fx_out_t o;
    chicago_fx_map(CHI_FX_COMP, 90, 0, 0, &o);
    CHECK(o.fx_comp == 90, "Comp Edit1 → FX_COMP (global)");
    CHECK(o.touch & CHI_FX_W_CLEAR_SENDS, "Comp clears part sends");
    chicago_fx_map(CHI_FX_DECIM, 70, 0, 0, &o);
    CHECK(o.fx_dist == 7, "Decimator → Crush mode");
    CHECK(o.fx_drive == 70, "Decimator Edit1 → Drive");
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
    CHECK(strstr(CHI_FX_TYPE_N[CHI_FX_RING], "—") != 0, "Ring labeled NOT_YET");
    CHECK(strstr(CHI_FX_TYPE_N[CHI_FX_EQ], "—") != 0, "EQ labeled NOT_YET");
    CHECK(strcmp(CHI_FX_TYPE_N[CHI_FX_HALL], "Hall") == 0, "Hall plain name");
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
    test_comp_crush();
    test_factory_decode();
    test_labels_honest();
    if (fails) {
        printf("%d FAIL(s)\n", fails);
        return 1;
    }
    printf("PASS chicago_fx_bridge\n");
    return 0;
}
