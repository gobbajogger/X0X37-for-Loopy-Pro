/* SPDX-License-Identifier: MPL-2.0 */
/* Native adapter boundary only: no proprietary HCI stack or device traffic. */
#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "x0x_ble_abi.h"
#include "x0x_smc_host_messages.h"
#define CTL_PORT_H
#define CTL_VENDOR_HOST_TEST 1
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32;
static xb_mbox_t box;
static xsm_boot_t boot;
static const xb_boot_t *ctl_bp = &boot.base;
static xb_mbox_t *ctl_mb = &box;
static unsigned feedback_count;
static int feedback_error;
static u16 feedback_handle;
static u8 feedback_length;
static u8 feedback_bytes[5];
static u32 ctl_ms(void) { return 1000; }
static u32 ctl_port_irq_save(void) { return 0; }
static void ctl_port_irq_restore(u32 f) { (void)f; }
int ctl_vendor_host_ready(void) { return 1; }
void ctl_vendor_events_reset(void) {}
int ctl_os_bootstrap_begin(u32 ms) { (void)ms; return 0; }
void ctl_os_bootstrap_end(void) {}
#include "../../ble/ctl/src/ctl_vendor_profile.c"
#include "../../ble/ctl/src/ctl_vendor_bridge.c"
int le_controller_set_mac(void *p) { (void)p; return 0; }
int btstack_init(void) { return 0; }
void os_time_dly(int n) { (void)n; }
void le_device_db_init(void) {}
void ble_stack_gatt_role(u8 r) { (void)r; }
void sm_init(void) {}
void sm_set_io_capabilities(int v) { (void)v; }
void sm_set_authentication_requirements(u8 v) { (void)v; }
void sm_set_encryption_key_size_range(u8 lo,u8 hi) { (void)lo; (void)hi; }
void sm_set_master_request_pair(int v) { (void)v; }
void sm_event_callback_set(packet_cb cb) { (void)cb; }
void sm_just_works_confirm(u16 h) { (void)h; }
void gatt_client_init(void) {}
void gatt_client_register_packet_handler(packet_cb cb) { (void)cb; }
void hci_event_callback_set(packet_cb cb) { (void)cb; }
void le_l2cap_register_packet_handler(packet_cb cb) { (void)cb; }
u16 ble_vendor_set_default_att_mtu(u16 mtu) { return mtu; }
void user_client_init(u16 h,u8 *p,u16 n) { (void)h; assert(p && n == 512); }
int ble_user_cmd_prepare(int cmd,int argc,...)
{
    va_list ap; va_start(ap,argc);
    int result = 0;
    if (cmd == CMD_ATT_SEND) {
        int h = va_arg(ap,int); const u8 *p = va_arg(ap,const u8 *);
        int n = va_arg(ap,int), kind = va_arg(ap,int);
        if (kind == 6) {
            result = feedback_error;
            if (!result) {
                assert(n >= 3 && n <= 5); feedback_length = (u8)n; ++feedback_count;
                feedback_handle = (u16)h; memcpy(feedback_bytes,p,(unsigned)n);
            }
        }
    }
    va_end(ap); return result;
}
static void connect_peer(u8 address_seed,u16 value)
{
    u8 address[6]; for (unsigned i=0;i<6;++i) address[i]=(u8)(address_seed+i);
    assert(ctl_vendor_connect(1,address) == 0);
    u8 up[21] = {0x3e,19,1,0,0,0,0,1}; memcpy(up+8,address,6);
    packet_event(4,0,up,sizeof up);
    assert(connection == 0 && discovering && !retiring);
    struct search_report found = {0};
    const u8 svc[16] = {0x03,0xb8,0x0e,0x5a,0xed,0xe8,0x4b,0x33,0xa7,0x51,0x6c,0xe3,0x4e,0xc4,0xc7,0x00};
    const u8 chr[16] = {0x77,0x72,0xe5,0xdb,0x38,0x68,0x41,0x12,0xa1,0xa9,0xf2,0x66,0x9d,0x10,0x6b,0xf3};
    memcpy(found.service.uuid128,svc,16); memcpy(found.characteristic.uuid128,chr,16);
    found.service.start = 1; found.service.end = 20;
    found.characteristic.start = value-1; found.characteristic.value = value;
    found.characteristic.end = value+2; found.characteristic.properties = 0x14;
    user_client_report_search_result(&found);
    struct descriptor_report desc = {value+1,0x2902,{0}};
    user_client_report_descriptor_result(&desc);
    user_client_report_search_result((void *)UINTPTR_MAX);
    u8 payload[5] = {0x80,0x81,0xb0,16,1};
    struct data_report data = {0xa7,value,0,sizeof payload,payload,0};
    user_client_report_data_callback(&data);
    assert(receiving && midi_handle == value);
}
static void down(void)
{
    u8 event[6] = {5,4,0,0,0,0x13}; packet_event(4,0,event,sizeof event);
    assert(connection == ABSENT && !receiving && !connection_epoch);
}
static void reset_controller(void)
{
    /* Model reset of controller-owned data/BSS only; application-owned rings
     * and boot extension deliberately survive. This is not SDK/HCI replay. */
    connection = ABSENT; connection_epoch = down_epoch = 0;
    midi_handle = midi_end = cccd = 0;
    retiring = connecting = discovering = wanted = receiving = 0;
    down_pending = off_pending = scan_pending = started = 0;
    ctl_vendor_mailbox_reset(); ctl_vendor_mailbox_poll();
}
static void drain_output(void)
{
    for (unsigned i = 0; i < 200u; ++i) ctl_vendor_mailbox_poll();
    assert(!have_pending && !xb_ring_used(&box.h2c));
}
int main(int argc, char **argv)
{
    const char *mode = argc == 2 ? argv[1] : "queued";
    boot.base.abi = XSM_ABI; boot.ext_magic = XSM_BOOT_EXT_MAGIC;
    boot.ext_size = sizeof boot; boot.radio_desired = 1;
    ctl_vendor_mailbox_reset(); ctl_vendor_mailbox_poll();
    connect_peer(1,6);
    if (!strcmp(mode,"up-pressure")) {
        u8 filler[128] = {0}, payload[5] = {0x80,0x81,0xb0,16,1};
        struct data_report data = {0xa7,6,0,sizeof payload,payload,0};
        xb_ring_init(&box.c2h); receiving = 0;
        for (unsigned i=0; i<7; ++i) assert(!xb_frame_put(&box.c2h,XSM_EVENT,filler,128));
        assert(!xb_frame_put(&box.c2h,XSM_EVENT,filler,94));
        assert(xb_ring_free(&box.c2h) == 9);
        user_client_report_data_callback(&data);
        assert(!receiving && connection_epoch == 1 && boot.feedback_epoch == 1);
        u8 type; unsigned ups=0;
        while (xb_frame_get(&box.c2h,&type,filler,sizeof filler) > 0) ups += type == XSM_LINK_SESSION;
        assert(ups == 1);
        user_client_report_data_callback(&data);
        assert(receiving && connection_epoch == 1 && boot.feedback_epoch == 1);
        assert(xb_frame_get(&box.c2h,&type,filler,sizeof filler) == 6);
        assert(type == XSM_LINK_SESSION && xsm_epoch_read(filler) == 1);
    }
    u32 old_epoch = connection_epoch;
    assert(old_epoch == 1 && boot.feedback_epoch == old_epoch);
    const u8 old_feedback[3] = {0x90,44,127};
    assert(xsm_feedback_put(&box.h2c,old_epoch,old_feedback,3) == 0);
    if (!strcmp(mode,"backlog"))
        for (unsigned i=0; i<90; ++i)
            assert(xsm_feedback_put(&box.h2c,old_epoch,old_feedback,3) == 0);
    if (!strcmp(mode,"pending")) {
        feedback_error = -100; ctl_vendor_mailbox_poll();
        assert(have_pending && !feedback_count); feedback_error = 0;
    }
    if (!strcmp(mode,"duplicate")) {
        u8 event[21] = {0x3e,19,1,0,0,0,0,1,1,2,3,4,5,6};
        packet_event(4,0,event,sizeof event);
        assert(connection_epoch == old_epoch && boot.feedback_epoch == old_epoch && !retiring);
    }
    if (!strcmp(mode,"pressure")) {
        u8 filler[128] = {0};
        while (!xb_frame_put(&box.c2h,XSM_NOTIFY,filler,sizeof filler)) {}
        /* Leave less than a six-byte link frame plus framing. */
        u32 space = xb_ring_free(&box.c2h);
        if (space >= 3) assert(!xb_frame_put(&box.c2h,XSM_EVENT,filler,space-3));
    }
    if (!strcmp(mode,"controller-reset")) reset_controller();
    else down();
    if (!strcmp(mode,"drained") || !strcmp(mode,"backlog")) {
        ctl_vendor_mailbox_poll(); assert(!have_pending && !feedback_count);
    }
    if (!strcmp(mode,"pressure")) {
        assert(down_pending && down_epoch == old_epoch);
        /* App has not received DOWN. Its late producer still holds epoch 1. */
        assert(xsm_feedback_put(&box.h2c,old_epoch,old_feedback,3) == 0);
        xb_ring_init(&box.c2h); /* emulate consumer drain, not production flush */
    }
    connect_peer(21,9);
    assert(connection_epoch == old_epoch + 1 && boot.feedback_epoch == connection_epoch);
    if (!strcmp(mode,"late")) {
        /* Producer took its snapshot before reconnect, then published after. */
        assert(xsm_feedback_put(&box.h2c,old_epoch,old_feedback,3) == 0);
    }
    drain_output();
    assert(feedback_count == 0 && "old-session feedback reached the new peer");
    /* Both the ring and direct profile boundary must reject legacy messages. */
    assert(xb_frame_put(&box.h2c,XSM_MIDI_OUT,old_feedback,3) == 0);
    assert(ctl_vendor_command(XSM_MIDI_OUT,old_feedback,3) < 0);
    drain_output(); assert(!feedback_count);
    const u8 fresh[3] = {0x90,45,0};
    assert(xsm_feedback_put(&box.h2c,connection_epoch,fresh,3) == 0);
    ctl_vendor_mailbox_poll(); assert(feedback_count == 1 && feedback_handle == 9);
    assert(!memcmp(feedback_bytes+2,fresh,3));
    /* Wrong connection-handle events must not advance identity. */
    u8 wrong_down[6] = {5,4,0,7,0,0x13};
    packet_event(4,0,wrong_down,sizeof wrong_down);
    assert(connection == 0 && connection_epoch == old_epoch + 1);
    /* A pending retry never lets a stale queued ON override desired OFF. */
    assert(xsm_feedback_put(&box.h2c,connection_epoch,fresh,3) == 0);
    feedback_error = -97; ctl_vendor_mailbox_poll(); assert(have_pending);
    u8 on = 1; assert(!xb_frame_put(&box.h2c,XSM_RADIO,&on,1));
    boot.radio_desired = 0; ctl_vendor_mailbox_poll();
    feedback_error = 0; drain_output(); assert(!wanted && feedback_count == 1);
    down(); boot.radio_desired = 1; ctl_vendor_mailbox_poll();
    boot.feedback_epoch = UINT32_MAX - 1u;
    connect_peer(41,11); assert(connection_epoch == UINT32_MAX);
    assert(xsm_feedback_put(&box.h2c,connection_epoch,fresh,3) == 0);
    ctl_vendor_mailbox_poll(); assert(feedback_count == 2);
    if (!strcmp(mode,"exhaustion-pressure")) {
        u8 filler[128] = {0};
        while (!xb_frame_put(&box.c2h,XSM_EVENT,filler,sizeof filler)) {}
        u32 space = xb_ring_free(&box.c2h);
        if (space >= 3) assert(!xb_frame_put(&box.c2h,XSM_EVENT,filler,space-3));
    }
    down();
    if (!strcmp(mode,"exhaustion-pressure")) assert(down_pending && down_epoch == UINT32_MAX);
    u8 address[6] = {61,62,63,64,65,66}; assert(!ctl_vendor_connect(1,address));
    u8 exhausted[21] = {0x3e,19,1,0,0,0,0,1,61,62,63,64,65,66};
    packet_event(4,0,exhausted,sizeof exhausted);
    assert(!connection_epoch && retiring && boot.feedback_epoch == UINT32_MAX);
    if (!strcmp(mode,"exhaustion-pressure")) {
        down(); assert(down_pending && down_epoch == UINT32_MAX);
        u8 frame[128], type;
        while (xb_frame_get(&box.c2h,&type,frame,sizeof frame) > 0) {}
        ctl_vendor_mailbox_poll(); assert(!down_pending);
        assert(xb_frame_get(&box.c2h,&type,frame,sizeof frame) == 6);
        assert(type == XSM_LINK_SESSION && xsm_epoch_read(frame) == UINT32_MAX && !frame[4]);
    }
    reset_controller();
    assert(ctl_vendor_session_next() == 0 && boot.feedback_epoch == UINT32_MAX);
    /* Coordinated full app restart clears both rings before initializing epoch. */
    xb_ring_init(&box.h2c); xb_ring_init(&box.c2h); boot.feedback_epoch = 0;
    reset_controller(); connect_peer(81,6); assert(connection_epoch == 1);
    assert(xsm_feedback_put(&box.h2c,connection_epoch,fresh,3) == 0);
    ctl_vendor_mailbox_poll(); assert(feedback_count == 3);
    for (u8 n=1; n<=2; ++n) {
        assert(!xsm_feedback_put(&box.h2c,connection_epoch,fresh,n));
        ctl_vendor_mailbox_poll(); assert(feedback_length == n+2);
    }
    assert(feedback_count == 5);
    u8 bad[7]={0}; assert(ctl_vendor_command(XSM_MIDI_SESSION,bad,7)<0);
    xsm_epoch_write(bad,connection_epoch+1);
    assert(ctl_vendor_command(XSM_MIDI_SESSION,bad,7)<0);
    assert(ctl_vendor_command(XSM_MIDI_SESSION,bad,4)<0);
    printf("feedback session %s: stale/legacy rejected, fresh/handles/OFF/resets/exhaustion PASS\n",mode);
    return 0;
}
