/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef DNBOX_BOX_ENTRY_SCHEDULER_H
#define DNBOX_BOX_ENTRY_SCHEDULER_H

#include <stdint.h>

/* U3 source-only integration contract for the existing 36-frame v13r2 cut.
 * Not linked or called by production firmware until a separately qualified
 * small frame payload exists and the current HOME renderer owner approves it.
 *
 * The 90,324-byte H.264 preview is NOT a firmware frame payload. No decoder,
 * timer, audio hook, framebuffer, storage write or blocking delay lives here.
 * The caller must initialize/render the actual HOME first. The caller only
 * invokes this scheduler on the ordinary foreground UI tick, with a monotonic
 * wrapping millisecond clock, and paints frames only if independently safe.
 */
#define DNBOX_ENTRY_FRAMES       36u
#define DNBOX_ENTRY_FPS          70u
#define DNBOX_ENTRY_END_MS      515u /* ceil(36 * 1000 / 70) */
#define DNBOX_ENTRY_NONE          0u
#define DNBOX_ENTRY_DRAW          1u
#define DNBOX_ENTRY_RESTORE_HOME  2u
#define DNBOX_ENTRY_UNDRAWN     255u

typedef struct dnbox_entry_scheduler {
    uint32_t start_ms;
    uint8_t active;
    uint8_t last_frame;
} dnbox_entry_scheduler_t;

/* assets_verified must be 0 until a genuinely verified, budget-qualified
 * native 240x240-frame payload and a HOME endpoint matching the runtime exist.
 * Reduced-motion and unavailable-asset starts leave the live HOME untouched.
 */
static inline void dnbox_entry_begin(dnbox_entry_scheduler_t *s,
                                     uint32_t now_ms,
                                     uint8_t assets_verified,
                                     uint8_t reduced_motion)
{
    if (!s)
        return;
    s->start_ms = now_ms;
    s->active = (uint8_t)(!!assets_verified && !reduced_motion);
    s->last_frame = DNBOX_ENTRY_UNDRAWN;
}

/* Use on foreground UI ticks only, never in audio/DMA/IRQ context.
 * At most one DRAW is returned per call, and missing frames are dropped rather
 * than caught up with a rendering loop. Any input / reduced-motion switch /
 * asset withdrawal returns RESTORE_HOME immediately, exactly once. The HOME
 * should already be live beneath the nonblocking visual layer.
 *
 * The caller must NOT delay sound initialization, scanning, transport, MIDI,
 * or incoming input to wait for any DRAW. Frame drawing costs require separate
 * exact hardware proof; this scheduler alone cannot qualify those costs.
 */
static inline uint8_t dnbox_entry_poll(dnbox_entry_scheduler_t *s,
                                       uint32_t now_ms,
                                       uint8_t user_input,
                                       uint8_t reduced_motion,
                                       uint8_t assets_still_verified,
                                       uint8_t *frame_out)
{
    uint32_t elapsed;
    uint8_t frame;
    if (!s || !s->active)
        return DNBOX_ENTRY_NONE;

    elapsed = (uint32_t)(now_ms - s->start_ms);
    if (user_input || reduced_motion || !assets_still_verified ||
        elapsed >= DNBOX_ENTRY_END_MS) {
        s->active = 0;
        return DNBOX_ENTRY_RESTORE_HOME;
    }

    /* 70 fps maps exactly to 7 frames per 100ms, without float/64-bit math.
     * Multiplication is bounded by 514*7 and cannot overflow uint32_t.
     */
    frame = (uint8_t)((elapsed * 7u) / 100u);
    if (frame >= DNBOX_ENTRY_FRAMES) { /* defensive: never index past asset */
        s->active = 0;
        return DNBOX_ENTRY_RESTORE_HOME;
    }
    if (frame == s->last_frame)
        return DNBOX_ENTRY_NONE;
    s->last_frame = frame;
    if (frame_out)
        *frame_out = frame;
    return DNBOX_ENTRY_DRAW;
}

#endif /* DNBOX_BOX_ENTRY_SCHEDULER_H */
