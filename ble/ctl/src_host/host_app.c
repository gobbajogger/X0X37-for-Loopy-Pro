// SPDX-License-Identifier: MPL-2.0
/* Copyright (C) 2026 M. Geisler (genkigenki/makkuro)
 *
 * BLE MIDI on JieLi's own BLE host (btstack.a from the official AC79 SDK, fetched and linked
 * on the user's computer, never bundled). Written from the public BLE-MIDI and GATT specs and
 * the SDK's public headers (Apache-2.0); no SDK source is copied.
 *
 * The image now holds controller + host. To the app it passes BLE-MIDI packets, not HCI:
 *   c2h type HX_MIDI  : one BLE-MIDI packet the central wrote (header byte first)
 *   c2h type HX_STATE : state byte (HX_ST_*), conn interval (u16), mtu (u16), end reason, bonded
 *   h2c type HX_MIDI  : one BLE-MIDI packet to notify (<= mtu - 3)
 *   h2c type HX_CMD   : cmd byte: 1 radio on, 0 radio off (disconnect + advertising off)
 * The app keeps its BLE-MIDI parser and packer (ble_midi_host.c) and the APC40 handling. */
#include "ctl_port.h"
#include "host_abi.h"

/* ---- the host's API (signatures from include_lib/btstack, Apache-2.0) ---- */
typedef void (*hx_ph_t)(u8 packet_type, u16 channel, u8 *packet, u16 size);
extern void btstack_init(void);
extern void le_device_db_init(void);
extern void sm_init(void);
extern void sm_set_io_capabilities(int io);
extern void sm_set_authentication_requirements(u8 auth);
extern void sm_set_encryption_key_size_range(u8 min, u8 max);
extern void sm_set_request_security(int en);
extern void sm_event_callback_set(hx_ph_t cb);
extern void att_server_init(const u8 *db, u16 (*rd)(u16, u16, u16, u8 *, u16),
                            int (*wr)(u16, u16, u16, u16, u8 *, u16));
extern void att_server_register_packet_handler(hx_ph_t h);
extern void hci_event_callback_set(hx_ph_t h);
extern void le_l2cap_register_packet_handler(hx_ph_t h);
extern u16 ble_vendor_set_default_att_mtu(u16 mtu);
extern int ble_user_cmd_prepare(int cmd, int argc, ...);

enum { BLE_CMD_ADV_ENABLE = 1, BLE_CMD_ADV_PARAM, BLE_CMD_ADV_DATA, BLE_CMD_RSP_DATA, BLE_CMD_DISCONNECT,
       BLE_CMD_REGIEST_THREAD, BLE_CMD_ATT_SEND_INIT, BLE_CMD_ATT_MTU_SIZE, BLE_CMD_ATT_VAILD_LEN,
       BLE_CMD_ATT_SEND_DATA };
#define IO_NO_INPUT_NO_OUTPUT 3
#define SM_AUTHREQ_BONDING 0x01
#define ATT_OP_NOTIFY 1                       /* att_op_type_e: notify, no CCC check (we check) */
#define HCI_EVENT_PACKET 0x04
#define HCI_EVENT_DISCONNECTION_COMPLETE 0x05
#define HCI_EVENT_ENCRYPTION_CHANGE 0x08
#define HCI_EVENT_LE_META 0x3E
#define ATT_EVENT_MTU_EXCHANGE_COMPLETE 0xB5
#define BTSTACK_EVENT_STATE 0x60

/* ---- the GATT database (BTstack ATT DB layout: len, flags, handle, type, value; 0 0 ends) ----
 * GAP 1800 (device name), GATT 1801, BLE-MIDI 03B80E5A-EDE8-4B33-A751-6CE34EC4C700 with the
 * MIDI I/O characteristic 7772E5DB-3868-4112-A1A9-F2669D106BF3: read, write without response,
 * write, notify; and its CCCD. */
