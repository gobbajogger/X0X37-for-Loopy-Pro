/* SPDX-License-Identifier: GPL-3.0-only */
/* Shared APC40 mkII identity / introduction SysEx payloads for USB and BLE.
 * Cable / notify / midi_send_sysex vs bleh_sysex_send stay at the call sites. */
#ifndef X0X_APC40_SYSEX_H
#define X0X_APC40_SYSEX_H
#include <stdint.h>

#ifndef X0X_APC40_SYSEX_DEFINED
#define X0X_APC40_SYSEX_DEFINED
/* Type 0x61: nine fader/slider values (SMK has none — zeros). */
static const uint8_t APC40_INTRO_MSG[17] = {
    0xF0, 0x47, 0x7F, 0x29, 0x61, 0x00, 0x09,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0xF7
};

/* Universal identity: Akai (47), APC40 mkII (29), 25 bytes of data after family. */
__attribute__((noinline)) static void apc40_fill_identity(uint8_t msg[35], uint8_t channel)
{
    static const uint8_t tail[25] = {
        0x01, 0x02, 0x00, 0x00, /* firmware version 1.2 */
        0x7F,                   /* SysEx device ID */
        0, 0, 0, 0,             /* serial reserved */
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 /* manufacturing data */
    };
    uint32_t i;
    msg[0] = 0xF0; msg[1] = 0x7E; msg[2] = channel; msg[3] = 0x06; msg[4] = 0x02;
    msg[5] = 0x47; msg[6] = 0x29; msg[7] = 0x00; msg[8] = 0x19;
    for (i = 0; i < sizeof tail; i++)
        msg[9u + i] = tail[i];
    msg[34] = 0xF7;
}
#endif
#endif
