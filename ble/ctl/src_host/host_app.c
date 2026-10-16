// SPDX-License-Identifier: MPL-2.0
/* Copyright (C) 2026 M. Geisler (genkigenki/makkuro)
 *
 * BLE MIDI on JieLi's own BLE host (btstack.a from the official AC79 SDK, fetched and linked on the
 * user's computer, never bundled). Written from the public BLE-MIDI and GATT specifications and the
 * SDK's public headers (Apache-2.0); no SDK source is copied.
 *
 * To the app (firmware/hal/x0x_ble_abi.h, ABI 3) it passes BLE-MIDI packets, not HCI:
 *   c2h XB_HX_MIDI   one BLE-MIDI packet the central wrote (header byte first)
 *   c2h XB_HX_STATE  a link change (the 8 link bytes of xb_hx_t); the mailbox's hx has them too
 *   h2c XB_HX_MIDI   one BLE-MIDI packet to notify (up to MTU - 3 bytes)
 *   h2c XB_HX_CMD    [0] = 0 radio off (link down, no advertising), 1 on, 2 forget the bonds
 * The app keeps its BLE-MIDI parser and packer and all the APC40 handling.
 *
 * The GATT table is the same as the app's own host had (handles 1..7: GAP name, the BLE-MIDI
 * service, its characteristic read / write without response / notify, the CCCD): a central that
 * cached it before sees the same table. No parameter request (attempt 37). Just Works pairing with
 * bonding when the central asks for it; the bonds go to the bond store (ctl_syscfg.c). */
#include "ctl_port.h"
#include "host_abi.h"

/* ---- the host's API (declarations as in include_lib/btstack, Apache-2.0) ---- */
typedef void (*hx_ph_t)(u8 packet_type, u16 channel, u8 *packet, u16 size);
extern void le_device_db_init(void);
extern void sm_init(void);
extern void sm_set_io_capabilities(int io);
extern void sm_set_authentication_requirements(u8 auth);
extern void sm_set_encryption_key_size_range(u8 min, u8 max);
extern void sm_set_request_security(int en);
extern void sm_event_callback_set(hx_ph_t cb);
extern void sm_just_works_confirm(u16 con_handle);
extern void att_server_init(const u8 *db, u16 (*rd)(u16, u16, u16, u8 *, u16),
                            int (*wr)(u16, u16, u16, u16, u8 *, u16));
extern void att_server_register_packet_handler(hx_ph_t h);
extern void att_set_ccc_config(u16 handle, u16 cfg);
extern void hci_event_callback_set(hx_ph_t h);
extern void le_l2cap_register_packet_handler(hx_ph_t h);
extern u16 ble_vendor_set_default_att_mtu(u16 mtu);
extern int ble_user_cmd_prepare(int cmd, int argc, ...);
extern int le_controller_set_random_mac(void *addr);
extern u32 ctl_ms(void);
/* a62: stock btstack GAP conn-param update (resolved at LTO link). Apple-compliant values. */
extern int gap_le_request_connection_parameter_update(u16 handle, u16 min, u16 max, u16 latency, u16 timeout);
/* a63: Apple-compliant conn-param values, a compile-time pair (1.25 ms units).
 * a63-15 (default): 12/24 = 15..30 ms.  a63-7: -DA63_CP_MIN=6 -DA63_CP_MAX=12 = 7.5..15 ms. */
#ifndef A63_CP_MIN
#define A63_CP_MIN 12u
#endif
#ifndef A63_CP_MAX
#define A63_CP_MAX 24u
#endif

enum { BLE_CMD_ADV_ENABLE = 1, BLE_CMD_ADV_PARAM, BLE_CMD_ADV_DATA, BLE_CMD_RSP_DATA, BLE_CMD_DISCONNECT,
       BLE_CMD_REGIEST_THREAD, BLE_CMD_ATT_SEND_INIT, BLE_CMD_ATT_MTU_SIZE, BLE_CMD_ATT_VAILD_LEN,
       BLE_CMD_ATT_SEND_DATA, BLE_CMD_SET_HCI_CFG = 21 };          /* ble_api.h ble_cmd_type_e */