#define F_RD 0x0002u
#define F_WWR 0x0004u
#define F_WR 0x0008u
#define F_NT 0x0010u
#define F_DYN 0x0100u
#define F_U128 0x0200u
#define L16(v) (u8)((v) & 0xFF), (u8)((v) >> 8)
#define U_MIDI_SVC 0x00,0xC7,0xC4,0x4E,0xE3,0x6C,0x51,0xA7,0x33,0x4B,0xE8,0xED,0x5A,0x0E,0xB8,0x03
#define U_MIDI_IO  0xF3,0x6B,0x10,0x9D,0x66,0xF2,0xA9,0xA1,0x12,0x41,0x68,0x38,0xDB,0xE5,0x72,0x77
enum { H_GAP = 1, H_NAME_DECL, H_NAME, H_GATT, H_MIDI_SVC, H_IO_DECL, H_IO, H_IO_CCC };
static const u8 hx_db[] = {
    /* 1: primary service 1800 */
    10, 0, L16(F_RD), L16(H_GAP), L16(0x2800), L16(0x1800),
    /* 2: characteristic: read, name at 3 */
    13, 0, L16(F_RD), L16(H_NAME_DECL), L16(0x2803), 0x02, L16(H_NAME), L16(0x2A00),
    /* 3: device name (dynamic: hx_read) */
    8, 0, L16(F_RD | F_DYN), L16(H_NAME), L16(0x2A00),
    /* 4: primary service 1801 */
    10, 0, L16(F_RD), L16(H_GATT), L16(0x2800), L16(0x1801),
    /* 5: primary service BLE-MIDI (128-bit value) */
    24, 0, L16(F_RD), L16(H_MIDI_SVC), L16(0x2800), U_MIDI_SVC,
    /* 6: characteristic: read | write w/o rsp | write | notify, value at 7, 128-bit uuid */
    27, 0, L16(F_RD), L16(H_IO_DECL), L16(0x2803), 0x1E, L16(H_IO), U_MIDI_IO,
    /* 7: MIDI I/O value */
    22, 0, L16(F_RD | F_WWR | F_WR | F_NT | F_DYN | F_U128), L16(H_IO), U_MIDI_IO,
    /* 8: CCCD */
    10, 0, L16(F_RD | F_WR | F_DYN), L16(H_IO_CCC), L16(0x2902), 0, 0,
    0, 0
};

/* ---- state ---- */
#define HX_ATT_BUF (188u + HX_MTU_MAX + 512u)       /* ATT_CTRL_BLOCK_SIZE + local MTU + send buffer */
static u8 hx_att_buf[HX_ATT_BUF] __attribute__((aligned(4)));
static u16 hx_conn, hx_interval, hx_mtu = 23;
static u8 hx_ccc, hx_state, hx_reason, hx_bonded, hx_radio = 1, hx_started;
static char hx_name[24] = "X0X";

static void hx_report(void)
{
    u8 s[8];
    s[0] = hx_state; s[1] = (u8)hx_interval; s[2] = (u8)(hx_interval >> 8);
    s[3] = (u8)hx_mtu; s[4] = (u8)(hx_mtu >> 8); s[5] = hx_reason; s[6] = hx_bonded; s[7] = hx_ccc;
    hx_to_app(HX_STATE, s, sizeof s);
}

static void hx_adv(int on)
{
    static u8 adv[31], rsp[31];
    u32 n = 0, k, nl = strlen(hx_name);
    if (on) {
        adv[n++] = 2; adv[n++] = 0x01; adv[n++] = 0x06;                /* flags: LE general, no BR/EDR */
        adv[n++] = 17; adv[n++] = 0x07;                                /* complete 128-bit UUIDs: BLE-MIDI */
        { static const u8 u[16] = {U_MIDI_SVC}; for (k = 0; k < 16; k++) adv[n++] = u[k]; }
        rsp[0] = (u8)(nl + 1); rsp[1] = 0x09;
        for (k = 0; k < nl; k++) rsp[2 + k] = (u8)hx_name[k];
        ble_user_cmd_prepare(BLE_CMD_ADV_PARAM, 3, 0x40, 0, 7);       /* 40 ms, ADV_IND, all channels */
        ble_user_cmd_prepare(BLE_CMD_ADV_DATA, 2, n, adv);
        ble_user_cmd_prepare(BLE_CMD_RSP_DATA, 2, nl + 2, rsp);
    }
    ble_user_cmd_prepare(BLE_CMD_ADV_ENABLE, 1, on ? 1 : 0);
    hx_state = on ? HX_ST_ADV : HX_ST_IDLE;
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
    if (h == H_IO) return 0;                                           /* BLE-MIDI: a read gives no data */
    if (h == H_IO_CCC) {
        if (!buf) return 2;
        if (size >= 2) { buf[0] = hx_ccc; buf[1] = 0; }
        return 2;
    }
    return 0;
}

