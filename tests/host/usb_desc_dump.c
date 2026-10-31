/* SPDX-License-Identifier: GPL-3.0-only */
/* Print the real USB device and configuration descriptors of usb.c as hex, one per line
 * (tests/host/usb_audio_descriptors_test.py parses and validates them). */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#define RING_PUBLISH() __asm__ volatile("" ::: "memory")
#define FM1_IRQ_TIMER5 63u
static uint32_t fm1_irq_mask_save(unsigned n) { (void)n; return 0; }
static void fm1_irq_mask_restore(unsigned n, uint32_t s) { (void)n; (void)s; }
static void fm1_delay_ms(uint32_t ms) { (void)ms; }
#include "usb.c"

static void dump(const char *tag, uint32_t wvalue)
{
    const uint8_t *d;
    uint16_t l, i;
    if (!get_desc(wvalue, &d, &l)) {
        printf("%s -\n", tag);
        return;
    }
    printf("%s ", tag);
    for (i = 0; i < l; i++)
        printf("%02x", d[i]);
    printf("\n");
}

int main(void)
{
    dump("device", 0x0100);
    dump("config", 0x0200);
    dump("string2", 0x0302);
    return 0;
}
