/* SPDX-License-Identifier: MPL-2.0 */
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#define CTL_PORT_H
#define CTL_VENDOR_HOST_TEST 1
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32;
static int command_error, confirmations;
static int command_errors[129], command_calls[129];
static int disconnected_handle;
static u32 now = 1234;
static u16 confirmed_handle;
static int ready, boot_open, last_command, notify_count, link_count, blocked, stage;
static u8 notified[128], sent[128];
static u16 sent_len;
static u32 ctl_ms(void) { return now; }
int ctl_vendor_address(u8 a[6]) { memset(a, 0x51, 6); a[5] = 0xc1; return 0; }
int ctl_vendor_host_ready(void) { return ready; }
void ctl_vendor_events_reset(void) { ready = 0; }
int ctl_os_bootstrap_begin(u32 ms) { assert(ms == 5000); boot_open = 1; return 0; }
void ctl_os_bootstrap_end(void) { boot_open = 0; }
#include "../../ble/ctl/src/ctl_vendor_profile.c"
int le_controller_set_mac(void *p) { assert(((u8 *)p)[5] == 0xc1); return 0; }
int btstack_init(void) { assert(boot_open); ble_profile_init(); return 0; }
void os_time_dly(int n) { assert(n == 1 && boot_open); ready = 1; }
void le_device_db_init(void) { stage = 1; }
void ble_stack_gatt_role(u8 role) { assert(stage == 1 && role == 1); stage = 2; }
void sm_init(void) { assert(stage == 2); stage = 3; }
void sm_set_io_capabilities(int v) { assert(v == 3); }
void sm_set_authentication_requirements(u8 v) { assert(v == 0); }
void sm_set_encryption_key_size_range(u8 min, u8 max) { assert(min == 7 && max == 16); }
void sm_set_master_request_pair(int v) { assert(v == 0); }
void sm_event_callback_set(packet_cb cb) { assert(stage == 3 && cb == security_event); }
void sm_just_works_confirm(u16 h) { ++confirmations; confirmed_handle = h; }
void gatt_client_init(void) { assert(stage == 3); stage = 4; }
void gatt_client_register_packet_handler(packet_cb cb) { assert(stage == 4 && cb == packet_event); }
void hci_event_callback_set(packet_cb cb) { assert(cb == packet_event); }
void le_l2cap_register_packet_handler(packet_cb cb) { assert(cb == packet_event); }
u16 ble_vendor_set_default_att_mtu(u16 mtu) { assert(mtu == 128); return mtu; }
void user_client_init(u16 h, u8 *p, u16 len) { assert(h == 0 && p && len == 512); }
int ble_user_cmd_prepare(int cmd, int argc, ...)
{
    last_command = cmd;
    assert(cmd >= 0 && cmd < 129); ++command_calls[cmd];
    va_list ap; va_start(ap, argc);
    if (cmd == CMD_DISCONNECT) disconnected_handle = va_arg(ap, int);
    if (cmd == CMD_ATT_SEND) {
        (void)va_arg(ap, int); const u8 *p = va_arg(ap, const u8 *);
        int n = va_arg(ap, int); assert(n <= 128); memcpy(sent, p, n); sent_len = n;
    }
    va_end(ap); return command_error ? command_error : command_errors[cmd];
}
int ctl_vendor_link_copy(u8 up, u8 reason) { (void)reason; if (blocked) return -1; link_count += up ? 1 : 10; return 0; }
int ctl_vendor_notification_copy(const u8 *p, u32 n) { if (blocked) return -1; memcpy(notified,p,n); ++notify_count; return 0; }

/* Exercise cleanup from four real adapter paths, not an HCI-stack replay.
 * A transient disconnect refusal must not strand an owned retiring link. */
