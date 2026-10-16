/* SPDX-License-Identifier: GPL-3.0-only
 * Map the reported APC40 mkII clip grid onto the SMK-37's 8x2 pad grid. */
#pragma once
#include <stdint.h>

#define SMK_APC40_CLIP_COUNT 40u
#define SMK_APC40_PAD_COUNT 16u
#define SMK_APC40_PAD_ROW_SIZE 8u
#define SMK_APC40_SOURCE_COLS 8u
#define SMK_APC40_SOURCE_ROWS 5u
#define SMK_APC40_INVALID_NOTE 0xFFu
#define SMK_APC40_PLAY_NOTE 0x5Bu
#define SMK_APC40_RECORD_NOTE 0x5Du
enum { SMK_APC40_BUTTON_IGNORED, SMK_APC40_BUTTON_CONSUMED,
       SMK_APC40_BUTTON_ON, SMK_APC40_BUTTON_OFF };

/* The APC40 transport sends momentary Play and Record button notes. */
static inline uint8_t smk_apc40_transport_note(uint8_t is_record)
{
    return is_record ? SMK_APC40_RECORD_NOTE : SMK_APC40_PLAY_NOTE;
}

/* Route only presses that start in clip mode. Always consume their matching
 * release, even if clip mode exits before the user releases the button. */
static inline uint8_t smk_apc40_transport_event(uint8_t *sent, uint8_t enabled, uint8_t pressed)
{
    if (pressed) {
        if (!enabled)
            return SMK_APC40_BUTTON_IGNORED;
        if (*sent)
            return SMK_APC40_BUTTON_CONSUMED;
        *sent = 1u;
        return SMK_APC40_BUTTON_ON;
    }
    if (!*sent)
        return SMK_APC40_BUTTON_IGNORED;
    *sent = 0u;
    return SMK_APC40_BUTTON_OFF;
}

/* APC clip RGB LEDs use the note's MIDI channel to select static, one-shot,
 * pulsing, or blinking display modes. Velocity zero means off. */
static inline uint8_t smk_apc40_feedback_set(volatile uint8_t velocity_state[SMK_APC40_CLIP_COUNT],
                                            volatile uint8_t channel_state[SMK_APC40_CLIP_COUNT],
                                            uint8_t note, uint8_t channel, uint8_t velocity)
{
    if (note >= SMK_APC40_CLIP_COUNT)
        return 0u;
    velocity_state[note] = velocity & 0x7Fu;
    channel_state[note] = channel & 0x0Fu;
    return 1u;
}

/* APC animation rates are subdivisions of a beat expressed in 24-PPQN MIDI
 * clock ticks. The same divisions are used for pulsing and blinking. */
static inline uint8_t smk_apc40_animation_clock_period(uint8_t channel)
{
    /* APC40 mkII protocol: 1/24, 1/16, 1/8, 1/4, 1/2 note = 4, 6, 12, 24, 48 ticks. */
    static const uint8_t periods[5] = {4u, 6u, 12u, 24u, 48u};
    if (channel < 6u || channel > 15u)
        return 0u;
    return periods[(channel - 6u) % 5u];
}

/* APC channels 11-15 are the blinking states. Loopy uses them for queued clips.
 * Pulsing (6-10) and one-shot (1-5) states draw steady, so playing clips do not blink. */
static inline uint8_t smk_apc40_is_queued(uint8_t channel)
{
    return (uint8_t)(channel >= 11u && channel <= 15u);
}

/* Queued clips blink once per beat: lit for the first half of each 24-tick beat. */
#define SMK_APC40_QUEUED_BLINK_TICKS 24u

/* Return a visual brightness for one 0..255 phase of an animation cycle.
 * APC pulsing/blinking states are rendered as a smooth breath on the SMK.
 * Static occupied cells receive a subtler beat pulse when MIDI Clock is live. */
