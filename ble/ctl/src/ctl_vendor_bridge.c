// SPDX-License-Identifier: MPL-2.0
/* Bounded byte-copy bridge only. The vendor retains ownership of every receive
 * buffer; no private DMA pointer leaves its task/core or survives this callback. */
#include "ctl_port.h"
#if CTL_VENDOR_HOST_TEST
#include "x0x_smc_host_messages.h"
/* Return0 accepted, positive busy (retry unchanged), negative rejected. */
extern int ctl_vendor_command(u8 type, const u8 *data, u32 len);
extern void ctl_vendor_poll(void);
static u8 pending[7], pending_type, pending_len, have_pending, radio_seen;

static xsm_boot_t *session_boot(void)
{
    /* Check the base ABI before accessing any extended bytes. */
    if (!ctl_bp || ctl_bp->abi != XSM_ABI) return 0;
    xsm_boot_t *boot = (xsm_boot_t *)(uintptr_t)ctl_bp;
    return boot->ext_magic == XSM_BOOT_EXT_MAGIC && boot->ext_size == sizeof(*boot) ? boot : 0;
}
u32 ctl_vendor_session_next(void)
{
    xsm_boot_t *boot = session_boot();
    if (!boot) return 0;
    u32 f = ctl_port_irq_save();
    u32 epoch = boot->feedback_epoch;
    if (epoch != UINT32_MAX) {
        boot->feedback_epoch = ++epoch;
        XB_SYNC();
    } else epoch = 0; /* exhausted: fail closed until coordinated app restart */
    ctl_port_irq_restore(f);
    return epoch;
}
int ctl_vendor_address(u8 address[6])
{
    const xsm_boot_t *boot = session_boot();
    if (!boot || (boot->address[5] & 0xc0u) != 0xc0u) return -1;
    u8 any = 0, non_ff = (u8)((boot->address[5] & 0x3fu) ^ 0x3fu);
    for (unsigned i = 0; i < 5; ++i) {
        any |= boot->address[i]; non_ff |= (u8)(boot->address[i] ^ 0xffu);
    }
    any |= boot->address[5] & 0x3fu;
    if (!any || !non_ff) return -1;
    memcpy(address, boot->address, 6);
    return 0;
}

static int copy_out(u8 type, const u8 *data, u32 len)
{
    if (!ctl_mb) return -1;
    u32 f = ctl_port_irq_save();
    int rc = xb_frame_put(&ctl_mb->c2h, type, data, len);
    if (rc) ++ctl_mb->c2h_drops;
    ctl_port_irq_restore(f);
    return rc;
}
int ctl_vendor_event_copy(u32 from, const u8 *event, u32 len)
{
    if (!event || len != 12u) return -1;
    u8 copy[16];
    for (unsigned i = 0; i < 4; ++i) copy[i] = (u8)(from >> (8u * i));
    memcpy(copy + 4, event, 12);
    return copy_out(XSM_EVENT, copy, sizeof copy);
}
int ctl_vendor_notification_copy(const u8 *data, u32 len)
{
    if (!data || len < 3u || len > XSM_NOTIFY_MAX) return -1;
    return copy_out(XSM_NOTIFY, data, len);
}
int ctl_vendor_link_copy(u32 epoch, u8 up, u8 reason)
{
    if (!epoch) return -1;
    u8 copy[6]; xsm_epoch_write(copy, epoch);
    copy[4] = !!up; copy[5] = reason;
    return copy_out(XSM_LINK_SESSION, copy, sizeof copy);
}
void ctl_vendor_mailbox_reset(void) { have_pending = 0; radio_seen = 0xffu; }
void ctl_vendor_mailbox_poll(void)
{
    const xsm_boot_t *boot = session_boot();
    if (!ctl_mb || !boot) return;
    XB_SYNC();
    u32 desired = boot->radio_desired;
    if (desired > 1u) { have_pending = 0; return; }
    if (desired != radio_seen) {
        u8 on = (u8)desired;
        if (ctl_vendor_command(XSM_RADIO, &on, 1u) == 0) {
            radio_seen = on;
            if (!on) have_pending = 0; /* user off cancels unsent feedback */
        }
    }
    ctl_vendor_poll();
    /* One command per poll bounds time spent outside the vendor tasks. The
     * pending copy survives a busy response; later commands cannot overtake it. */
    if (!have_pending) {
        u32 n; u8 type;
        int rc = xb_frame_peek(&ctl_mb->h2c, &type, &n);
        if (!rc) return;
        if (rc < 0 || !((type == XSM_MIDI_SESSION && n >= 5u && n <= 7u) ||
                        (type == XSM_RADIO && n == 1u))) {
            /* Existing ring decoder flushes malformed/oversized input. A valid
             * unknown short frame is consumed and rejected, never sent as HCI. */
            xb_frame_get(&ctl_mb->h2c, &type, pending, sizeof pending);
            return;
        }
        rc = xb_frame_get(&ctl_mb->h2c, &pending_type, pending, sizeof pending);
        if (rc <= 0) return;
        pending_len = (u8)rc;
        if (pending_type == XSM_RADIO && pending[0] > 1u) return;
        have_pending = 1;
    }
    /* Desired radio state has one authoritative source. A stale queued ON must never
     * override a newer desired OFF after congestion. */
    if (pending_type == XSM_RADIO || !desired || !xsm_epoch_read(pending) ||
        xsm_epoch_read(pending) != boot->feedback_epoch) {
        have_pending = 0;
        return;
    }
    if (ctl_vendor_command(pending_type, pending, pending_len) <= 0)
        have_pending = 0;
}
#endif