#define HCI_CFG_OWN_ADDRESS_TYPE 0
#define IO_NO_INPUT_NO_OUTPUT 3
#define SM_AUTHREQ_BONDING 0x01
#define SM_AUTHREQ_MITM 0x04
#define ATT_OP_NOTIFY 1                                            /* att_op_type_e */
#define HCI_EVENT_PACKET 0x04
#define EV_DISCONNECT 0x05
#define EV_ENCRYPTION 0x08
#define EV_LE_META 0x3E
#define EV_MTU 0xB5
#define EV_STATE 0x60
#define EV_JUST_WORKS 0xD0
#define EV_PAIR_PROCESS 0xDF

/* ---- the GATT table (BTstack ATT DB layout: len, flags, handle, type, value; 0 0 ends) ---- */
#define F_RD 0x0002u
#define F_WWR 0x0004u
#define F_WR 0x0008u
#define F_NT 0x0010u
#define F_DYN 0x0100u
#define F_U128 0x0200u
#define L16(v) (u8)((v) & 0xFF), (u8)((v) >> 8)
#define U_MIDI_SVC 0x00,0xC7,0xC4,0x4E,0xE3,0x6C,0x51,0xA7,0x33,0x4B,0xE8,0xED,0x5A,0x0E,0xB8,0x03
#define U_MIDI_IO  0xF3,0x6B,0x10,0x9D,0x66,0xF2,0xA9,0xA1,0x12,0x41,0x68,0x38,0xDB,0xE5,0x72,0x77
enum { H_GAP = 1, H_NAME_DECL, H_NAME, H_MIDI_SVC, H_IO_DECL, H_IO, H_IO_CCC };
static const u8 hx_db[] = {
    10, 0, L16(F_RD), L16(H_GAP), L16(0x2800), L16(0x1800),                         /* GAP */
    13, 0, L16(F_RD), L16(H_NAME_DECL), L16(0x2803), 0x02, L16(H_NAME), L16(0x2A00),/* name: read */
    8, 0, L16(F_RD | F_DYN), L16(H_NAME), L16(0x2A00),
    24, 0, L16(F_RD), L16(H_MIDI_SVC), L16(0x2800), U_MIDI_SVC,                    /* BLE-MIDI */
    27, 0, L16(F_RD), L16(H_IO_DECL), L16(0x2803), 0x16, L16(H_IO), U_MIDI_IO,      /* read, write w/o rsp, notify */
    22, 0, L16(F_RD | F_WWR | F_WR | F_NT | F_DYN | F_U128), L16(H_IO), U_MIDI_IO,
    10, 0, L16(F_RD | F_WR | F_DYN), L16(H_IO_CCC), L16(0x2902), 0, 0,              /* CCCD */
    0, 0
};

/* ---- state ---- */
#define HX_ATT_BUF (188u + HX_MTU_MAX + 256u)       /* ATT_CTRL_BLOCK_SIZE + local MTU + send buffer */
static u8 hx_att_buf[HX_ATT_BUF] __attribute__((aligned(4), section(".ctl_ram2")));
static u16 hx_conn;
static u32 hx_ccc_ms, hx_cp_last;   /* a62/a63: when CCCD went on, and when we last sent the conn-param request */
static u8 hx_cp_sent, hx_cp_rc, hx_setup_flags;   /* a63: conn-param send count, last rc; setup flags b0 conn b1 mtu b2 att b3 ccc */
/* a63 setup ring: a rolling 8-nibble history of the per-connection setup (newest in low nibble),
 * surfaced as DIAG d_smlog. tags: 1 conn-complete, 2 MTU, 3 first ATT write, 4 CCCD-on,
 * 5 conn-param request sent, 6 conn-update-complete, 7 disconnect. */
#define SR(tag) do { HX.smlog = (HX.smlog << 4) | ((u32)(tag) & 0xfu); } while (0)
static u8 hx_radio = 1, hx_started;
static char hx_name[10];
#define HX (ctl_mb->hx)

__attribute__((section(".ble_sm_code.a62"))) static void hx_cp_pack(void)
{
    HX.pad_a62 = (u16)((hx_cp_sent & 0xfu) | ((u32)(hx_cp_rc & 0xfu) << 4)
        | ((u32)(hx_setup_flags & 0xfu) << 8) | ((u32)(HX.updates ? 1u : 0u) << 12));
}