static int hx_write(u16 con, u16 h, u16 mode, u16 off, u8 *buf, u16 size)
{
    (void)con; (void)mode; (void)off;
    if (h == H_IO && size >= 2) {
        hx_to_app(HX_MIDI, buf, size);
    } else if (h == H_IO_CCC && size >= 2) {
        hx_ccc = buf[0] & 1u;
        if (hx_ccc)
            ble_user_cmd_prepare(BLE_CMD_ATT_SEND_INIT, 4, hx_conn, hx_att_buf, HX_ATT_BUF, HX_MTU_MAX);
        hx_report();
    }
    return 0;
}

static void hx_events(u8 type, u16 ch, u8 *p, u16 size)
{
    (void)ch;
    if (type != HCI_EVENT_PACKET || size < 2) return;
    switch (p[0]) {
    case BTSTACK_EVENT_STATE:
        if (p[2] == 2 && !hx_started) {                                /* HCI_STATE_WORKING */
            hx_started = 1;
            if (hx_radio) hx_adv(1);
        }
        break;
    case HCI_EVENT_LE_META:
        if ((p[2] == 0x01 || p[2] == 0x0A) && p[3] == 0) {             /* connection complete, success */
            u32 o = p[2] == 0x0A ? 26u : 14u;
            hx_conn = (u16)(p[4] | (p[5] << 8));
            hx_interval = (u16)(p[o] | (p[o + 1] << 8));
            hx_ccc = 0; hx_mtu = 23; hx_bonded = 0;
            hx_state = HX_ST_CONN;
            hx_report();
        } else if (p[2] == 0x03 && p[3] == 0) {                        /* connection update complete */
            hx_interval = (u16)(p[6] | (p[7] << 8));
            hx_report();
        }
        break;
    case HCI_EVENT_DISCONNECTION_COMPLETE:
        hx_reason = size >= 6 ? p[5] : 0;
        hx_conn = 0; hx_ccc = 0;
        ble_user_cmd_prepare(BLE_CMD_ATT_SEND_INIT, 4, 0, 0, 0, 0);
        if (hx_radio) hx_adv(1); else { hx_state = HX_ST_IDLE; hx_report(); }
        break;
    case HCI_EVENT_ENCRYPTION_CHANGE:
        if (size >= 6 && p[2] == 0 && p[5]) { hx_bonded = 1; hx_report(); }
        break;
    case ATT_EVENT_MTU_EXCHANGE_COMPLETE:
        if (size >= 6) {
            u16 m = (u16)(p[4] | (p[5] << 8));
            hx_mtu = m > HX_MTU_MAX ? HX_MTU_MAX : m;
            ble_user_cmd_prepare(BLE_CMD_ATT_MTU_SIZE, 1, hx_mtu - 3);
            hx_report();
        }
        break;
    default:
        break;
    }
}
static void hx_sm(u8 type, u16 ch, u8 *p, u16 size) { (void)type; (void)ch; (void)p; (void)size; }

/* from the app (h2c frames) */
void hx_from_app(u8 type, const u8 *p, u32 n)
{
    if (type == HX_MIDI) {
        if (hx_conn && hx_ccc && n && n <= (u32)(hx_mtu - 3))
            ble_user_cmd_prepare(BLE_CMD_ATT_SEND_DATA, 4, H_IO, p, n, ATT_OP_NOTIFY);
    } else if (type == HX_CMD && n >= 1) {
        hx_radio = p[0] & 1u;
        if (!hx_radio && hx_conn) ble_user_cmd_prepare(BLE_CMD_DISCONNECT, 1, hx_conn);
        if (hx_started && !hx_conn) hx_adv(hx_radio);
    } else if (type == HX_NAME && n && n < sizeof hx_name) {
        memcpy(hx_name, p, n); hx_name[n] = 0;
    }
}

/* the host's set-up, after btstack_init (called from ctl_init) */
void hx_init(void)
{
    le_device_db_init();
    sm_init();
    sm_set_io_capabilities(IO_NO_INPUT_NO_OUTPUT);                   /* Just Works */
    sm_set_authentication_requirements(SM_AUTHREQ_BONDING);
    sm_set_encryption_key_size_range(7, 16);
    sm_set_request_security(0);                                      /* the central decides (iOS pairs when it wants) */
    sm_event_callback_set(hx_sm);
    att_server_init(hx_db, hx_read, hx_write);
    att_server_register_packet_handler(hx_events);
    hci_event_callback_set(hx_events);
    le_l2cap_register_packet_handler(hx_events);
    ble_vendor_set_default_att_mtu(HX_MTU_MAX);
}