static inline uint8_t smk_apc40_brightness(uint8_t channel, uint8_t phase,
                                          uint8_t beat_pulse)
{
    /* Pending clips (APC pulse and blink channels 6-15) blink on and off in sync
     * with the clock. Static cells stay steady, so other pads do not flicker. */
    (void)beat_pulse;
    if (smk_apc40_is_queued(channel))
        return phase < 128u ? 255u : 0u;
    return 255u;
}

/* Cell model. The last color received for a cell is the truth: velocity 0 or
 * note off clears the cell at once.
 * Exception: Loopy flashes a queued clip itself (color, then 0, about every 100 ms
 * on channel 0, captured 2026-10-15). After two off-to-on toggles within
 * SMK_APC40_TOGGLE_TICKS each, the cell is queued: it keeps its color, ignores the
 * flash, and blinks locally once per beat. A Loopy refresh (all off, then all
 * colors) gives only one toggle, so it does not make cells queued.
 * Time is in 16 ms ticks (fm1_ms >> 4), 8 bits per cell. */
#define SMK_APC40_CELL_OFF 0x01u        /* last message was off */
#define SMK_APC40_CELL_QUEUED 0x02u
#define SMK_APC40_CELL_TOGGLE_MASK 0x0Cu
#define SMK_APC40_CELL_TOGGLE_ONE 0x04u
#define SMK_APC40_TOGGLE_TICKS 16u      /* 256 ms: off-to-on gap of a flash */
#define SMK_APC40_QUEUED_END_TICKS 24u  /* 384 ms without a flash ends the queued state */

/* Apply one clip feedback message. Return 1 when the stored color changed. */
static inline uint8_t smk_apc40_cell_event(volatile uint8_t velocity[SMK_APC40_CLIP_COUNT],
                                           volatile uint8_t channel[SMK_APC40_CLIP_COUNT],
                                           volatile uint8_t flags[SMK_APC40_CLIP_COUNT],
                                           volatile uint8_t tick[SMK_APC40_CLIP_COUNT],
                                           uint8_t note, uint8_t ch, uint8_t vel, uint8_t now)
{
    uint8_t f, toggles;
    if (note >= SMK_APC40_CLIP_COUNT)
        return 0u;
    vel &= 0x7Fu;
    f = flags[note];
    if (!vel) {
        /* An off less than SMK_APC40_TOGGLE_TICKS after an on can be the first flash of a
         * queued clip. Keep the color: smk_apc40_cells_age clears it if no on follows. */
        uint8_t hold = (uint8_t)(!(f & SMK_APC40_CELL_OFF) &&
                                 (uint8_t)(now - tick[note]) < SMK_APC40_TOGGLE_TICKS);
        f |= SMK_APC40_CELL_OFF;
        flags[note] = f;
        tick[note] = now;
        if ((f & SMK_APC40_CELL_QUEUED) || hold)
            return 0u;                      /* flash off phase: keep the color */
        if (!velocity[note])
            return 0u;
        velocity[note] = 0u;
        channel[note] = 0u;
        return 1u;
    }
    toggles = (uint8_t)(f & SMK_APC40_CELL_TOGGLE_MASK);
    if ((f & SMK_APC40_CELL_OFF) && (uint8_t)(now - tick[note]) < SMK_APC40_TOGGLE_TICKS) {
        if (toggles != SMK_APC40_CELL_TOGGLE_MASK)
            toggles = (uint8_t)(toggles + SMK_APC40_CELL_TOGGLE_ONE);
    } else if (!(f & SMK_APC40_CELL_QUEUED)) {
        toggles = 0u;
    }
    f = (uint8_t)((f & SMK_APC40_CELL_QUEUED) | toggles);
    if (toggles >= 2u * SMK_APC40_CELL_TOGGLE_ONE)
        f |= SMK_APC40_CELL_QUEUED;
    flags[note] = f;
    tick[note] = now;
    if (velocity[note] == vel && channel[note] == (uint8_t)(ch & 0x0Fu))
        return 0u;
    velocity[note] = vel;
    channel[note] = (uint8_t)(ch & 0x0Fu);
    return 1u;
}