static void hx_report(void)
{
    u8 s[8];
    s[0] = HX.state; s[1] = HX.ccc; s[2] = HX.bonded; s[3] = HX.reason;
    s[4] = (u8)HX.interval; s[5] = (u8)(HX.interval >> 8); s[6] = (u8)HX.mtu; s[7] = (u8)(HX.mtu >> 8);
    hx_to_app(XB_HX_STATE, s, sizeof s);
}

__attribute__((section(".ble_sm_code.a62"))) static void hx_adv(int on)
{
    static u8 adv[31], rsp[31];
    static const u8 u[16] = {U_MIDI_SVC};
    u32 n = 0, k, nl = strlen(hx_name);
    if (on) {
        adv[n++] = 2; adv[n++] = 0x01; adv[n++] = 0x06;            /* flags: LE general, no BR/EDR */
        adv[n++] = 17; adv[n++] = 0x07;                            /* complete 128-bit UUIDs: BLE-MIDI */
        for (k = 0; k < 16; k++) adv[n++] = u[k];
        rsp[0] = (u8)(nl + 1); rsp[1] = 0x09;                      /* complete local name */
        for (k = 0; k < nl; k++) rsp[2 + k] = (u8)hx_name[k];
        ble_user_cmd_prepare(BLE_CMD_ADV_PARAM, 3, 0x30, 0, 7);   /* 30 ms, ADV_IND, all channels */
        ble_user_cmd_prepare(BLE_CMD_ADV_DATA, 2, n, adv);
        ble_user_cmd_prepare(BLE_CMD_RSP_DATA, 2, nl + 2, rsp);
    }
    ble_user_cmd_prepare(BLE_CMD_ADV_ENABLE, 1, on ? 1 : 0);
    HX.state = on ? XB_LK_ADV : XB_LK_IDLE;
    hx_report();
}

static u16 hx_read(u16 con, u16 h, u16 off, u8 *buf, u16 size)
{
    (void)con;
    if (h == H_NAME) {
        u16 n = (u16)strlen(hx_name);
        if (!buf) return n;
        if (off >= n) return 0;
        n = (u16)(n - off); if (n > size) n = size;
        memcpy(buf, hx_name + off, n);
        return n;
    }
    if (h == H_IO_CCC) {
        if (!buf) return 2;
        if (size >= 2) { buf[0] = HX.ccc; buf[1] = 0; }
        return 2;
    }
    return 0;                                                      /* BLE-MIDI: a read gives no data */
}

static int hx_write(u16 con, u16 h, u16 mode, u16 off, u8 *buf, u16 size)
{
    (void)con; (void)mode; (void)off;
    if (h == H_IO && size >= 2) {
        HX.rx++;
        if (!(hx_setup_flags & 4u)) { hx_setup_flags |= 4u; SR(3); hx_cp_pack(); }   /* a63 first ATT write */
        hx_to_app(XB_HX_MIDI, buf, size);
    } else if (h == H_IO_CCC && size >= 1) {
        u8 was = HX.ccc;
        HX.ccc = buf[0] & 1u;
        att_set_ccc_config(h, HX.ccc ? ATT_OP_NOTIFY : 0);
        if (HX.ccc && !was) { hx_ccc_ms = ctl_ms(); hx_cp_sent = 0; hx_cp_last = 0;
                              hx_setup_flags |= 8u; SR(4); hx_cp_pack(); }   /* a62/a63: arm the conn-param update */
        hx_report();
    }
    return 0;
}

/* Attempt 50: start the link layer use (address, advertising). The SDK's apps do this on
 * BT_STATUS_INIT_OK (bt_event_notify, a no-op here); this host build never emits a STATE event
 * (disassembly: le_hci_emit_event only forwards controller events, and the controller answers
 * only commands), so EV_STATE never came and nothing ever asked the controller for anything.
 * ctl_poll calls this until it starts: once the host task ran btstack_task_init and waits in
 * its queue (its user command buffer is set up then). */
extern u8 ctl_d_ran;
u32 ctl_os_d_waiting(void);
__attribute__((noinline, section(".bt_stack_code.d50"))) void hx_start(void)
{
    if (hx_started || !(ctl_d_ran & 2u) || !(ctl_os_d_waiting() & 2u)) return;
    hx_started = 1;
    hx_init();                                                    /* attempt 59: profiles/handlers after btstack_task_init (stock order), so le_hci_stack keeps hx_events */
    le_controller_set_random_mac((void *)ctl_bp->addr);           /* the app's random static address */
    ble_user_cmd_prepare(BLE_CMD_SET_HCI_CFG, 2, HCI_CFG_OWN_ADDRESS_TYPE, 1);
    if (hx_radio) hx_adv(1);
}
u8 ctl_d_ev_n, ctl_d_ev_last;          /* attempt 45 DIAG: host events seen, the last event code */
static u8 hx_disc_i;
static void ev_push(volatile u32 *w, u8 c) { *w = (*w << 8) | c; }

