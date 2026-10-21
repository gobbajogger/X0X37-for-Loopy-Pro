// SPDX-License-Identifier: MPL-2.0
/* Copyright (C) 2026 M. Geisler (genkigenki/makkuro)
 * Application adapter for the exact AC79 host's public central APIs. Vendor
 * startup, HCI, L2CAP, SM, GATT discovery and ATT transport remain in btstack.a.
 * API ordering follows SDK le_net_central.c (Apache-2.0), not copied GPL glue.
 * The sole named SMC selection and radio request live here; BLE-MIDI decoding
 * and musical mapping remain in the app. */
#include "ctl_port.h"
#include "ctl_host_scope.h"
#include "ctl_highcpu.h"
#include "ctl_vendor_profile.h"
#if defined(CTL_VENDOR_HOST_TEST) && CTL_VENDOR_HOST_TEST

extern int memcmp(const void *, const void *, size_t);
typedef void (*packet_cb)(u8, u16, u8 *, u16);
extern int btstack_init(void);
extern int le_controller_set_mac(void *);
extern void le_device_db_init(void), sm_init(void), gatt_client_init(void);
extern void ble_stack_gatt_role(u8);
extern void sm_set_io_capabilities(int);
extern void sm_set_authentication_requirements(u8);
extern void sm_set_encryption_key_size_range(u8, u8);
extern void sm_set_master_request_pair(int);
extern void sm_event_callback_set(packet_cb);
extern void sm_just_works_confirm(u16);
extern void gatt_client_register_packet_handler(packet_cb);
extern void hci_event_callback_set(packet_cb);
extern void le_l2cap_register_packet_handler(packet_cb);
extern u16 ble_vendor_set_default_att_mtu(u16);
extern int ble_user_cmd_prepare(int, int, ...);
extern void user_client_init(u16, u8 *, u16);

/* Public SDK att.h report layouts; no private stack layout is reproduced. */
struct svc_report { u16 start, end, uuid16; u8 uuid128[16]; };
struct chr_report { u16 start, value, end, properties, uuid16; u8 uuid128[16]; };
struct search_report { struct svc_report service; struct chr_report characteristic; u16 service_index, characteristic_index; };
struct descriptor_report { u16 handle, uuid16; u8 uuid128[16]; };
struct data_report { u16 kind, handle, offset, length; u8 *data; u16 connection; };
struct connection_parameters { u16 interval, latency, timeout; u8 address_type, address[6]; } __attribute__((packed));
_Static_assert(sizeof(struct search_report) == 52, "SDK search report");
_Static_assert(offsetof(struct search_report, characteristic) == 22, "SDK characteristic offset");
_Static_assert(sizeof(struct connection_parameters) == 13, "SDK connection parameters");
#if UINTPTR_MAX == 0xffffffffu
_Static_assert(sizeof(struct data_report) == 16, "SDK data report");
_Static_assert(offsetof(struct data_report, data) == 8, "SDK data pointer");
#endif

enum { ABSENT = 0xffff, MTU = 128, CMD_DISCONNECT = 5, CMD_ATT_INIT = 7,
       CMD_ATT_SEND = 10, CMD_SCAN_PARAM = 13, CMD_CONNECT = 15,
       CMD_CANCEL = 16, CMD_SCAN = 22, CMD_SEARCH = 128,
       RETIRING_WAIT = 1, RETIRING_RETRY = 2, RETIRING_FAILED = 3,
       DISCONNECT_RETRY_MS = 20 };
/* Exact reference application allocation sizes; no speculative shrink. */
static u8 att_storage[188 + MTU + 512] __attribute__((aligned(4))) CTL_HIGHCPU_BSS;
static u8 search_storage[512] __attribute__((aligned(4))) CTL_HIGHCPU_BSS;
static struct connection_parameters request CTL_HIGHCPU_BSS;
static u16 connection = ABSENT; /* initialized data deliberately stays low */
enum { SUB_NONE, SUB_PENDING, SUB_ENABLED, SUB_SECURITY_PENDING, SUB_RETRY };
static u16 midi_handle CTL_HIGHCPU_BSS, midi_end CTL_HIGHCPU_BSS, cccd CTL_HIGHCPU_BSS;
static u32 subscription_deadline CTL_HIGHCPU_BSS, disconnect_retry_at CTL_HIGHCPU_BSS;
static u8 started CTL_HIGHCPU_BSS, down_reason CTL_HIGHCPU_BSS, retiring CTL_HIGHCPU_BSS, subscription CTL_HIGHCPU_BSS;
/* Private core-1 state: callbacks run synchronously in the cooperative host,
 * never from an ISR or the application core. Only genuine 0/1 flags are packed;
 * subscription, retirement and reason codes remain full bytes. No layout escapes
 * this file. Local aliases keep the state-machine expressions readable. */