/* End the queued state when the flash stops. If the last message was off, the cell
 * clears; else it stays on. Return 1 when a stored color changed. */
static inline uint8_t smk_apc40_cells_age(volatile uint8_t velocity[SMK_APC40_CLIP_COUNT],
                                          volatile uint8_t channel[SMK_APC40_CLIP_COUNT],
                                          volatile uint8_t flags[SMK_APC40_CLIP_COUNT],
                                          volatile uint8_t tick[SMK_APC40_CLIP_COUNT],
                                          uint8_t now)
{
    uint8_t note, changed = 0u;
    for (note = 0u; note < SMK_APC40_CLIP_COUNT; note++) {
        uint8_t f = flags[note];
        if (!(f & SMK_APC40_CELL_QUEUED)) {
            /* A held off (smk_apc40_cell_event) with no on after it: clear the cell now. */
            if ((f & SMK_APC40_CELL_OFF) && velocity[note] &&
                (uint8_t)(now - tick[note]) >= SMK_APC40_TOGGLE_TICKS) {
                velocity[note] = 0u;
                channel[note] = 0u;
                changed = 1u;
            }
            continue;
        }
        if ((uint8_t)(now - tick[note]) < SMK_APC40_QUEUED_END_TICKS)
            continue;
        if (f & SMK_APC40_CELL_OFF) {
            velocity[note] = 0u;
            channel[note] = 0u;
            changed = 1u;
        }
        flags[note] = (uint8_t)(f & SMK_APC40_CELL_OFF);
    }
    return changed;
}

/* Bluetooth link state for the clip feedback. Bit 0: the link is up. Bit 1: notify is on.
 * Return 1 when a bit went from 1 to 0: the link ended, or the central turned notify off
 * (CCCD write 0). Then Loopy is gone and its feedback is not true any more. */
static inline uint8_t smk_apc40_link_lost(uint8_t *prev, uint8_t connected, uint8_t notifying)
{
    uint8_t now = (uint8_t)((connected ? 1u : 0u) | (notifying ? 2u : 0u));
    uint8_t lost = (uint8_t)((*prev & (uint8_t)~now) != 0u);
    *prev = now;
    return lost;
}

/* a76: 1 = a Bluetooth note-on with a colour (velocity 1-127, a USB-MIDI event packet) for a clip cell
 * (notes 0-39, the pad colours) while notify is off (Loopy does not listen). The cell does not change.
 * CLP-18: an off (note-off, or note-on velocity 0, any channel) always goes to the cell: an off cannot
 * show an old colour, and Loopy can send its offs when it closes. */
static inline uint8_t smk_apc40_ble_colour_unheard(uint32_t pkt, uint8_t notifying)
{
    return (uint8_t)(!notifying && ((pkt >> 8) & 0xF0u) == 0x90u && ((pkt >> 24) & 0x7Fu) &&
                     ((pkt >> 16) & 0x7Fu) < SMK_APC40_CLIP_COUNT);
}

/* a76: the clear runs again SMK_APC40_RECLEAR_MS after a link loss, to remove a late write
 * (a write that was on its way, or USB), if Loopy did not come back (link up and notify on). */
#define SMK_APC40_RECLEAR_MS 300u
typedef struct { uint8_t link, armed; uint32_t due; } smk_apc40_lost_t;
/* 1 = clear the clip feedback now */
static inline uint8_t smk_apc40_lost_step(smk_apc40_lost_t *s, uint8_t connected, uint8_t notifying, uint32_t ms)
{
    if (smk_apc40_link_lost(&s->link, connected, notifying)) {
        s->armed = 1u;
        s->due = ms + SMK_APC40_RECLEAR_MS;
        return 1u;
    }
    if (!s->armed || (int32_t)(ms - s->due) < 0)
        return 0u;
    s->armed = 0u;
    return (uint8_t)(s->link != 3u);
}

