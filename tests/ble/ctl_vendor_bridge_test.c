/* SPDX-License-Identifier: MPL-2.0 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "x0x_ble_abi.h"
#include "x0x_smc_host_messages.h"
#define CTL_PORT_H
#define CTL_VENDOR_HOST_TEST 1
typedef uint8_t u8; typedef uint32_t u32;
static xb_mbox_t box;
static const xb_boot_t *ctl_bp;
static xb_mbox_t *ctl_mb = &box;
static u32 ctl_port_irq_save(void) { return 0; }
static void ctl_port_irq_restore(u32 f) { (void)f; }
static u8 command[3], command_type;
static u32 command_len, calls;
static int busy;
void ctl_vendor_poll(void) {}
int ctl_vendor_command(u8 type, const u8 *p, u32 n)
{
    ++calls;
    if (busy && type != XSM_RADIO) return 1;
    assert(n <= sizeof command); memcpy(command, p, n);
    command_type = type; command_len = n;
    return 0;
}
#include "../../ble/ctl/src/ctl_vendor_bridge.c"
int main(void)
{
    xsm_boot_t boot = {0}; u8 address[6];
    ctl_bp = &boot.base;
    assert(ctl_vendor_address(address) < 0);
    boot.base.abi = XSM_ABI; boot.ext_magic = XSM_BOOT_EXT_MAGIC;
    boot.ext_size = sizeof boot; boot.address[0] = 1; boot.address[5] = 0xc0;
    assert(ctl_vendor_address(address) == 0 && address[0] == 1);
    boot.address[0] = 0; assert(ctl_vendor_address(address) < 0);
    memset(boot.address, 0xff, 6); assert(ctl_vendor_address(address) < 0);
    boot.base.abi = XB_ABI; assert(ctl_vendor_address(address) < 0);
    ctl_bp = NULL;
    u8 payload[129], readback[128], type;
    memset(payload, 0x55, sizeof payload);
    assert(ctl_vendor_notification_copy(payload, 128) == 0);
    memset(payload, 0xaa, sizeof payload);
    assert(xb_frame_get(&box.c2h, &type, readback, sizeof readback) == 128);
    assert(type == XSM_NOTIFY);
    for (unsigned i=0; i<128; ++i) assert(readback[i] == 0x55);
    assert(ctl_vendor_notification_copy(payload, 129) < 0);
    assert(ctl_vendor_notification_copy(NULL, 3) < 0);
    assert(ctl_vendor_notification_copy(payload, 2) < 0);
    while (!ctl_vendor_notification_copy(payload, 128)) {}
    assert(box.c2h_drops == 1);
    while (xb_frame_get(&box.c2h, &type, readback, sizeof readback) > 0) {}
    u8 event[12] = {3};
    assert(ctl_vendor_event_copy(0x12345678, event, 12) == 0);
    event[0] = 0;
    assert(xb_frame_get(&box.c2h, &type, readback, sizeof readback) == 16);
    assert(type == XSM_EVENT && readback[0] == 0x78 && readback[3] == 0x12 && readback[4] == 3);
    assert(ctl_vendor_link_copy(1, 7) == 0);
    assert(xb_frame_get(&box.c2h, &type, readback, sizeof readback) == 2);
    assert(type == XSM_LINK && readback[0] == 1 && readback[1] == 7);
    u8 midi[3] = {0xb0, 7, 127}, on = 1;
    assert(xb_frame_put(&box.h2c, XSM_MIDI_OUT, midi, 3) == 0);
    busy = 1; ctl_vendor_mailbox_poll(); assert(calls == 1);
    memset(midi, 0, 3);
    assert(xb_frame_put(&box.h2c, XSM_RADIO, &on, 1) == 0);
    busy = 0; ctl_vendor_mailbox_poll();
    assert(command_type == XSM_MIDI_OUT && command_len == 3 && command[0] == 0xb0 && command[2] == 127);
    ctl_vendor_mailbox_poll(); assert(command_type == XSM_RADIO && command_len == 1);
    unsigned before = calls;
    assert(xb_frame_put(&box.h2c, 0xee, midi, 3) == 0);
    ctl_vendor_mailbox_poll(); assert(calls == before);
    assert(xb_frame_put(&box.h2c, XSM_MIDI_OUT, payload, 128) == 0);
    ctl_vendor_mailbox_poll(); assert(calls == before);
    assert(xb_frame_put(&box.h2c, XSM_MIDI_OUT, midi, 3) == 0);
    busy = 1; ctl_vendor_mailbox_poll(); ctl_vendor_mailbox_reset();
    busy = 0; before = calls; ctl_vendor_mailbox_poll(); assert(calls == before);
    boot.base.abi = XSM_ABI; boot.radio_desired = 1; ctl_bp = &boot.base;
    ctl_vendor_mailbox_reset(); ctl_vendor_mailbox_poll();
    assert(command_type == XSM_RADIO && command[0] == 1);
    assert(xb_frame_put(&box.h2c, XSM_MIDI_OUT, midi, 3) == 0);
    busy = 1; ctl_vendor_mailbox_poll();
    on = 1; assert(xb_frame_put(&box.h2c, XSM_RADIO, &on, 1) == 0);
    boot.radio_desired = 0; ctl_vendor_mailbox_poll();
    assert(command_type == XSM_RADIO && command[0] == 0);
    busy = 0; before = calls; ctl_vendor_mailbox_poll(); assert(calls == before);
    on = 2; assert(xb_frame_put(&box.h2c, XSM_RADIO, &on, 1) == 0);
    ctl_vendor_mailbox_poll(); assert(calls == before);
    puts("vendor bridge: copied128-byte ingress, congestion, FIFO busy retry, rejected commands PASS");
}