static struct {
    unsigned wanted:1, off_pending:1, connecting:1, security_retry_used:1;
    unsigned down_pending:1, discovering:1, receiving:1, scan_pending:1;
} profile_state CTL_HIGHCPU_BSS;
#define security_retry_used profile_state.security_retry_used
#define connecting profile_state.connecting
#define discovering profile_state.discovering
#define wanted profile_state.wanted
#define receiving profile_state.receiving
#define down_pending profile_state.down_pending
#define off_pending profile_state.off_pending
#define scan_pending profile_state.scan_pending
extern int ctl_vendor_address(u8 address[6]);
extern void os_time_dly(int);

static int retire_connection(u16 handle)
{
    if (handle == connection) {
        if (retiring == RETIRING_WAIT) return 0; /* await actual down event */
        if (retiring == RETIRING_FAILED) return -1; /* terminal refusal, not success */
        if (retiring == RETIRING_RETRY &&
            (int32_t)(ctl_ms() - disconnect_retry_at) < 0) return -100;
        retiring = RETIRING_FAILED; /* stop input/security before the command */
    }
    int result = ble_user_cmd_prepare(CMD_DISCONNECT, 1, (int)handle);
    /* Only this owned connection may be retried. A rejected unrelated peer
     * must never transfer its handle or retirement state onto the active link.
     * Preserve the exact SDK's transient statuses; do not spin on other errors. */
    if (handle == connection) {
        if (!result) retiring = RETIRING_WAIT;
        else if (result == -100 || result == -97) {
            retiring = RETIRING_RETRY;
            disconnect_retry_at = ctl_ms() + DISCONNECT_RETRY_MS;
        }
    }
    return result;
}

/* Share cold actions and ownership predicates without weakening any caller's
 * guards. Inlining choices are measured with the pinned target toolchain. */
static __attribute__((noinline)) void retire_subscription(void)
{
    subscription = SUB_NONE;
    retire_connection(connection);
}
static __attribute__((noinline)) int send_subscription(void)
{
    static const u8 enable_notify[2] = {1, 0};
    subscription = SUB_PENDING;
    return ble_user_cmd_prepare(CMD_ATT_SEND, 4, (int)cccd, enable_notify, 2, 5);
}
static __attribute__((always_inline)) inline u16 read16(const u8 *p) { return (u16)p[0] | (u16)p[1] << 8; }
static __attribute__((always_inline)) inline int profile_enabled(void)
{
    return wanted && !off_pending && !retiring;
}
static __attribute__((noinline)) int profile_owned(void)
{
    return profile_enabled() && connection != ABSENT;
}
static __attribute__((always_inline)) inline int security_recovery_pending(void)
{
    return subscription == SUB_SECURITY_PENDING || subscription == SUB_RETRY ||
           (subscription == SUB_PENDING && security_retry_used);
}
/* Exact64afb SDK emits D0 as16 bytes (length14), not the generic11-byte
 * upstream shape. It dispatches synchronously from its SM engine. The callback
 * only confirms the currently owned requested peer; vendor SM retains all key
 * exchange/encryption state and resumes itself through its public confirm API. */
static void security_event(u8 type, u16 channel, u8 *p, u16 n)
{
    (void)channel;
    if (type != 4 || !p || n != 16 || (p[0] != 0xd0 && p[0] != 0xdf) || p[1] != 14 ||
        !profile_owned() || connecting ||
        read16(p + 2) != connection || p[4] != request.address_type ||
        memcmp(p + 5, request.address, 6)) return;
    if (p[0] == 0xd0) sm_just_works_confirm(connection);
    else if (security_recovery_pending() &&
             ((p[11] == 3 && p[13] <= 1 && !p[14]) ||
              (p[11] == 4 && !p[12] && !p[13] && !p[14]))) {
        /* Exact SDK PAIR_PROCESS tag: reason<<8 | origin<<16 | 3 for
         * local/received Pairing Failed, or exactly 4 for SM timeout. */
        retire_subscription();
    }
}