/* Clear the clip cells that have their bit set in mask (bit n = cell n): no colour, no queued state. */
static inline void smk_apc40_cells_clear_mask(volatile uint8_t velocity[SMK_APC40_CLIP_COUNT],
                                              volatile uint8_t channel[SMK_APC40_CLIP_COUNT],
                                              volatile uint8_t flags[SMK_APC40_CLIP_COUNT],
                                              volatile uint8_t tick[SMK_APC40_CLIP_COUNT], uint64_t mask)
{
    uint8_t note;
    for (note = 0u; note < SMK_APC40_CLIP_COUNT; note++, mask >>= 1)
        if (mask & 1u)
            velocity[note] = channel[note] = flags[note] = tick[note] = 0u;
}
/* Clear all clip cells: no colour, no queued state. */
static inline void smk_apc40_cells_clear(volatile uint8_t velocity[SMK_APC40_CLIP_COUNT],
                                         volatile uint8_t channel[SMK_APC40_CLIP_COUNT],
                                         volatile uint8_t flags[SMK_APC40_CLIP_COUNT],
                                         volatile uint8_t tick[SMK_APC40_CLIP_COUNT])
{
    smk_apc40_cells_clear_mask(velocity, channel, flags, tick, ~(uint64_t)0u);
}

/* Beat phase 0..255 from the 24 PPQN clock count: one cycle per quarter note. */
static inline uint8_t smk_apc40_beat_phase(uint32_t clock_count)
{
    return (uint8_t)(((clock_count % 24u) * 256u) / 24u);
}

/* Page-change snapshot detection. Loopy sends the colors of all clips of the new
 * page in one fast burst, but it does not send "off" for cells of the old page.
 * A burst that touches many cells is a full snapshot: cells it did not touch are
 * cleared. Small bursts (cue, play, record of some clips) change only their cells. */
#define SMK_APC40_BURST_GAP_MS 40u
#define SMK_APC40_SNAPSHOT_MIN_CELLS 8u

static inline void smk_apc40_burst_touch(uint64_t *mask, uint8_t *count, uint8_t note)
{
    uint64_t bit;
    if (note >= SMK_APC40_CLIP_COUNT)
        return;
    bit = (uint64_t)1u << note;
    if (!(*mask & bit)) {
        *mask |= bit;
        (*count)++;
    }
}

static inline uint8_t smk_apc40_bit_count(uint8_t bits)
{
    uint8_t n = 0u;
    for (; bits; bits &= (uint8_t)(bits - 1u))
        n++;
    return n;
}

/* Close a burst. Return 1 when it was a page snapshot and stale cells were cleared.
 * A burst is a snapshot only when all three are true:
 *  - it touches 8 or more cells,
 *  - no touched cell is in a cue (blink/pulse, channel 6-15) state,
 *  - it touches at least half of all lit cells.
 * A scene launch or a cue change of some clips then keeps the other clips. */
static inline uint8_t smk_apc40_burst_finish(volatile uint8_t velocity[SMK_APC40_CLIP_COUNT],
                                             volatile uint8_t channel[SMK_APC40_CLIP_COUNT],
                                             uint64_t *mask, uint8_t *count)
{
    uint8_t note, lit = 0u, cue = 0u, snapshot, rows = 0u, cols = 0u;
    for (note = 0u; note < SMK_APC40_CLIP_COUNT; note++) {
        uint8_t touched = (uint8_t)((*mask >> note) & 1u);
        if (velocity[note])
            lit++;
        if (touched && velocity[note] && channel[note] >= 6u)
            cue = 1u;
        if (touched) {
            rows |= (uint8_t)(1u << (note / SMK_APC40_SOURCE_COLS));
            cols |= (uint8_t)(1u << (note % SMK_APC40_SOURCE_COLS));
        }
    }
    /* A page snapshot covers 3 or more rows and 3 or more columns. A scene or
     * play-group launch (one or two rows or columns) can then never clear clips. */
    snapshot = (uint8_t)(*count >= SMK_APC40_SNAPSHOT_MIN_CELLS && !cue &&
                         (uint16_t)*count * 2u >= lit &&
                         smk_apc40_bit_count(rows) >= 3u && smk_apc40_bit_count(cols) >= 3u);
    if (snapshot)
        for (note = 0u; note < SMK_APC40_CLIP_COUNT; note++)
            if (!(*mask & ((uint64_t)1u << note))) {
                velocity[note] = 0u;
                channel[note] = 0u;
            }
    *mask = 0u;
    *count = 0u;
    return snapshot;
}

