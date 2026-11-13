/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual OTA implementation/fixture, actual editor service and main-loop block.
 * ed_handle is a dispatch/ownership sentinel, not editor reply or USB emulation. */
#define main fixture_main
#define ota_service fixture_ota_service
#include "ota_fixture.inc"
#undef ota_service
#undef main
#include "editor_definitions.inc"

static struct { uint32_t mode, recovery, at; } ed_project;
static struct { unsigned ota_req, uboot_req; } usb;
static struct { unsigned pending; } bootguard;
static uint32_t fm1_ms, ed_reboot_at;
static uint8_t ed_install_pending;
static void ed_install_forget(void) { ed_install_pending = 0; }
static int seq, flash_ok;
static unsigned dispatches, ownership_errors, ota_polls, platform_errors;
static uint8_t dispatched[1024];
static uint32_t dispatched_n;
static const uint8_t *late_frame;
static uint32_t late_n;
static const uint8_t ping[] = {0x7D, 0x46, 0x4C, 25};
static const uint8_t ctl[] = {0x7D, 0x46, 0x4C, 37, 32, 111, 3, 112};
static const uint8_t identity[] = {0, 0x59, 0x11, 0, 0, 0, 0xFF};

static void inject(const uint8_t *p, uint32_t n)
{
    if (rx_full || n > sizeof rx) { puts("invalid fixture injection"); exit(2); }
    memcpy(rx, p, n); rx_len = n; rx_full = 1;
}
static int ed_project_end(int apply)
{
    (void)apply; ed_project.mode = 0; return 0;
}
static int ui_project_busy(void) { return ed_project.mode != 0; }
/* Production ed_service calls these; boot payload bodies stay out of this fixture. */
static int boot_cmd(uint32_t cmd)
{
    return cmd == ED_BOOT_INFO || (cmd >= ED_BOOT_BEGIN && cmd <= ED_BOOT_ABORT);
}
static void boot_housekeep(void) {}
static void ed_handle(const uint8_t *p, uint32_t n)
{
    int retained = p[3] == ED_PROJECT || (p[3] >= ED_SMP_BEGIN && p[3] <= ED_SMP_READ) ||
                   boot_cmd(p[3]);
    dispatches++;
    ownership_errors += retained ? (!rx_full || p != rx) : (rx_full || p == rx);
    memcpy(dispatched, p, n); dispatched_n = n;
}
#define ed_service actual_ed_service
#include "editor_service.inc"
#undef ed_service