/* a64 DIAG record. JieLi's btstack_task gives each HCI event to the handlers with size = parameter
 * length + 1 (disassembly: hci_vendor.c:1009/1014 pass b[ev+1] + 1), one byte short. So a
 * Disconnection Complete (code, length 4, status, handle, reason) arrives with size 5, but its
 * reason is at p[5], where the SDK apps read it. The record is in the mailbox's c2h ring pads: only
 * the image writes them, the app never uses them, and the app clears the mailbox before each start.
 *   pad0[0]    0xA64 << 20 | disconnects << 8 | connections (both mod 256)
 *   pad0[1..2] per connection (slot = count & 1): interval (0-11), supervision timeout (12-23),
 *              latency (24-29, max 63), role (30, 1 = peripheral), peer address type (31, 1 = random)
 *   pad0[3..6] per disconnect (slot = count & 1, 2 words each): the event's first 8 bytes, raw
 *   pad1[0..1] per disconnect: size given (0-7), we asked to disconnect (8), notify on (9), setup
 *              flags (12-15), ms from connection complete to disconnect (16-31, max 65535)
 *   pad1[2]    the last 4 HCI event codes (newest in the low byte) */
#define DG_MAGIC 0xA6400000u
static u32 hx_conn_ms;
static u8 hx_local;                 /* a64: we asked the controller to end this link */
__attribute__((noinline, section(".ble_sm_code.a64"))) static void dg_conn(const u8 *p, u32 o)
{
    u32 *w = ctl_mb->c2h.pad0, h = w[0], n, lat = (u32)(p[o + 2] | (p[o + 3] << 8));
    if ((h & 0xFFF00000u) != DG_MAGIC) h = DG_MAGIC;
    n = h & 0xFFu;
    w[1u + (n & 1u)] = ((u32)(p[o] | (p[o + 1] << 8)) & 0xFFFu) |
        (((u32)(p[o + 4] | (p[o + 5] << 8)) & 0xFFFu) << 12) | ((lat > 63u ? 63u : lat) << 24) |
        ((u32)(p[6] & 1u) << 30) | ((u32)(p[7] & 1u) << 31);
    w[0] = (h & ~0xFFu) | ((n + 1u) & 0xFFu);
    hx_conn_ms = ctl_ms();
    hx_local = 0;
}
__attribute__((noinline, section(".ble_sm_code.a64"))) static void dg_disc(const u8 *p, u32 size, u32 flags)
{
    u32 *w = ctl_mb->c2h.pad0, h = w[0], n, s, d = ctl_ms() - hx_conn_ms;
    if ((h & 0xFFF00000u) != DG_MAGIC) h = DG_MAGIC;
    n = (h >> 8) & 0xFFu;
    s = n & 1u;
    w[3u + 2u * s] = (u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) | ((u32)p[3] << 24);
    w[4u + 2u * s] = (u32)p[4] | ((u32)p[5] << 8) | ((u32)p[6] << 16) | ((u32)p[7] << 24);
    ctl_mb->c2h.pad1[s] = (size & 0xFFu) | ((u32)(hx_local & 1u) << 8) | (flags << 9) |
        ((d > 0xFFFFu ? 0xFFFFu : d) << 16);
    w[0] = (h & ~0xFF00u) | (((n + 1u) & 0xFFu) << 8);
}