/* Infer the reported clip footprint. APC feedback does not distinguish an
 * empty cell from a cell outside the project, so use the colored-cell bounds.
 * Keep the first occupied APC row/column as an origin: this removes unused
 * viewport margins without collapsing holes inside the reported footprint. */
static inline void smk_apc40_build_layout(const uint8_t velocity[SMK_APC40_CLIP_COUNT],
                                          uint8_t *columns, uint8_t *rows,
                                          uint8_t *origin_col, uint8_t *origin_row,
                                          uint8_t *has_clips)
{
    uint8_t note, min_col = 0xFFu, min_row = 0xFFu, max_col = 0u, max_row = 0u, found = 0u;
    for (note = 0u; note < SMK_APC40_CLIP_COUNT; note++) {
        uint8_t col, row;
        if (velocity[note] == 0u)
            continue;
        col = (uint8_t)(note % SMK_APC40_SOURCE_COLS);
        row = (uint8_t)(note / SMK_APC40_SOURCE_COLS);
        if (!found || col < min_col)
            min_col = col;
        if (!found || row < min_row)
            min_row = row;
        if (!found || col > max_col)
            max_col = col;
        if (!found || row > max_row)
            max_row = row;
        found = 1u;
    }
    *has_clips = found;
    if (!found) {
        *columns = SMK_APC40_SOURCE_COLS;
        *rows = 2u;
        *origin_col = 0u;
        *origin_row = 0u;
        return;
    }
    *origin_col = min_col;
    *origin_row = min_row;
    /* A footprint of 3 or 4 rows and at most 4 columns folds onto one 8x2 view (0.9).
     * A 4x4 grid with a late or empty edge row or column still folds. Wider
     * footprints keep the 0.7 8-wide drawing.
     * An empty edge column of a 4x4 project can make the span narrower than 4. */
    *columns = (uint8_t)((unsigned)(max_col - min_col) <= 3u && (unsigned)(max_row - min_row) >= 2u &&
                                 (unsigned)(max_row - min_row) <= 3u
                             ? 4u : SMK_APC40_SOURCE_COLS);
    *rows = (uint8_t)(max_row - min_row + 1u);
}

/* Freeze the first non-empty layout snapshot for the current viewport. Playback
 * feedback can turn a clip LED off; it must not shrink the grid and shift pads. */
static inline uint8_t smk_apc40_latch_layout(const uint8_t velocity[SMK_APC40_CLIP_COUNT],
                                             uint8_t *columns, uint8_t *rows,
                                             uint8_t *origin_col, uint8_t *origin_row,
                                             volatile uint8_t *locked)
{
    uint8_t has_clips;
    if (*locked)
        return 0u;
    smk_apc40_build_layout(velocity, columns, rows, origin_col, origin_row, &has_clips);
    if (!has_clips)
        return 0u;
    *locked = 1u;
    return 1u;
}

/* Number of local 8x2 views for the latched footprint. A folded 4-column grid
 * uses one view. An 8-wide grid uses one view for each two rows (8x4 = 2). */
static inline uint8_t smk_apc40_view_count(uint8_t columns, uint8_t rows)
{
    if (columns <= 4u || rows <= 2u)
        return 1u;
    return (uint8_t)((rows + 1u) / 2u);
}

/* First footprint row of a local view. The last view is clamped so that it
 * never shows a black row below the footprint. */
