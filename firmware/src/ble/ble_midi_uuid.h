/* SPDX-License-Identifier: GPL-3.0-only */
/* BLE-MIDI service and characteristic UUIDs (little endian). host and central each
 * carried an identical pair; both land in the unity TU (ble.c via x0x.c), so one
 * include keeps a single XIP copy. */
#ifndef BLE_MIDI_UUID_H
#define BLE_MIDI_UUID_H
#include <stdint.h>

#ifndef BLE_MIDI_UUID_DEFINED
#define BLE_MIDI_UUID_DEFINED
/* service 03B80E5A-EDE8-4B33-A751-6CE34EC4C700, characteristic 7772E5DB-3868-4112-A1A9-F2669D106BF3 */
static const uint8_t BLE_MIDI_UUID_SVC[16] = {
    0x00, 0xC7, 0xC4, 0x4E, 0xE3, 0x6C, 0x51, 0xA7, 0x33, 0x4B, 0xE8, 0xED, 0x5A, 0x0E, 0xB8, 0x03};
static const uint8_t BLE_MIDI_UUID_CHR[16] = {
    0xF3, 0x6B, 0x10, 0x9D, 0x66, 0xF2, 0xA9, 0xA1, 0x12, 0x41, 0x68, 0x38, 0xDB, 0xE5, 0x72, 0x77};
#endif
#endif
