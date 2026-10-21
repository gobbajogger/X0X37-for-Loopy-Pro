// SPDX-License-Identifier: MPL-2.0
/* Hardware compatibility adapter: SDK chip_crc16 uses CRC-16/XMODEM
 * (poly 0x1021, initial 0, no reflection or final XOR). No hardware register
 * dependency and no protocol implementation. Enabled only in the host test. */
#include "ctl_port.h"
#if CTL_VENDOR_HOST_TEST
u16 chip_crc16(const void *data, u32 n)
{
    const u8 *p = data;
    u16 crc = 0;
    while (n--) {
        crc ^= (u16)*p++ << 8;
        for (unsigned bit = 0; bit < 8; ++bit)
            crc = (u16)((crc << 1) ^ ((crc & 0x8000u) ? 0x1021u : 0));
    }
    return crc;
}
#endif