static inline uint8_t smk_apc40_view_row_start(uint8_t columns, uint8_t rows, uint8_t view)
{
    uint8_t count = smk_apc40_view_count(columns, rows), start;
    if (count <= 1u)
        return 0u;
    if (view >= count)
        view = (uint8_t)(count - 1u);
    start = (uint8_t)(view * 2u);
    return (uint8_t)(start + 2u > rows ? rows - 2u : start);
}

/* Move the local view up (-1) or down (+1). The result is clamped to the footprint. */
static inline uint8_t smk_apc40_view_after_move(uint8_t columns, uint8_t rows,
                                                uint8_t view, int8_t direction)
{
    uint8_t count = smk_apc40_view_count(columns, rows);
    if (view >= count)
        view = (uint8_t)(count - 1u);
    if (direction < 0 && view > 0u)
        return (uint8_t)(view - 1u);
    if (direction > 0 && view + 1u < count)
        return (uint8_t)(view + 1u);
    return view;
}

/* A clip-mode press of PLAY or REC lights its LED at once. Loopy feedback can
 * then replace the local state. */
static inline uint8_t smk_apc40_transport_led_after_press(uint8_t state)
{
    return (uint8_t)!state;
}

/* PLAY LED in clip mode. Without live MIDI Clock the LED stays on. With live
 * clock it is on for the first quarter of each beat (6 of 24 PPQN ticks), as the
 * X0X sequencer blinks PLAY on every fourth step. */
static inline uint8_t smk_apc40_play_led(uint8_t playing, uint8_t clock_live, uint32_t clock_count)
{
    if (!playing)
        return 0u;
    if (!clock_live)
        return 1u;
    return (uint8_t)(clock_count % 24u < 6u);
}

static inline uint8_t smk_apc40_note_for_pad_view(uint8_t columns, uint8_t rows,
                                                  uint8_t origin_col, uint8_t origin_row,
                                                  uint8_t view, uint8_t pad);

/* LCD clip map (1.6): the area that the pads use (the latched layout), in source
 * cells. A 4-or-less wide layout uses its columns. An 8-wide layout uses the columns
 * from its origin to the right edge. With no clips, the full 8x5 grid. The cell pitch
 * is the largest square that fits w x h, so a smaller area gets bigger cells. */
typedef struct { uint8_t col0, row0, ncols, nrows, pitch; int16_t x0, y0; } smk_apc40_lcd_map_t;
static inline smk_apc40_lcd_map_t smk_apc40_lcd_map(uint8_t columns, uint8_t rows,
                                                   uint8_t origin_col, uint8_t origin_row,
                                                   uint8_t has_clips, uint16_t w, uint16_t h)
{
    smk_apc40_lcd_map_t m;
    uint16_t px, py;
    if (!has_clips || !columns || !rows || origin_col >= SMK_APC40_SOURCE_COLS ||
        origin_row >= SMK_APC40_SOURCE_ROWS) {
        m.col0 = 0u; m.row0 = 0u;
        m.ncols = SMK_APC40_SOURCE_COLS; m.nrows = SMK_APC40_SOURCE_ROWS;
    } else {
        m.col0 = origin_col; m.row0 = origin_row;
        m.ncols = columns <= 4u ? columns : (uint8_t)(SMK_APC40_SOURCE_COLS - origin_col);
        m.nrows = rows;
        if ((uint8_t)(m.row0 + m.nrows) > SMK_APC40_SOURCE_ROWS)
            m.nrows = (uint8_t)(SMK_APC40_SOURCE_ROWS - m.row0);
    }
    px = (uint16_t)((w - 8u) / m.ncols);
    py = (uint16_t)((h - 4u) / m.nrows);
    m.pitch = (uint8_t)(px < py ? px : py);
    m.x0 = (int16_t)((w - (uint16_t)m.pitch * m.ncols) / 2u);
    m.y0 = (int16_t)((h - (uint16_t)m.pitch * m.nrows) / 2u);
    return m;
}

