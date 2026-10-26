/* SPDX-License-Identifier: GPL-3.0-only */
/* Chicago Fx Type / Edit 1 / Edit 2 — SEND bridge + Crush/Ring/EQ/Cho/Ens/Phaser/
 * SDly/BPM/Hall/GateRev insert (FCDC-205).
 *
 * Chicago AU is a per-voice insert strip (12 algorithms). Product audio is a
 * shared T_FX fxbus + per-part T_MIX sends, plus Crush / Ring / EQ / Cho / Ens /
 * Phaser / Short Delay / BPM Delay / Hall / GateRev inserts on the 303 dry voice
 * (chicago_crush.h / chicago_ring.h / chicago_eq.h / chicago_cho.h /
 * chicago_ens.h / chicago_phaser.h / chicago_sdly.h / chicago_bpmdly.h /
 * chicago_hall.h / chicago_gaterev.h). All 12 AU types are supported (Off/Comp
 * SEND; the rest insert).
 *
 * Supported SEND / master stage (labeled SEND FX in the UI):
 *   Off, Comp
 * Supported INSERT (per-voice DSP, not Mix theft):
 *   Crush (Decimator), Ring Mod, EQ, Cho/Flg, Ensemble, Phaser, Short Delay,
 *   BPM Delay, Hall, Gate Rev
 *
 * Stored in sound_t after bass303 DSP pots (CHI_FX_*). Not consumed by
 * bass303_set. Global Comp theft is intentional and labeled.
 * Miami wobble stays on WBL1 — not mapped here. */
#pragma once
#include <stdint.h>
#include "../dsp/x0x_param.h"

enum {
    /* After Chicago Arp 23..26 (CHI_ARP_NPARAMS_END == 27). Do not retarget arp/ADSR. */
    CHI_FX_TYPE = 27,
    CHI_FX_EDIT1,
    CHI_FX_EDIT2,
    CHI_FX_END /* SEND FX last index+1; Chicago aux (PBend/Damp) continues at 30..31 */
};

/* Canonical Chicago Type indices (AU Fx Type 0..11). */
enum {
    CHI_FX_OFF = 0,
    CHI_FX_EQ,
    CHI_FX_COMP,
    CHI_FX_DECIM, /* Crush — per-voice insert (chicago_crush.h) */
    CHI_FX_RING,
    CHI_FX_CHO,
    CHI_FX_ENS,
    CHI_FX_PHASER,
    CHI_FX_SHORT_DLY,
    CHI_FX_BPM_DLY,
    CHI_FX_HALL,
    CHI_FX_GATE_REV,
    CHI_FX_NTYPES
};

/* Which Mix / fxbus fields a map write touches. */
enum {
    CHI_FX_W_MX_REV = 1u << 0,
    CHI_FX_W_MX_DLY = 1u << 1,
    CHI_FX_W_COMP = 1u << 2,
    CHI_FX_W_DIST = 1u << 3,
    CHI_FX_W_DRIVE = 1u << 4,
    CHI_FX_W_DL_TIME = 1u << 5,
    CHI_FX_W_DL_LVL = 1u << 6,
    CHI_FX_W_RV_DEC = 1u << 7,
    CHI_FX_W_RV_LVL = 1u << 8,
    CHI_FX_W_CLEAR_SENDS = 1u << 9 /* MX_REV=MX_DLY=0 for this 303 part */
};

typedef struct {
    uint16_t touch; /* CHI_FX_W_* */
    uint8_t mx_rev, mx_dly;
    uint8_t fx_comp, fx_dist, fx_drive;
    uint8_t fx_dl_time, fx_dl_level;
    uint8_t fx_rv_decay, fx_rv_level;
} chicago_fx_out_t;

/* UI names: Crush/Ring/EQ/Cho/Ens/Phas/SDly/BPM/Hall/Gate are plain insert names.
 * Page title SEND FX still covers Comp; remaining types are insert.
 * Tables in chicago_param_tables.c (one copy for engine.o + x0x.o). */
extern const char *const CHI_FX_TYPE_N[CHI_FX_NTYPES];

extern const x0x_param_t CHI_FX_PARAMS[3];

