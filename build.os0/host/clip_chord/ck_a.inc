/* --------------------------------------------- clip mode keybed (Loopy Pro) --- */
/* In clip mode the keybed plays MIDI notes to Loopy only: key -> SCALE -> CHORD -> ARP -> the CLIP KEYS
 * channel (GLO > MIDI) on USB and Bluetooth (apc_out). The logic is in smk_clip_keys.h.
 * The notes of a key come from its key number and the settings (ck_key_notes). A change that changes the
 * notes first ends the notes that sound, so a release always ends exactly the notes that its key sent.
 * A note that two keys hold ends when the last of the two keys goes up. No table of notes: key bit maps. */
typedef struct {
    uint8_t on, pg, fl, vel;          /* on: CK_M_* bits; pg: page and selected field; fl: CK_F_* bits; vel: the last
                                       * key velocity */
    uint8_t arp_note, ch, btn_prev, nk;   /* arp_note: note + 1 (0 = none); ch: the channel of all notes;
                                           * nk: keys in korder. ARP off: a strum (korder[0] the key, nk - 1
                                           * notes sent, acc the ms since the press); 0 = none */
    uint8_t tick, gate_left, ext_seen;
    volatile uint8_t off_req;         /* MIDI Stop, USB reset: all notes off (set in interrupts) */
    uint16_t step, last_ms;
    uint32_t acc;                     /* the internal arp clock (600000 = one tick) */
    uint32_t keys[2], snd[2];         /* bit k: key k is down / key k has notes that sound */
    uint8_t korder[8];                /* the keys of the arp, in press order (more keys: not in the arp) */
} ck_t;
static ck_t ck SMK_BIG;
void smk_ck_midi_stop(void) { ck.off_req = 1; }
/* a base register for ck (short code: no full address for each field) */
#include "fm1_opt.h"
#define CK_BASE ck_t *cp = &ck; FM1_BASE(cp)
#define CK_SET settings_t *sp = &proj.set
enum { CK_F_LATCH = 1u, CK_F_NRUSED = 2u, CK_F_BLE = 4u, CK_F_ACTIVE = 8u };
#define CK_BIT(map, k) (((map)[(k) >> 5] >> ((k) & 31u)) & 1u)

/* HUMAN timing: the note-ons that wait (main loop only). ck_qe: note | velocity << 7; ck_qt: the low 8 bits of
 * fm1_ms when it goes out. The channel is ck.ch: a channel change first sends all (ck_all_off). */
#define CK_QN 24u
static uint16_t ck_qe[CK_QN];
static uint8_t ck_qt[CK_QN], ck_qn;
static uint32_t ck_strum_keep[4];
static uint32_t ck_clip_live[4];
static void ck_q_run(uint32_t only);
__attribute__((minsize)) static void ck_send(uint32_t st, uint32_t d1, uint32_t d2)
{
    if ((st & 0xF0u) == 0x80u)
        ck_q_run(d1 + 1u);                          /* a note-off never goes before its note-on */
    apc_out((st >> 4) | st << 8 | d1 << 16 | d2 << 24);
}
/* send the note-ons that wait: only 0: the ones that are due (ck_q_due); note + 1: the ones of this note;
 * 0x100: all */
CK_FN static void ck_q_run(uint32_t only)
{
    uint32_t i, j, e;
    for (i = j = 0; i < ck_qn; i++) {
        e = ck_qe[i];
        if (only ? only > 0xFFu || (e & 127u) + 1u == only : ck_q_due(ck_qt[i], fm1_ms)) {
            apc_out(9u | (0x90u | ck.ch) << 8 | (e & 127u) << 16 | (e >> 7) << 24);
            continue;
        }
        ck_qe[j] = (uint16_t)e;
        ck_qt[j++] = ck_qt[i];
    }
    ck_qn = (uint8_t)j;
}
/* the notes of key k with the settings now */
CK_FN static uint32_t ck_key_notes(uint32_t k, uint8_t *out)
{
    CK_BASE;
    CK_SET;
    uint32_t c = cp->on & CK_M_CHORD;
    return ck_notes(CK_BASE_NOTE + k, ck_mask(sp->ck_scale, sp->ck_root), cp->on & CK_M_SCALE ? sp->ck_snap + 1u : 0u,
                    c ? (int32_t)sp->ck_chord : -1, c ? sp->ck_inv : 0u, out);
}
CK_FN static void ck_arp_stop_note(void)
{
    CK_BASE;
    if (cp->arp_note) {
        ck_send(0x80u | cp->ch, cp->arp_note - 1u, 0u);
        cp->arp_note = 0;
    }
}
/* a note-on: the key velocity, HUMAN moves it by up to 4 x HUMAN at random and sends it 0 to 4 x HUMAN ms late
 * (a full queue: at once) */
