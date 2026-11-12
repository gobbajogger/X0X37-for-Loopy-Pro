/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
 * X0X changes: Copyright (C) 2026 M. Geisler (genkigenki/makkuro) */
#include "ble_midi_host.h"
#include "ble_midi_uuid.h"             /* one SVC/CHR UUID pair shared with ble_midi_central.c */

enum { OP_DISC = 0x0406, OP_EVMASK = 0x0C01, OP_RESET = 0x0C03, OP_LE_EVMASK = 0x2001, OP_LE_BUF = 0x2002, OP_RAND = 0x2005, OP_ADV_PARAM = 0x2006,
       OP_ADV_DATA = 0x2008, OP_SCAN_RSP = 0x2009, OP_ADV_EN = 0x200A };
enum { ST_RESET, ST_EVMASK, ST_LE_EVMASK, ST_BUF, ST_RAND, ST_PARAM, ST_DATA, ST_SCAN, ST_DONE };
enum { H_GAP = 1, H_NAME_DECL, H_NAME, H_MIDI_SVC, H_MIDI_DECL, H_MIDI, H_CCCD, H_LAST = H_CCCD };
enum { A_ERR = 1, A_MTU = 2, A_MTU_R = 3, A_INFO = 4, A_INFO_R = 5, A_FVAL = 6, A_FVAL_R = 7, A_RTYPE = 8, A_RTYPE_R = 9,
       A_READ = 10, A_READ_R = 11, A_GROUP = 16, A_GROUP_R = 17, A_WRITE = 18, A_WRITE_R = 19, A_NOTIFY = 27,
       A_WCMD = 0x52 };
enum { E_HANDLE = 1, E_READ = 2, E_WRITE = 3, E_PDU = 4, E_UNSUPP = 6, E_NOTFOUND = 10, E_GROUP = 16 };
enum { CID_ATT = 4, CID_SIG = 5, CID_SMP = 6 };

#define U_SVC BLE_MIDI_UUID_SVC
#define U_CHR BLE_MIDI_UUID_CHR
static const uint8_t T_PRIM[2] = {0x00, 0x28}, T_DECL[2] = {0x03, 0x28}, T_NAME[2] = {0x00, 0x2A}, T_CCCD[2] = {0x02, 0x29};
static const uint8_t V_GAP[2] = {0x00, 0x18};
static const uint8_t D_NAME[5] = {0x02, H_NAME, 0x00, 0x00, 0x2A};                      /* read; value handle 3; 0x2A00 */
static const uint8_t D_MIDI[19] = {0x16, H_MIDI, 0x00, 0xF3, 0x6B, 0x10, 0x9D, 0x66, 0xF2, 0xA9, 0xA1, 0x12, 0x41,
                                   0x68, 0x38, 0xDB, 0xE5, 0x72, 0x77};                  /* read, write w/o response, notify */
static const struct { uint8_t tlen; const uint8_t *type; } ATTR[H_LAST] = {
    {2, T_PRIM}, {2, T_DECL}, {2, T_NAME}, {2, T_PRIM}, {2, T_DECL}, {16, U_CHR}, {2, T_CCCD}};

static uint8_t value_of(const bleh_t *h, unsigned hd, const uint8_t **v)
{
    switch (hd) {
    case H_GAP: *v = V_GAP; return 2;
    case H_NAME_DECL: *v = D_NAME; return 5;
    case H_NAME: *v = (const uint8_t *)h->name; return h->namelen;
    case H_MIDI_SVC: *v = U_SVC; return 16;
    case H_MIDI_DECL: *v = D_MIDI; return 19;
    case H_MIDI: *v = 0; return 0;                  /* reads of the MIDI characteristic: empty */
    case H_CCCD: *v = h->cccd; return 2;
    default: *v = 0; return 0;
    }
}
static unsigned group_end(unsigned hd) { return hd <= H_NAME ? H_NAME : H_LAST; }
static unsigned rd16(const uint8_t *p) { return (unsigned)p[0] | (unsigned)p[1] << 8; }
static void wr16(uint8_t *p, unsigned v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }
static int tmatch(unsigned hd, const uint8_t *t, unsigned tl)
{
    unsigned i;
    if (ATTR[hd - 1].tlen != tl) return 0;
    for (i = 0; i < tl; i++) if (ATTR[hd - 1].type[i] != t[i]) return 0;
    return 1;
}