static void retirement_case(int failure, int busy, int off_then_on, u32 start)
{
    memset(command_errors, 0, sizeof command_errors);
    memset(command_calls, 0, sizeof command_calls);
    command_error = blocked = 0; ready = wanted = connecting = 1;
    connection = ABSENT; retiring = discovering = receiving = 0;
    down_pending = off_pending = scan_pending = 0; now = start;
    request.address_type = 1;
    for (unsigned i = 0; i < 6; ++i) request.address[i] = (u8)(i + 1);
    command_errors[CMD_DISCONNECT] = busy;
    if (failure == 0) command_errors[CMD_ATT_INIT] = -95;
    if (failure == 1) command_errors[CMD_SEARCH] = -95;
    u8 connected[21] = {0x3e,19,1,0,0,0,0,1,1,2,3,4,5,6};
    packet_event(4, 0, connected, sizeof connected);
    if (failure >= 2) {
        struct search_report found = {0};
        const u8 svc[16] = {0x03,0xb8,0x0e,0x5a,0xed,0xe8,0x4b,0x33,0xa7,0x51,0x6c,0xe3,0x4e,0xc4,0xc7,0x00};
        const u8 chr[16] = {0x77,0x72,0xe5,0xdb,0x38,0x68,0x41,0x12,0xa1,0xa9,0xf2,0x66,0x9d,0x10,0x6b,0xf3};
        memcpy(found.service.uuid128, svc, 16); memcpy(found.characteristic.uuid128, chr, 16);
        found.service.start = 4; found.service.end = 12;
        found.characteristic.start = 5; found.characteristic.value = 6;
        found.characteristic.end = 10; found.characteristic.properties = 0x14;
        user_client_report_search_result(&found);
        if (failure == 2) {
            struct descriptor_report descriptor = {9,0x2902,{0}};
            user_client_report_descriptor_result(&descriptor);
            command_errors[CMD_ATT_SEND] = -95;
        }
        user_client_report_search_result((void *)UINTPTR_MAX);
    }
    assert(connection == 0 && retiring && command_calls[CMD_DISCONNECT] == 1);
    int old_notifications = notify_count, old_links = link_count, old_confirms = confirmations;
    u8 midi[5] = {0x80,0x81,0xb0,1,2};
    struct data_report data = {0xa7,6,0,5,midi,0};
    u8 jw[16] = {0xd0,14,0,0,1,1,2,3,4,5,6};
    user_client_report_data_callback(&data); security_event(4,0,jw,16);
    assert(notify_count == old_notifications && link_count == old_links && confirmations == old_confirms);
    assert(ctl_vendor_write(midi, sizeof midi) == -1);
    if (off_then_on) {
        u8 off = 0, on = 1;
        assert(ctl_vendor_command(0x74, &off, 1) == 0);
        if (off_then_on == 1) assert(ctl_vendor_command(0x74, &on, 1) == 0);
    }
    /* No spin or early retry, including across the millisecond wrap. */
    for (unsigned i = 0; i < 20; ++i) { now = start + i; ctl_vendor_poll(); }
    assert(command_calls[CMD_DISCONNECT] == 1 && command_calls[CMD_SCAN] == 0);
    now = start + 20; ctl_vendor_poll();
    assert(command_calls[CMD_DISCONNECT] == 2 && connection == 0 && retiring);
    command_errors[CMD_DISCONNECT] = 0;
    for (unsigned i = 21; i < 40; ++i) { now = start + i; ctl_vendor_poll(); }
    assert(command_calls[CMD_DISCONNECT] == 2);
    now = start + 40; ctl_vendor_poll();
    assert(command_calls[CMD_DISCONNECT] == 3);
    for (unsigned i = 0; i < 10; ++i) ctl_vendor_poll();
    assert(command_calls[CMD_DISCONNECT] == 3 && connection == 0 && retiring);
    assert(command_calls[CMD_SCAN] == 0); /* acceptance is not a disconnect event */
    u8 off = 0, desired = wanted;
    for (unsigned i = 0; i < 3; ++i)
        assert(ctl_vendor_command(0x74,&off,1) == 0 && !off_pending);
    assert(command_calls[CMD_DISCONNECT] == 3);
    assert(ctl_vendor_command(0x74,&desired,1) == 0);
    u8 down[6] = {5,4,0,0,0,0x13};
    packet_event(4,0,down,sizeof down);
    assert(connection == ABSENT && !retiring && command_calls[CMD_SCAN] == (wanted ? 1 : 0));
    int scans = command_calls[CMD_SCAN];
    now += 100; ctl_vendor_poll(); packet_event(4,0,down,sizeof down);
    assert(command_calls[CMD_DISCONNECT] == 3 && command_calls[CMD_SCAN] == scans);
    /* A fresh requested connection does not inherit the old retry deadline. */
    memset(command_errors, 0, sizeof command_errors);
    wanted = connecting = 1;
    packet_event(4,0,connected,sizeof connected);
    assert(connection == 0 && !retiring && discovering);
    now += 100; ctl_vendor_poll();
    assert(command_calls[CMD_DISCONNECT] == 3);
    packet_event(4,0,down,sizeof down);
}

