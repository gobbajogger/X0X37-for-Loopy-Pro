/* SPDX-License-Identifier: GPL-3.0-only */
/* Execute real usb.c with a register model. A blocking retry, early attach,
 * lost cancellation, changed SIE order or broken wrap handling must fail. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define RING_PUBLISH() __asm__ volatile("" ::: "memory")
#define FM1_IRQ_TIMER5 63u
#define FM1_TICKS_PER_US 24u
static uint32_t fm1_irq_mask_save(unsigned n) { (void)n; return 0; }
static void fm1_irq_mask_restore(unsigned n, uint32_t s) { (void)n; (void)s; }
static uint32_t ticks, resets, attaches, delays, offs, events[32], event_count;
static uint8_t sie_alive = 1;
static void *attached_ep0;
static uint32_t fm1_ticks(void) { return ticks; }
static void event(uint32_t value) { if (event_count < 32u) events[event_count++] = value; }
static void fm1_delay_ms(uint32_t ms) { delays += ms; ticks += ms * 24000u; event(2); }
#define fm1_usb_reset base_reset
#define fm1_usb_attach base_attach
#define fm1_usb_off base_off
#define fm1_usb_ep_enable base_enable
#define fm1_usb_sie_on base_sie_on
#define fm1_usb_sie_done base_sie_done
#define fm1_usb_sie_wr_start base_write
#include "hal/fm1_usb.h"
#undef fm1_usb_reset
#undef fm1_usb_attach
#undef fm1_usb_off
#undef fm1_usb_ep_enable
#undef fm1_usb_sie_on
#undef fm1_usb_sie_done
#undef fm1_usb_sie_wr_start
static void fm1_usb_reset(void) { resets++; ticks += 48u; event(1); }
static void fm1_usb_attach(void *p) { attaches++; attached_ep0 = p; event(3); }
static void fm1_usb_off(void) { offs++; event(4); }
static void fm1_usb_ep_enable(uint32_t mask) { event(0x20000u | mask); }
static uint32_t fm1_usb_sie_on(void) { return 1; }
static uint32_t fm1_usb_sie_done(void) { return sie_alive; }
static void fm1_usb_sie_wr_start(uint32_t reg, uint32_t value)
{
    event(0x10000u | reg << 8 | value);
    base_write(reg, value);
}
#include "src/usb.c"

static unsigned failures, checks;
#define CHECK(condition, label) do { checks++; if (!(condition)) { failures++; printf("FAIL %s\n", label); } } while (0)
static void clear_events(void) { event_count = 0; }
static void check_attach_order(void)
{
    static const uint32_t expected[] = {
        3, 0x10160, 0x10b07, 0x10701, 0x10800, 0x10900, 0x10a00, 0x2001f
    };
    CHECK(event_count == 8 && !memcmp(events, expected, sizeof expected),
          "attach, power, interrupt masks and endpoint enable keep their order");
    CHECK(attached_ep0 == ep0buf, "attach uses EP0 DMA buffer");
}

int main(void)
{
    uint32_t reset_tick, prior;
    /* Boot is deliberately blocking and still uses the same attach sequence. */
    usb.timeouts = 9;
    usb_start();
    CHECK(delays == 25 && resets == 1 && attaches == 1 && usb.up && !usb.timeouts,
          "boot resets, waits 25 ms and attaches");
    CHECK(event_count == 10 && events[0] == 1 && events[1] == 2,
          "boot reset precedes delay and attach");
    memmove(events, events + 2, 8 * sizeof *events); event_count = 8;
    check_attach_order();
    usb.up = 0; delays = 0; clear_events();
    usb_retry(999);
    CHECK(resets == 1 && !usb.retries, "retry waits for one-second admission");
    ticks = 500000u; usb.timeouts = 9;
    usb_retry(1000); reset_tick = ticks;
    CHECK(resets == 2 && attaches == 1 && usb.retries == 1 && !usb.timeouts,
          "admitted retry resets only and clears stale timeouts");
    CHECK(!delays && event_count == 1 && events[0] == 1,
          "retry returns without a blocking delay or attach");
    clear_events();
    /* Simulated foreground dispatch can run repeatedly before the deadline. */
    for (unsigned i = 0; i < 25; i++) {
        ticks = reset_tick + i * 24000u;
        usb_retry(1001 + i);
    }
    ticks = reset_tick + 599999u; usb_retry(1025);
    CHECK(attaches == 1 && resets == 2 && !event_count && !delays,
          "foreground calls below 25 ms do no hardware work");
    ticks = reset_tick + 600000u; usb_retry(1025);
    CHECK(attaches == 2 && usb.up && !delays, "attach occurs at exactly 25 ms");
    check_attach_order();
    clear_events(); usb_retry(2000);
    CHECK(!event_count, "up link suppresses retry");
    usb.up = 0; usb_retry(1999);
    CHECK(!event_count, "retry cadence is measured from reset admission");
    usb_retry(2000); reset_tick = ticks; clear_events();
    usb_detach(); ticks = reset_tick + 600000u; usb_retry(2025);
    CHECK(offs == 1 && attaches == 2 && usb.detached, "detach cancels pending attach");
    usb.detached = 0; clear_events(); usb_retry(2999);
    CHECK(!event_count, "cancelled reset is not resumed");
    usb_retry(3000); usb.up = 1; usb_retry(3001); usb.up = 0;
    clear_events(); ticks += 600000u; usb_retry(3025);
    CHECK(!event_count, "up transition cancels pending attach");
    /* Both independent unsigned clocks may wrap. */
    ticks = UINT32_MAX - 300000u; usb_retry(UINT32_MAX - 10u); reset_tick = ticks;
    clear_events(); ticks = reset_tick + 599999u; usb_retry(14);
    CHECK(!event_count, "TIMER4 wrap still waits the full settle interval");
    ticks = reset_tick + 600000u; usb_retry(15);
    CHECK(usb.up, "TIMER4 wrap attaches at the boundary");
    check_attach_order(); usb.up = 0; clear_events();
    usb_retry(988);
    CHECK(!event_count, "millisecond wrap preserves the one-second interval");
    sie_alive = 0; usb_retry(989); reset_tick = ticks;
    clear_events(); ticks = reset_tick + 600000u; prior = attaches; usb_retry(1014);
    CHECK(attaches == prior + 1 && !usb.up && usb.timeouts >= 3,
          "dead SIE keeps USB down after settled attach");
    clear_events(); usb_retry(1988);
    CHECK(!event_count, "dead SIE cannot cause a retry storm");
    sie_alive = 1; usb_retry(1989); reset_tick = ticks;
    CHECK(!usb.timeouts, "next retry clears dead-SIE timeout count");
    clear_events(); ticks = reset_tick + 600000u; usb_retry(2014);
    CHECK(usb.up && !usb.timeouts && !delays, "later live SIE recovers without blocking");
    check_attach_order();
    printf("USB cooperative retry: %u checks, %u fail\n", checks, failures);
    return failures != 0;
}