/* ---- HCI out ---- */
/* The core-to-controller mailbox can refuse an HCI frame while full.  Never
 * wait for a command-complete event for a command that was not enqueued. */
static int cmd(bleh_t *h, unsigned op, const uint8_t *p, unsigned n)
{
    uint8_t b[3 + 32];
    unsigned i;
    wr16(b, op);
    b[2] = (uint8_t)n;
    for (i = 0; i < n; i++) b[3 + i] = p[i];
    if (h->send(BLEH_H4_CMD, b, (uint16_t)(3 + n), h->ctx) != 0) return 0;
    h->cmd_op = (uint16_t)op;
    h->cmd_wait = 1;
    return 1;
}
static void adv_enable(bleh_t *h, unsigned on)
{
    uint8_t e = (uint8_t)on;
    if (cmd(h, OP_ADV_EN, &e, 1))
        h->want_adv = (uint8_t)on;                  /* what the controller was last told */
}
static void next_cmd(bleh_t *h)
{
    uint8_t b[32];
    unsigned i, n;
    switch (h->step) {
    case ST_RESET: cmd(h, OP_RESET, 0, 0); break;
    case ST_EVMASK:                                 /* attempt 28: the events, with LE meta (bit 61) */
        for (i = 0; i < 8; i++) b[i] = 0xFF;
        b[7] = 0x3F;
        cmd(h, OP_EVMASK, b, 8);
        break;
    case ST_LE_EVMASK:                              /* the LE events: connection complete, update, ... */
        for (i = 0; i < 8; i++) b[i] = 0;
        b[0] = 0xFF; b[1] = 0x0F;
        cmd(h, OP_LE_EVMASK, b, 8);
        break;
    case ST_BUF: cmd(h, OP_LE_BUF, 0, 0); break;
    case ST_RAND: cmd(h, OP_RAND, h->addr, 6); break;
    case ST_PARAM:                                  /* 40..80 ms, ADV_IND, random address, all 3 channels, no filter */
        wr16(b, 0x40); wr16(b + 2, 0x80);
        b[4] = 0; b[5] = 1; b[6] = 0;
        for (i = 7; i < 13; i++) b[i] = 0;
        b[13] = 7; b[14] = 0;
        cmd(h, OP_ADV_PARAM, b, 15);
        break;
    case ST_DATA:                                   /* flags + the 128-bit service UUID */
        for (i = 0; i < 32; i++) b[i] = 0;
        b[0] = 21; b[1] = 2; b[2] = 1; b[3] = 6; b[4] = 17; b[5] = 7;
        for (i = 0; i < 16; i++) b[6 + i] = U_SVC[i];
        cmd(h, OP_ADV_DATA, b, 32);
        break;
    case ST_SCAN:                                   /* the name */
        for (i = 0; i < 32; i++) b[i] = 0;
        n = h->namelen;
        b[0] = (uint8_t)(n + 2); b[1] = (uint8_t)(n + 1); b[2] = 9;
        for (i = 0; i < n; i++) b[3 + i] = (uint8_t)h->name[i];
        cmd(h, OP_SCAN_RSP, b, 32);
        break;
    default: break;
    }
}

/* ---- L2CAP out (one PDU, no fragmentation: everything here fits the 27-byte LE buffer) ---- */
static int l2cap_send(bleh_t *h, unsigned cid, const uint8_t *d, unsigned n)
{
    uint8_t b[8 + BLEH_MTU];
    unsigned i;
    int rc;
    if (n > BLEH_MTU || !h->credits) return -1;
    wr16(b, h->conn & 0x0FFFu);                     /* PB = 0: first packet, host to controller */
    wr16(b + 2, 4 + n);
    wr16(b + 4, n);                                 /* L2CAP length, then the channel */
    wr16(b + 6, cid);
    for (i = 0; i < n; i++) b[8 + i] = d[i];
    rc = h->send(BLEH_H4_ACL, b, (uint16_t)(8 + n), h->ctx);
    if (rc == 0 && h->credits) h->credits--;         /* rejected mailbox write consumed no ACL buffer */
    return rc;
}
static int att_send(bleh_t *h, const uint8_t *att, unsigned n) { return l2cap_send(h, CID_ATT, att, n); }
/* A full mailbox must not silently discard an ATT request response. Neither
 * CCCD nor MTU replies may depend on notifications having been enabled. */