CK_FN static void ck_on(uint32_t note)
{
    CK_BASE;
    uint32_t late, v = ck_human(proj.set.ck_x1 >> 3 & 7u, (fm1_ms ^ note << 8) * 2654435761u, cp->vel, &late);
    if (!late || ck_qn >= CK_QN) {
        ck_send(0x90u | cp->ch, note, v);
        return;
    }
    ck_qe[ck_qn] = (uint16_t)(note | v << 7);
    ck_qt[ck_qn++] = (uint8_t)(fm1_ms + late);
}
/* Non-ARP CLIP KEYS owns only pitches actually triggered or queued.
 * Queued HUMAN notes are flushed by ck_send before their matching NoteOff. */
CK_FN static void ck_clip_on(uint32_t note)
{
    if (CK_BIT(ck_clip_live, note))
        return;
    ck_clip_live[note >> 5] |= 1u << (note & 31u);
    ck_on(note);
}
CK_FN static void ck_clip_off(uint32_t note)
{
    CK_BASE;
    if (!CK_BIT(ck_clip_live, note))
        return;
    ck_clip_live[note >> 5] &= ~(1u << (note & 31u));
    ck_send(0x80u | cp->ch, note, 0u);
}
/* key k: note on (st 0x90), note off (0x80), or note off that keeps the notes of the other sounding keys
 * (0x81: a key release) */
CK_FN static void ck_key_send(uint32_t k, uint32_t st)
{
    CK_BASE;
    uint8_t out[CK_MAXN];
    uint32_t keep[4] = {0, 0, 0, 0}, n, i, q;
    /* Private dispatch bits: single-key events compare all other owners;
     * whole-voice off/on compares only earlier visited keys (one MIDI event per pitch). */
    if (st & 3u)
        for (q = 0; q < CK_NKEY; q++)
            if (q != k && CK_BIT(cp->snd, q) && (!(st & 2u) || q < k))
                for (i = 0, n = ck_key_notes(q, out); i < n; i++)
                    keep[out[i] >> 5] |= 1u << (out[i] & 31u);
    for (i = 0, n = ck_key_notes(k, out); i < n; i++)
        if (!CK_BIT(keep, out[i])) {
            if (st & 0x10u)
                ck_clip_on(out[i]);
            else
                ck_clip_off(out[i]);
        }
}
/* all keys that sound: note off (0x80) or on again (0x90), round a setting change */
CK_FN static void ck_voice(uint32_t st)
{
    CK_BASE;
    uint32_t k;
    /* Full revoice/panic supersedes any unfinished non-ARP strum. */
    if (!(cp->on & CK_M_ARP))
        cp->nk = 0;
    for (k = 0; k < CK_NKEY; k++)
        if (CK_BIT(cp->snd, k))
            ck_key_send(k, st | 3u);
}
/* All notes off: the keys, the arp note, the arp list */
CK_FN static void ck_all_off(void)
{
    CK_BASE;
    ck_q_run(0x100u);                                       /* (the notes that wait go out first) */
    ck_voice(0x80u);
    cp->snd[0] = cp->snd[1] = 0;
    ck_arp_stop_note();
    cp->nk = 0;
    memset(ck_clip_live, 0, sizeof ck_clip_live);
}

/* Project replacement ends notes using the old scale/chord and channel. Calibration,
 * Bluetooth ownership and Loopy's clip feedback are device state and are left in place. */