/* Screen row of a source (APC) row: the highest APC row is the top Loopy row, so it
 * is drawn on top. This is the same for 4x4 and 8-wide layouts. */
static inline uint8_t smk_apc40_lcd_screen_row(const smk_apc40_lcd_map_t *m, uint8_t source_row)
{
    return (uint8_t)(m->nrows - 1u - (uint8_t)(source_row - m->row0));
}

/* Loopy Undo / Redo (1.8). In clip mode, PATCH (X0X HOME) + SEQ REC sends note 0x70
 * and PATCH + SEQ PLAY sends note 0x71, on MIDI channel 16. These notes are outside the
 * APC40 mkII protocol, so the Loopy APC40 auto-map does not use them. The user binds
 * them to Undo and Redo in Loopy's Control Settings. */
#define SMK_APC40_UNDO_NOTE 0x70u
#define SMK_APC40_REDO_NOTE 0x71u
#define SMK_APC40_HISTORY_CHANNEL 0x0Fu
/* USB-MIDI packet: note on (velocity 127) on press, note off (velocity 0) on release. */
static inline uint32_t smk_apc40_history_packet(uint8_t redo, uint8_t on)
{
    uint32_t status = (on ? 0x90u : 0x80u) | SMK_APC40_HISTORY_CHANNEL;
    uint32_t note = redo ? SMK_APC40_REDO_NOTE : SMK_APC40_UNDO_NOTE;
    return (on ? 0x09u : 0x08u) | status << 8 | note << 16 | (uint32_t)(on ? 0x7Fu : 0x00u) << 24;
}

/* Outside clip mode, PLAY also sends note 0x5B on MIDI channel 16, the channel of the Undo / Redo notes
 * (MID-14), for a MIDI-learn in Loopy Pro. X0X starts and stops as before. In clip mode, PLAY is the APC
 * PLAY (smk_apc40_transport_event), so this sends nothing. HOME + PLAY (X0X redo) sends nothing.
 * The release sends note off only after a press that sent note on, also if clip mode started between. */
#define SMK_APC40_PLAY_CTL_NOTE 0x5Bu
static inline uint8_t smk_apc40_play_note_event(uint8_t *sent, uint8_t clip_mode, uint8_t home_held, uint8_t pressed)
{
    return smk_apc40_transport_event(sent, (uint8_t)(!clip_mode && !home_held), pressed);
}
/* USB-MIDI packet: note on 0x5B velocity 127, or note off 0x5B velocity 0, on channel 16. */
static inline uint32_t smk_apc40_play_note_packet(uint8_t on)
{
    uint32_t status = (on ? 0x90u : 0x80u) | SMK_APC40_HISTORY_CHANNEL;
    return (on ? 0x09u : 0x08u) | status << 8 | SMK_APC40_PLAY_CTL_NOTE << 16 | (uint32_t)(on ? 0x7Fu : 0x00u) << 24;
}

/* Return 1 when the current footprint fully contains the latched one and differs.
 * The latched layout can then grow (a late or partial feedback burst), but it never
 * shrinks when a clip LED turns off, so cue changes cannot shift pads. */
static inline uint8_t smk_apc40_layout_grows(uint8_t old_cols, uint8_t old_rows,
                                             uint8_t old_col, uint8_t old_row,
                                             uint8_t cols, uint8_t rows,
                                             uint8_t col, uint8_t row)
{
    uint8_t old_w = old_cols <= 4u ? 4u : (uint8_t)(SMK_APC40_SOURCE_COLS - old_col);
    uint8_t new_w = cols <= 4u ? 4u : (uint8_t)(SMK_APC40_SOURCE_COLS - col);
    if (cols == old_cols && rows == old_rows && col == old_col && row == old_row)
        return 0u;
    if (cols > 4u)   /* growth only completes a 4x4 fold; 8-wide layouts stay as latched (0.7) */
        return 0u;
    if (col > old_col || row > old_row)
        return 0u;
    if ((uint8_t)(row + rows) < (uint8_t)(old_row + old_rows))
        return 0u;
    if ((uint8_t)(col + new_w) < (uint8_t)(old_col + old_w) && cols > 4u)
        return 0u;
    return 1u;
}