static void att_reply(bleh_t *h, const uint8_t *p, unsigned n)
{
    unsigned i;
    if (n > BLEH_MTU || h->att_rsp_len) return;
    if (h->credits && att_send(h, p, n) == 0) return;
    for (i = 0; i < n; i++) h->att_rsp[i] = p[i];
    h->att_rsp_len = (uint8_t)n;
}
static void att_err(bleh_t *h, unsigned op, unsigned hd, unsigned code)
{
    uint8_t r[5] = {A_ERR, (uint8_t)op, 0, 0, (uint8_t)code};
    wr16(r + 2, hd);
    att_reply(h, r, 5);
}

/* ---- MIDI in: a BLE-MIDI packet written by the central ----
 * header (10hhhhhh), then events. An event is a timestamp-low byte (1ttttttt) and a message.
 * Running status: a channel message may leave out its status byte, and then also its
 * timestamp byte (it takes the last one). SysEx: F0 after a timestamp, data bytes (they go on in
 * the next packets, after the header, without a timestamp), a timestamp, F7. System real time
 * may come inside SysEx, after a timestamp. */
static void in_put(bleh_t *h, const bleh_msg_t *m)
{
    if ((uint8_t)(h->in_w - h->in_r) < BLEH_INQ)
        h->in_q[h->in_w++ % BLEH_INQ] = *m;
    else
        h->drops_in++;
}
static void sysex_end(bleh_t *h, int how)
{
    if (!h->in_sysex) return;
    h->in_sysex = 0;
    if (how > 0) h->sysex_in++;
    if (h->sysex) h->sysex(0, 0, how, h->ctx);
}
static unsigned sysex_data(bleh_t *h, const uint8_t *p, unsigned i, unsigned n)
{
    unsigned s = i;
    while (i < n && !(p[i] & 0x80u)) i++;
    if (i > s && h->sysex) h->sysex(p + s, i - s, 0, h->ctx);
    return i;
}
static unsigned msg_need(uint8_t st)                /* bytes in a message with this status */
{
    if (st < 0xF0u) return (st & 0xE0u) == 0xC0u ? 2u : 3u;     /* Cx Dx: 2 */
    switch (st) {
    case 0xF1: case 0xF3: return 2u;
    case 0xF2: return 3u;
    default: return 1u;                             /* F6, F8-FF (F4 F5: undefined, one byte) */
    }
}
/* the data bytes of a message whose status is st (taken from p[i] on): the next index, or n + 1
 * when the message is cut short (the rest of the packet is not trusted) */
static unsigned msg_data(bleh_t *h, uint8_t st, uint16_t ts, const uint8_t *p, unsigned i, unsigned n)
{
    bleh_msg_t m;
    unsigned j, need = msg_need(st);
    m.b[0] = st; m.b[1] = m.b[2] = 0;
    m.len = (uint8_t)need;
    m.ts = ts;
    for (j = 1; j < need; j++) {
        if (i >= n || (p[i] & 0x80u)) return n + 1u;
        m.b[j] = p[i++];
    }
    in_put(h, &m);
    return i;
}
static void midi_write(bleh_t *h, const uint8_t *p, unsigned n)
{
    unsigned i = 1, hi, have_ts = 0;
    uint16_t ts = 0;
    if (n < 2 || (p[0] & 0xC0u) != 0x80u) return;
    hi = p[0] & 0x3Fu;
    while (i < n) {
        uint8_t b = p[i];
        if (!(b & 0x80u)) {                         /* a data byte with no timestamp before it */
            if (h->in_sysex) i = sysex_data(h, p, i, n);
            else if (h->in_running && have_ts) i = msg_data(h, h->in_running, ts, p, i, n);
            else i++;                               /* nothing it can belong to */
            continue;
        }
        ts = (uint16_t)(hi << 7 | (b & 0x7Fu));      /* a timestamp byte */
        have_ts = 1;
        if (++i >= n) break;
        b = p[i];
        if (!(b & 0x80u)) {                         /* running status, with its own timestamp */
            if (h->in_sysex) i = sysex_data(h, p, i, n);
            else if (h->in_running) i = msg_data(h, h->in_running, ts, p, i, n);
            else i++;
            continue;
        }
        i++;
        if (b >= 0xF8u) {                           /* real time: anywhere, keeps running status and SysEx */
            bleh_msg_t m = {1, {b, 0, 0}, ts};
            in_put(h, &m);
        } else if (b == 0xF7u) {
            sysex_end(h, 1);
        } else if (b == 0xF0u) {
            sysex_end(h, -1);
            h->in_sysex = 1;
            h->in_running = 0;
            i = sysex_data(h, p, i, n);
        } else {
            sysex_end(h, -1);
            h->in_running = b < 0xF0u ? b : 0;      /* system common ends running status */
            i = msg_data(h, b, ts, p, i, n);
        }
    }
}