CK_FN void smk_project_reset(void)
{
    uint32_t i;
    /* Project reset discards HUMAN notes that never reached MIDI. Releasing
     * those queued pitches would cut unrelated Loopy voices sharing a note. */
    for (i = 0; i < ck_qn; i++) {
        uint32_t note = ck_qe[i] & 127u;
        ck_clip_live[note >> 5] &= ~(1u << (note & 31u));
        if (ck.arp_note == note + 1u)
            ck.arp_note = 0;
    }
    ck_qn = 0;
    ck_all_off();
    memset(&ck, 0, sizeof ck);
    ck.keys[0] = ((const volatile uint32_t *)&fm1_in.keys)[0];
    ck.keys[1] = ((const volatile uint32_t *)&fm1_in.keys)[1] & 0x1Fu;
    ck.btn_prev = 0x7Fu;
    ck.last_ms = (uint16_t)fm1_ms;
    ck_start_req = 0;
    for (i = 0; i < SMK_APC40_PAD_COUNT; i++) {
        if (clip_pad_active && clip_pad_note[i] != 0xFFu)
            apc_note(clip_pad_note[i], 0u);
        clip_pad_note[i] = 0xFFu;
    }
    clip_pad_keys = fm1_in.pnotes;
    clip_pad_active = clip_grid_mode;
    knob_bank_prev = (uint8_t)((fm1_in.buttons >> SMK_BTN_KNOB_BANK) & 1u);
    was_on = 0u;                                    /* redraw clip-key settings after the swap */
}
/* Finish the previous strum before snapshotting the tones owned by held keys.
 * Keep the snapshot through this strum: a previously held key might be released
 * before a shared delayed note, but its sounding pitch is now owned by this key. */
CK_FN static void ck_strum_snapshot(uint32_t key)
{
    CK_BASE;
    uint8_t out[CK_MAXN];
    uint32_t q, i, n;
    for (q = 0; q < 4u; q++)
        ck_strum_keep[q] = 0;
    for (q = 0; q < CK_NKEY; q++)
        if (q != key && CK_BIT(cp->snd, q))
            for (i = 0, n = ck_key_notes(q, out); i < n; i++)
                ck_strum_keep[out[i] >> 5] |= 1u << (out[i] & 31u);
}
/* STRUM (ARP off): the notes of the strum key, low to high, one each STRUM x 10 ms after the press.
 * dt: the ms since the last call (a big dt sends all the notes that wait). */
