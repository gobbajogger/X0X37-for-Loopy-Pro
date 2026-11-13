/* ------- clip mode screen: the KEYS panel (the bottom strip) and the SCALE / CHORD / ARP page (main area) --- */
#include "app/smk_chord_detect.h"

/* Logical notes before ARP, STRUM and HUMAN. Raw keys also contain blocked keys and keys held through
 * all-notes-off, so use the sounding sources (or the arp's accepted list), never the physical key map. */
CK_FN static uint32_t ck_chord_state(ck_chord_input_t *chord)
{
    CK_BASE;
    uint8_t out[CK_MAXN];
    uint32_t arp = cp->on & CK_M_ARP, a, k, i, n, used = 0, latched = 0;
    chord->pcs = chord->upper_pcs = 0;
    chord->bass = chord->upper_bass = 0xFFu;
    for (a = 0; a < (arp ? cp->nk : CK_NKEY); a++) {
        k = arp ? cp->korder[a] : a;
        if (!arp && !CK_BIT(cp->snd, k))
            continue;
        for (i = 0, n = ck_key_notes(k, out); i < n && (!arp || used < CK_ARPN); i++, used++) {
            uint32_t note = out[i], bit = 1u << (note % 12u);
            if (note < chord->bass) {
                chord->upper_pcs = chord->pcs;
                chord->upper_bass = chord->bass;
                chord->bass = (uint8_t)note;
            } else if (note > chord->bass) {
                chord->upper_pcs |= (uint16_t)bit;
                if (note < chord->upper_bass)
                    chord->upper_bass = (uint8_t)note;
            }
            chord->pcs |= (uint16_t)bit;
            if (arp && (cp->fl & CK_F_LATCH) && !CK_BIT(cp->keys, k))
                latched = 1u;
        }
    }
    return latched;
}

/* No guessed chord for an unmatched set: show up to four pitch names, then + for the remaining notes. */
CK_FN static void ck_chord_text(const ck_chord_input_t *chord, char *text)
{
    ck_chord_result_t result;
    uint32_t pc, shown = 0;
    char *q = text;
    if (ck_chord_detect(chord, &result)) {
        ck_chord_format(&result, text, 32u);
        return;
    }
    for (pc = 0; pc < 12u; pc++)
        if (chord->pcs >> pc & 1u) {
            if (shown == 4u) {
                put_s(q, " +");
                return;
            }
            if (shown++)
                q = put_s(q, " ");
            q = put_s(q, ck_chord_note_name(pc));
        }
    if (!shown)
        put_s(text, "-");
}

static const uint8_t CK_WI[12] = {0, 0, 1, 1, 2, 3, 3, 4, 4, 5, 5, 6};   /* the white key of each pitch class */
#define CK_BLK(k) (0x54Au >> (k) % 12u & 1u)                             /* 1: a black key */
/* the left x of key k on a keyboard drawing with white keys ww wide from x0 */
CK_FN static int32_t ck_key_x(uint32_t k, int32_t x0, int32_t ww)
{
    return x0 + (int32_t)(k / 12u * 7u + CK_WI[k % 12u]) * ww + (int32_t)CK_BLK(k) * (ww - ww * 3 / 10);
}
/* a keyboard of n keys from key 0 (C), the white keys ww wide and h high, key k in col[k] */
CK_FN static void ck_kbd(int32_t x0, int32_t y, int32_t ww, int32_t h, uint32_t n, const uint16_t *col)
{
    uint32_t k, pass;
    for (pass = 0; pass < 2u; pass++)
        for (k = 0; k < n; k++) {
            int32_t x = ck_key_x(k, x0, ww), bw = ww * 3 / 5;
            if (CK_BLK(k) != pass)
                continue;
            if (pass) {
                cv_rect(x - 1, y, bw + 2, h * 3 / 5 + 1, C_BLACK);
                cv_rect(x, y, bw, h * 3 / 5, col[k]);
            } else {
                cv_rect(x, y, ww - 1, h, col[k]);
            }
        }
}
/* 1: the arp follows the MIDI clock (a tick in the last 0.5 s), 0: the internal tempo (for the header) */
uint8_t smk_ck_ext(void)
{
    return (uint8_t)(apc_clock_seen && fm1_ms - apc_clock_last_ms <= 500u);
}
/* The KEYS panel (ui.c draw_knobs, clip mode, no menu): the stage chips, one octave with the scale (the root
 * marked, the notes that sound lit), the scale and chord names, the arp line and its steps. *last: the last
 * drawing's signature; force: draw anyway. Return 1 when it drew on the canvas. */
