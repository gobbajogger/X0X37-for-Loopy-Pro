/* Execute the production main-loop editor/update block across transfer boundaries. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

static struct { unsigned ota_req, uboot_req; } usb;
static struct { unsigned pending; } bootguard;
static uint32_t fm1_ms, ed_reboot_at;
static uint8_t ed_install_pending;
static void ed_install_forget(void) { ed_install_pending = 0; }
static int busy, next_busy, inject_update, flash_ok = 1, seq, pending_ota;
static unsigned reboots, detaches, ota_polls, ota_entries, uboot_entries, stops;
enum { C_BLACK = 0 };

static int ui_project_busy(void) { return busy; }
static int ed_service(void)
{
    if (inject_update)
        usb.ota_req = usb.uboot_req = 1;
    if (next_busy >= 0)
        busy = next_busy;
    return pending_ota;
}
static void usb_detach(void) { detaches++; }
static void fm1_delay_ms(unsigned ms) { assert(ms == 30); }
static void fm1_reboot(void) { reboots++; }
static void ota_service(void) { ota_polls++; }
static void seq_stop(int *s) { assert(s == &seq); stops++; }
static void ota_session(void) { ota_entries++; }
static void lcd_fill(int x, int y, int w, int h, int c)
{
    assert(!x && !y && w == 240 && h == 240 && c == C_BLACK);
}
static void ui_init(void) {}
static void enter_uboot(const char *why) { assert(why); uboot_entries++; }

static void poll(void)
{
    int ota_pending;
#include "project_updates.inc"
}
static void reset(void)
{
    usb.ota_req = usb.uboot_req = 0;
    bootguard.pending = 1;
    fm1_ms = 1000;
    ed_reboot_at = 0;
    busy = inject_update = 0;
    next_busy = -1;
    pending_ota = 1;                                /* the editor peek left a non-editor frame */
    reboots = detaches = ota_polls = ota_entries = uboot_entries = stops = 0;
}

int main(void)
{
    reset();
    busy = 1;
    ed_reboot_at = 1;
    usb.ota_req = usb.uboot_req = 1;
    poll();
    assert(!reboots && !detaches && bootguard.pending && ed_reboot_at == 1);
    assert(!ota_polls && !ota_entries && !uboot_entries && !stops);
    assert(!usb.ota_req && !usb.uboot_req);
    busy = 0;
    poll();
    assert(reboots == 1 && detaches == 1 && !bootguard.pending);
    assert(!ota_entries && !uboot_entries);           /* cleared requests cannot replay */

    reset();
    next_busy = 1;                                  /* BEGIN acquires the hold in ed_service */
    inject_update = 1;
    ed_reboot_at = 1;
    poll();
    assert(busy && !reboots && !ota_polls && !ota_entries && !uboot_entries);
    assert(!usb.ota_req && !usb.uboot_req);

    reset();
    busy = 1;
    next_busy = 0;                                  /* COMMIT / ABORT releases the hold in ed_service */
    inject_update = 1;                              /* interrupt arrives while that handler runs */
    poll();
    assert(!busy && ota_polls == 1 && !ota_entries && !uboot_entries && !stops);
    assert(!usb.ota_req && !usb.uboot_req);

    reset();
    usb.ota_req = 1;
    poll();
    assert(ota_polls == 1 && ota_entries == 1 && stops == 1 && !usb.ota_req);
    reset();
    usb.uboot_req = 1;
    poll();
    assert(uboot_entries == 1);
    reset();
    ed_reboot_at = 800;                              /* retain the diagnostic reply's 200 ms grace */
    poll();
    assert(!reboots);
    fm1_ms++;
    poll();
    assert(reboots == 1);
    reset();
    pending_ota = 0;                                /* no non-editor frame from this editor check */
    poll();
    assert(!ota_polls && !ota_entries && !uboot_entries);
    puts("project_update_guard: held restart, BEGIN/COMMIT update discard and idle updates ok");
    return 0;
}