/* Deterministic IRQ publication after the editor call but before the OTA check. */
#if ED_SERVICE_RETURNS_PENDING
static int ed_service(void)
{
    int pending = actual_ed_service();
    if (late_frame) { inject(late_frame, late_n); late_frame = NULL; }
    return pending;
}
#else
static void ed_service(void)
{
    actual_ed_service();
    if (late_frame) { inject(late_frame, late_n); late_frame = NULL; }
}
#endif
static void ota_service(void) { ota_polls++; fixture_ota_service(); }
enum { C_BLACK = 0 };
static void usb_detach(void) { platform_errors++; }
static void fm1_delay_ms(unsigned ms) { (void)ms; platform_errors++; }
static void fm1_reboot(void) { platform_errors++; }
static void seq_stop(int *s) { (void)s; platform_errors++; }
static void lcd_fill(int x, int y, int w, int h, int c)
{ (void)x; (void)y; (void)w; (void)h; (void)c; platform_errors++; }
static void ui_init(void) { platform_errors++; }
static void enter_uboot(const char *why) { (void)why; platform_errors++; }
static void poll(void)
{
    int ota_pending;
#include "main_dispatch.inc"
}
static void reset_case(void)
{
    rx_full = rx_len = now_ms = ident_replies = requests = 0;
    dispatches = ownership_errors = ota_polls = platform_errors = 0;
    dispatched_n = 0; late_frame = NULL; late_n = 0;
    fm1_ms = ed_reboot_at = 0;
    memset(&ed_project, 0, sizeof ed_project);
    memset(&usb, 0, sizeof usb);
}
static int request_cases(const char *label, const uint8_t *p, uint32_t n)
{
    int bad = 0;
    char name[128];
    reset_case(); inject(p, n); poll(); poll();
    snprintf(name, sizeof name, "%s before editor: exactly once, ownership intact", label);
    bad += check(name, dispatches == 1 && dispatched_n == n && !memcmp(dispatched, p, n) &&
                 !rx_full && !ownership_errors && !ident_replies);

    reset_case(); late_frame = p; late_n = n; poll();
    snprintf(name, sizeof name, "%s between consumers: retained unchanged", label);
    bad += check(name, rx_full && rx_len == n && !memcmp(rx, p, n) && !dispatches && !ident_replies);
    poll(); poll();
    snprintf(name, sizeof name, "%s next loop: exactly once, ownership intact", label);
    bad += check(name, dispatches == 1 && dispatched_n == n && !memcmp(dispatched, p, n) &&
                 !rx_full && !ownership_errors);

    reset_case(); poll(); inject(p, n); poll(); poll();
    snprintf(name, sizeof name, "%s after services: exactly once", label);
    bad += check(name, dispatches == 1 && !rx_full && !ownership_errors);
    return bad;
}
int main(void)
{
    const uint8_t project[] = {0x7D, 0x46, 0x4C, ED_PROJECT, 64};
    const uint8_t sample[] = {0x7D, 0x46, 0x4C, ED_SMP_WRITE, 0, 1, 2, 3};
    const uint8_t boot_write[] = {0x7D, 0x46, 0x4C, ED_BOOT_WRITE, 0, 1, 2, 3};
    const uint8_t unknown_editor[] = {0x7D, 0x46, 0x4C, 127};
    const uint8_t malformed[] = {0x7D, 0x46, 0x4C};
    uint8_t invalid[sizeof identity];
    int bad = 0;
    bad += request_cases("PING", ping, sizeof ping);
    bad += request_cases("CTL offset63392/count224", ctl, sizeof ctl);
    bad += request_cases("PROJECT", project, sizeof project);
    bad += request_cases("SMP_WRITE", sample, sizeof sample);
    bad += request_cases("BOOT_WRITE", boot_write, sizeof boot_write);
    bad += request_cases("unknown editor command", unknown_editor, sizeof unknown_editor);

    reset_case(); inject(ping, sizeof ping); late_frame = ctl; late_n = sizeof ctl; poll();
    bad += check("late CTL after PING release remains pending", dispatches == 1 && rx_full && !ownership_errors);
    poll(); poll();
    bad += check("PING then late CTL each dispatch once", dispatches == 2 && !rx_full &&
                 dispatched_n == sizeof ctl && !memcmp(dispatched, ctl, sizeof ctl) && !ownership_errors);

    reset_case(); host_reply(identity, sizeof identity); poll(); poll();
    bad += check("OTA identity routed once with exact model", !rx_full && !dispatches && ident_replies == 1 &&
                 !memcmp(ota_msg + 6, FELUCCA_ID, sizeof FELUCCA_ID - 1));
    reset_case(); poll(); host_reply(identity, sizeof identity); poll();
    bad += check("late OTA identity progresses next loop", !rx_full && !dispatches && ident_replies == 1);
    reset_case(); inject(malformed, sizeof malformed); poll();
    bad += check("short editor prefix drains as before", !rx_full && !dispatches && !ident_replies);
    reset_case(); memcpy(invalid, identity, sizeof invalid); invalid[6] ^= 1;
    host_reply(invalid, sizeof invalid); poll();
    bad += check("bad OTA checksum drains without reply", !rx_full && !ident_replies && !dispatches);
    reset_case(); inject((const uint8_t *)"junk", 4); poll();
    host_reply(identity, sizeof identity); poll();
    bad += check("unknown traffic drains; subsequent identity works", !rx_full && ident_replies == 1 && !dispatches);

    reset_case(); ed_project.mode = 1; host_reply(identity, sizeof identity); poll();
    bad += check("project hold discards OTA without polling scratch", !rx_full && !ota_polls && !ident_replies);
    inject(ping, sizeof ping); poll();
    bad += check("editor PING still dispatches under project hold", dispatches == 1 && !ownership_errors && !rx_full);
    reset_case(); ed_project.mode = ed_project.recovery = 1; fm1_ms = 15000;
    host_reply(identity, sizeof identity); poll();
    bad += check("recovery hold keeps OTA blocked", ed_project.mode && !rx_full && !ota_polls && !ident_replies);

    /* Safe mode has no editor consumer: production ota_service must still drain it. */
    reset_case(); inject(ping, sizeof ping); ota_service(); host_reply(identity, sizeof identity); ota_service();
    bad += check("safe-mode OTA drains editor and answers next identity", !rx_full && ident_replies == 1 && !dispatches);
    /* ota_read uses ota_take directly. Non-OTA input must not pin a session slot. */
    reset_case(); inject(ctl, sizeof ctl);
    bad += check("session consumer still drains unrelated editor frame", ota_take() == 0 && !rx_full);
    host_reply(identity, sizeof identity);
    bad += check("session consumer accepts next valid OTA frame", ota_take() == 7 && !rx_full && ota_dec[2] == 0x11);
    bad += check("no session/flash/reset/platform operation invoked", !requests && !committed && !bad_erase && !platform_errors);
    puts(bad ? "EDITOR/OTA DISPATCH FAILED" : "EDITOR/OTA DISPATCH PASS (host source routing only)");
    return bad != 0;
}