uint8_t smk_ck_panel(uint32_t *last, uint32_t force)
{
    CK_BASE;
    CK_SET;
    uint8_t out[CK_MAXN];
    uint16_t col[12];
    char t[64], *q;
    ck_chord_input_t chord;
    uint32_t latched = ck_chord_state(&chord);
    uint32_t k, i, n, snd = 0, sig = 2166136261u, m = ck_mask(sp->ck_scale, sp->ck_root);
    uint32_t arp = cp->on & CK_M_ARP, rh = sp->ck_x2 & 15u, len = rh ? (CK_RHY_T[rh] & 15u) + 1u : 8u;
    uint32_t pos = arp && cp->nk && cp->step ? (cp->step - 1u) % len + 1u : 0u;
    for (k = 0; k < CK_NKEY; k++)
        if (CK_BIT(cp->snd, k) || (cp->nk && cp->korder[0] == k)) {
            n = ck_key_notes(k, out);
            for (i = 0; i < n && !arp; i++)
                snd |= 1u << out[i] % 12u;
        }
    if (cp->arp_note)
        snd |= 1u << (cp->arp_note - 1u) % 12u;
    for (i = 0; i < 12u; i++)
        sig = (sig ^ (&sp->ck_ch)[i]) * 16777619u;
    sig = (sig ^ (cp->on | (cp->fl & CK_F_LATCH) << 3 | snd << 4 | (uint32_t)cp->pg << 20)) * 16777619u;
    sig = (sig ^ ((uint32_t)chord.pcs | (uint32_t)chord.upper_pcs << 16)) * 16777619u;
    sig = (sig ^ (chord.bass | (uint32_t)chord.upper_bass << 8 | latched << 16)) * 16777619u;
    sig = (sig ^ pos) * 16777619u;
    if (!force && sig == *last)
        return 0u;
    *last = sig;
    cv_begin(240, 82, C_BLACK);
    cv_rect(0, 0, 240, 1, C_LINE);
    for (i = 0; i < 4u; i++) {                              /* SCALE CHORD ARP LATCH */
        uint32_t on = i < 3u ? cp->on >> i & 1u : cp->fl & CK_F_LATCH;
        uint16_t c = i < 3u ? CK_MODE_C[i] : C_AMB;
        int32_t x = 3 + (int32_t)i * 59;
        if (on)
            rbox(x, 4, 55, 13, 4, c);
        else
            rframe(x, 4, 55, 13, 4, dim(c, 7));
        text_c(x + 27, vc(&FONT_T, 4, 13), &FONT_T, i < 3u ? ck_nm(CK_MODE_N, i) : "LATCH", on ? C_BLACK : dim(c, 11));
        if ((cp->pg & 3u) == i + 1u)                        /* the page on the screen */
            cv_rect(x + 8, 18, 39, 2, C_WHITE);
    }
    for (i = 0; i < 12u; i++)                               /* one octave: the scale, the notes that sound */
        col[i] = snd >> i & 1u ? C_AMB : ck_in(m, i) ? (CK_BLK(i) ? RGB(140, 140, 140) : RGB(220, 220, 220))
                                                    : (CK_BLK(i) ? RGB(30, 30, 30) : RGB(70, 70, 70));
    ck_kbd(4, 23, 15, 36, 12u, col);
    cv_rect(ck_key_x(sp->ck_root, 4, 15) + 2, 23 + (CK_BLK(sp->ck_root) ? 15 : 31), 5, 3, CK_MODE_C[0]);
    q = put_s(put_s(put_s(t, ck_nm(CK_ROOT_N, sp->ck_root)), " "), ck_nm(CK_SCALE_N, sp->ck_scale));
    if (sp->ck_snap)
        put_s(q, " FILTER");
    cv_text(114, vc(&FONT_XS, 22, 12), &FONT_XS, t, cp->on & CK_M_SCALE ? CK_MODE_C[0] : C_DIM);
    {
        const felucca_font_t *font = &FONT_B;
        ck_chord_text(&chord, t);
        if (tw(font, t) > 122)
            font = &FONT_XS;
        if (tw(font, t) > 122)
            font = &FONT_T;
        cv_text(114, vc(font, 35, 15), font, t, chord.pcs ? CK_MODE_C[1] : C_DIM);
    }
    q = put_s(t, latched ? "LATCH " : "");
    if ((cp->on & CK_M_CHORD) && (sp->ck_x2 >> 4))
        ck_val_txt(CK_E_STRUM, sp->ck_x2 >> 4, put_s(q, "STRUM "));
    cv_text(114, vc(&FONT_T, 51, 10), &FONT_T, t, C_GRAY);
    q = put_s(put_s(put_s(put_s(t, ck_nm(CK_ORD_N, sp->ck_ord)), " "), ck_nm(CK_RATE_N, sp->ck_rate)), " X");
    q = put_s(put_s(put_i(q, sp->ck_oct + 1), " "), ck_val_txt(CK_E_GATE, sp->ck_gate, t + 52));
    if (sp->ck_x1 & 7u)
        q = put_s(put_s(q, " SW"), ck_val_txt(CK_E_SWING, sp->ck_x1 & 7u, t + 52));
    if (sp->ck_x1 >> 3 & 7u)
        q = put_i(put_s(q, " HUM"), sp->ck_x1 >> 3 & 7u);
    if (rh)
        put_s(put_s(q, " "), ck_nm(CK_RHY_N, rh));
    cv_text(4, vc(&FONT_XS, 61, 12), &FONT_XS, t, arp ? CK_MODE_C[2] : C_DIM);
    for (i = 0; i < len && arp; i++)                        /* the steps: hits, rests, the step now */
        cv_rect(4 + (int32_t)i * 8, 76, 5, 4, i + 1u == pos ? C_WHITE : ck_rhy_hit(rh, i) ? dim(CK_MODE_C[2], 9) : C_LINE);
    return 1u;
}