/* ---- SMP and L2CAP signalling: no pairing, no other channels ---- */
static void smp_in(bleh_t *h, const uint8_t *p, unsigned n)
{
    static const uint8_t NOT_SUPPORTED[2] = {0x05, 0x05};   /* Pairing Failed: Pairing Not Supported */
    if (n >= 1 && p[0] == 0x01) {                   /* Pairing Request */
        h->smp_rejects++;
        l2cap_send(h, CID_SMP, NOT_SUPPORTED, 2);
    }
}
static void sig_in(bleh_t *h, const uint8_t *p, unsigned n)
{
    uint8_t r[6] = {0x01, 0, 2, 0, 0, 0};           /* Command Reject: command not understood */
    if (n < 4) return;
    if (p[0] == 0x13 && n >= 6 && h->cpu_state) {   /* attempt 34: our parameter request: accepted (0) or not */
        h->cpu_result = p[4] == 0 && p[5] == 0 ? 1u : 2u;
        return;
    }
    switch (p[0]) {
    case 0x01: case 0x07: case 0x13: case 0x15: case 0x18: return;   /* responses: nothing to answer */
    default: break;
    }
    if (h->sig_reqs < 255u) h->sig_reqs++;
    r[1] = p[1];
    l2cap_send(h, CID_SIG, r, 6);
}