static void retirement_ownership(void)
{
    memset(command_errors, 0, sizeof command_errors);
    memset(command_calls, 0, sizeof command_calls);
    command_error = 0; connection = 7; retiring = connecting = 0;
    down_pending = off_pending = scan_pending = 0; wanted = 1; now = 100;
    command_errors[CMD_DISCONNECT] = -100;
    /* Rejecting an unrelated successful connection must not retire this peer. */
    u8 unrelated[21] = {0x3e,19,1,0,8,0,0,1,1,2,3,4,5,6};
    packet_event(4,0,unrelated,sizeof unrelated);
    assert(connection == 7 && !retiring && disconnected_handle == 8);
    now += 100; ctl_vendor_poll();
    assert(command_calls[CMD_DISCONNECT] == 1);
    assert(ctl_vendor_disconnect() == -100 && retiring);
    packet_event(4,0,unrelated,sizeof unrelated);
    assert(connection == 7 && retiring && disconnected_handle == 8);
    u8 wrong_down[6] = {5,4,0,8,0,0x13};
    packet_event(4,0,wrong_down,sizeof wrong_down);
    assert(connection == 7 && retiring);
    now += 20; ctl_vendor_poll();
    assert(command_calls[CMD_DISCONNECT] == 4 && disconnected_handle == 7);
    /* A terminal SDK refusal is not retried or reported as acceptance. */
    command_errors[CMD_DISCONNECT] = -95;
    now += 20; ctl_vendor_poll();
    assert(command_calls[CMD_DISCONNECT] == 5 && retiring);
    u8 off = 0; assert(ctl_vendor_command(0x74,&off,1) == 0 && off_pending);
    for (unsigned i = 0; i < 10; ++i) { now += 100; ctl_vendor_poll(); }
    assert(command_calls[CMD_DISCONNECT] == 5 && off_pending && connection == 7);
    command_errors[CMD_DISCONNECT] = 0;
    u8 down[6] = {5,4,0,7,0,0x13}; packet_event(4,0,down,sizeof down);
    assert(connection == ABSENT && !retiring && !off_pending);
}
int main(void)
{
    assert(ctl_vendor_startup() == 0 && ready && !boot_open);
    u8 on = 1; assert(ctl_vendor_command(0x74, &on, 1) == 0);
    /* Exact SMC name; handle zero remains valid. */
    u8 ad[23] = {0xe2,21,0,1,1,2,3,4,5,6,0,11,10,9,'S','M','C','-','M','i','x','e','r'};
    packet_event(4,0,ad,sizeof(ad)); assert(connecting && last_command == CMD_CONNECT);
    u8 connected[21] = {0x3e,19,1,0,0,0,0,1,1,2,3,4,5,6};
    connected[6]=1;packet_event(4,0,connected,sizeof connected);
    assert(connection==ABSENT && last_command==CMD_DISCONNECT);
    packet_event(4,0,ad,sizeof ad);connected[6]=0;connected[8]^=1;
    packet_event(4,0,connected,sizeof connected);assert(connection==ABSENT);
    packet_event(4,0,ad,sizeof ad);connected[8]^=1;connected[7]=0;
    packet_event(4,0,connected,sizeof connected);assert(connection==ABSENT);
    packet_event(4,0,ad,sizeof ad);connected[7]=1;
    packet_event(4,0,connected,sizeof(connected));
    assert(connection == 0 && discovering && last_command == CMD_SEARCH);
    u8 jw[16]={0xd0,14,0,0,1,1,2,3,4,5,6};
    security_event(4,0,jw,sizeof jw);assert(confirmations==1 && confirmed_handle==0);
    for(unsigned n=0;n<16;n++) security_event(4,0,jw,n);
    security_event(2,0,jw,16);security_event(4,0,0,16);
    jw[0]=0xd1;security_event(4,0,jw,16);jw[0]=0xd0;
    jw[1]=9;security_event(4,0,jw,16);jw[1]=14;
    jw[2]=1;security_event(4,0,jw,16);jw[2]=0;
    jw[4]=0;security_event(4,0,jw,16);jw[4]=1;
    jw[5]^=1;security_event(4,0,jw,16);jw[5]^=1;
    assert(confirmations==1);
    struct search_report found = {0};
    const u8 uuid[16] = {0x77,0x72,0xe5,0xdb,0x38,0x68,0x41,0x12,0xa1,0xa9,0xf2,0x66,0x9d,0x10,0x6b,0xf3};
    memcpy(found.characteristic.uuid128,uuid,16); found.characteristic.value=6;
    found.characteristic.properties=0x14;
    const u8 svc[16]={0x03,0xb8,0x0e,0x5a,0xed,0xe8,0x4b,0x33,0xa7,0x51,0x6c,0xe3,0x4e,0xc4,0xc7,0x00};
    memcpy(found.service.uuid128,svc,16); found.service.start=4; found.service.end=12;
    found.characteristic.start=5; found.characteristic.end=10;
    user_client_report_search_result(&found);
    struct descriptor_report descriptor={9,0x2902,{0}};
    user_client_report_descriptor_result(&descriptor);
    user_client_report_search_result((void *)UINTPTR_MAX);
    assert(!discovering && midi_handle == 6 && sent_len == 2 && sent[0] == 1);
    u8 midi[5] = {0x80,0x81,0xb0,1,2};
    struct data_report data = {0xa7,6,0,5,midi,0};
    blocked=1; user_client_report_data_callback(&data); assert(!receiving && !notify_count);
    blocked=0; user_client_report_data_callback(&data); assert(receiving && notify_count==1 && link_count==1);
    midi[4]=99; assert(notified[4]==2);
    u8 feedback[3]={0xb0,8,127}; assert(ctl_vendor_command(0x73,feedback,3)==0);
    assert(sent_len==5 && sent[2]==0xb0 && sent[4]==127);
    command_error = -100; assert(ctl_vendor_command(0x73,feedback,3)==1);
    command_error = -97; assert(ctl_vendor_command(0x73,feedback,3)==1);
    command_error = -95; assert(ctl_vendor_command(0x73,feedback,3)==-95);
    /* A disconnect request retires ownership even when the SDK reports busy;
     * security must not be confirmed while failure cleanup waits for HCI down. */
    command_error=-100;assert(ctl_vendor_disconnect()==-100 && retiring);
    { int notifications=notify_count, links=link_count;
      user_client_report_data_callback(&data);
      assert(notify_count==notifications && link_count==links);
      assert(ctl_vendor_write(feedback,3)==-1);
      assert(ctl_vendor_command(0x73,feedback,3)==-1); }
    security_event(4,0,jw,16);assert(confirmations==1);
    u8 off=0; command_error=-100;
    assert(ctl_vendor_command(0x74,&off,1)==0 && off_pending);
    security_event(4,0,jw,16);assert(confirmations==1);
    assert(ctl_vendor_command(0x73,feedback,3)==-1);
    command_error=0; now += 20; ctl_vendor_poll(); assert(!off_pending);
    assert(ctl_vendor_command(0x74,&on,1)==0);
    data.connection=1; user_client_report_data_callback(&data); assert(notify_count==1);
    u8 down[6]={5,4,0,0,0,0x13}; blocked=1;
    packet_event(4,0,down,sizeof(down)); assert(down_pending && connection==ABSENT);
    security_event(4,0,jw,16);assert(confirmations==1);
    blocked=0; ctl_vendor_poll(); assert(!down_pending && link_count==11);
    assert(ctl_vendor_command(0x73,feedback,3)==-1);
    for (int failure = 0; failure < 4; ++failure)
        for (int busy = 0; busy < 2; ++busy)
            retirement_case(failure, busy ? -97 : -100, failure == 2 ? 2 : failure & 1,
                            failure == 3 ? UINT32_MAX - 10u : 1234u);
    retirement_ownership();
    puts("vendor profile: startup hook, SMC peer ownership, JustWorks callback, discovery, copy/feedback, link reset PASS");
    puts("vendor retirement: ATT init/search/CCCD/missing descriptor, transient retry, pacing, OFF/ON, late events PASS");
}
