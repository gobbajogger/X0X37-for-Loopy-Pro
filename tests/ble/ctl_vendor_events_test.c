/* SPDX-License-Identifier: MPL-2.0 */
#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#define CTL_PORT_H
#define CTL_VENDOR_HOST_TEST 1
typedef uint8_t u8; typedef uint32_t u32;
static u8 copied[12];
static int accept = 1;
static u32 source;
int ctl_vendor_event_copy(u32 from, const u8 *event, u32 len)
{
    assert(len == sizeof(copied));
    if (!accept) return -1;
    source = from;
    memcpy(copied, event, len);
    return 0;
}
#include "../../ble/ctl/src/ctl_vendor_events.c"
int main(void)
{
    struct ctl_sdk_bt_event e = {3, {1,2,3,4,5,6,7}, 0x12345678};
    ctl_vendor_events_reset();
    assert(!ctl_vendor_host_ready());
    assert(bt_event_notify(3, NULL) != 0);
    assert(bt_event_notify(2, &e) == 0 && !ctl_vendor_host_ready());
    assert(source == 2 && !memcmp(copied, &e, 12));
    accept = 0;
    assert(bt_event_notify(3, &e) != 0 && !ctl_vendor_host_ready());
    accept = 1;
    assert(bt_event_notify(3, &e) == 0 && ctl_vendor_host_ready());
    e.value = 0; assert(copied[8] == 0x78);
    ctl_vendor_events_reset(); assert(!ctl_vendor_host_ready());
    puts("vendor event adapter: copy ownership, startup event, congestion PASS");
}
