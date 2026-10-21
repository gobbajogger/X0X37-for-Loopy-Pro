/* SPDX-License-Identifier: MPL-2.0 */
#define _GNU_SOURCE
#include <assert.h>
#include <sys/mman.h>
#include <unistd.h>
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
static u8 command[7], command_type;
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
/* Legacy and truncated extension bytes end exactly at a protected page. */
static void mixed_boot_guards(void)
{
    size_t page = (size_t)sysconf(_SC_PAGESIZE);
    u8 *memory = mmap(NULL, page * 2u, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    assert(memory != MAP_FAILED && !mprotect(memory + page, page, PROT_NONE));
    xb_boot_t *old = (xb_boot_t *)(void *)(memory + page - sizeof(xb_boot_t));
    old->abi = 4; ctl_bp = old;
    u8 address[6]; u32 before = calls;
    assert(ctl_vendor_address(address) < 0 && !ctl_vendor_session_next());
    ctl_vendor_mailbox_poll(); assert(calls == before);
    xsm_boot_t *short_boot = (xsm_boot_t *)(void *)(memory + page - offsetof(xsm_boot_t, feedback_epoch));
    short_boot->base.abi = XSM_ABI; short_boot->ext_magic = XSM_BOOT_EXT_MAGIC;
    short_boot->ext_size = offsetof(xsm_boot_t, feedback_epoch); ctl_bp = &short_boot->base;
    assert(ctl_vendor_address(address) < 0 && !ctl_vendor_session_next());
    ctl_vendor_mailbox_poll(); assert(calls == before);
    assert(!munmap(memory, page * 2u)); ctl_bp = NULL;
}
int main(void)
{
    mixed_boot_guards();
    xsm_boot_t boot = {0}; u8 address[6];
    ctl_bp = &boot.base;
    assert(ctl_vendor_address(address) < 0);
    boot.base.abi = XSM_ABI; boot.ext_magic = XSM_BOOT_EXT_MAGIC;
    boot.ext_size = sizeof boot; boot.address[0] = 1; boot.address[5] = 0xc0;
    assert(ctl_vendor_address(address) == 0 && address[0] == 1);
    boot.address[0] = 0; assert(ctl_vendor_address(address) < 0);
    memset(boot.address, 0xff, 6); assert(ctl_vendor_address(address) < 0);
    boot.base.abi = XB_ABI; assert(ctl_vendor_address(address) < 0);
    boot.base.abi = XSM_ABI; boot.feedback_epoch = 1; boot.radio_desired = 1;
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
    assert(ctl_vendor_link_copy(1, 1, 7) == 0);
    assert(xb_frame_get(&box.c2h, &type, readback, sizeof readback) == 6);
    assert(type == XSM_LINK_SESSION && xsm_epoch_read(readback) == 1 && readback[4] == 1 && readback[5] == 7);
    u8 midi[3] = {0xb0, 7, 127}, on = 1;
    assert(xsm_feedback_put(&box.h2c, 1, midi, 3) == 0);
    ctl_vendor_mailbox_reset();
    busy = 1; ctl_vendor_mailbox_poll(); assert(calls == 2);
    memset(midi, 0, 3);
    assert(xb_frame_put(&box.h2c, XSM_RADIO, &on, 1) == 0);
    busy = 0; ctl_vendor_mailbox_poll();
    assert(command_type == XSM_MIDI_SESSION && command_len == 7 && command[4] == 0xb0 && command[6] == 127);
    unsigned queued_on_calls = calls; ctl_vendor_mailbox_poll(); assert(calls == queued_on_calls);
    unsigned before = calls;
    assert(xb_frame_put(&box.h2c, 0xee, midi, 3) == 0);
    ctl_vendor_mailbox_poll(); assert(calls == before);
    assert(xb_frame_put(&box.h2c, XSM_MIDI_OUT, payload, 128) == 0);
    ctl_vendor_mailbox_poll(); assert(calls == before);
    assert(xsm_feedback_put(&box.h2c, 1, midi, 3) == 0);
    busy = 1; ctl_vendor_mailbox_poll(); ctl_vendor_mailbox_reset();
    busy = 0; before = calls; ctl_vendor_mailbox_poll(); assert(calls == before + 1);
    boot.base.abi = XSM_ABI; boot.radio_desired = 1; ctl_bp = &boot.base;
    ctl_vendor_mailbox_reset(); ctl_vendor_mailbox_poll();
    assert(command_type == XSM_RADIO && command[0] == 1);
    assert(xsm_feedback_put(&box.h2c, 1, midi, 3) == 0);
    busy = 1; ctl_vendor_mailbox_poll();
    on = 1; assert(xb_frame_put(&box.h2c, XSM_RADIO, &on, 1) == 0);
    boot.radio_desired = 0; ctl_vendor_mailbox_poll();
    assert(command_type == XSM_RADIO && command[0] == 0);
    busy = 0; before = calls; ctl_vendor_mailbox_poll(); assert(calls == before);
    on = 2; assert(xb_frame_put(&box.h2c, XSM_RADIO, &on, 1) == 0);
    ctl_vendor_mailbox_poll(); assert(calls == before);
    boot.radio_desired = 2; before = calls; ctl_vendor_mailbox_poll(); assert(calls == before);
    puts("vendor bridge: copied128-byte ingress, congestion, FIFO busy retry, rejected commands PASS");
}
