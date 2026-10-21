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
#define XSM_LINK_SESSION 0x75u  /* LE32 epoch, up(0/1), disconnect reason */
#define XSM_MIDI_SESSION 0x76u  /* LE32 epoch, complete MIDI message, 1..3 bytes */
#define XSM_RADIO 0x74u          /* desired on(0/1) */
#define XSM_NOTIFY_MAX 128u

#include <stddef.h>
#include "x0x_ble_abi.h"
#define XSM_ABI 5u
#define XSM_BOOT_EXT_MAGIC 0x534D4354u
/* The app derives address from its existing factory-trim address routine.
 * This extension is only read after rejecting the normal ABI2 boot contract. */
typedef struct {
    xb_boot_t base;
    uint32_t ext_magic, ext_size;
    uint8_t address[6], reserved[2];
    volatile uint32_t radio_desired; /* app writes0/1; priority over queued feedback */
    /* App initializes only while both rings are cleared and controller stopped.
     * Controller is the sole runtime writer. Preserve across controller restart.
     * Zero is invalid; UINT32_MAX is the last epoch, never wrap/reuse it. */
    volatile uint32_t feedback_epoch;
} xsm_boot_t;
_Static_assert(offsetof(xsm_boot_t, feedback_epoch) == sizeof(xb_boot_t) + 20u,
               "ABI5 epoch is a named extension, never reserved-byte reuse");
static inline uint32_t xsm_epoch_read(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
static inline void xsm_epoch_write(uint8_t *p, uint32_t epoch)
{
    for (unsigned i = 0; i < 4u; ++i) p[i] = (uint8_t)(epoch >> (8u * i));
}
/* Pass a snapshot taken BEFORE inspecting app link flags. A delayed producer
 * retains the old identity even if a new link arrives before it publishes. */
static inline int xsm_feedback_put(xb_ring_t *ring, uint32_t epoch,
                                   const uint8_t *midi, uint32_t length)
{
    uint8_t frame[7];
    if (!epoch || !midi || !length || length > 3u) return -1;
    xsm_epoch_write(frame, epoch);
    for (unsigned i = 0; i < length; ++i) frame[4u + i] = midi[i];
    return xb_frame_put(ring, XSM_MIDI_SESSION, frame, length + 4u);
}