CK_FN static void ck_strum(uint32_t dt)
{
    CK_BASE;
    uint8_t out[CK_MAXN];
    uint32_t n, i, j, x, gap = (proj.set.ck_x2 >> 4) * 10u;
    if (!cp->nk)
        return;
    n = ck_key_notes(cp->korder[0], out);
    for (i = 1; i < n; i++)                                 /* (an inversion puts low notes up) */
        for (j = i, x = out[i]; j && out[j - 1u] > x; j--) {
            out[j] = out[j - 1u];
            out[j - 1u] = (uint8_t)x;
        }
    cp->acc += dt;
    for (; cp->nk <= n && (cp->nk - 1u) * gap <= cp->acc; cp->nk++)
        if (!CK_BIT(ck_strum_keep, out[cp->nk - 1u]))
            ck_clip_on(out[cp->nk - 1u]);
    if (cp->nk > n)
        cp->nk = 0;
}
CK_FN static void ck_press(uint32_t k, uint32_t vel)
{
    CK_SET;
    CK_BASE;
    cp->vel = (uint8_t)(vel ? vel : 1u);
    if (!(cp->on & CK_M_ARP)) {
        if ((sp->ck_x2 >> 4) && (cp->on & CK_M_CHORD)) {     /* STRUM: the last strum ends at once */
            ck_strum(1u << 20);
            ck_strum_snapshot(k);                       /* before the new physical owner is added */
            cp->snd[k >> 5] |= 1u << (k & 31u);
            cp->korder[0] = (uint8_t)k;
            cp->nk = 1;
            cp->acc = 0;
            ck_strum(0);
            return;
        }
        cp->snd[k >> 5] |= 1u << (k & 31u);
        ck_key_send(k, 0x91u);  /* bit 0 checks existing owners; ck_on emits real NoteOn */
        return;
    }
    cp->keys[k >> 5] ^= 1u << (k & 31u);                     /* (for a moment: the other keys only) */
    if ((cp->fl & CK_F_LATCH) && !(cp->keys[0] | cp->keys[1])) {
        ck_arp_stop_note();                                 /* LATCH: the first key after all keys up: a new list */
        cp->nk = 0;
    }
    cp->keys[k >> 5] ^= 1u << (k & 31u);
    if (!cp->nk) {
        cp->step = 0;
        cp->tick = (uint8_t)(CK_RATE_T[sp->ck_rate] - 1u);   /* the first step plays at the next tick */
        cp->acc = 599999u;
    }
    if (cp->nk < sizeof cp->korder)
        cp->korder[cp->nk++] = (uint8_t)k;
}
CK_FN static void ck_release(uint32_t k)
{
    CK_BASE;
    uint32_t i, j;
    if (CK_BIT(cp->snd, k)) {
        cp->snd[k >> 5] &= ~(1u << (k & 31u));
        ck_key_send(k, 0x81u);
    }
    if (!(cp->on & CK_M_ARP)) {                             /* (a strum of this key stops) */
        if (cp->korder[0] == k)
            cp->nk = 0;
        return;
    }
    if (cp->fl & CK_F_LATCH)
        return;
    for (i = j = 0; i < cp->nk; i++)
        if (cp->korder[i] != k)
            cp->korder[j++] = cp->korder[i];
    cp->nk = (uint8_t)j;
    if (!j)
        ck_arp_stop_note();
}
/* one arp step: the notes of the arp keys, sorted (not for PLAYED), then the note of this step */
CK_FN static void ck_arp_step(uint32_t rate, uint32_t pos)
{
    CK_BASE;
    CK_SET;
    uint8_t list[CK_ARPN], out[CK_MAXN];
    uint32_t n = 0, i, j, c;
    int note;
    for (i = 0; i < cp->nk; i++)
        for (j = 0, c = ck_key_notes(cp->korder[i], out); j < c && n < CK_ARPN; j++) {
            uint32_t x = out[j], p = n++;
            if (sp->ck_ord != CK_ORD_PLAYED)
                for (; p && list[p - 1u] > x; p--)
                    list[p] = list[p - 1u];
            list[p] = (uint8_t)x;
        }
    ck_arp_stop_note();
    if (!n)
        return;
    note = ck_arp_pick(list, n, sp->ck_ord, sp->ck_oct + 1u, pos, fm1_ms * 2654435761u >> 7);
    if (note >= 0) {
        ck_on((uint32_t)note);
        cp->arp_note = (uint8_t)(note + 1);
        j = rate * (sp->ck_gate + 1u) / 10u;                 /* gate 10-100 % of a step, at least 1 tick */
        cp->gate_left = (uint8_t)(j ? j : 1u);
    }
}
/* the arp clock: incoming MIDI clock while a tick came in the last 0.5 s, else the internal tempo */
CK_FN static void ck_arp(uint32_t dt)
{
    CK_SET;
    CK_BASE;
    uint32_t rate = CK_RATE_T[sp->ck_rate], ticks, e = apc_clock_count, rh = sp->ck_x2 & 15u;
    uint32_t sw = rate * 3u * (sp->ck_x1 & 7u) / 50u;      /* SWING: the odd steps start sw ticks late */
    if (ck_start_req) {                                     /* MIDI Start: the next tick is the downbeat */
        ck_start_req = 0;
        cp->tick = (uint8_t)(rate - 1u);
        cp->step = 0;
    }
    if (!(cp->on & CK_M_ARP)) {                             /* (acc and nk are the strum's) */
        cp->ext_seen = (uint8_t)e;
        ck_strum(dt);
        return;
    }
    if (apc_clock_seen && fm1_ms - apc_clock_last_ms <= 500u) {
        ticks = (uint8_t)(e - cp->ext_seen);
        ticks = ticks > 8u ? 0u : ticks;                    /* a jump (Start, Song Position): no burst */
        cp->acc = 0;
    } else {
        cp->acc += (dt < 100u ? dt : 100u) * (uint32_t)(seq.bpm * 240.0f);   /* ms x BPM x 240 */
        for (ticks = 0; cp->acc >= 600000u; ticks++)        /* (dt <= 100 ms: at most 12 ticks) */
            cp->acc -= 600000u;
    }
    cp->ext_seen = (uint8_t)e;
    while (ticks--) {
        if (cp->gate_left && !--cp->gate_left)
            ck_arp_stop_note();
        if (++cp->tick < (cp->step & 1u ? rate + sw : rate - sw))
            continue;
        cp->tick = 0;
        if (cp->nk) {                                       /* RHYTHM: a rest step plays no note */
            if (ck_rhy_hit(rh, cp->step))
                ck_arp_step(rate, ck_rhy_pos(rh, cp->step));
            cp->step++;
        }
    }
}
/* 1: clip mode and no menu on the screen: knobs 2-8 go to ck_controls (a stage or CC 48-54), X0X gets no knob turns */
uint8_t smk_ck_knobs_owned(void)
{
    return (uint8_t)(clip_grid_mode && ui.overlay == O_NONE && !ui.help);
}
static const char CK_MODE_N[] = "SCALE\0CHORD\0ARP";
static const uint16_t CK_MODE_C[3] = {RGB(90, 200, 255), RGB(255, 150, 40), RGB(130, 240, 90)};
#define CK_O(f) __builtin_offsetof(settings_t, f)
/* the editor fields (CK_E_*): the settings_t byte and the bit shift (the highest value: CK_FMAX); the names;
 * the value lists (0: a number) */