/* Return the APC note for a logical SMK pad. This keeps empty cells in place;
 * the caller checks feedback before it launches a note. */
static inline uint8_t smk_apc40_note_for_pad(uint8_t columns, uint8_t rows,
                                             uint8_t origin_col, uint8_t origin_row,
                                             uint8_t pad)
{
    return smk_apc40_note_for_pad_view(columns, rows, origin_col, origin_row, 0u, pad);
}

static inline uint8_t smk_apc40_note_for_pad_view(uint8_t columns, uint8_t rows,
                                                  uint8_t origin_col, uint8_t origin_row,
                                                  uint8_t view, uint8_t pad)
{
    uint8_t source_col, source_row, row_in_view;
    if (pad >= SMK_APC40_PAD_COUNT || columns == 0u || columns > SMK_APC40_SOURCE_COLS ||
        rows == 0u)
        return SMK_APC40_INVALID_NOTE;
    if (columns <= 4u) {
        uint8_t block_col = (uint8_t)(pad % SMK_APC40_PAD_ROW_SIZE);
        source_col = (uint8_t)(origin_col + block_col % 4u);
        row_in_view = (uint8_t)(pad / SMK_APC40_PAD_ROW_SIZE + (block_col >= 4u ? 0u : 2u));
    } else {
        source_col = (uint8_t)(origin_col + pad % SMK_APC40_PAD_ROW_SIZE);
        row_in_view = (uint8_t)(pad / SMK_APC40_PAD_ROW_SIZE +
                                smk_apc40_view_row_start(columns, rows, view));
    }
    source_row = (uint8_t)(origin_row + row_in_view);
    if (source_col >= SMK_APC40_SOURCE_COLS || source_row >= SMK_APC40_SOURCE_ROWS || row_in_view >= rows)
        return SMK_APC40_INVALID_NOTE;
    return (uint8_t)(source_row * SMK_APC40_SOURCE_COLS + source_col);
}

static inline uint8_t smk_apc40_transport_feedback_note(uint8_t note)
{
    return (uint8_t)(note == SMK_APC40_PLAY_NOTE || note == SMK_APC40_RECORD_NOTE);
}

static inline uint8_t smk_apc40_transport_feedback_on(uint8_t velocity)
{
    return (uint8_t)(velocity != 0u);
}

/* Permit movement toward the starting viewport at all times. Permit outward
 * movement only while the current reported viewport has at least one clip. */
static inline uint8_t smk_apc40_can_scroll(int16_t offset, int8_t direction,
                                           uint8_t has_clips)
{
    if ((offset > 0 && direction < 0) || (offset < 0 && direction > 0))
        return 1u;
    return has_clips;
}

static inline int16_t smk_apc40_scroll_after_move(int16_t offset, int8_t direction)
{
    if (direction > 0 && offset < 127)
        return (int16_t)(offset + 1);
    if (direction < 0 && offset > -127)
        return (int16_t)(offset - 1);
    return offset;
}

/* The SMK's scan indices start at its upper physical row. Loopy row zero is shown below it. */
static inline uint8_t smk_apc40_logical_pad_for_physical(uint8_t physical_pad)
{
    return physical_pad < SMK_APC40_PAD_COUNT ? (uint8_t)(physical_pad ^ SMK_APC40_PAD_ROW_SIZE)
                                              : SMK_APC40_INVALID_NOTE;
}

static inline uint8_t smk_apc40_physical_pad_for_logical(uint8_t logical_pad)
{
    return logical_pad < SMK_APC40_PAD_COUNT ? (uint8_t)(logical_pad ^ SMK_APC40_PAD_ROW_SIZE)
                                             : SMK_APC40_INVALID_NOTE;
}
