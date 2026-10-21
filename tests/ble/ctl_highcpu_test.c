/* SPDX-License-Identifier: MPL-2.0 */
#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#define CTL_VENDOR_HOST_TEST 1
#define CTL_VENDOR_HIGHCPU_TEST 1
unsigned char __ctl_highcpu_start[1], __ctl_highcpu_end[1];
#include "../../ble/ctl/src/ctl_highcpu.c"
int main(void)
{
    unsigned char *p = mmap((void *)0x01c7d000u, 0x3000, PROT_READ|PROT_WRITE,
                            MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE, -1, 0);
    assert(p == (void *)0x01c7d000u);
    memset(p, 0xa5, 0x3000);
    assert(highcpu_clear(0x01c7d000u, 0x01c7f000u) == -1);
    assert(highcpu_clear(0x01c7d400u, 0x01c7f004u) == -1);
    assert(highcpu_clear(0x01c7d400u, 0x01c7d400u) == -1);
    assert(highcpu_clear(0x01c7d400u, 0x01c7d401u) == -1);
    for (unsigned i = 0; i < 0x3000; ++i) assert(p[i] == 0xa5);
    assert(highcpu_clear(0x01c7d400u, 0x01c7de00u) == 0);
    for (unsigned i = 0; i < 0x3000; ++i)
        assert(p[i] == ((i >= 0x400 && i < 0x2000) ? 0 : 0xa5));
    p[0x400] = 0x42;
    assert(highcpu_clear(0x01c7d400u, 0x01c7f000u) == -1);
    assert(p[0x400] == 0x42); /* cannot clear live task state twice */
    assert(munmap(p, 0x3000) == 0);
    puts("high CPU BSS: exact bounds, full clear, outside guards, single init PASS");
}
