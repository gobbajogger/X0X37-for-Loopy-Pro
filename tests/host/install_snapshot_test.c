/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual editor parser and actual main-loop handoff. Platform boundaries are
 * sentinels; samples and flash exist only in RAM. No firmware/device writes. */
#define main project_protocol_fixture_main
#include "project_protocol_test.c"
#undef main

static unsigned checks, ota_entries, ota_polls, reboots;
static int inject_session_request;
static unsigned flush_calls, flush_before_ota;
void engine_brk_flush(void) { assert(!held && !ed_install_pending); flush_calls++; }
#include "../../build/gen/adaptive_native_brk_service.h"
seq_t seq;
enum { C_BLACK = 0 };
static void usb_detach(void) {}
static void fm1_delay_ms(unsigned ms) { (void)ms; }
static void fm1_reboot(void) { reboots++; }
static void enter_uboot(const char *why) { (void)why; reboots++; }
static void ota_service(void) { ota_polls++; }
void seq_stop(seq_t *s) { assert(s == &seq); }
static void lcd_fill(int x, int y, int w, int h, int c)
{ (void)x; (void)y; (void)w; (void)h; (void)c; }
void ui_init(void) {}
static void ota_session(void)
{
    assert(!ed_install_pending && ed_install.state != 1u && ed_install.state != 2u);
    assert(!flash_writes && !flash_erases);
    flush_before_ota=flush_calls;
    ota_entries++;
    if (inject_session_request) usb.ota_req = usb.uboot_req = 1;
}
static void poll(void)
{
    int ota_pending;
#include "project_updates.inc"
    native_break_service();
}
static void check(int ok) { checks++; assert(ok); }
static void reset(void)
{
    memset(&ed_install, 0, sizeof ed_install);
    memset(&ed_project, 0, sizeof ed_project);
    memset(&bst, 0, sizeof bst);
    memset(smp_importing, 0, sizeof smp_importing);
    memset(smp_sec0_erased, 0, sizeof smp_sec0_erased);
    memset(usr_nz, 0, sizeof usr_nz);
    ed_install_pending = 0;
    flash_writes = flash_erases = held = wire_fail = stage_held = 0;
    flash_ok = 1;
    fm1_ms = ed_reboot_at = frame_n = reply_n = 0;
    usb.ota_req = usb.uboot_req = 0;
    ota_entries = ota_polls = reboots = inject_session_request = 0;
    wire_after_send = 0;
    flush_calls=flush_before_ota=0;
}
static uint8_t token[ED_INSTALL_TOKEN] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};
static uint32_t command(uint8_t op, const uint8_t *t, uint32_t ticket, int expected, int through_main)
{
    uint8_t args[1 + ED_INSTALL_TOKEN + 4];
    uint32_t k, n = 1 + ED_INSTALL_TOKEN;
    args[0] = op;
    memcpy(args + 1, t, ED_INSTALL_TOKEN);
    if (op != 1) {
        for (k = 0; k < 4; k++) args[n++] = (uint8_t)(ticket >> (7 * k)) & 127;
    }
    if (through_main) {
        frame[0]=ED_HDR0; frame[1]=ED_HDR1; frame[2]=ED_HDR2; frame[3]=ED_INSTALL;
        memcpy(frame + 4, args, n); frame_n = n + 4; reply_n = 0;
        poll();
    } else request(ED_INSTALL, args, n);
    check(reply_n == 28 && reply[5] == op && reply[6] == expected);
    check(!memcmp(reply + 7, t, ED_INSTALL_TOKEN));
    ticket = 0;
    for (k=0; k<4; k++) ticket |= (uint32_t)reply[7 + ED_INSTALL_TOKEN + k] << (7 * k);
    return ticket;
}
static void late_sample(void)
{
    wire_after_send = 0;
    /* Synthetic IRQ after COMMIT ACK enqueue; do not execute a handler here. */
    frame[0]=ED_HDR0; frame[1]=ED_HDR1; frame[2]=ED_HDR2; frame[3]=ED_SMP_ERASE; frame[4]=0;
    frame_n = 5;
}
static void late_legacy_request(void)
{
    wire_after_send = 0;
    usb.ota_req = usb.uboot_req = 1;
}
int main(void)
{
    uint32_t t, old;
    uint8_t other[ED_INSTALL_TOKEN], malformed[32] = {1};
    memcpy(other, token, sizeof other); other[0]++;
    reset();
    request(ED_INFO, 0, 0);
    check(reply_n > 11 && !memcmp(reply + reply_n - 6, "XSG\1\60", 5));
    t = command(1, token, 0, 0, 0);
    check(t == 1 && ed_install.state == 1 && !ed_install_pending);
    check(command(1, token, 0, 0, 0) == t); /* duplicate START is not a new snapshot */
    command(3, token, t, 0, 1);
    check(ota_entries == 1 && !ed_install_pending && !flash_writes && !flash_erases);
    command(3, token, t, 3, 1);
    check(ota_entries == 1);
    command(1, token, 0, 3, 0); /* consumed token cannot rearm */

    /* Invalidate before argument checks: malformed mutation attempts also fail closed. */
    for (unsigned cmd=ED_SMP_BEGIN; cmd<=ED_SMP_ERASE; cmd++) {
        reset(); t = command(1, token, 0, 0, 0);
        uint8_t f[] = {ED_HDR0, ED_HDR1, ED_HDR2, (uint8_t)cmd};
        ed_handle(f, sizeof f);
        check(ed_install.state == 2);
        command(1, token, 0, 4, 0);
        command(3, token, t, 4, 1);
        check(!ota_entries && !flash_writes && !flash_erases);
    }
    reset(); old = command(1, token, 0, 0, 0);
    check(command(1, other, 0, 0, 0) == old+1);
    t = command(1, token, 0, 0, 0); check(t == old+2);
    command(3, token, old, 3, 1); check(!ota_entries);
    command(2, token, old, 3, 0); check(ed_install.state == 1);
    command(2, token, t, 0, 0); check(ed_install.state == 3);
    command(3, token, t, 3, 1); check(!ota_entries);
    command(1, token, 0, 3, 0);

    reset(); t = command(1, token, 0, 0, 0);
    memset(&ed_install, 0, sizeof ed_install); /* volatile reset loses authorization */
    command(3, token, t, 3, 1); check(!ota_entries);
    for (unsigned busy=0; busy<9; busy++) {
        reset();
        switch(busy) {
        case 0: smp_importing[0]=1; break; case 1: smp_sec0_erased[0]=1; break;
        case 2: ed_project.mode=ed_project.recovery=1; held=1; break;
        case 3: bst.armed=1; break; case 4: stage_held=1; break;
        case 5: ed_reboot_at=1; break; case 6: usb.ota_req=1; break;
        case 7: usb.uboot_req=1; break; case 8: flash_ok=0; break;
        }
        command(1, token, 0, 2, 0); check(!ed_install.state && !ed_install_pending);
    }
    reset(); ed_install.serial=ED_INSTALL_TICKET_MAX;
    command(1, token, 0, 5, 0); check(!ed_install.state);
    reset(); request(ED_INSTALL, malformed, 1); check(reply_n==8 && reply[6]==1);
    request(ED_INSTALL, malformed, 17); check(reply_n==8 && reply[6]==1); /* zero token */
    memcpy(malformed+1, token, sizeof token); malformed[1]|=128;
    request(ED_INSTALL, malformed, 17); check(reply_n==8 && reply[6]==1);

    reset(); t=command(1, token, 0, 0, 0); wire_fail=-1;
    command(3, token, t, 0, 1);
    check(!ota_entries && !ed_install_pending && ed_install.state==3);
    wire_fail=0; command(3, token, t, 3, 1); check(!ota_entries);
    reset(); t=command(1, token, 0, 0, 0); wire_fail=-1; wire_after_send=late_legacy_request;
    command(3, token, t, 0, 1);
    check(!ota_entries && !reboots && !usb.ota_req && !usb.uboot_req && !ed_install_pending);
    reset(); t=command(1, token, 0, 0, 0); usb.ota_req=1;
    command(3, token, t, 2, 1);
    check(!ota_entries && !usb.ota_req && !ed_install_pending);
    reset(); t=command(1, token, 0, 0, 0);
    wire_after_send=late_sample; inject_session_request=1;
    command(3, token, t, 0, 1);
    check(ota_entries==1 && frame_n==5 && !flash_writes && !flash_erases);
    check(!usb.ota_req && !usb.uboot_req && !ed_install_pending);
    frame_n=0; poll(); check(ota_entries==1); /* return cannot replay authorization */
    reset(); command(1, token, 0, 0, 0); usb.ota_req=1; poll();
    check(ota_entries==1 && ed_install.state==3);
    reset(); command(1, token, 0, 0, 0); usb.uboot_req=1; poll();
    check(reboots==1 && ed_install.state==3);
    reset(); command(1, token, 0, 0, 0); ed_reboot_at=1; fm1_ms=202; poll();
    check(reboots==1 && ed_install.state==3);
    reset();held=1;poll();check(flush_calls==0);held=0;poll();check(flush_calls==1);
    reset();t=command(1,token,0,0,0);command(3,token,t,0,1);
    check(ota_entries==1 && flush_before_ota==0 && flush_calls==1);
    printf("install snapshot: %u checks, parser + same-loop OTA boundary PASS (host only)\n", checks);
    return 0;
}