static void packet_event(u8 type, u16 channel, u8 *p, u16 n)
{
    if (type != 4 || !p || n < 2 || (u16)p[1] + 2u > n) return;
    if (p[0] == 8 && n != 6) return;
    n = (u16)p[1] + 2u;
    (void)channel;
    /* SDK GAP_EVENT_ADVERTISING_REPORT contains one bounded AD payload. Never
     * connect to a different MIDI controller simply because its service matches. */
    if (p[0] == 0xe2 && n >= 12 && p[11] <= n - 12 && wanted &&
        !connecting && connection == ABSENT) {
        const u8 *ad = p + 12;
        unsigned left = p[11];
        while (left && ad[0]) {
            unsigned len = ad[0];
            if (len + 1 > left) break;
            if ((ad[1] == 8 || ad[1] == 9) && len == 10 &&
                !memcmp(ad + 2, "SMC-Mixer", 9)) {
                ctl_vendor_connect(p[3], p + 4);
                break;
            }
            left -= len + 1; ad += len + 1;
        }
    } else if (p[0] == 0x3e && n >= 3 &&
               ((p[2] == 1 && n >= 21) || (p[2] == 10 && n >= 33))) {
        if (p[3]) { connecting = 0; if (wanted) { scan_pending = 1; ctl_vendor_poll(); } }
        else {
            u16 handle = read16(p + 4);
            if (!wanted || !connecting || connection != ABSENT || p[6] != 0 ||
                p[7] != request.address_type || memcmp(p + 8, request.address, 6)) {
                retire_connection(handle);
                if (connection == ABSENT) { connecting = 0; if (wanted) scan_pending = 1; }
            } else {
                connection = handle; retiring = 0; connecting = 0; midi_handle = midi_end = cccd = 0; receiving = 0;
                subscription = SUB_NONE;
                security_retry_used = 0;
                if (ble_user_cmd_prepare(CMD_ATT_INIT, 4, (int)handle, att_storage,
                                         (int)sizeof(att_storage), MTU)) {
                    retire_connection(handle);
                } else {
                    discovering = 1;
                    user_client_init(handle, search_storage, sizeof(search_storage));
                    if (ble_user_cmd_prepare(CMD_SEARCH, 2, 3, 0)) {
                        discovering = 0;
                        retire_connection(handle);
                    }
                }
            }
        }
    } else if (p[0] == 5 && n >= 6 && read16(p + 3) == connection) {
        connection = ABSENT; midi_handle = 0; discovering = receiving = retiring = 0;
        subscription = SUB_NONE;
        security_retry_used = 0;
        ble_user_cmd_prepare(CMD_ATT_INIT, 4, 0, (void *)0, 0, 0);
        down_reason = p[5]; down_pending = 1;
        ctl_vendor_poll();
        if (wanted) { scan_pending = 1; ctl_vendor_poll(); }
    } else if (p[0] == 8 && n == 6 && profile_owned() && !connecting && read16(p + 3) == connection &&
               security_recovery_pending()) {
        /* HCI Encryption Change: act only after this peer's CCCD security
         * error. Do not start pairing. Wait until all SDK HCI handlers return
         * before the next poll submits the one allowed retry. */
        if (p[2] || !p[5]) {
            retire_subscription();
        } else if (p[5] == 1 && subscription == SUB_SECURITY_PENDING) {
            subscription = SUB_RETRY;
            subscription_deadline = ctl_ms() + 5000u;
        }
    }
}

void ble_profile_init(void)
{
    ctl_scope_end(1);
    ctl_scope_begin(2);
    le_device_db_init();
    ctl_scope_end(2);
    ble_stack_gatt_role(1);
    ctl_scope_begin(3);
    sm_init();
    ctl_scope_end(3);
    sm_set_io_capabilities(3);
    sm_set_authentication_requirements(0);
    sm_set_encryption_key_size_range(7, 16);
    sm_set_master_request_pair(0);
    sm_event_callback_set(security_event);
    gatt_client_init();
    gatt_client_register_packet_handler(packet_event);
    hci_event_callback_set(packet_event);
    le_l2cap_register_packet_handler(packet_event);
    ble_vendor_set_default_att_mtu(MTU);
}

/* SDK UUID arrays use canonical byte order; accept reverse representation as
 * well for compatibility with its discovery report variants. These public UUIDs
 * need equality, not a constant-time secret comparison. */