static const uint8_t CK_FLD[CK_NE][2] = {
    {CK_O(ck_root), 0}, {CK_O(ck_scale), 0}, {CK_O(ck_snap), 0}, {CK_O(ck_chord), 0}, {CK_O(ck_inv), 0},
    {CK_O(ck_x2), 4}, {CK_O(ck_ord), 0}, {CK_O(ck_rate), 0}, {CK_O(ck_oct), 0}, {CK_O(ck_gate), 0},
    {CK_O(ck_x1), 0}, {CK_O(ck_x1), 3}, {CK_O(ck_x2), 0}, {0, 0}, {0, 0}};
static const char CK_FLD_N[] = "ROOT\0SCALE\0SNAP\0CHORD\0INV\0STRUM\0ORDER\0RATE\0OCT\0GATE\0SWING\0HUMAN\0RHYTHM\0LATCH\0ON";
static const char *const CK_LIST[CK_NE] = {CK_ROOT_N, CK_SCALE_N, "NEAR\0FILTER", CK_CHORD_N, "ROOT\0/1\0/2\0/3", 0,
                                            CK_ORD_N, CK_RATE_N, 0, 0, 0, 0, CK_RHY_N, "OFF\0ON", "OFF\0ON"};
/* the mask of a field's bits (its highest value rounded up to 2^n - 1) */
CK_FN static uint32_t ck_fmask(uint32_t f)
{
    return (2u << (31 - __builtin_clz(CK_FMAX[f]))) - 1u;
}
CK_FN static uint32_t ck_get(uint32_t f)
{
    if (f == CK_E_LATCH)
        return ck.fl & CK_F_LATCH;
    if (f == CK_E_ON)
        return (ck.on >> ((ck.pg & 3u) - 1u)) & 1u;
    return (uint32_t)((const uint8_t *)&proj.set)[CK_FLD[f][0]] >> CK_FLD[f][1] & ck_fmask(f);
}
/* Set field f to v. A note field change ends the old notes and plays the new ones. */
CK_FN static void ck_set(uint32_t f, uint32_t v)
{
    CK_BASE;
    uint8_t *b = (uint8_t *)&proj.set + CK_FLD[f][0];
    uint32_t m = ck_fmask(f) << CK_FLD[f][1], nt = f <= CK_E_INV;
    cp->pg = (uint8_t)((cp->pg & 3u) | f << 4);
    ui.act_t = plat_ms();                                   /* (the autosave waits for quiet) */
    if (f == CK_E_ON) {
        uint32_t bit = 1u << ((cp->pg & 3u) - 1u);
        if (!!(cp->on & bit) != !!v) {
            ck_all_off();
            cp->on ^= (uint8_t)bit;
        }
        return;
    }
    if (f == CK_E_LATCH) {
        if (v != (cp->fl & CK_F_LATCH)) {
            cp->fl ^= CK_F_LATCH;
            if (!v)
                ck_all_off();
        }
        return;
    }
    if (!(cp->on & CK_M_ARP))
        cp->nk = 0;                                         /* (a strum: its key plays all its notes below) */
    if (nt)
        ck_voice(0x80u);
    *b = (uint8_t)((*b & ~m) | (v << CK_FLD[f][1] & m));
    if (nt)
        ck_voice(0x90u);
}
/* the text of value v of field f: a list name, or a number in buf */
CK_FN static const char *ck_val_txt(uint32_t f, uint32_t v, char *buf)
{
    const char *l = CK_LIST[f];
    if (l)
        return ck_nm(l, v);
    if (!v && (f == CK_E_STRUM || f == CK_E_HUMAN))
        return "OFF";
    v = f == CK_E_STRUM ? v * 10u : f == CK_E_OCT ? v + 1u : f == CK_E_GATE ? v * 10u + 10u
        : f == CK_E_SWING ? 50u + 3u * v : v;
    put_s(put_i(buf, (int)v), f == CK_E_STRUM ? "MS" : f == CK_E_GATE || f == CK_E_SWING ? "%" : "");
    return buf;
}
/* 1: a stage page is open. */
static uint32_t ck_page_on(void)
{
    return (ck.pg & 3u) != 0u;
}
/* SMC encoders edit the first eight visible clip fields. Navigation below also
 * reaches ARP LATCH. Faders keep the main mix levels and master filter here. */
