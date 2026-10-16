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
static u8 hx_radio = 1, hx_started;
static char hx_name[10];
#define HX (ctl_mb->hx)

static void hx_report(void)
{
    u8 s[8];
    s[0] = HX.state; s[1] = HX.ccc; s[2] = HX.bonded; s[3] = HX.reason;
    s[4] = (u8)HX.interval; s[5] = (u8)(HX.interval >> 8); s[6] = (u8)HX.mtu; s[7] = (u8)(HX.mtu >> 8);
    hx_to_app(XB_HX_STATE, s, sizeof s);
}

static void hx_adv(int on)
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
        hx_to_app(XB_HX_MIDI, buf, size);
    } else if (h == H_IO_CCC && size >= 1) {
        HX.ccc = buf[0] & 1u;
        att_set_ccc_config(h, HX.ccc ? ATT_OP_NOTIFY : 0);
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
    le_controller_set_random_mac((void *)ctl_bp->addr);           /* the app's random static address */
    ble_user_cmd_prepare(BLE_CMD_SET_HCI_CFG, 2, HCI_CFG_OWN_ADDRESS_TYPE, 1);
    if (hx_radio) hx_adv(1);
}
u8 ctl_d_ev_n, ctl_d_ev_last;          /* attempt 45 DIAG: host events seen, the last event code */
static void hx_events(u8 type, u16 ch, u8 *p, u16 size)
{
    (void)ch;
    if (ctl_d_ev_n < 255u) ctl_d_ev_n++;
    if (size) ctl_d_ev_last = p[0];
    if (type != HCI_EVENT_PACKET || size < 2) return;
    switch (p[0]) {
    case EV_STATE:
        if (p[2] == 2) hx_start();                                 /* HCI_STATE_WORKING (if it ever comes) */
        break;
    case EV_LE_META:
        if ((p[2] == 0x01 && p[3] == 0 && size >= 16) ||           /* connection complete, success */
            (p[2] == 0x0A && p[3] == 0 && size >= 28)) {           /* enhanced (+ 2 RPAs), as the SDK app */
            u32 o = p[2] == 0x0A ? 26u : 14u;
            hx_conn = (u16)(p[4] | (p[5] << 8));
            HX.interval = (u16)(p[o] | (p[o + 1] << 8));   /* sub, status, handle, role, peer type, peer address, [RPAs,] interval */
            HX.ccc = 0; HX.mtu = 23; HX.bonded = 0;
            HX.state = XB_LK_CONN;
            HX.links++;
            ble_user_cmd_prepare(BLE_CMD_ATT_SEND_INIT, 4, hx_conn, hx_att_buf, HX_ATT_BUF, HX_MTU_MAX);
            hx_report();
        } else if (p[2] == 0x03 && p[3] == 0 && size >= 8) {       /* connection update complete */
            HX.interval = (u16)(p[6] | (p[7] << 8));
            hx_report();
        }
        break;
    case EV_DISCONNECT:
        HX.reason = size >= 6 ? p[5] : 0;
        hx_conn = 0; HX.ccc = 0;
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
            ble_user_cmd_prepare(BLE_CMD_ATT_MTU_SIZE, 1, HX.mtu - 3);
            hx_report();
        }
        break;
    default:
        break;
    }
}

static void hx_sm(u8 type, u16 ch, u8 *p, u16 size)
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
        if (!hx_radio && hx_conn) ble_user_cmd_prepare(BLE_CMD_DISCONNECT, 1, hx_conn);
        if (hx_started && !hx_conn) hx_adv(hx_radio);
    }
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
