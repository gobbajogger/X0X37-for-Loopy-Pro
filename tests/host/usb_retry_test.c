/* SPDX-License-Identifier: GPL-3.0-only */
/* Runtime USB retry schedule. usb_retry is the new cooperative path. usb_retry_old is the
 * previous main-loop retry: one call reset the block, waited 25 ms, and attached.
 * Boot still uses usb_start, which waits. */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#define RING_PUBLISH() __asm__ volatile("" ::: "memory")
#define FM1_IRQ_TIMER5 63u
static uint32_t fm1_irq_mask_save(unsigned n) { (void)n; return 0; }
static void fm1_irq_mask_restore(unsigned n, uint32_t s) { (void)n; (void)s; }
#define FM1_TICKS_PER_US 24u
static uint32_t fake_ticks;
static uint32_t fm1_ticks(void) { return fake_ticks; }
static void fm1_delay_ms(uint32_t ms) { fake_ticks += ms * 1000u * FM1_TICKS_PER_US; }
#include "src/usb.c"

void smc_mmc_host(uint8_t cmd, uint8_t src) { (void)cmd; (void)src; }

#define SPAN (25000u * FM1_TICKS_PER_US)

/* The previous retry: the whole bring-up, including the 25 ms wait, in one call. */
static void usb_retry_old(uint32_t now_ms)
{
    static uint32_t t;
    if (usb.up || usb.detached || now_ms - t < 1000u)
        return;
    t = now_ms;
    usb.retries++;
    usb_start();
}

static int fails;

static void expect(int cond, const char *what)
{
    if (!cond) {
        printf("RED usb-retry: %s (resets %u attaches %u up %u detached %u ticks %u)\n",
               what, fake_resets, fake_attaches, usb.up, usb.detached, fake_ticks);
        fails++;
    }
}

static void clear_link(void)
{
    usb.up = 0;
    usb.detached = 0;
    usb.timeouts = 0;
}

int main(void)
{
    unsigned r0, a0;
    memset(&usb, 0, sizeof usb);
    fake_ticks = 0;
    fake_resets = fake_attaches = 0;

    usb_retry(1000u);
    expect(fake_resets == 1u && fake_attaches == 0u && usb.up == 0u, "first call only resets");
    usb_retry(1000u);
    usb_retry(1000u);
    expect(fake_resets == 1u && fake_attaches == 0u, "no re-entry while the reset settles");
    fake_ticks = SPAN - 1u;
    usb_retry(1000u);
    expect(fake_resets == 1u && fake_attaches == 0u, "no attach before 25 ms");
    fake_ticks = SPAN;
    usb_retry(1000u);
    expect(fake_resets == 1u && fake_attaches == 1u && usb.up == 1u, "one attach at 25 ms");
    usb_retry(1000u);
    usb_retry(2000u);
    expect(fake_resets == 1u && fake_attaches == 1u, "no second reset while USB is up");

    clear_link();
    usb.detached = 1;
    r0 = fake_resets;
    a0 = fake_attaches;
    usb_retry(3000u);
    expect(fake_resets == r0 && fake_attaches == a0, "detached cancels settle and does not attach");

    usb.detached = 0;
    usb.up = 1;
    usb_retry(4000u);
    expect(fake_resets == r0 && fake_attaches == a0, "usb.up does not start a reset");

    /* Unsigned tick wrap: 25 ms measured with the same subtraction as fm1_delay_us. */
    clear_link();
    fake_ticks = 0xFFFFF000u;
    usb_retry(5000u);
    expect(fake_resets == r0 + 1u && fake_attaches == a0, "wrap case resets");
    fake_ticks = 0xFFFFF000u + SPAN - 1u;
    usb_retry(5000u);
    expect(fake_attaches == a0, "no attach before 25 ms across the tick wrap");
    fake_ticks = 0xFFFFF000u + SPAN;
    usb_retry(5000u);
    expect(fake_attaches == a0 + 1u && fake_resets == r0 + 1u, "one attach after the wrapped 25 ms");

    /* Cancel while reset is pending, before any attach can occur. */
    clear_link();
    fake_ticks = 0;
    r0 = fake_resets;
    a0 = fake_attaches;
    usb_retry(6000u);
    expect(fake_resets == r0 + 1u && fake_attaches == a0, "pending detach case resets");
    usb.detached = 1;
    usb_retry(6000u);
    fake_ticks = SPAN;
    usb_retry(6000u);
    expect(fake_attaches == a0, "detach cancels a pending attach after 25 ms");
    clear_link();
    usb_retry(6000u);
    expect(fake_resets == r0 + 1u && fake_attaches == a0, "cancelled retry keeps the one-second gate");
    usb_retry(7000u);
    expect(fake_resets == r0 + 2u && fake_attaches == a0, "later retry starts a fresh reset");
    usb.up = 1;
    usb_retry(7000u);
    clear_link();
    fake_ticks += SPAN;
    usb_retry(7000u);
    expect(fake_attaches == a0, "usb.up cancels a pending attach");
    usb_retry(8000u);
    fake_ticks += SPAN;
    usb_retry(8000u);
    expect(fake_resets == r0 + 3u && fake_attaches == a0 + 1u, "retry recovers after cancellation");

    /* Old retry still does both in the call that passes the one-second gate. */
    r0 = fake_resets;
    a0 = fake_attaches;
    clear_link();
    fake_ticks = 0;
    usb_retry_old(1000u);
    expect(fake_resets == r0 + 1u && fake_attaches == a0 + 1u && usb.up == 1u,
           "old retry resets and attaches in one call");
    r0 = fake_resets;
    a0 = fake_attaches;
    usb_retry_old(5000u);
    expect(fake_resets == r0 && fake_attaches == a0, "old retry stays idle while usb.up");
    clear_link();
    usb.detached = 1;
    usb_retry_old(9000u);
    expect(fake_resets == r0 && fake_attaches == a0, "old retry stays idle while detached");

    if (fails)
        return 1;
    puts("GREEN usb-retry: reset, no attach before 25 ms, one attach after, no re-entry; idle if up or detached");
    return 0;
}