static void att_in(bleh_t *h, const uint8_t *p, unsigned n)
{
    uint8_t r[BLEH_MTU];
    unsigned op = p[0], s, e, hd, k, rl;
    if (op == A_WCMD) {
        if (n >= 4 && rd16(p + 1) == H_MIDI) midi_write(h, p + 3, n - 3);
        return;
    }
    if (h->att_rsp_len) return; /* preserve first pending response until delivered */
    if (op & 0x40u) return;
    switch (op) {
    case A_MTU:
        if (n != 3) { att_err(h, op, 0, E_PDU); return; }
        h->mtu_req = (uint16_t)rd16(p + 1);
        r[0] = A_MTU_R; wr16(r + 1, BLEH_MTU);
        att_reply(h, r, 3);
        return;
    case A_INFO:
    case A_GROUP:
    case A_RTYPE:
    case A_FVAL:
        if (n < 5) { att_err(h, op, 0, E_PDU); return; }
        s = rd16(p + 1); e = rd16(p + 3);
        if (!s || s > e) { att_err(h, op, s, E_HANDLE); return; }
        break;
    case A_READ:
    case A_WRITE:
        if (n < 3) { att_err(h, op, 0, E_PDU); return; }
        s = rd16(p + 1); e = 0;
        break;
    default:
        att_err(h, op, 0, E_UNSUPP);
        return;
    }
    if (op == A_INFO) {
        unsigned fmt = 0, len = 2;
        for (hd = s; hd <= e && hd <= H_LAST; hd++) {
            unsigned f = ATTR[hd - 1].tlen == 2 ? 1 : 2, el = 2 + ATTR[hd - 1].tlen;
            if (!fmt) { fmt = f; r[0] = A_INFO_R; r[1] = (uint8_t)f; }
            else if (f != fmt) break;
            if (len + el > BLEH_MTU) break;
            wr16(r + len, hd);
            for (k = 0; k < ATTR[hd - 1].tlen; k++) r[len + 2 + k] = ATTR[hd - 1].type[k];
            len += el;
        }
        if (!fmt) att_err(h, op, s, E_NOTFOUND); else att_reply(h, r, len);
    } else if (op == A_GROUP) {
        unsigned len = 2, el = 0;
        if (n != 7 || rd16(p + 5) != 0x2800) { att_err(h, op, s, E_GROUP); return; }
        for (hd = s; hd <= e && hd <= H_LAST; hd++) {
            const uint8_t *v;
            unsigned vl;
            if (!tmatch(hd, T_PRIM, 2)) continue;
            vl = value_of(h, hd, &v);
            if (!el) { el = 4 + vl; r[0] = A_GROUP_R; r[1] = (uint8_t)el; }
            else if (4 + vl != el) break;
            if (len + el > BLEH_MTU) break;
            wr16(r + len, hd); wr16(r + len + 2, group_end(hd));
            for (k = 0; k < vl; k++) r[len + 4 + k] = v[k];
            len += el;
        }
        if (!el) att_err(h, op, s, E_NOTFOUND); else att_reply(h, r, len);
    } else if (op == A_FVAL) {
        unsigned len = 1, found = 0;
        if (n < 7 || rd16(p + 5) != 0x2800) { att_err(h, op, s, E_NOTFOUND); return; }
        r[0] = A_FVAL_R;
        for (hd = s; hd <= e && hd <= H_LAST && len + 4 <= BLEH_MTU; hd++) {
            const uint8_t *v;
            unsigned vl = value_of(h, hd, &v);
            if (!tmatch(hd, T_PRIM, 2) || vl != n - 7) continue;
            for (k = 0; k < vl && v[k] == p[7 + k]; k++) ;
            if (k != vl) continue;
            wr16(r + len, hd); wr16(r + len + 2, group_end(hd));
            len += 4; found = 1;
        }
        if (!found) att_err(h, op, s, E_NOTFOUND); else att_reply(h, r, len);
    } else if (op == A_RTYPE) {
        unsigned len = 2, el = 0, tl = n - 5;
        if (tl != 2 && tl != 16) { att_err(h, op, s, E_PDU); return; }
        for (hd = s; hd <= e && hd <= H_LAST; hd++) {
            const uint8_t *v;
            unsigned vl;
            if (!tmatch(hd, p + 5, tl)) continue;
            vl = value_of(h, hd, &v);
            if (vl > BLEH_MTU - 4) vl = BLEH_MTU - 4;
            if (!el) { el = 2 + vl; r[0] = A_RTYPE_R; r[1] = (uint8_t)el; }
            else if (2 + vl != el) break;
            if (len + el > BLEH_MTU) break;
            wr16(r + len, hd);
            for (k = 0; k < vl; k++) r[len + 2 + k] = v[k];
            len += el;
        }
        if (!el) att_err(h, op, s, E_NOTFOUND); else att_reply(h, r, len);
    } else if (op == A_READ) {
        const uint8_t *v;
        unsigned vl;
        if (s < 1 || s > H_LAST) { att_err(h, op, s, E_HANDLE); return; }
        vl = value_of(h, s, &v);
        if (vl > BLEH_MTU - 1) vl = BLEH_MTU - 1;
        r[0] = A_READ_R;
        for (k = 0; k < vl; k++) r[1 + k] = v[k];
        att_reply(h, r, 1 + vl);
    } else {                                        /* A_WRITE */
        if (s != H_CCCD) { att_err(h, op, s, s >= 1 && s <= H_LAST ? E_WRITE : E_HANDLE); return; }
        rl = n - 3;
        if (rl != 2) { att_err(h, op, s, E_PDU); return; }
        h->cccd[0] = p[3] & 1u; h->cccd[1] = 0;     /* notifications only */
        r[0] = A_WRITE_R;
        att_reply(h, r, 1);
    }
}

/* ---- ACL in: collect one L2CAP PDU (the controller may split it), hand it on by channel ---- */
static void acl_in(bleh_t *h, const uint8_t *p, unsigned n)
{
    unsigned hd, pb, dl;
    if (n < 4) return;
    hd = rd16(p); pb = (hd >> 12) & 3u; dl = rd16(p + 2);
    if ((hd & 0x0FFFu) != h->conn || dl > n - 4) return;
    p += 4;
    if (pb != 1u) {                                /* PB 0/2: first, PB 3: complete PDU */
        unsigned ll, cid;
        h->rx_on = 0;
        if (dl < 4) return;
        ll = rd16(p); cid = rd16(p + 2);
        if (ll > BLEH_MTU || (cid != CID_ATT && cid != CID_SIG && cid != CID_SMP) ||
            (pb == 3u && dl != ll + 4u)) return;  /* complete PDU cannot be fragmented */
        h->rx_need = (uint8_t)(ll + 4); h->rx_have = 0; h->rx_on = 1; h->rx_cid = (uint16_t)cid;
    } else if (pb != 1 || !h->rx_on) {
        return;
    }
    if (h->rx_have + dl > h->rx_need) { h->rx_on = 0; return; }
    { unsigned i; for (i = 0; i < dl; i++) h->rx[h->rx_have + i] = p[i]; }
    h->rx_have = (uint8_t)(h->rx_have + dl);
    if (h->rx_have == h->rx_need) {
        unsigned m = (unsigned)h->rx_need - 4u;
        h->rx_on = 0;
        if (!m) return;
        if (h->rx_cid == CID_ATT) att_in(h, h->rx + 4, m);
        else if (h->rx_cid == CID_SMP) smp_in(h, h->rx + 4, m);
        else sig_in(h, h->rx + 4, m);
    }
}