int smk_ck_smc_turn(int n, int delta)
{
    uint32_t pg, f;
    int v;
    char name[20], value[12];
    if (!clip_grid_mode)
        return 0;
    if (!ck_page_on())
        return 1;                       /* the grid hides the X0X parameter page */
    pg = (ck.pg & 3u) - 1u;
    if (n < 0 || n >= (pg == 2u ? 8 : 4))
        return 1;
    f = CK_PAGE_FIELDS[pg][n];
    v = (int)ck_get(f) + delta;
    v = v < 0 ? 0 : v > CK_FMAX[f] ? CK_FMAX[f] : v;
    if ((uint32_t)v != ck_get(f)) {
        ck_set(f, (uint32_t)v);
        mark_dirty();
    }
    put_s(name, ck_nm(CK_FLD_N, f));
    smc_ui_clip_show(n, name, ck_val_txt(f, ck_get(f), value));
    return 1;
}
/* SMC arrows select stages and edit the highlighted field; bank/channel buttons
 * select fields, including ARP's ninth field (LATCH). The grid consumes them too. */
int smk_ck_smc_nav(uint8_t note)
{
    int d = (note & 1u) ? 1 : -1;
    uint32_t pg = ck.pg & 3u, f;
    if (!clip_grid_mode || !((note >= 0x60 && note <= 0x63) ||
        note == 0x2E || note == 0x2F || note == 0x30 || note == 0x31 || note == 6))
        return 0;
    smc_ui_cancel();
    if (note == 6)
        ck.pg = 0;
    else if (note == 0x60 || note == 0x61)
        ck.pg = (uint8_t)(((int)pg + d + 4) % 4 | CK_E_ON << 4);
    else if (pg) {
        f = ck.pg >> 4;
        if (note == 0x2E || note == 0x2F || note == 0x30 || note == 0x31)
            ck.pg = (uint8_t)(pg | ck_page_field(pg, f, d) << 4);
        else {
            uint32_t v = ck_step(ck_get(f), d, CK_FMAX[f]);
            if (v != ck_get(f)) { ck_set(f, v); mark_dirty(); }
        }
    }
    ui.act_t = plat_ms();
    return 1;
}
/* knobs 2-5 of each stage (CLP-63): the field (CK_E_*), CK_NE = none */
static const uint8_t CK_KNOB[3][4] = {{CK_E_ROOT, CK_E_SCALE, CK_E_SNAP, CK_NE}, {CK_E_CHORD, CK_E_INV, CK_NE, CK_NE},
                                      {CK_E_ORD, CK_E_RATE, CK_E_OCT, CK_E_GATE}};
/* A stage press opens or closes its page. Knob 2 picks a field, knob 3 and < > change its value.
 * The ON field controls the stage. Knobs 4-8 keep their direct assignments when a page is open. */
