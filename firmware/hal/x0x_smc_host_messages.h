/* SPDX-License-Identifier: MPL-2.0 */
/* Copied messages for the opt-in vendor-host experiment, not ABI2 HCI traffic.
 * An admitted image must use the segmented host ABI; these types must never be
 * sent to the normal raw-HCI controller. Payloads contain no pointer or handle
 * into vendor storage. A successful enqueue owns a complete independent copy. */
#pragma once
#define XSM_EVENT 0x70u          /* LE32 source, SDK bt_event[12] */
#define XSM_NOTIFY 0x71u         /* copied BLE-MIDI characteristic value */
#define XSM_LINK 0x72u           /* up(0/1), disconnect reason */
#define XSM_MIDI_OUT 0x73u       /* complete MIDI message, 1..3 bytes */
#define XSM_RADIO 0x74u          /* desired on(0/1) */
#define XSM_NOTIFY_MAX 128u

#include "x0x_ble_abi.h"
#define XSM_ABI 4u
#define XSM_BOOT_EXT_MAGIC 0x534D4354u
/* The app derives address from its existing factory-trim address routine.
 * This extension is only read after rejecting the normal ABI2 boot contract. */
typedef struct {
    xb_boot_t base;
    uint32_t ext_magic, ext_size;
    uint8_t address[6], reserved[2];
    volatile uint32_t radio_desired; /* app writes0/1; priority over queued feedback */
} xsm_boot_t;