/* ---- events ---- */
static void link_reset(bleh_t *h)
{
    h->cccd[0] = h->cccd[1] = 0;
    h->att_rsp_len = 0;
    /* A new accepted BLE link starts a fresh MIDI epoch, even when the old
     * disconnect event was lost. Never replay old Note Off or SysEx. */
    h->q_r = h->q_w;
    h->sx_on = h->sx_len = h->sx_pos = 0;
    h->cpu_state = 0;
    h->rx_on = 0;
    h->in_r = h->in_w = h->in_running = 0;
    sysex_end(h, -1);
}
static void evt_in(bleh_t *h, const uint8_t *p, unsigned n)
{
    unsigned code, pl;
    if (n < 2) return;
    code = p[0]; pl = p[1];
    if (pl > n - 2) return;
    p += 2;
    if (code == 0x0E && pl >= 4) {                  /* command complete: credits, opcode, status, return parameters */
        if (!h->cmd_wait || rd16(p + 1) != h->cmd_op) return;
        h->cmd_wait = 0;
        if (h->state != BLEH_STARTING) return;
        if (p[3] != 0) { h->state = BLEH_ERROR; return; }
        if (h->step == ST_BUF) {
            h->acl_total = pl >= 7 && p[6] ? p[6] : 4;
            h->credits = h->acl_total;
        }
        if (++h->step == ST_DONE) { h->state = BLEH_ADVERTISING; h->want_adv = 0; return; }
        next_cmd(h);
    } else if (code == 0x0F && pl >= 4) {           /* command status (disconnect): status, credits, opcode */
        if (h->cmd_wait && rd16(p + 2) == h->cmd_op) {
            h->cmd_wait = 0;
            /* Command Status failure means Disconnect was never accepted. Retry
             * radio-off on the next ordinary poll, without dropping MIDI. */
            if (p[0] != 0 && h->cmd_op == OP_DISC) h->cmd_op = 0;
        }
    } else if (code == 0x3E && pl >= 5 && (p[0] == 0x01 || p[0] == 0x0A)) {     /* (enhanced) connection complete */
        h->want_adv = 0;                            /* the controller stops advertising either way */
        if (p[1] == 0 && p[4] == 1) {               /* success, we are the peripheral */
            unsigned o = p[0] == 0x0A ? 24u : 12u;
            h->conn = (uint16_t)(rd16(p + 2) & 0x0FFFu);
            if (pl >= o + 4u) { h->conn_int = (uint16_t)rd16(p + o); h->conn_lat = (uint16_t)rd16(p + o + 2u); }
            h->state = BLEH_CONNECTED;
            h->credits = h->acl_total;
            link_reset(h);
        }
    } else if (code == 0x3E && pl >= 8 && p[0] == 0x03 && p[1] == 0) {   /* attempt 34: connection update complete */
        h->conn_int = (uint16_t)rd16(p + 4);
        h->conn_lat = (uint16_t)rd16(p + 6);
    } else if (code == 0x05 && pl >= 4) {           /* disconnection complete */
        /* A failed HCI disconnection is NOT a link-down notification. A live
         * Note Off/SysEx outbox must survive it. Allow radio-off to retry an
         * unsuccessful matching Disconnect command instead of parking it. */
        if (p[0] != 0) {
            if (h->state == BLEH_CONNECTED && h->cmd_op == OP_DISC &&
                (rd16(p + 1) & 0x0FFFu) == h->conn) {
                h->cmd_wait = 0;
                h->cmd_op = 0;
            }
            return;
        }
        h->disc_reason = p[3];                      /* attempt 34 (DIAG) */
        if (h->state == BLEH_CONNECTED && (rd16(p + 1) & 0x0FFFu) == h->conn) {
            h->state = BLEH_ADVERTISING;
            h->want_adv = 0;
            h->q_r = h->q_w;
            h->sx_on = 0;
            link_reset(h);
        } else if (h->state == BLEH_ADVERTISING) {
            h->want_adv = 0;                        /* attempt 28: a link the host did not see ended: advertise again */
        }
    } else if (code == 0x13 && pl >= 1) {           /* HCI handle/completion pairs, four bytes each */
        unsigned i, c = p[0];
        if (pl < 1 + 4 * c) return;
        for (i = 0; i < c; i++)
            if ((rd16(p + 1 + 4 * i) & 0x0FFFu) == h->conn) {
                unsigned k = (unsigned)h->credits + rd16(p + 3 + 4 * i);
                h->credits = (uint8_t)(k > h->acl_total ? h->acl_total : k);
            }
    }
}