static int uuid_equal(const u8 *uuid, const u8 expected[16])
{
    if (!memcmp(uuid, expected, 16)) return 1;
    for (int i = 0; i < 16; ++i)
        if (uuid[i] != expected[15-i]) return 0;
    return 1;
}
void user_client_report_search_result(struct search_report *r)
{
    if (!profile_owned() || down_pending || !discovering) return;
    if ((uintptr_t)r == UINTPTR_MAX) {
        discovering = 0;
        if (midi_handle && cccd) {
            /* Use the descriptor discovered by the vendor, never value+1.
             * Command acceptance is NOT an ATT acknowledgment / control PASS. */
            if (send_subscription()) {
                retire_subscription();
            }
        } else retire_connection(connection);
    } else if (r && !midi_handle && !r->characteristic.uuid16 && !r->service.uuid16 &&
               (r->characteristic.properties & 0x14) == 0x14) {
        static const u8 service_uuid[16] = {0x03,0xb8,0x0e,0x5a,0xed,0xe8,0x4b,0x33,0xa7,0x51,0x6c,0xe3,0x4e,0xc4,0xc7,0x00};
        static const u8 characteristic_uuid[16] = {0x77,0x72,0xe5,0xdb,0x38,0x68,0x41,0x12,0xa1,0xa9,0xf2,0x66,0x9d,0x10,0x6b,0xf3};
        if (uuid_equal(r->service.uuid128, service_uuid) &&
            uuid_equal(r->characteristic.uuid128, characteristic_uuid) &&
            r->characteristic.start >= r->service.start &&
            r->characteristic.value > r->characteristic.start &&
            r->characteristic.value < r->characteristic.end &&
            r->characteristic.end <= r->service.end) {
            midi_handle = r->characteristic.value;
            midi_end = r->characteristic.end;
        }
    }
}
u8 user_client_search_descriptor_is_enable(void) { return 1; }
void user_client_report_descriptor_result(struct descriptor_report *d)
{
    if (profile_owned() && !down_pending && discovering && d && midi_handle && d->uuid16 == 0x2902 &&
        d->handle > midi_handle && d->handle <= midi_end) cccd = d->handle;
}

void user_client_report_data_callback(struct data_report *r)
{
    /* The exact SDK reports ATT query completion as kind A0, a one-byte
     * status, and the connection/attribute handles. Queue acceptance is not
     * a successful CCCD write. Only this pending write owns its completion;
     * unrelated reads, stale completions and other links cannot change it. */
    if (profile_owned() && !down_pending && r && r->connection == connection &&
        subscription == SUB_PENDING && r->kind == 0xa0 &&
        cccd && r->handle == cccd && !r->offset && r->length == 1 && r->data) {
        u8 status = r->data[0];
        if (status == 0x05 || status == 0x08 || status == 0x0c || status == 0x0f) {
            if (security_retry_used) {
                retire_subscription();
            } else {
                /* SDK may pair for 05, or the peer may already be pairing.
                 * Its SM timer is 30s; allow 5s scheduling margin, then stop
                 * rather than retain an unusable link indefinitely. */
                subscription = SUB_SECURITY_PENDING;
                subscription_deadline = ctl_ms() + 35000u;
            }
        } else {
            subscription = status ? SUB_NONE : SUB_ENABLED;
            if (status) retire_connection(connection);
        }
        return;
    }
    if (profile_enabled() && r && r->kind == 0xa7 && r->connection == connection && subscription != SUB_NONE &&
        midi_handle && r->handle == midi_handle && !r->offset &&
        r->data && r->length >= 3 && r->length <= MTU - 3) {
        ctl_vendor_poll();
        if (down_pending || retiring) return;
        if (!receiving) {
            if (ctl_vendor_link_copy(1, 0)) return;
        }
        if (!ctl_vendor_notification_copy(r->data, r->length)) receiving = 1;
    }
}