static void hx_events(u8 type, u16 ch, u8 *p, u16 size)
{
    (void)ch;
    if (ctl_d_ev_n < 255u) ctl_d_ev_n++;
    if (size) ctl_d_ev_last = p[0];
    if (type != HCI_EVENT_PACKET || size < 2) return;
    ctl_mb->c2h.pad1[2] = (ctl_mb->c2h.pad1[2] << 8) | p[0];      /* a64: the last 4 event codes */
    switch (p[0]) {
    case EV_STATE:
        if (p[2] == 2) hx_start();                                 /* HCI_STATE_WORKING (if it ever comes) */
        break;
    case EV_LE_META:
        ev_push(&HX.evlog, p[2]);                                  /* a61: LE subevent trace */
        if ((p[2] == 0x01 && p[3] == 0 && size >= 16) ||           /* connection complete, success */
            (p[2] == 0x0A && p[3] == 0 && size >= 28)) {           /* enhanced (+ 2 RPAs), as the SDK app */
            u32 o = p[2] == 0x0A ? 26u : 14u;
            hx_conn = (u16)(p[4] | (p[5] << 8));
            HX.interval = (u16)(p[o] | (p[o + 1] << 8));   /* sub, status, handle, role, peer type, peer address, [RPAs,] interval */
            if (size >= o + 6u) { HX.latency = (u16)(p[o + 2] | (p[o + 3] << 8));   /* a61: latency, supervision timeout */
                                  HX.timeout = (u16)(p[o + 4] | (p[o + 5] << 8)); }
            HX.updates = 0;                                        /* a61: per-link update count */
            HX.ccc = 0; HX.mtu = 23; HX.bonded = 0;
            HX.state = XB_LK_CONN;
            HX.links++;
            hx_cp_sent = 0; hx_cp_rc = 0; hx_cp_last = 0; hx_setup_flags = 1u;   /* a63 */
            dg_conn(p, o);                                         /* a64: raw role, interval, latency, timeout */
            SR(1); hx_cp_pack();
            ble_user_cmd_prepare(BLE_CMD_ATT_SEND_INIT, 4, hx_conn, hx_att_buf, HX_ATT_BUF, HX_MTU_MAX);
            hx_report();
        } else if (p[2] == 0x03 && p[3] == 0 && size >= 8) {       /* connection update complete */
            HX.interval = (u16)(p[6] | (p[7] << 8));
            HX.updates++;                                          /* a61 */
            SR(6); hx_cp_pack();                                   /* a63 */
            hx_report();
        }
        break;
    case EV_DISCONNECT:
        HX.reason = p[5];                          /* a64: the reason is p[5] (size is one short, see above) */
        HX.disc[hx_disc_i & 3u] = (u32)HX.reason | ((u32)(size & 0xffu) << 8) |   /* a61: reason, ev size, status, ccc-was, updates */
            ((u32)p[2] << 16) | ((u32)(HX.ccc & 1u) << 24) | ((u32)(HX.updates & 0x7fu) << 25);
        hx_disc_i++;
        dg_disc(p, size, (HX.ccc & 1u) | ((u32)(hx_setup_flags & 0xFu) << 3));   /* a64: raw bytes, side, time */
        SR(7);                                                    /* a63 */
        hx_conn = 0; HX.ccc = 0; hx_cp_sent = 0; hx_setup_flags = 0;
        ble_user_cmd_prepare(BLE_CMD_ATT_SEND_INIT, 4, 0, 0, 0, 0);
        if (hx_radio) hx_adv(1); else { HX.state = XB_LK_IDLE; hx_report(); }
        break;
    case EV_ENCRYPTION:
        if (size >= 6 && p[2] == 0 && p[5]) { HX.bonded = 1; hx_report(); }
        break;
    case EV_MTU:
        if (size >= 6) {
            u16 m = (u16)(p[4] | (p[5] << 8));
            HX.mtu = m > HX_MTU_MAX ? HX_MTU_MAX : m;
            hx_setup_flags |= 2u; SR(2); hx_cp_pack();             /* a63 */
            ble_user_cmd_prepare(BLE_CMD_ATT_MTU_SIZE, 1, HX.mtu - 3);
            hx_report();
        }
        break;
    default:
        break;
    }
}

/* a64: in piece 2b (it runs only for a pairing), so piece 1 keeps room for the a64 DIAG calls */
__attribute__((noinline, section(".bt_stack_code.a64"))) static void hx_sm(u8 type, u16 ch, u8 *p, u16 size)
{
    (void)ch;
    if (type != HCI_EVENT_PACKET || size < 4) return;
    if (p[0] == EV_JUST_WORKS) {
        sm_just_works_confirm((u16)(p[2] | (p[3] << 8)));
        HX.pairs++;
    } else if (p[0] == EV_PAIR_PROCESS && size >= 5 && (p[4] == 0x03 || p[4] == 0x04 || p[4] == 0x05)) {
        HX.pairs += 0x10000u;                                      /* failed, timed out, encryption failed */
    }
}

