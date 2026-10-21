/* SPDX-License-Identifier: MPL-2.0 */
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include "x0x_ble_abi.h"
static uint32_t ticks;
static xb_boot_t boot;
const xb_boot_t *ctl_bp = &boot;
void ctl_ms_init(void);
void ctl_timer_poll(void);
uint16_t usr_timeout_add(void *, void (*)(void *), uint32_t, uint8_t);
void usr_timeout_del(uint16_t);
static void timer_hit(void *p) { ++*(unsigned *)p; }
uint8_t __ctl_heap_start[32768] __attribute__((aligned(8)));
__asm__(".global __ctl_heap_end\n.set __ctl_heap_end, __ctl_heap_start + 32768");
uint32_t ctl_port_irq_save(void) { return 0; }
void ctl_port_irq_restore(uint32_t f) { (void)f; }
void ctl_heap_init(void);
void *study_malloc(size_t n);
void study_free(void *p);
void *lbuf_alloc(void *head, uint32_t n);
void *lbuf_realloc(void *p, uint32_t n);
int lbuf_real_size(void *p);
int lbuf_free(void *p);
uint16_t chip_crc16(const void *, uint32_t);
int main(void)
{
    ctl_heap_init();
    assert(!study_malloc(SIZE_MAX));
    assert(!lbuf_alloc(NULL, UINT32_MAX));
    assert(!lbuf_realloc(NULL, 0));
    uint8_t *p = lbuf_alloc(NULL, 4096);
    uint8_t *q = lbuf_alloc(NULL, 4096);
    assert(p && q);
    memset(p, 0xa5, 4096); memset(q, 0x5a, 4096);
    assert(!lbuf_realloc(p, 4096));
    assert(!lbuf_realloc(p, 4097));
    assert(lbuf_real_size(p) == 4096);
    assert(lbuf_realloc(p, 32) == p);
    assert(lbuf_real_size(p) == 32);
    for (unsigned i=0; i<32; ++i) assert(p[i] == 0xa5);
    for (unsigned i=0; i<4096; ++i) assert(q[i] == 0x5a);
    void *released = study_malloc(2048);
    assert(released && (uintptr_t)released > (uintptr_t)p &&
           (uintptr_t)released < (uintptr_t)q);
    study_free(released); lbuf_free(p); lbuf_free(q);
    void *merged = study_malloc(32000); assert(merged); study_free(merged);
    assert(chip_crc16("123456789", 9) == 0x31c3);
    assert(chip_crc16(NULL, 0) == 0);
    unsigned hits = 0;
    boot.tick_addr = (uint32_t)(uintptr_t)&ticks; boot.ticks_per_ms = 1;
    ctl_ms_init();
    uint16_t id = usr_timeout_add(&hits, timer_hit, 3, 1); assert(id);
    ticks = 2; ctl_timer_poll(); assert(hits == 0);
    ticks = 3; ctl_timer_poll(); assert(hits == 1);
    ticks = 4; ctl_timer_poll(); assert(hits == 1);
    id = usr_timeout_add(&hits, timer_hit, 1, 0); assert(id);
    usr_timeout_del(id); ticks = 5; ctl_timer_poll(); assert(hits == 1);
    puts("host heap/shrink ownership, CRC vectors and user timeouts: PASS");
}
