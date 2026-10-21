// SPDX-License-Identifier: MPL-2.0
/* Copyright (C) 2026 M. Geisler (genkigenki/makkuro)
 * SDK event adapter only. Protocol engines and task dispatch stay in the vendor
 * host. The public SDK bt_event ABI is 12 bytes: event, args[7], value. */
#include "ctl_port.h"

#if defined(CTL_VENDOR_HOST_TEST) && CTL_VENDOR_HOST_TEST
struct ctl_sdk_bt_event { u8 event, args[7]; u32 value; };
_Static_assert(sizeof(struct ctl_sdk_bt_event) == 12, "SDK bt_event ABI");
_Static_assert(offsetof(struct ctl_sdk_bt_event, value) == 8, "SDK bt_event value");

/* Implemented by the controller/app bridge. It must copy all bytes before
 * returning, never pass the vendor pointer to another core, and return 0 only
 * when accepted. This required symbol deliberately has no weak/drop stub. */
extern int ctl_vendor_event_copy(u32 from, const u8 *event, u32 len);
static u8 host_ready;

void ctl_vendor_events_reset(void) { host_ready = 0; }
int ctl_vendor_host_ready(void) { return host_ready; }

int bt_event_notify(int from, const struct ctl_sdk_bt_event *event)
{
    if (!event) return -1;
    int result = ctl_vendor_event_copy((u32)from, (const u8 *)event, sizeof(*event));
    if (result) return result;
    /* Public SDK BT_EVENT_FROM_CON=3, BT_STATUS_INIT_OK=3. This is startup
     * readiness, never evidence of a connection, notification or SMC control. */
    if (from == 3 && event->event == 3) host_ready = 1;
    return 0;
}
#endif