/* ---- public ---- */
void bleh_init(bleh_t *h, bleh_send_fn send, void *ctx, const char *name, const uint8_t addr[6])
{
    unsigned i;
    uint8_t *z = (uint8_t *)h;
    for (i = 0; i < sizeof *h; i++) z[i] = 0;
    h->send = send; h->ctx = ctx;
    for (i = 0; name && name[i] && i < BLEH_NAME_MAX; i++) h->name[i] = name[i];
    h->namelen = (uint8_t)i;
    for (i = 0; i < 6; i++) h->addr[i] = addr ? addr[i] : (uint8_t)(0x5A + i);
    h->addr[5] |= 0xC0u;                            /* a random static address */
    h->acl_total = 4;
}
void bleh_set_sysex(bleh_t *h, bleh_sysex_fn fn) { h->sysex = fn; }
void bleh_start(bleh_t *h)
{
    h->state = BLEH_STARTING; h->step = ST_RESET; h->cmd_wait = 0; h->want_adv = 0;
    next_cmd(h);
}
void bleh_rx(bleh_t *h, uint8_t t, const uint8_t *p, uint16_t n)
{
    if (t == BLEH_H4_EVT) evt_in(h, p, n);
    else if (t == BLEH_H4_ACL && h->state == BLEH_CONNECTED) acl_in(h, p, n);
}
void bleh_radio(bleh_t *h, int on)
{
    h->radio_off = (uint8_t)!on;                    /* bleh_poll does the rest */
}
int bleh_midi_send(bleh_t *h, const uint8_t *m, uint8_t len, uint16_t ts)
{
    bleh_msg_t *q;
    unsigned i;
    if (!len || len > 3 || !(m[0] & 0x80u) || m[0] == 0xF0 || m[0] == 0xF7) return -1;   /* SysEx: bleh_sysex_send */
    if (len != msg_need(m[0])) return -1;
    for (i = 1; i < len; i++) if (m[i] & 0x80u) return -1;
    if ((uint8_t)(h->q_w - h->q_r) >= BLEH_MIDIQ) return -2;
    q = &h->q[h->q_w % BLEH_MIDIQ];
    q->len = len; q->ts = ts & 0x1FFFu;
    q->b[1] = q->b[2] = 0;
    for (i = 0; i < len; i++) q->b[i] = m[i];
    h->q_w++;
    return 0;
}
int bleh_sysex_send(bleh_t *h, const uint8_t *m, uint32_t len, uint16_t ts)
{
    uint32_t i;
    /* Check the advertised length before dereferencing the caller's tail. */
    if (!m || len < 2u || len - 2u > BLEH_SYSEX_MAX ||
        m[0] != 0xF0u || m[len - 1u] != 0xF7u) return -1;
    for (i = 1; i + 1 < len; i++) if (m[i] & 0x80u) return -1;
    if (h->sx_on) return -2;
    for (i = 0; i + 2 < len; i++) h->sx[i] = m[i + 1];
    h->sx_len = (uint8_t)(len - 2); h->sx_pos = 0; h->sx_ts = ts & 0x1FFFu; h->sx_on = 1;
    return 0;
}
int bleh_midi_next(bleh_t *h, bleh_msg_t *m)
{
    if (h->in_r == h->in_w) return 0;
    *m = h->in_q[h->in_r++ % BLEH_INQ];
    return 1;
}