int ctl_vendor_startup(void)
{
    if (started || ctl_vendor_host_ready()) return -1;
    u8 address[6];
    if (ctl_vendor_address(address) || (address[5] & 0xc0) != 0xc0) return -1;
    ctl_vendor_events_reset();
    connection = ABSENT; midi_handle = 0;
    subscription = SUB_NONE;
    security_retry_used = 0;
    connecting = discovering = wanted = receiving = down_pending = 0;
    if (le_controller_set_mac(address)) return -1;
    if (ctl_os_bootstrap_begin(5000)) return -1;
    started = 1;
    int status = btstack_init();
    while (!status && !ctl_vendor_host_ready()) os_time_dly(1);
    ctl_os_bootstrap_end();
    return status;
}
int ctl_vendor_scan(int enable)
{
    /* Finish publishing the old link-down before starting a new peer. This
     * also keeps its one-shot discovery/security callbacks out of old backlog. */
    if (!ctl_vendor_host_ready() || connection != ABSENT || connecting ||
        (enable && down_pending)) return -1;
    if (enable && ble_user_cmd_prepare(CMD_SCAN_PARAM, 3, 1, 80, 48)) return -1;
    return ble_user_cmd_prepare(CMD_SCAN, 2, !!enable, 1);
}
int ctl_vendor_connect(u8 address_type, const u8 address[6])
{
    if (!address || address_type > 1 || !ctl_vendor_host_ready() ||
        connection != ABSENT || connecting || down_pending) return -1;
    if (ble_user_cmd_prepare(CMD_SCAN, 2, 0, 1)) return -1;
    request.interval = 12; request.latency = 0; request.timeout = 400;
    request.address_type = address_type; memcpy(request.address, address, 6);
    connecting = 1;
    int status = ble_user_cmd_prepare(CMD_CONNECT, 1, &request);
    if (status) { connecting = 0; if (wanted) scan_pending = 1; }
    return status;
}
int ctl_vendor_disconnect(void)
{
    if (connecting) {
        int result = ble_user_cmd_prepare(CMD_CANCEL, 0);
        if (!result) connecting = 0;
        return result;
    }
    if (connection == ABSENT) return ble_user_cmd_prepare(CMD_SCAN, 2, 0, 1);
    return retire_connection(connection);
}
int ctl_vendor_write(const u8 *payload, u16 length)
{
    if (!payload || !length || length > MTU-3 || connection == ABSENT || !midi_handle || discovering || retiring) return -1;
    return ble_user_cmd_prepare(CMD_ATT_SEND, 4, (int)midi_handle, payload, (int)length, 6);
}
void ctl_vendor_poll(void)
{
    if (down_pending && !ctl_vendor_link_copy(0, down_reason)) down_pending = 0;
    if (retiring == RETIRING_RETRY && connection != ABSENT &&
        (int32_t)(ctl_ms() - disconnect_retry_at) >= 0)
        (void)retire_connection(connection);
    if (off_pending && !ctl_vendor_disconnect()) off_pending = 0;
    if (scan_pending && wanted && !connecting && connection == ABSENT && !ctl_vendor_scan(1)) scan_pending = 0;
    if (profile_owned() && !connecting && (subscription == SUB_SECURITY_PENDING || subscription == SUB_RETRY)) {
        if ((int32_t)(ctl_ms() - subscription_deadline) >= 0) {
            retire_subscription();
        } else if (subscription == SUB_RETRY && !down_pending) {
            /* Reserve the one accepted retry before sending: a completion
             * callback can run synchronously. BUSY/FULL never enqueue it. */
            security_retry_used = 1;
            int status = send_subscription();
            if (status == -100 || status == -97) {
                if (!retiring && subscription == SUB_PENDING) {
                    subscription = SUB_RETRY;
                    security_retry_used = 0;
                }
            } else if (status) {
                retire_subscription();
            }
        }
    }
}
static int command_result(int status)
{
    /* Exact public SDK ble_api.h: BUSY=-100, BUFFER_FULL=-97. */
    return status == -100 || status == -97 ? 1 : status;
}
int ctl_vendor_command(u8 type, const u8 *payload, u32 length)
{
    /* Apply desired radio state before polling: OFF cancels a scheduled CCCD
     * retry rather than allowing that retry immediately before retirement. */
    if (type != 0x74) ctl_vendor_poll();
    if (!payload) return -1;
    if (!ctl_vendor_host_ready()) return 1; /* keep queued during startup */
    if (type == 0x74) {
        if (length != 1 || payload[0] > 1) return -1;
        wanted = payload[0];
        if (!wanted) {
            scan_pending = 0; off_pending = 1; ctl_vendor_poll();
            return 0; /* accepted desired state; stop retries independently of MIDI */
        }
        off_pending = 0;
        if (connection == ABSENT && !connecting) scan_pending = 1;
        ctl_vendor_poll();
        return 0;
    }
    if (type == 0x73) {
        if (!length || length > 3) return -1;
        if (!wanted || !receiving) return -1; /* never retain stale feedback across a link */
        u8 packet[5];
        u32 timestamp = ctl_ms() & 8191u;
        packet[0] = (u8)(0x80u | (timestamp >> 7));
        packet[1] = (u8)(0x80u | (timestamp & 127));
        memcpy(packet + 2, payload, length);
        return command_result(ctl_vendor_write(packet, (u16)length + 2));
    }
    return -1;
}

u16 ctl_vendor_connection(void) { return connection; }
u16 ctl_vendor_midi_handle(void) { return midi_handle; }
#endif
