/* SPDX-License-Identifier: GPL-3.0-only */
/* One copy of Chicago Arp / SEND FX / aux pot tables.
 * engine.c is a separate -O2 unit from x0x.c (project/ui); headers used to emit
 * a static copy in each TU. Keep the labels here and extern them from the headers.
 * chi_arp_run_set also lives here so midi_in does not keep two inlined copies. */
#include "chicago_arp.h"
#include "chicago_fx.h"
#include "chicago_aux.h"

#if defined(X0X_HOST) && !defined(__clang__)
__attribute__((noinline))
#else
__attribute__((minsize, noinline))
#endif
void chi_arp_run_set(chi_arp_run_t *r, const uint8_t *notes,
                                                        uint32_t n, uint8_t mode, uint8_t range,
                                                        uint8_t speed)
{
    uint32_t i, j;
    if (!r)
        return;
    if (!notes || n == 0) {
        chi_arp_run_clear(r);
        r->mode = mode;
        r->range = range;
        r->speed = speed;
        return;
    }
    if (n > CHI_ARP_MAXN)
        n = CHI_ARP_MAXN;
    r->n = 0;
    for (i = 0; i < n; i++) {
        uint8_t x = notes[i];
        uint32_t p = r->n++;
        for (; p && r->notes[p - 1u] > x; p--)
            r->notes[p] = r->notes[p - 1u];
        r->notes[p] = x;
        /* drop duplicates after insert */
        for (j = 0; j + 1u < r->n; j++)
            if (r->notes[j] == r->notes[j + 1u]) {
                uint32_t k;
                for (k = j + 1u; k + 1u < r->n; k++)
                    r->notes[k] = r->notes[k + 1u];
                r->n--;
                break;
            }
    }
    r->mode = mode;
    r->range = range;
    r->speed = speed;
    r->step = 0;
    r->to_next = 0;
    r->cur = -1;
}

const uint8_t CHI_ARP_RATE_IX[CHI_ARP_NSPEED] = {0, 1, 2, 3, 4, 5, 6, 8, 9, 11};

const char *const CHI_ARP_SWITCH_N[] = {"Off", "On"};
const char *const CHI_ARP_MODE_N[] = {"Down", "Up", "Up/Dn"};
const char *const CHI_ARP_RANGE_N[] = {"1 Oct", "2Octs", "Full"};
const char *const CHI_ARP_SPEED_N[] = {"1/2", "3/8", "1/3", "1/4", "3/16", "1/6", "1/8", "1/12",
                                        "1/16", "1/32"};

const x0x_param_t CHI_ARP_PARAMS[4] = {
    {"ArpSw", 1, CHI_ARP_OFF, CHI_ARP_SWITCH_N},
    {"ArpMd", CHI_ARP_NMODE - 1, CHI_ARP_UP, CHI_ARP_MODE_N},
    {"ArpRg", CHI_ARP_NRANGE - 1, CHI_ARP_OCT1, CHI_ARP_RANGE_N},
    {"ArpSp", CHI_ARP_NSPEED - 1, CHI_ARP_SPD_1_16, CHI_ARP_SPEED_N},
};

const char *const CHI_FX_TYPE_N[CHI_FX_NTYPES] = {
    "Off", "EQ", "Comp", "Crush", "Ring", "Cho", "Ens", "Phas", "SDly", "BPM", "Hall", "Gate"
}; /* Crush + Ring + EQ + Cho + Ens + Phaser + SDly + BPM + Hall + Gate = insert (plain names) */

const x0x_param_t CHI_FX_PARAMS[3] = {
    {"FxTyp", CHI_FX_NTYPES - 1, CHI_FX_OFF, CHI_FX_TYPE_N},
    {"FxEd1", 127, 64, 0},
    {"FxEd2", 127, 64, 0},
};

const x0x_param_t CHI_AUX_PARAMS[2] = {
    {"PBend", 127, 64, 0}, /* centre = no bend; 0 = −2 st, 127 = +2 st */
    {"Damp", 127, 0, 0},   /* 0..63 Off, 64..127 On (sustain) */
};