/* 1 = a notification of n bytes fits the host's send buffer now (or there is no link: it is dropped) */
int hx_can_send(unsigned n)
{
    u32 room = 0;
    if (!hx_conn || !HX.ccc)
        return 1;
    if (ble_user_cmd_prepare(BLE_CMD_ATT_VAILD_LEN, 1, &room) != 0)
        return 1;
    return room >= n + 8u;                                         /* + its record in the buffer */
}

/* from the app (h2c frames, ctl_host_poll) */
void hx_from_app(unsigned char type, const unsigned char *p, unsigned n)
{
    if (type == XB_HX_MIDI) {
        if (hx_conn && HX.ccc && n && n <= (unsigned)(HX.mtu - 3)) {
            if (ble_user_cmd_prepare(BLE_CMD_ATT_SEND_DATA, 4, H_IO, p, n, ATT_OP_NOTIFY) == 0) HX.tx++;
            else HX.tx_drops++;
        } else
            HX.tx_drops++;
    } else if (type == XB_HX_CMD && n >= 1) {
        if (p[0] == 2) {
            ctl_nv_forget();                                       /* ctl_syscfg.c: from the next start on */
            return;
        }
        hx_radio = p[0] & 1u;
        if (!hx_radio && hx_conn) { hx_local = 1; ble_user_cmd_prepare(BLE_CMD_DISCONNECT, 1, hx_conn); }
        if (hx_started && !hx_conn) hx_adv(hx_radio);
    }
}

/* a62: ~1.5 s after the central turns on notifications, ask for Apple-compliant connection
 * parameters (15-30 ms, latency 0, supervision timeout 2000 ms). iOS is strict and our stock
 * advertise/accept leaves the fast 7.5 ms interval it picks; a longer, stable interval with a 2 s
 * timeout tolerates the core-1 cooperative servicing. The result shows up as a conn-update-complete
 * (HX.interval, HX.updates). Called from ctl_poll. */
__attribute__((section(".ble_sm_code.a62"))) void hx_tick(void)
{
    u32 now, due, wait;
    if (!hx_conn || !HX.ccc) return;        /* a63: only after a link is up and notifications are on */
    if (HX.updates) return;                 /* the central already applied our parameters */
    if (hx_cp_sent >= 3u) return;           /* a63: gave up after 3 tries */
    now = ctl_ms();
    due = hx_cp_sent ? hx_cp_last : hx_ccc_ms;
    wait = hx_cp_sent ? 1000u : 1500u;      /* first ~1.5 s after CCCD, then retry every ~1 s */
    if ((s32)(now - due) < (s32)wait) return;
    hx_cp_rc = (u8)gap_le_request_connection_parameter_update(hx_conn, A63_CP_MIN, A63_CP_MAX, 0u, 200u);
    hx_cp_sent++; hx_cp_last = now; SR(5); hx_cp_pack();   /* a63: observable, retried, never one-shot-lost */
}

/* the host's set-up, before btstack_init (ctl_init) */
void hx_init(void)
{
    u32 i;
    for (i = 0; i + 1 < sizeof hx_name && ctl_bp->name[i]; i++) hx_name[i] = ctl_bp->name[i];
    hx_name[i] = 0;
    ctl_mark(0x311);
    le_device_db_init();
    ctl_mark(0x312);
    sm_init();
    ctl_mark(0x313);
    sm_set_io_capabilities(IO_NO_INPUT_NO_OUTPUT);               /* Just Works */
    sm_set_authentication_requirements(SM_AUTHREQ_BONDING | SM_AUTHREQ_MITM); /* as the SDK app: NoIO keeps it Just Works */
    sm_set_encryption_key_size_range(7, 16);
    sm_set_request_security(0);                                  /* the central decides (iOS pairs when it wants) */
    sm_event_callback_set(hx_sm);
    ctl_mark(0x314);
    att_server_init(hx_db, hx_read, hx_write);
    ctl_mark(0x315);
    att_server_register_packet_handler(hx_events);
    hci_event_callback_set(hx_events);
    le_l2cap_register_packet_handler(hx_events);
    ble_vendor_set_default_att_mtu(HX_MTU_MAX);
    ctl_mark(0x316);
}