/* one notification of a SysEx in progress: the first has timestamp + F0, the last timestamp + F7 */
static unsigned sysex_packet(bleh_t *h, uint8_t *r)
{
    unsigned len = 4, lo = (unsigned)(0x80u | (h->sx_ts & 0x7Fu));
    r[3] = (uint8_t)(0x80u | ((h->sx_ts >> 7) & 0x3Fu));
    if (h->sx_pos == 0 && !(h->sx_on & 2u)) {
        r[len++] = (uint8_t)lo;
        r[len++] = 0xF0;
        h->sx_on |= 2u;                             /* F0 sent */
    }
    while (h->sx_pos < h->sx_len && len < BLEH_MTU)
        r[len++] = h->sx[h->sx_pos++];
    if (h->sx_pos == h->sx_len && len + 2 <= BLEH_MTU) {
        r[len++] = (uint8_t)lo;
        r[len++] = 0xF7;
        h->sx_on = 0;
    }
    return len;
}

/* attempt 34: an L2CAP connection parameter update request (peripheral to central), once per link.
 * 1 sent, 0 not now (not connected, or sent already). */
int bleh_conn_update(bleh_t *h, uint16_t min, uint16_t max, uint16_t lat, uint16_t to)
{
    uint8_t r[12] = {0x12, 0x31, 8, 0};
    if (h->state != BLEH_CONNECTED || h->cpu_state) return 0;
    wr16(r + 4, min); wr16(r + 6, max); wr16(r + 8, lat); wr16(r + 10, to);
    if (l2cap_send(h, CID_SIG, r, 12) != 0) return 0;
    h->cpu_state = 1; h->cpu_result = 0;
    return 1;
}
void bleh_poll(bleh_t *h)
{
    uint8_t r[BLEH_MTU];
    unsigned len = 4, hi, last = 0;
    uint8_t qr;
    if (h->state == BLEH_STARTING && !h->cmd_wait) {
        next_cmd(h);                               /* retry rejected startup HCI command once per poll */
        return;
    }
    if (h->state == BLEH_CONNECTED && h->radio_off && !h->cmd_wait && h->cmd_op != OP_DISC) {
        uint8_t d[3];
        wr16(d, h->conn); d[2] = 0x13;              /* remote user terminated connection */
        cmd(h, OP_DISC, d, 3);
        return;
    }
    if (h->state == BLEH_ADVERTISING && !h->cmd_wait && h->want_adv != !h->radio_off) {
        adv_enable(h, !h->radio_off);
        return;
    }
    if (h->state != BLEH_CONNECTED && h->cmd_op == OP_DISC && !h->cmd_wait)
        h->cmd_op = 0;
    if (h->state == BLEH_CONNECTED && h->att_rsp_len) {
        if (h->credits && att_send(h, h->att_rsp, h->att_rsp_len) == 0)
            h->att_rsp_len = 0;
        return;
    }
    if (!bleh_notifying(h)) return;
    if (h->credits < (h->acl_total > 1 ? 2u : 1u)) return;                  /* keep one buffer for ATT answers */
    r[0] = A_NOTIFY; wr16(r + 1, H_MIDI);
    if (h->sx_on) {
        uint8_t pos = h->sx_pos, on = h->sx_on;
        unsigned n = sysex_packet(h, r);
        if (att_send(h, r, n) != 0) {
            h->sx_pos = pos;                        /* restore unsent SysEx frame for next poll */
            h->sx_on = on;
        }
        return;
    }
    if (h->q_r == h->q_w) return;
    qr = h->q_r;
    hi = (h->q[qr % BLEH_MIDIQ].ts >> 7) & 0x3Fu;
    r[3] = (uint8_t)(0x80u | hi);                   /* header: timestamp high */
    while (qr != h->q_w) {
        const bleh_msg_t *q = &h->q[qr % BLEH_MIDIQ];
        unsigned i = 0, run = h->tx_running && q->b[0] < 0xF0u && q->b[0] == last;
        if (((q->ts >> 7) & 0x3Fu) != hi || len + 1 + q->len - run > BLEH_MTU) break;
        r[len++] = (uint8_t)(0x80u | (q->ts & 0x7Fu));              /* timestamp low, then the message */
        if (run) i = 1;                             /* running status: the status byte left out */
        for (; i < q->len; i++) r[len++] = q->b[i];
        if (q->b[0] < 0xF0u) last = q->b[0];
        else if (q->b[0] < 0xF8u) last = 0;         /* system common ends running status */
        qr++;
    }
    if (att_send(h, r, len) == 0)
        h->q_r = qr;                                /* only dequeue MIDI accepted into the mailbox */
}
