/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "../../firmware/src/app/smc_protocol.h"

int main(void)
{
    static const uint8_t global[11] = {0x5E, 0x5D, 0x5F, 0x5B, 0x5C, 0x2E, 0x2F,
                                       0x60, 0x61, 0x62, 0x63};
    int strip, row, i;
    const uint8_t base[4] = {0x10, 0x08, 0x00, 0x18};
    for (strip = 0; strip < 8; strip++)
        for (row = 0; row < 4; row++)
            assert(smc_mmc_note((uint8_t)(strip * 4 + row + 1)) == base[row] + strip);
    for (i = 0; i < 11; i++)
        assert(smc_mmc_note((uint8_t)(33 + i)) == global[i]);
    assert(smc_mmc_note(0) == 0xFF && smc_mmc_note(44) == 0xFF);

    /* A new fader must first reach its current target, or cross it. A target
     * change later makes the same physical position unpicked again. */
    assert(!smc_pickup_reached(0, 0, 90, 20));
    assert(!smc_pickup_reached(20, 1, 90, 87));
    assert(smc_pickup_reached(87, 1, 90, 88));
    assert(smc_pickup_reached(88, 1, 90, 94));
    assert(!smc_pickup_reached(94, 0, 40, 92));
    assert(smc_pickup_reached(92, 1, 40, 35));
    assert(smc_relative(1) == 1 && smc_relative(0x41) == -1);
    assert(smc_relative(0x3F) == 63 && smc_relative(0x7F) == -63);
    assert(smc_relative(0x40) == 0);

    /* Channel 16 opt-in keeps normal music and APC40 traffic out. */
    assert(!smc_host_accept(0x9Fu, 16, 0));
    assert(!smc_host_accept(0x90u, 16, 1));
    assert(!smc_host_accept(0xBFu, 7, 1));
    assert(!smc_host_accept(0x9Fu, 60, 1));
    assert(smc_host_accept(0xBFu, 40, 1));
    assert(smc_host_accept(0xBFu, 23, 1));
    assert(smc_host_accept(0x9Fu, 16, 1));
    assert(smc_host_accept(0x8Fu, 8, 1));
    puts("SMC protocol: 43 buttons, pickup, encoders and host isolation ok");
    return 0;
}