CK_FN static void ck_controls(void)
{
    CK_BASE;
    static const uint8_t ROLE[4] = {B_SEL, B_ENV, B_ARP, B_LFO};   /* SCALE, CHORD, ARP, NOTE REPEAT */
    uint32_t raw = fm1_in.buttons, i, cur = 0, press, rel, f, own = smk_ck_knobs_owned(), v, nv;
    int32_t st;
    for (i = 0; i < 4u; i++)
        cur |= ((raw >> panel.btn[ROLE[i]]) & 1u) << i;
    cur |= (raw >> SMK_BTN_LEFT & 3u) << 5;                  /* < and > */
    press = cur & ~(uint32_t)cp->btn_prev;
    rel = (uint32_t)cp->btn_prev & ~cur;
    if (press || rel) smc_ui_cancel();
    cp->btn_prev = (uint8_t)cur;
    if (!own)
        press &= ~0x60u;                                    /* a menu: < and > belong to smk_arrows */
    if (press & 8u)
        cp->fl &= (uint8_t)~CK_F_NRUSED;
    if ((press & 4u) && (cur & 8u)) {                       /* NOTE REPEAT + ARP: LATCH */
        ck_set(CK_E_LATCH, !(cp->fl & CK_F_LATCH));
        cp->fl |= CK_F_NRUSED;
        ui_say("LATCH", cp->fl & CK_F_LATCH ? " ON" : " OFF");
    }
    cp->pg = (uint8_t)ck_page_press(cp->pg, press, cur);
    f = cp->pg >> 4;
    st = ck_rpt(own && ck_page_on() ? cur >> 5 & 3u : 0u, press >> 5 & 3u, fm1_ms, &ck_rpt_st);
    if (st) {                                               /* < >: the last field one step down / up */
        v = ck_get(f);
        nv = ck_step(v, st, CK_FMAX[f]);
        if (nv != v)                                        /* at an end: no step, the notes stay */
            ck_set(f, nv);
    }
    if ((rel & 8u) && !(cp->fl & CK_F_NRUSED)) {             /* NOTE REPEAT alone: panic */
        ck_all_off();
        ck_send(0xB0u | cp->ch, 123u, 0u);                   /* All Notes Off */
        ui_say("PANIC", 0);
    }
    v = (cp->pg & 3u) ? (cp->pg & 3u) - 1u : 3u;
    if (own)
        for (i = 0; i < NE; i++) {                          /* a stage: knobs 2-5 (knobs 6-8 do nothing) */
            int32_t d = fm1_enc_take(panel.enc[i]) * panel.dir[i];
            char n[12], t[12];
            if (!d)
                continue;
            smc_ui_cancel();
            ui.act_t = fm1_ms;                               /* CC turns also keep flash writes away from playing */
            if (v > 2u) {                                   /* no stage: CC 48-54 for MIDI-learn */
                ck_send(0xB0u, 48u + i, (uint32_t)(d < -63 ? -63 : d > 63 ? 63 : d) & 127u);   /* (relative) */
                continue;
            }
            if (i == 0u) {                                  /* knob 2: select the next or previous field */
                d %= v == 2u ? 9 : 4;                       /* bound work after a fast turn */
                while (d) {
                    f = ck_page_field(v + 1u, cp->pg >> 4, d);
                    cp->pg = (uint8_t)((cp->pg & 3u) | f << 4);
                    d += d > 0 ? -1 : 1;
                }
                ui_say(ck_nm(CK_FLD_N, f), ck_val_txt(f, ck_get(f), t));
                continue;
            }
            f = i == 1u ? cp->pg >> 4 : i == 2u ? CK_E_ON : i - 1u < 4u ? CK_KNOB[v][i - 1u] : CK_NE;
            if (f >= CK_NE)
                continue;
            d += (int32_t)ck_get(f);
            nv = cp->pg >> 4;
            ck_set(f, (uint32_t)(d < 0 ? 0 : d > CK_FMAX[f] ? CK_FMAX[f] : d));
            if (i == 2u)                                /* direct ON keeps knob 3 on the selected field */
                cp->pg = (uint8_t)((cp->pg & 15u) | nv << 4);
            put_s(put_s(n, ck_nm(CK_FLD_N, f)), " ");
            ui_say(n, ck_val_txt(f, ck_get(f), t));        /* for example "RATE 1/16" */
        }
}
/* All 37 raw keybed bits, including the ten keys outside X0X's 27-key panel. */
uint8_t smk_ck_keybed_held(void)
{
    const volatile uint32_t *keys = (const volatile uint32_t *)&fm1_in.keys;
    return (uint8_t)(clip_grid_mode && (keys[0] || (keys[1] & 0x1Fu)));
}
/* A frame may reach autosave before the clip poll consumes a new knob turn. */
uint8_t smk_ck_flash_busy(void)
{
    uint32_t i;
    if (smk_ck_keybed_held())
        return 1u;
    if (smk_ck_knobs_owned())
        for (i = 0; i < NE; i++)
            if (fm1_in.enc_steps[panel.enc[i]])
                return 1u;
    return 0u;
}
CK_FN int smk_project_active(void)
{
    return smk_ck_flash_busy() || fm1_in.pnotes || ck.snd[0] || ck.snd[1] ||
           ck.arp_note || ck.nk || ck_qn || ck_start_req;
}