static inline int chi_fx_is_insert(uint8_t type)
{
    return type == CHI_FX_DECIM || type == CHI_FX_RING || type == CHI_FX_EQ ||
           type == CHI_FX_CHO || type == CHI_FX_ENS || type == CHI_FX_PHASER ||
           type == CHI_FX_SHORT_DLY || type == CHI_FX_BPM_DLY || type == CHI_FX_HALL ||
           type == CHI_FX_GATE_REV;
}

static inline int chi_fx_supported(uint8_t type)
{
    switch (type) {
    case CHI_FX_OFF:
    case CHI_FX_COMP:
    case CHI_FX_DECIM:     /* insert — chicago_crush.h */
    case CHI_FX_RING:      /* insert — chicago_ring.h */
    case CHI_FX_EQ:        /* insert — chicago_eq.h */
    case CHI_FX_CHO:       /* insert — chicago_cho.h */
    case CHI_FX_ENS:       /* insert — chicago_ens.h */
    case CHI_FX_PHASER:    /* insert — chicago_phaser.h */
    case CHI_FX_SHORT_DLY: /* insert — chicago_sdly.h */
    case CHI_FX_BPM_DLY:   /* insert — chicago_bpmdly.h */
    case CHI_FX_HALL:      /* insert — chicago_hall.h */
    case CHI_FX_GATE_REV:  /* insert — chicago_gaterev.h */
        return 1;
    default:
        return 0;
    }
}

static inline int chi_fx_is_not_yet(uint8_t type)
{
    return type < CHI_FX_NTYPES && !chi_fx_supported(type);
}

/* Factory / AU 0..255 → Type 0..11 (round). */
static inline uint8_t chi_fx_type_from_byte(uint8_t v)
{
    return (uint8_t)(((uint32_t)v * (CHI_FX_NTYPES - 1u) + 127u) / 255u);
}

static inline uint8_t chi_fx_edit_from_byte(uint8_t v)
{
    return (uint8_t)(((uint32_t)v * 127u + 127u) / 255u);
}

static inline void chi_fx_decode_factory(const uint8_t pots[3], uint8_t out[3])
{
    out[0] = chi_fx_type_from_byte(pots[0]);
    out[1] = chi_fx_edit_from_byte(pots[1]);
    out[2] = chi_fx_edit_from_byte(pots[2]);
}

/* Map Type+edits → Mix/fxbus writes. Off returns touch=0 (inactive: do not
 * wipe factory Mix on load). NOT_YET returns CLEAR_SENDS only when clear_ny=1
 * (user just selected an unavailable type). Supported types always write. */
static inline void chicago_fx_map(uint8_t type, uint8_t e1, uint8_t e2, int clear_ny,
                                 chicago_fx_out_t *o)
{
    uint8_t send;
    o->touch = 0;
    o->mx_rev = o->mx_dly = 0;
    o->fx_comp = o->fx_dist = o->fx_drive = 0;
    o->fx_dl_time = o->fx_dl_level = 0;
    o->fx_rv_decay = o->fx_rv_level = 0;
    if (type >= CHI_FX_NTYPES)
        type = CHI_FX_OFF;
    if (type == CHI_FX_OFF)
        return;
    if (chi_fx_is_insert(type)) {
        /* Crush / Ring / EQ / Cho / Ens / Phaser / SDly / BPM / Hall / GateRev:
         * clear part sends so Mix does not smear the insert; audio path applies
         * DSP from sound_t FX pots. */
        o->touch = CHI_FX_W_CLEAR_SENDS;
        return;
    }
    if (chi_fx_is_not_yet(type)) {
        if (clear_ny)
            o->touch = CHI_FX_W_CLEAR_SENDS;
        return;
    }
    send = e1;
    (void)e2;
    switch (type) {
    case CHI_FX_COMP:
        /* Global master glue — not per-voice insert. */
        o->touch = CHI_FX_W_COMP | CHI_FX_W_CLEAR_SENDS;
        o->fx_comp = send;
        break;
    default:
        break;
    }
}

/* Part index for 303 voice 0/1 (PART_303A + voice). */
static inline int chi_fx_mix_part(int voice)
{
    return 2 + (voice & 1); /* PART_303A=2, PART_303B=3 */
}
