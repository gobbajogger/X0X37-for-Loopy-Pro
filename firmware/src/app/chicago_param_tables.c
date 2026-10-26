/* SPDX-License-Identifier: GPL-3.0-only */
/* One copy of Chicago Arp / SEND FX / aux pot tables.
 * engine.c is a separate -O2 unit from x0x.c (project/ui); headers used to emit
 * a static copy in each TU. Keep the labels here and extern them from the headers. */
#include "chicago_arp.h"
#include "chicago_fx.h"
#include "chicago_aux.h"

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
    "Off", "EQ—", "Comp", "Crush", "Ring—", "Cho—", "Ens—", "Phas—", "SDly", "BPM", "Hall", "Gate—"
};

const x0x_param_t CHI_FX_PARAMS[3] = {
    {"FxTyp", CHI_FX_NTYPES - 1, CHI_FX_OFF, CHI_FX_TYPE_N},
    {"FxEd1", 127, 64, 0},
    {"FxEd2", 127, 64, 0},
};

const x0x_param_t CHI_AUX_PARAMS[2] = {
    {"PBend", 127, 64, 0}, /* centre = no bend; 0 = −2 st, 127 = +2 st */
    {"Damp", 127, 0, 0},   /* 0..63 Off, 64..127 On (sustain) */
};
