/* SPDX-License-Identifier: GPL-3.0-only
 * Boot, audio ISR and main loop adapted from Felucca's main.c / audio.c, Copyright (C) 2026
 * Leo Kuroshita (@kurogedelic), Hugelton Instruments. */
/* X0X on the FM-1: boot (WDT first, boot-loop guard, guards), LCD, input (TIMER5 10 kHz),
 * audio (ALNK0), USB; then the main loop: UI at ~60 frames/s, input polled in between. */
extern uint32_t _data_start[], _data_end[], _data_load[], _bss_start[], _bss_end[];
extern uint32_t _pool_start[], _pool_end[], _rt_start[], _rt_end[], _rt_load[], _c1_start[], _c1_end[], _c1_load[];

/* --------------------------------------------------------------- audio --- */
/* X0X: 512-frame halves (11.6 ms; Felucca 256). The render runs inside the DMA's deadline, and a
 * busy pattern averaging ~80 % ran over it about one half in 300 (a heavy step): a dropped half.
 * Twice the half averages a heavy step over twice the time, for one more half of latency. */
#define HALF_FRAMES 512u                    /* I2S half buffer: 11.6 ms at 44.1 kHz */
#define HALF_WORDS (HALF_FRAMES * 2u)
#define REND_FRAMES 256u                    /* engine_render's most per call */
#if FELUCCA_UAC
_Static_assert(REND_FRAMES == UA_HALF, "usb.c sizes the USB audio ring band for this block");
#endif
static int32_t abuf[2u * HALF_WORDS] __attribute__((aligned(4)));

#define DBG_MAGIC 0x44424731u                       /* "DBG1": read with `fm1t memr` */
struct x0x_dbg {
    uint32_t magic, halves, max_us, nested, in_audio, late, timer_irqs, ui_frames;
    uint32_t last_us, cpu_q8, boots, stage;
    uint32_t crash_seen;                            /* the crash count already reported at a boot */
} x0x_dbg __attribute__((section(".noinit")));
static uint8_t safe_mode;
#if X0X_DEBUG
/* development: the slowest block since boot, taken apart (read over USB: tools/fm1_debug.py) */
extern volatile uint32_t eng_dbg_wait, eng_dbg_nsub;
static volatile uint32_t dbg_tmr;                   /* 24 MHz ticks of TIMER5 nested in the render */
struct x0x_worst_s {
    uint32_t magic, us, wait, tmr, nsub, half_no, over90, over100, blocks, late_in;
} x0x_worst;
#endif                           /* two failed boots: no audio, USB on (safe_main) */

static void render_block(int32_t *o)                /* ALNK0: one half, measured */
{
    uint32_t t0 = fm1_ticks(), us, budget = HALF_FRAMES * 1000000u / FS;
    x0x_dbg.in_audio = 1;
#if FELUCCA_UAC
    uac_render_start();
#endif
    {
        uint32_t k;
        for (k = 0; k < HALF_FRAMES; k += REND_FRAMES) {
            engine_render(o + 2u * k, REND_FRAMES);
#if FELUCCA_UAC
            uac_tap(o + 2u * k, REND_FRAMES);       /* the USB audio input: the same master output */
#endif
            engine_volume(o + 2u * k, REND_FRAMES);     /* USB AUDIO FULL: the MASTER knob after the tap */
        }
    }
    us = (fm1_ticks() - t0) / FM1_TICKS_PER_US;
#if X0X_DEBUG
    x0x_worst.magic = 0x57525354u;                  /* "WRST" */
    x0x_worst.blocks++;
    if (us * 100u > budget * 90u)
        x0x_worst.over90++;
    if (us > budget)
        x0x_worst.over100++;
    if (us > x0x_worst.us) {
        x0x_worst.us = us;
        x0x_worst.wait = eng_dbg_wait;
        x0x_worst.tmr = dbg_tmr;
        x0x_worst.nsub = eng_dbg_nsub;
        x0x_worst.half_no = x0x_dbg.halves;
    }
    eng_dbg_wait = eng_dbg_nsub = 0;
    dbg_tmr = 0;
#endif
    x0x_dbg.cpu_q8 = (x0x_dbg.cpu_q8 * 15u + (us * 256u) / budget) / 16u;
    audio_cpu_pct = (x0x_dbg.cpu_q8 * 100u) >> 8;
    if (us * 100u / budget > audio_peak_pct)
        audio_peak_pct = us * 100u / budget;        /* the PERF screen's peak (read and reset) */
    engine_load(us * 100u / budget, HALF_FRAMES);
    x0x_dbg.halves++;
    x0x_dbg.last_us = us;
    if (us > x0x_dbg.max_us)
        x0x_dbg.max_us = us;
    x0x_dbg.in_audio = 0;
}

void fm1_alnk0_irq(void)                            /* via isr_alnk0 (hal/fm1_isr.S) */
{
    uint8_t p = fm1_audio_pending();
    fm1_audio_ack_aux(p);
    if (p & FM1_AUDIO_HALF) {
        uint32_t half = fm1_audio_free_half();
        render_block(&abuf[half * HALF_WORDS]);
        fm1_audio_ack_half();
        if (fm1_audio_free_half() != half) {
#if X0X_DEBUG
            x0x_worst.late_in++;
#endif
            x0x_dbg.late++;                         /* the DMA moved on while we rendered */
            audio_xruns++;
        }
    }
}
extern void isr_alnk0(void);

static void audio_init(void)
{
    uint32_t i;
    for (i = 0; i < 2u * HALF_WORDS; i++)
        abuf[i] = 0;
    fm1_audio_init(abuf, HALF_WORDS, isr_alnk0, 3);
}

static void audio_silence(void)                     /* IRQs off (flash erase): the DMA would loop stale audio */
{
    uint32_t i;
    for (i = 0; i < sizeof abuf / sizeof abuf[0]; i++)
        abuf[i] = 0;
}

/* ------------------------------------------------------------ timer --- */
static uint8_t usb_due;
void fm1_timer5_irq(void)
{
    static uint32_t sub;
#if X0X_DEBUG
    uint32_t tt0 = fm1_ticks();
#endif
    fm1_timer5_ack();
#if X0X_BLE
    fm1_t5w_begin();                                /* test 22: who changes this frame */
#endif
    x0x_dbg.timer_irqs++;
    if (x0x_dbg.in_audio)
        x0x_dbg.nested++;
    fm1_input_tick();
#if X0X_BLE
    fm1_t5w_check(1u, fm1_ms);
#endif
    /* 2 kHz: all USB SIE traffic lives here. This ISR now nests into the audio render (so the key
     * matrix keeps its rhythm, below); USB shares the MIDI queues with the audio ISR, so a poll that
     * falls inside the render waits for the first tick outside it, as it did before the nesting. */
    if (sub % 5u == 0u) {
        usb_due = 1;
#if FELUCCA_UAC
        /* the USB audio stream cannot wait for the render (one packet per 1 ms frame, a render takes
         * up to ~11.6 ms): it runs nested too. It touches only EP4 (INDEX is set on every access) and
         * the consumer side of its ring, and usb_poll never runs nested, so the two never interleave */
        if (x0x_dbg.in_audio)
            uac_service();
#endif
    }
    if (usb_due && !x0x_dbg.in_audio) {
        usb_due = 0;
        usb_poll();
#if X0X_TRS
        uart_midi_poll();                           /* the TRS jack: same context, same queue */
#endif
#if X0X_BLE
        fm1_t5w_check(2u, fm1_ms);
        ble_service();                              /* Bluetooth MIDI: same context, same queue */
        fm1_t5w_check(3u, fm1_ms);
#endif
    }
    if (++sub == 10u)
        sub = 0;
    {
        static uint32_t last, acc;
        uint32_t now = fm1_ticks();
        acc += now - last;
        last = now;
        while (acc >= 1000u * FM1_TICKS_PER_US) {
            acc -= 1000u * FM1_TICKS_PER_US;
            fm1_ms++;
        }
    }
#if X0X_BLE
    fm1_t5w_check(4u, fm1_ms);
    fm1_t5w_end(fm1_ms);
#endif
#if X0X_DEBUG
    if (x0x_dbg.in_audio)
        dbg_tmr += fm1_ticks() - tt0;
#endif
}
extern void isr_timer5(void);

#if BOARD_SMK37
#include "app/smk_apc40.h"
/* ------------------------------------------------------------ SMK-37 Pro --- */
/* What the SMK-37 Pro has beyond the FM-1's panel (smk_input.h), after the UI (ui.c) in this unity build.
 * The pads: the 16 white keys' lights (plat_fm1.c plat_leds) in the colour of the part on screen, lit half
 * bright, dim an eighth, else dark; sent between LCD transfers (lcd.c padled_flush). */
static volatile uint8_t clip_grid_mode;   /* APC40 mkII compatibility; long KNOB BANK toggles */
static volatile uint8_t apc_host_on;      /* attempt 35: an APC40 host did the handshake (smk_apc_host_seen) */
/* attempt 30: what clip mode sends to the APC40 host goes to USB and to a Bluetooth MIDI connection
 * (Loopy Pro on the BLE port). ble_out has one producer (the audio ISR): IRQs off around it here. */
static void apc_out(uint32_t pkt)
{
    midi_out_event(pkt);
#if X0X_BLE
    {
        uint32_t f = irq_save();
        ble_out(pkt);
        irq_restore(f);
    }
#endif
}
static void apc_note(uint32_t note, uint32_t vel)
{
    uint32_t on = vel != 0u;
    apc_out((on ? 0x09u : 0x08u) | (on ? 0x90u : 0x80u) << 8 | (note & 0x7Fu) << 16 | (on ? vel : 0x7Fu) << 24);
}
#include "app/smk_clip_keys.h"
static volatile uint8_t ck_start_req;   /* (set in an interrupt; smk_ck_poll clears it) */
void smk_ck_midi_stop(void);            /* all notes off: ck_t off_req (in the pool) */
static uint32_t knob_bank_hold_start;
static uint8_t knob_bank_toggled;
static volatile uint8_t apc_led_velocity[SMK_APC40_CLIP_COUNT];
static volatile uint8_t apc_cell_flags[SMK_APC40_CLIP_COUNT];   /* off pending, queued */
static volatile uint8_t apc_led_channel[SMK_APC40_CLIP_COUNT];
static volatile uint8_t apc_cell_tick[SMK_APC40_CLIP_COUNT];    /* 16 ms ticks */
static volatile uint8_t apc_play_feedback, apc_record_feedback;
static volatile uint32_t apc_clock_count, apc_clock_last_ms, apc_clock_period_ms;
static volatile uint8_t apc_clock_seen;
static uint8_t apc_cols = SMK_APC40_SOURCE_COLS;
static uint8_t apc_rows = 2u;
static uint8_t apc_origin_col, apc_origin_row;
static uint8_t apc_view_has_clips;
static volatile uint8_t apc_layout_locked, apc_layout_pending;
static volatile uint32_t apc_layout_deadline;
static uint8_t apc_view;   /* local 8x2 view of the latched footprint */
static uint64_t apc_burst_mask;   /* cells touched by the current feedback burst */
static uint8_t apc_burst_count;
static volatile uint32_t apc_burst_last_ms;
/* Close the current feedback burst. A page snapshot clears stale cells of the old
 * Loopy page and makes the layout latch again from the new page only. */
static void smk_apc_burst_close(void)
{
    if (!apc_burst_count)
        return;
    if (smk_apc40_burst_finish(apc_led_velocity, apc_led_channel, &apc_burst_mask,
                               &apc_burst_count)) {
        apc_layout_locked = 0u;
        apc_layout_pending = 1u;
        apc_layout_deadline = fm1_ms;
    }
}
#define SMK_APC40_LAYOUT_SETTLE_MS 250u
static const uint32_t APC40_RGB[128] = {
    0x000000,0x1E1E1E,0x7F7F7F,0xFFFFFF,0xFF4C4C,0xFF0000,0x590000,0x190000,
    0xFFBD6C,0xFF5400,0x591D00,0x271B00,0xFFFF4C,0xFFFF00,0x595900,0x191900,
    0x88FF4C,0x54FF00,0x1D5900,0x142B00,0x4CFF4C,0x00FF00,0x005900,0x001900,
    0x4CFF5E,0x00FF19,0x00590D,0x001902,0x4CFF88,0x00FF55,0x00591D,0x001F12,
    0x4CFFB7,0x00FF99,0x005935,0x001912,0x4CC3FF,0x00A9FF,0x004152,0x001019,
    0x4C88FF,0x0055FF,0x001D59,0x000819,0x4C4CFF,0x0000FF,0x000059,0x000019,
    0x874CFF,0x5400FF,0x190064,0x0F0030,0xFF4CFF,0xFF00FF,0x590059,0x190019,
    0xFF4C87,0xFF0054,0x59001D,0x220013,0xFF1500,0x993500,0x795100,0x436400,
    0x033900,0x005735,0x00547F,0x0000FF,0x00454F,0x2500CC,0x7F7F7F,0x202020,
    0xFF0000,0xBDFF2D,0xAFED06,0x64FF09,0x108B00,0x00FF87,0x00A9FF,0x002AFF,
    0x3F00FF,0x7A00FF,0xB21A7D,0x402100,0xFF4A00,0x88E106,0x72FF15,0x00FF00,
    0x3BFF26,0x59FF71,0x38FFCC,0x5B8AFF,0x3151C6,0x877FE9,0xD31DFF,0xFF005D,
    0xFF7F00,0xB9B000,0x90FF00,0x835D07,0x392B00,0x144C10,0x0D5038,0x15152A,
    0x16205A,0x693C1C,0xA8000A,0xDE513D,0xD86A1C,0xFFE126,0x9EE12F,0x67B50F,
    0x1E1E30,0xDCFF6B,0x80FFBD,0x9A99FF,0x8E66FF,0x404040,0x757575,0xE0FFFF,
    0xA00000,0x350000,0x1AD000,0x074200,0xB9B000,0x3F3100,0xB35F00,0x4B1502
};
void smk_apc_led_set(uint8_t note, uint8_t type, uint8_t velocity)
{
    if (note >= SMK_APC40_CLIP_COUNT)
        return;
    if (apc_burst_count && fm1_ms - apc_burst_last_ms > SMK_APC40_BURST_GAP_MS)
        smk_apc_burst_close();
    apc_burst_last_ms = fm1_ms;
    smk_apc40_burst_touch(&apc_burst_mask, &apc_burst_count, note);
    if (smk_apc40_cell_event(apc_led_velocity, apc_led_channel, apc_cell_flags, apc_cell_tick,
                             note, (uint8_t)(type & 0x0Fu), velocity, (uint8_t)(fm1_ms >> 4)) &&
        !apc_layout_locked && !apc_layout_pending) {
        /* Let the initial multi-cell feedback burst settle before we infer
         * dimensions. Cue-state updates cannot resize a latched viewport. */
        apc_layout_pending = 1u;
        apc_layout_deadline = fm1_ms + SMK_APC40_LAYOUT_SETTLE_MS;
    }
}
/* Called from the USB receive interrupt for each MIDI event. APC clip and transport
 * feedback is applied here, before the 64-event input queue. A Loopy refresh is
 * about 240 events in one burst; queue overflow dropped some of them, and old
 * colors stayed on the pads. Return 1 when the event must not go to the queue. */
void smk_apc_transport_led_set(uint8_t note, uint8_t velocity);
uint8_t smk_apc_usb_event(uint32_t pkt)
{
    uint8_t st = (uint8_t)(pkt >> 8), d1 = (uint8_t)((pkt >> 16) & 0x7Fu);
    uint8_t d2 = (uint8_t)((pkt >> 24) & 0x7Fu), ty = st & 0xF0u;
    /* Always store APC feedback. Keep it from the sound engine only while clip mode is on.
     * When clip mode is off, the original MIDI input path stays the same (agent rules, 10). */
    uint8_t owned = clip_grid_mode || apc_host_on;   /* attempt 35: or an APC40 host seen */
    if (ty == 0xB0u)                    /* attempt 35: APC40 knob LED rings (CC 16-31, 48-63) */
        return (uint8_t)(owned && ((d1 >= 16u && d1 <= 31u) || (d1 >= 48u && d1 <= 63u)));
    if (ty != 0x90u && ty != 0x80u)
        return 0u;
    if (ty == 0x80u)
        d2 = 0u;
    if (d1 < SMK_APC40_CLIP_COUNT) {
        smk_apc_led_set(d1, (uint8_t)(st & 0x0Fu), d2);
        return owned;
    }
    if (smk_apc40_transport_feedback_note(d1)) {
        smk_apc_transport_led_set(d1, d2);
        return owned;
    }
    /* attempt 35: the other APC40 mkII LED notes (track, scene, device and bank buttons, notes 0-98,
     * any channel: blink and pulse rates come on channels 2-16): kept from the sound engine in clip mode */
    if (d1 <= 98u)
        return owned;
    return 0u;
}
/* MIDI Start: count clock pulses from the downbeat. */
void smk_apc_clock_start(void)
{
    apc_clock_count = 0u;
    ck_start_req = 1;
}
/* Called for each incoming MIDI Clock pulse (24 pulses per quarter note). */
void smk_apc_clock_position(uint32_t sixteenths)
{
    apc_clock_count = sixteenths * 6u;   /* the blink phase follows the song position */
}
void smk_apc_clock_tick(void)
{
    uint32_t now = fm1_ms;
    if (apc_clock_seen) {
        uint32_t interval = now - apc_clock_last_ms;
        if (interval >= 4u && interval <= 250u)
            apc_clock_period_ms = apc_clock_period_ms
                                      ? (apc_clock_period_ms * 3u + interval) / 4u
                                      : interval;
        else if (interval > 250u)
            apc_clock_period_ms = 0u;
    }
    apc_clock_last_ms = now;
    apc_clock_count++;
    apc_clock_seen = 1u;
}
static uint8_t smk_apc_led_brightness(uint8_t channel, uint8_t queued)
{
    uint32_t now = fm1_ms, period_ms = apc_clock_period_ms;
    uint8_t clock_mode = (uint8_t)(queued || smk_apc40_is_queued(channel));
    uint8_t phase;
    if (!clock_mode)
        return 255u;
    if (!apc_clock_seen || !period_ms || now - apc_clock_last_ms > 500u)
        return (now / 250u) & 1u ? 0u : 255u;   /* no clock: 250 ms on, 250 ms off (120 BPM beat) */
    /* One cycle per beat (24 ticks), phase-locked to the clock count. MIDI Start
     * resets the count, so the lit half starts on the beat. */
    phase = smk_apc40_beat_phase(apc_clock_count);
    return phase < 128u ? 255u : 0u;   /* half a beat on, half a beat off */
}
void smk_apc_transport_led_set(uint8_t note, uint8_t velocity)
{
    if (note == SMK_APC40_PLAY_NOTE)
        apc_play_feedback = smk_apc40_transport_feedback_on(velocity);
    else if (note == SMK_APC40_RECORD_NOTE)
        apc_record_feedback = smk_apc40_transport_feedback_on(velocity);
}
uint8_t smk_apc_transport_led_state(uint8_t note)
{
    if (note == SMK_APC40_PLAY_NOTE) {
        /* Clock is live when a tick came in the last 500 ms. MIDI Start resets the
         * count (smk_apc_clock_start), so the blink follows the Loopy beat. */
        uint8_t live = (uint8_t)(apc_clock_seen && fm1_ms - apc_clock_last_ms <= 500u);
        return smk_apc40_play_led(apc_play_feedback, live, apc_clock_count);
    }
    if (note == SMK_APC40_RECORD_NOTE)
        return apc_record_feedback;
    return 0u;
}
uint8_t smk_clip_mode_on(void)
{
    return clip_grid_mode;
}
/* usb.c calls this when the host sends the APC40 introduction. No state is necessary:
 * APC feedback is stored at all times. */
/* attempt 35: an APC40 host answered the identity handshake (Loopy Pro, USB or Bluetooth): its LED
 * notes stay out of the sound engine also with clip mode off, until the USB host is lost. */
void smk_apc_host_seen(void)
{
    apc_host_on = 1u;
}
void smk_apc_host_lost(void)
{
    uint32_t i;
    apc_host_on = 0u;
    smk_ck_midi_stop();                              /* clip keys: all notes off (smk_ck_poll) */
    apc_play_feedback = apc_record_feedback = 0u;
    apc_cols = SMK_APC40_SOURCE_COLS;
    apc_rows = 2u;
    apc_origin_col = apc_origin_row = 0u;
    apc_view_has_clips = 0u;
    apc_layout_locked = apc_layout_pending = 0u;
    apc_view = 0u;
    apc_burst_mask = 0u;
    apc_burst_count = 0u;
    apc_clock_count = apc_clock_last_ms = apc_clock_period_ms = 0u;
    apc_clock_seen = 0u;
    for (i = 0; i < SMK_APC40_CLIP_COUNT; i++) {
        apc_led_velocity[i] = 0u;
        apc_led_channel[i] = 0u;
        apc_cell_flags[i] = 0u;
        apc_cell_tick[i] = 0u;
    }
}
/* Clip grid on the LCD (main loop only, from ui.c draw_main). It shows the 8x5 APC
 * cells: lit cells in the color from Loopy (dark shades lifted, see SMK_CLIP_LCD_FLOOR), unlit cells dim, queued cells with a white
 * dot. A white frame marks each cell that is on the 16 pads now (the OCT view, or the
 * full 4x4). It redraws when the cells or the view change, at most every 64 ms.
 * It uses the existing canvas (gfx.c cv_px): no new RAM buffer. */
#define SMK_CLIP_LCD_MIN_MS 64u
/* Lowest level (0..255) of the brightest channel of a lit clip on the LCD. */
#define SMK_CLIP_LCD_FLOOR 128u
static uint8_t was_on;   /* 0: the next smk_clip_lcd_draw draws at once */
static uint32_t ck_t0;   /* the time of the last SCALE / CHORD / ARP press (a tap or a hold) */
void smk_clip_lcd_force(void) { was_on = 0u; }
static uint32_t ck_page_on(void);
static void ck_page_draw(void);
uint8_t smk_clip_lcd_draw(uint32_t w, uint32_t h)
{
    static uint32_t last_ms, last_sig;
    uint32_t sig = 2166136261u, note;
    uint8_t pad, in_view[SMK_APC40_CLIP_COUNT];
    if (ck_page_on()) {                             /* the SCALE / CHORD / ARP page is over the grid */
        if (fm1_ms - last_ms < SMK_CLIP_LCD_MIN_MS)
            return 0u;
        last_ms = fm1_ms;
        was_on = 0u;                                /* (the grid draws at once when the page closes) */
        cv_begin(w, h, C_BLACK);
        cv_band(0, (int32_t)h);
        ck_page_draw();
        return 1u;
    }
    for (note = 0; note < SMK_APC40_CLIP_COUNT; note++) {
        in_view[note] = 0u;
        sig = (sig ^ apc_led_velocity[note]) * 16777619u;
        sig = (sig ^ (apc_cell_flags[note] & SMK_APC40_CELL_QUEUED)) * 16777619u;
    }
    for (pad = 0; pad < SMK_APC40_PAD_COUNT; pad++) {
        uint8_t n = smk_apc40_note_for_pad_view(apc_cols, apc_rows, apc_origin_col,
                                                apc_origin_row, apc_view, pad);
        if (n < SMK_APC40_CLIP_COUNT)
            in_view[n] = 1u;
    }
    for (note = 0; note < SMK_APC40_CLIP_COUNT; note++)
        sig = (sig ^ in_view[note]) * 16777619u;
    sig = (sig ^ ((uint32_t)apc_cols | (uint32_t)apc_rows << 8 | (uint32_t)apc_origin_col << 16 |
                  (uint32_t)apc_origin_row << 24)) * 16777619u;
    sig = (sig ^ apc_view_has_clips) * 16777619u;
    if (was_on && sig == last_sig && fm1_ms - last_ms < 1000u)
        return 0u;                                  /* nothing changed (1 s refresh anyway) */
    if (was_on && fm1_ms - last_ms < SMK_CLIP_LCD_MIN_MS)
        return 0u;                                  /* rate limit */
    was_on = 1u;
    last_sig = sig;
    last_ms = fm1_ms;
    cv_begin(w, h, C_BLACK);
    cv_band(0, (int32_t)h);
    {
        smk_apc40_lcd_map_t m = smk_apc40_lcd_map(apc_cols, apc_rows, apc_origin_col, apc_origin_row,
                                                  apc_view_has_clips, (uint16_t)w, (uint16_t)h);
        int32_t p = m.pitch, gap = p >= 24 ? 3 : 2;
        uint8_t col, row;
        for (row = 0u; row < m.nrows; row++)
            for (col = 0u; col < m.ncols; col++) {
                /* The highest APC row is the top Loopy row: draw it on top. */
                int32_t x = m.x0 + (int32_t)col * p, y = m.y0 + (int32_t)smk_apc40_lcd_screen_row(&m, (uint8_t)(m.row0 + row)) * p;
                uint8_t n = (uint8_t)((m.row0 + row) * SMK_APC40_SOURCE_COLS + m.col0 + col);
                uint8_t v = apc_led_velocity[n];
                uint32_t rgb = APC40_RGB[v];
                uint32_t rr = rgb >> 16, gg = (rgb >> 8) & 0xFFu, bb = rgb & 0xFFu;
                uint32_t mx = rr > gg ? rr : gg;
                uint16_t c;
                if (bb > mx)
                    mx = bb;
                /* Loopy idle shades are too dark on the LCD. Map the brightest channel
                 * from 0..255 to FLOOR..255. The hue stays the same. Playing (full)
                 * colours do not change. Idle stays darker than playing. */
                if (mx) {
                    uint32_t t = SMK_CLIP_LCD_FLOOR + mx * (255u - SMK_CLIP_LCD_FLOOR) / 255u;
                    rr = rr * t / mx;
                    gg = gg * t / mx;
                    bb = bb * t / mx;
                }
                c = (uint16_t)((rr >> 3) << 11 | (gg >> 2) << 5 | (bb >> 3));
                int32_t q = p / 4;
                if (in_view[n])
                    cv_rect(x, y, p, p, C_WHITE);
                cv_rect(x + gap, y + gap, p - 2 * gap, p - 2 * gap, C_BLACK);
                if (v)
                    cv_rect(x + gap + 1, y + gap + 1, p - 2 * gap - 2, p - 2 * gap - 2, c);
                else
                    cv_rect(x + p / 2 - 2, y + p / 2 - 2, 4, 4, C_DIM);
                if (apc_cell_flags[n] & SMK_APC40_CELL_QUEUED)
                    cv_rect(x + p / 2 - q / 2, y + p / 2 - q / 2, q, q, C_WHITE);
            }
    }
    return 1u;
}
/* Called from the USB interrupt (usb_poll, 2 kHz). The cell state and the burst
 * state change only in this interrupt, so the main loop only reads them. */
void smk_apc_usb_tick(void)
{
    static uint8_t last;
    uint8_t now = (uint8_t)(fm1_ms >> 4);
    if (now == last)
        return;
    last = now;
    (void)smk_apc40_cells_age(apc_led_velocity, apc_led_channel, apc_cell_flags, apc_cell_tick, now);
    if (apc_burst_count && fm1_ms - apc_burst_last_ms > SMK_APC40_BURST_GAP_MS)
        smk_apc_burst_close();
}
/* Clip mode entry: latch the layout from the stored feedback at once and show view 0.
 * Feedback is stored also outside clip mode, so the pads light on entry. */
void smk_apc_clip_entered(void)
{
    apc_layout_locked = 0u;
    apc_layout_pending = 1u;
    apc_layout_deadline = fm1_ms;
    apc_view = 0u;
}
static uint8_t smk_clip_map_refresh(void)
{
    uint8_t feedback[SMK_APC40_CLIP_COUNT], i, changed;
    uint8_t old_cols = apc_cols, old_rows = apc_rows;
    uint8_t old_origin_col = apc_origin_col, old_origin_row = apc_origin_row;
    uint8_t old_has_clips = apc_view_has_clips;
    uint8_t any_clip = 0u;

    for (i = 0; i < SMK_APC40_CLIP_COUNT; i++)
        if ((feedback[i] = apc_led_velocity[i]) != 0u)
            any_clip = 1u;
    apc_view_has_clips = any_clip;
    if (!apc_layout_locked && apc_layout_pending && (int32_t)(fm1_ms - apc_layout_deadline) >= 0) {
        (void)smk_apc40_latch_layout(feedback, &apc_cols, &apc_rows,
                                     &apc_origin_col, &apc_origin_row, &apc_layout_locked);
        apc_layout_pending = 0u;
        apc_view = smk_apc40_view_after_move(apc_cols, apc_rows, apc_view, 0);
    } else if (apc_layout_locked) {
        uint8_t c, r, oc, orow, has;
        smk_apc40_build_layout(feedback, &c, &r, &oc, &orow, &has);
        if (has && smk_apc40_layout_grows(apc_cols, apc_rows, apc_origin_col, apc_origin_row,
                                          c, r, oc, orow)) {
            apc_cols = c;
            apc_rows = r;
            apc_origin_col = oc;
            apc_origin_row = orow;
            apc_view = smk_apc40_view_after_move(apc_cols, apc_rows, apc_view, 0);
        }
    }
    changed = (uint8_t)(old_cols != apc_cols || old_rows != apc_rows ||
                        old_origin_col != apc_origin_col || old_origin_row != apc_origin_row ||
                        old_has_clips != apc_view_has_clips);
    return changed;
}
/* Loopy Undo / Redo in clip mode: PATCH held + SEQ REC / SEQ PLAY. Return 1 when the
 * button event is used here (then no APC transport note is sent). A press sends note on.
 * The release of the same button sends note off, also if PATCH was let go first. */
uint8_t smk_clip_history_button(uint8_t button, uint8_t pressed, uint8_t patch_held)
{
    static uint8_t sent;   /* bit 0: undo note is on, bit 1: redo note is on */
    uint8_t redo, bit;
    if (button == B_REC)
        redo = 0u;
    else if (button == B_PLAY)
        redo = 1u;
    else
        return 0u;
    bit = (uint8_t)(1u << redo);
    if (pressed) {
        if (!clip_grid_mode || !patch_held)
            return 0u;
        midi_out_event(smk_apc40_history_packet(redo, 1u));   /* USB only (the APC40 view) */
        sent |= bit;
        say(redo ? "LOOPY REDO" : "LOOPY UNDO", 0);
        return 1u;
    }
    if (!(sent & bit))
        return 0u;
    midi_out_event(smk_apc40_history_packet(redo, 0u));
    sent &= (uint8_t)~bit;
    return 1u;
}
uint8_t smk_clip_transport_button(uint8_t button, uint8_t pressed)
{
    static uint8_t play_sent, record_sent;
    uint8_t *sent, note, is_record, event;
    extern void midi_send_note(uint32_t note, uint32_t vel);

    if (button == B_PLAY) {
        sent = &play_sent;
        is_record = 0u;
    } else if (button == B_REC) {
        sent = &record_sent;
        is_record = 1u;
    } else {
        return 0u;
    }
    event = smk_apc40_transport_event(sent, clip_grid_mode, pressed);
    if (event == SMK_APC40_BUTTON_IGNORED)
        return 0u;
    if (event != SMK_APC40_BUTTON_ON && event != SMK_APC40_BUTTON_OFF)
        return 1u;
    if (event == SMK_APC40_BUTTON_ON) {
        /* Light the LED when the button triggers. Loopy feedback can override it. */
        if (is_record)
            apc_record_feedback = smk_apc40_transport_led_after_press(apc_record_feedback);
        else
            apc_play_feedback = smk_apc40_transport_led_after_press(apc_play_feedback);
    }
    note = smk_apc40_transport_note(is_record);
    apc_note(note, event == SMK_APC40_BUTTON_ON ? 127u : 0u);
    return 1u;
}
static void smk_pads(void)
{
    uint32_t logical, w;
    if (clip_grid_mode) {
        for (logical = 0; logical < SMK_APC40_PAD_COUNT; logical++) {
            uint8_t note = smk_apc40_note_for_pad_view(apc_cols, apc_rows, apc_origin_col,
                                                       apc_origin_row, apc_view, (uint8_t)logical);
            uint8_t v = note < SMK_APC40_CLIP_COUNT ? apc_led_velocity[note] : 0u;
            uint8_t channel = note < SMK_APC40_CLIP_COUNT ? apc_led_channel[note] : 0u;
            uint32_t rgb = APC40_RGB[v];   /* the last color from Loopy, drawn as sent */
            uint8_t white = smk_apc40_physical_pad_for_logical((uint8_t)logical);
            uint32_t pad = SMK_PAD_OF_WHITE(white);
            uint8_t queued = note < SMK_APC40_CLIP_COUNT &&
                             (apc_cell_flags[note] & SMK_APC40_CELL_QUEUED) ? 1u : 0u;
            uint8_t brightness = v ? smk_apc_led_brightness(channel, queued) : 0u;
            /* Loopy row zero maps to the SMK's lower physical pad row. */
            if (v) padled_set(pad, (uint8_t)(((rgb >> 16) * brightness) / 510u),
                              (uint8_t)((((rgb >> 8) & 0xFFu) * brightness) / 510u),
                              (uint8_t)(((rgb & 0xFFu) * brightness) / 510u));
            else padled_set(pad, 0, 0, 0);
        }
        padled_flush();
        return;
    }
    uint16_t c = PART_COL[ui.part % NPARTS];
    uint32_t r = (uint32_t)(c >> 11) * 255u / 31u, g = (uint32_t)((c >> 5) & 63u) * 255u / 63u,
             b = (uint32_t)(c & 31u) * 255u / 31u;
    for (w = 0; w < SMK_NPAD; w++) {
        uint32_t k = SMK_WHITE_NOTE[w], pad = SMK_PAD_OF_WHITE(w), sh;
        sh = (smk_keys_lit >> k) & 1u ? 1u : (smk_keys_dim >> k) & 1u ? 3u : 0u;
        if (sh)
            padled_set(pad, (uint8_t)(r >> sh), (uint8_t)(g >> sh), (uint8_t)(b >> sh));
        else
            padled_set(pad, 0, 0, 0);
    }
    padled_flush();
}

/* The four faders, by FADER BANK (its LED lit on bank B): A = the levels of 909, 808, 303 A, 303 B; B = the
 * BREAK level, the reverb and delay returns, the master FILTER (middle off, low-pass left, high-pass right).
 * They set the same values as the knobs (saved with the project; a knob move while REC runs records motion).
 * A fader takes over only once it moves (also after a bank change), so nothing jumps. */
static void smk_faders(void)
{
    if (clip_grid_mode)
        return;                         /* clip mode: the faders and FADER BANK do nothing */
    static const char *const NAME[2][4] = {{"909 LEVEL", "808 LEVEL", "303 A LEVEL", "303 B LEVEL"},
                                           {"BREAK LEVEL", "REV RETURN", "DLY RETURN", "FILTER"}};
    static uint8_t last[4] = {0xFF, 0xFF, 0xFF, 0xFF}, held;
    uint32_t i, down = (fm1_in.buttons >> SMK_BTN_FADER_BANK) & 1u;
    if (down && !held) {                                /* FADER BANK: the other bank, each fader waits to move */
        smk_fbank ^= 1u;
        for (i = 0; i < 4u; i++)
            last[i] = 0xFF;
        say(smk_fbank ? "FADERS: BRK REV DLY FILTER" : "FADERS: 909 808 303A 303B", 0);   /* (say: 27 chars) */
    }
    held = (uint8_t)down;
    for (i = 0; i < 4u; i++) {
        uint32_t v = fm1_in.fader[i];
        pref_t r;
        const x0x_param_t *d;
        char t[28], num[16], unit[8], *q;
        uint32_t n = 0;
        if (last[i] == 0xFF || fm1_in.frames < 200u) {   /* (the first readings settle) */
            last[i] = (uint8_t)v;
            continue;
        }
        if (v == last[i])
            continue;
        last[i] = (uint8_t)v;
        if (!smk_fbank)
            r = PR(R_ENG, T_MIX, i, MX_LEVEL);
        else if (i == 0)
            r = PR(R_ENG, T_MIX, PART_BRK, MX_LEVEL);
        else if (i == 1)
            r = PR(R_ENG, T_FX, 0, FX_RV_LEVEL);
        else if (i == 2)
            r = PR(R_ENG, T_FX, 0, FX_DL_LEVEL);
        else
            r = PR(R_ENG, T_MST, 0, MST_DJF);
        d = pref_desc(r);
        if (!d)
            continue;
        pref_set(r, (int)((v * d->max + 63u) / 127u));
        motion_touch(r);
        ui.act_t = plat_ms();                            /* (the autosave waits for quiet) */
        pref_value(r, num, unit);
        q = put_s(put_s(t, NAME[smk_fbank][i]), " ");
        while (n < 16u && num[n])
            n++;
        if (n < 8u)                                      /* NAME (<= 11) + " " + num (< 8) + unit (< 8): fits say() */
            put_s(put_s(q, num), unit);
        say(t, 0);
    }
}

/* The wheels (mux C address 4 = pitch, 5 = mod), self-calibrating as in the reference port: the pitch wheel's
 * rest is its centre (taken at power-on, followed slowly while it rests), each wheel's range grows to what it
 * has reached. On a 303 (the part on screen) they play its sound without changing what is saved: pitch = its
 * TUNE (about +-1.5 semitones), mod = its CUTOFF opened from the saved value up to the top. Released (or
 * another part chosen), the saved values sound again. Other parts: the wheels do nothing. */
static void smk_wheels(void)
{
    enum { DEAD = 12, SPAN0 = 80, MOD_LO = 485, MOD_HI = 615 };
    static int32_t pc, plo, phi, mlo, mhi, rest;
    static int8_t part = -1;
    static int16_t tune_now = -1, cut_now = -1;
    int32_t p = fm1_in.wheel[0], m = fm1_in.wheel[1], d, bend, mw, want;
    if (fm1_in.frames < 300u)                         /* (the readings settle) */
        return;
    if (!pc) {
        pc = p;
        plo = p - SPAN0;
        phi = p + SPAN0;
        mlo = m < MOD_LO ? m : MOD_LO;
        mhi = m > MOD_HI ? m : MOD_HI;
    }
    if (p < plo)
        plo = p;
    if (p > phi)
        phi = p;
    if (m < mlo)
        mlo = m;
    if (m > mhi)
        mhi = m;
    d = p - pc;
    if (d > -DEAD && d < DEAD) {
        bend = 0;
        if (++rest > 200) {                            /* resting: the centre follows a slow drift */
            pc += d > 0 ? 1 : d < 0 ? -1 : 0;
            rest = 0;
        }
    } else {
        rest = 0;
        bend = d > 0 ? (d - DEAD) * 63 / (phi - pc - DEAD > 1 ? phi - pc - DEAD : 1)
                     : (d + DEAD) * 63 / (pc - plo - DEAD > 1 ? pc - plo - DEAD : 1);
        bend = clamp(bend, -63, 63);
    }
    mw = mhi - m <= DEAD ? 0 : (mhi - m - DEAD) * 127 / (mhi - mlo - DEAD > 1 ? mhi - mlo - DEAD : 1);
    mw = clamp(mw, 0, 127);
    if (part != (is_303() ? bidx() : -1)) {          /* another part: the last 303 sounds as saved again */
        if (part >= 0 && tune_now >= 0)
            engine_set(T_303, part, BASS303_TUNE, proj.sound.v[T_303][part][BASS303_TUNE]);
        if (part >= 0 && cut_now >= 0)
            engine_set(T_303, part, BASS303_CUTOFF, proj.sound.v[T_303][part][BASS303_CUTOFF]);
        part = (int8_t)(is_303() ? bidx() : -1);
        tune_now = cut_now = -1;
    }
    if (part < 0)
        return;
    want = bend ? clamp(proj.sound.v[T_303][part][BASS303_TUNE] + bend, 0, 127) : -1;
    if (want != tune_now) {
        engine_set(T_303, part, BASS303_TUNE, want >= 0 ? want : proj.sound.v[T_303][part][BASS303_TUNE]);
        tune_now = (int16_t)want;
    }
    {
        int32_t base = proj.sound.v[T_303][part][BASS303_CUTOFF];
        want = mw ? base + (127 - base) * mw / 127 : -1;
    }
    if (want != cut_now) {
        engine_set(T_303, part, BASS303_CUTOFF, want >= 0 ? want : proj.sound.v[T_303][part][BASS303_CUTOFF]);
        cut_now = (int16_t)want;
    }
}

/* < and >: one detent of SELECT back / forward (the page, a list's row: what SELECT does there; not with HOME held,
 * where SELECT is the tempo), as on the other SMK-37 Pro ports. Not if HARDWARE CALIBRATION gave them an FM-1 label. */
static void smk_arrows(void)
{
    static uint32_t prev;
    uint32_t now = fm1_in.buttons & (1u << SMK_BTN_LEFT | 1u << SMK_BTN_RIGHT), id, i;
    uint32_t pressed = now & ~prev;
    prev = now;
    /* In clip mode < and > are local only. They never move the Loopy viewport:
     * Loopy does not report clips outside its 8x5 APC box, so a legal move cannot
     * be known, and moves to empty areas broke the pad map. The whole APC box fits
     * the pad window horizontally (8 columns), so there is no horizontal move. */
    if (clip_grid_mode) {
        return;
    }
    if (!pressed || (plat_buttons() & (1u << B_HOME)))
        return;
    for (id = SMK_BTN_LEFT; id <= SMK_BTN_RIGHT; id++) {
        if (!((pressed >> id) & 1u))
            continue;
        for (i = 0; i < NB && panel.btn[i] != id; i++)
            ;
        if (i == NB) {
            uint32_t l = fm1__lock();
            fm1_in.enc_steps[panel.enc[EN_SELECT]] += (int16_t)((id == SMK_BTN_RIGHT ? 1 : -1) * panel.dir[EN_SELECT]);
            fm1__unlock(l);
        }
    }
}

/* --------------------------------------------- clip mode keybed (Loopy Pro) --- */
/* In clip mode the keybed plays MIDI notes to Loopy only: key -> SCALE -> CHORD -> ARP -> the CLIP KEYS
 * channel (GLO > MIDI) on USB and Bluetooth (apc_out). The logic is in smk_clip_keys.h.
 * The notes of a key come from its key number and the settings (ck_key_notes). A change that changes the
 * notes first ends the notes that sound, so a release always ends exactly the notes that its key sent.
 * A note that two keys hold ends when the last of the two keys goes up. No table of notes: key bit maps. */
typedef struct {
    uint8_t on, pg, fl, vel;          /* on: CK_M_* bits; pg: the page (CK_P_*); fl: CK_F_* bits; vel: the last
                                       * key velocity */
    uint8_t arp_note, ch, btn_prev, nk;   /* arp_note: note + 1 (0 = none); ch: the channel of all notes;
                                           * nk: keys in korder. ARP off: a strum (korder[0] the key, nk - 1
                                           * notes sent, acc the ms since the press); 0 = none */
    uint8_t tick, gate_left, ext_seen;
    volatile uint8_t off_req;         /* MIDI Stop, USB reset: all notes off (set in interrupts) */
    uint16_t step, last_ms;
    uint32_t acc;                     /* the internal arp clock (600000 = one tick) */
    uint32_t keys[2], snd[2];         /* bit k: key k is down / key k has notes that sound */
    uint8_t korder[8];                /* the keys of the arp, in press order (more keys: not in the arp) */
} ck_t;
static ck_t ck SMK_BIG;
void smk_ck_midi_stop(void) { ck.off_req = 1; }
/* a base register for ck (short code: no full address for each field) */
#include "fm1_opt.h"
#define CK_BASE ck_t *cp = &ck; FM1_BASE(cp)
#define CK_SET settings_t *sp = &proj.set
enum { CK_F_LATCH = 1u, CK_F_NRUSED = 2u, CK_F_BLE = 4u, CK_F_ACTIVE = 8u };
#define CK_BIT(map, k) (((map)[(k) >> 5] >> ((k) & 31u)) & 1u)

/* HUMAN timing: the note-ons that wait (main loop only). ck_qe: note | velocity << 7; ck_qt: the low 8 bits of
 * fm1_ms when it goes out. The channel is ck.ch: a channel change first sends all (ck_all_off). */
#define CK_QN 24u
static uint16_t ck_qe[CK_QN];
static uint8_t ck_qt[CK_QN], ck_qn;
static void ck_q_run(uint32_t only);
__attribute__((minsize)) static void ck_send(uint32_t st, uint32_t d1, uint32_t d2)
{
    if ((st & 0xF0u) == 0x80u)
        ck_q_run(d1 + 1u);                          /* a note-off never goes before its note-on */
    apc_out((st >> 4) | st << 8 | d1 << 16 | d2 << 24);
}
/* send the note-ons that wait: only 0: the ones that are due (ck_q_due); note + 1: the ones of this note;
 * 0x100: all */
CK_FN static void ck_q_run(uint32_t only)
{
    uint32_t i, j, e;
    for (i = j = 0; i < ck_qn; i++) {
        e = ck_qe[i];
        if (only ? only > 0xFFu || (e & 127u) + 1u == only : ck_q_due(ck_qt[i], fm1_ms)) {
            apc_out(9u | (0x90u | ck.ch) << 8 | (e & 127u) << 16 | (e >> 7) << 24);
            continue;
        }
        ck_qe[j] = (uint16_t)e;
        ck_qt[j++] = ck_qt[i];
    }
    ck_qn = (uint8_t)j;
}
/* the notes of key k with the settings now */
CK_FN static uint32_t ck_key_notes(uint32_t k, uint8_t *out)
{
    CK_BASE;
    CK_SET;
    uint32_t c = cp->on & CK_M_CHORD;
    return ck_notes(CK_BASE_NOTE + k, ck_mask(sp->ck_scale, sp->ck_root), cp->on & CK_M_SCALE ? sp->ck_snap + 1u : 0u,
                    c ? (int32_t)sp->ck_chord : -1, c ? sp->ck_inv : 0u, out);
}
CK_FN static void ck_arp_stop_note(void)
{
    CK_BASE;
    if (cp->arp_note) {
        ck_send(0x80u | cp->ch, cp->arp_note - 1u, 0u);
        cp->arp_note = 0;
    }
}
/* a note-on: the key velocity, HUMAN moves it by up to 4 x HUMAN at random and sends it 0 to 4 x HUMAN ms late
 * (a full queue: at once) */
CK_FN static void ck_on(uint32_t note)
{
    CK_BASE;
    uint32_t late, v = ck_human(proj.set.ck_x1 >> 3 & 7u, (fm1_ms ^ note << 8) * 2654435761u, cp->vel, &late);
    if (!late || ck_qn >= CK_QN) {
        ck_send(0x90u | cp->ch, note, v);
        return;
    }
    ck_qe[ck_qn] = (uint16_t)(note | v << 7);
    ck_qt[ck_qn++] = (uint8_t)(fm1_ms + late);
}
/* key k: note on (st 0x90), note off (0x80), or note off that keeps the notes of the other sounding keys
 * (0x81: a key release) */
CK_FN static void ck_key_send(uint32_t k, uint32_t st)
{
    CK_BASE;
    uint8_t out[CK_MAXN];
    uint32_t keep[4] = {0, 0, 0, 0}, n, i, q;
    if (st & 1u)
        for (q = 0; q < CK_NKEY; q++)
            if (q != k && CK_BIT(cp->snd, q))
                for (i = 0, n = ck_key_notes(q, out); i < n; i++)
                    keep[out[i] >> 5] |= 1u << (out[i] & 31u);
    for (i = 0, n = ck_key_notes(k, out); i < n; i++)
        if (!CK_BIT(keep, out[i])) {
            if (st & 0x10u)
                ck_on(out[i]);
            else
                ck_send(0x80u | cp->ch, out[i], 0u);
        }
}
/* all keys that sound: note off (0x80) or on again (0x90), round a setting change */
CK_FN static void ck_voice(uint32_t st)
{
    CK_BASE;
    uint32_t k;
    for (k = 0; k < CK_NKEY; k++)
        if (CK_BIT(cp->snd, k))
            ck_key_send(k, st);
}
/* All notes off: the keys, the arp note, the arp list */
CK_FN static void ck_all_off(void)
{
    CK_BASE;
    ck_q_run(0x100u);                                       /* (the notes that wait go out first) */
    ck_voice(0x80u);
    cp->snd[0] = cp->snd[1] = 0;
    ck_arp_stop_note();
    cp->nk = 0;
}
/* STRUM (ARP off): the notes of the strum key, low to high, one each STRUM x 10 ms after the press.
 * dt: the ms since the last call (a big dt sends all the notes that wait). */
CK_FN static void ck_strum(uint32_t dt)
{
    CK_BASE;
    uint8_t out[CK_MAXN];
    uint32_t n, i, j, x, gap = (proj.set.ck_x2 >> 4) * 10u;
    if (!cp->nk)
        return;
    n = ck_key_notes(cp->korder[0], out);
    for (i = 1; i < n; i++)                                 /* (an inversion puts low notes up) */
        for (j = i, x = out[i]; j && out[j - 1u] > x; j--) {
            out[j] = out[j - 1u];
            out[j - 1u] = (uint8_t)x;
        }
    cp->acc += dt;
    for (; cp->nk <= n && (cp->nk - 1u) * gap <= cp->acc; cp->nk++)
        ck_on(out[cp->nk - 1u]);
    if (cp->nk > n)
        cp->nk = 0;
}
CK_FN static void ck_press(uint32_t k, uint32_t vel)
{
    CK_SET;
    CK_BASE;
    cp->vel = (uint8_t)(vel ? vel : 1u);
    if (!(cp->on & CK_M_ARP)) {
        cp->snd[k >> 5] |= 1u << (k & 31u);
        if ((sp->ck_x2 >> 4) && (cp->on & CK_M_CHORD)) {     /* STRUM: the last strum ends at once */
            ck_strum(1u << 20);
            cp->korder[0] = (uint8_t)k;
            cp->nk = 1;
            cp->acc = 0;
            ck_strum(0);
            return;
        }
        ck_key_send(k, 0x90u);
        return;
    }
    cp->keys[k >> 5] ^= 1u << (k & 31u);                     /* (for a moment: the other keys only) */
    if ((cp->fl & CK_F_LATCH) && !(cp->keys[0] | cp->keys[1])) {
        ck_arp_stop_note();                                 /* LATCH: the first key after all keys up: a new list */
        cp->nk = 0;
    }
    cp->keys[k >> 5] ^= 1u << (k & 31u);
    if (!cp->nk) {
        cp->step = 0;
        cp->tick = (uint8_t)(CK_RATE_T[sp->ck_rate] - 1u);   /* the first step plays at the next tick */
        cp->acc = 599999u;
    }
    if (cp->nk < sizeof cp->korder)
        cp->korder[cp->nk++] = (uint8_t)k;
}
CK_FN static void ck_release(uint32_t k)
{
    CK_BASE;
    uint32_t i, j;
    if (CK_BIT(cp->snd, k)) {
        cp->snd[k >> 5] &= ~(1u << (k & 31u));
        ck_key_send(k, 0x81u);
    }
    if (!(cp->on & CK_M_ARP)) {                             /* (a strum of this key stops) */
        if (cp->korder[0] == k)
            cp->nk = 0;
        return;
    }
    if (cp->fl & CK_F_LATCH)
        return;
    for (i = j = 0; i < cp->nk; i++)
        if (cp->korder[i] != k)
            cp->korder[j++] = cp->korder[i];
    cp->nk = (uint8_t)j;
    if (!j)
        ck_arp_stop_note();
}
/* one arp step: the notes of the arp keys, sorted (not for PLAYED), then the note of this step */
CK_FN static void ck_arp_step(uint32_t rate, uint32_t pos)
{
    CK_BASE;
    CK_SET;
    uint8_t list[CK_ARPN], out[CK_MAXN];
    uint32_t n = 0, i, j, c;
    int note;
    for (i = 0; i < cp->nk; i++)
        for (j = 0, c = ck_key_notes(cp->korder[i], out); j < c && n < CK_ARPN; j++) {
            uint32_t x = out[j], p = n++;
            if (sp->ck_ord != CK_ORD_PLAYED)
                for (; p && list[p - 1u] > x; p--)
                    list[p] = list[p - 1u];
            list[p] = (uint8_t)x;
        }
    ck_arp_stop_note();
    if (!n)
        return;
    note = ck_arp_pick(list, n, sp->ck_ord, sp->ck_oct + 1u, pos, fm1_ms * 2654435761u >> 7);
    if (note >= 0) {
        ck_on((uint32_t)note);
        cp->arp_note = (uint8_t)(note + 1);
        j = rate * (sp->ck_gate + 1u) / 10u;                 /* gate 10-100 % of a step, at least 1 tick */
        cp->gate_left = (uint8_t)(j ? j : 1u);
    }
}
/* the arp clock: incoming MIDI clock while a tick came in the last 0.5 s, else the internal tempo */
CK_FN static void ck_arp(uint32_t dt)
{
    CK_SET;
    CK_BASE;
    uint32_t rate = CK_RATE_T[sp->ck_rate], ticks, e = apc_clock_count, rh = sp->ck_x2 & 15u;
    uint32_t sw = rate * 3u * (sp->ck_x1 & 7u) / 50u;      /* SWING: the odd steps start sw ticks late */
    if (ck_start_req) {                                     /* MIDI Start: the next tick is the downbeat */
        ck_start_req = 0;
        cp->tick = (uint8_t)(rate - 1u);
        cp->step = 0;
    }
    if (!(cp->on & CK_M_ARP)) {                             /* (acc and nk are the strum's) */
        cp->ext_seen = (uint8_t)e;
        ck_strum(dt);
        return;
    }
    if (apc_clock_seen && fm1_ms - apc_clock_last_ms <= 500u) {
        ticks = (uint8_t)(e - cp->ext_seen);
        ticks = ticks > 8u ? 0u : ticks;                    /* a jump (Start, Song Position): no burst */
        cp->acc = 0;
    } else {
        cp->acc += (dt < 100u ? dt : 100u) * (uint32_t)(seq.bpm * 240.0f);   /* ms x BPM x 240 */
        for (ticks = 0; cp->acc >= 600000u; ticks++)        /* (dt <= 100 ms: at most 12 ticks) */
            cp->acc -= 600000u;
    }
    cp->ext_seen = (uint8_t)e;
    while (ticks--) {
        if (cp->gate_left && !--cp->gate_left)
            ck_arp_stop_note();
        if (++cp->tick < (cp->step & 1u ? rate + sw : rate - sw))
            continue;
        cp->tick = 0;
        if (cp->nk) {                                       /* RHYTHM: a rest step plays no note */
            if (ck_rhy_hit(rh, cp->step))
                ck_arp_step(rate, ck_rhy_pos(rh, cp->step));
            cp->step++;
        }
    }
}
/* 1: clip mode and no menu on the screen: knobs 2-8 send CC 48-54 (ck_controls), X0X gets no knob turns */
uint8_t smk_ck_knobs_owned(void)
{
    return (uint8_t)(clip_grid_mode && ui.overlay == O_NONE && !ui.help);
}
static const char CK_MODE_N[] = "SCALE\0CHORD\0ARP";
static const uint16_t CK_MODE_C[3] = {RGB(90, 200, 255), RGB(255, 150, 40), RGB(130, 240, 90)};
#define CK_O(f) __builtin_offsetof(settings_t, f)
/* the editor fields (CK_E_*): the settings_t byte and the bit shift (the highest value: CK_FMAX); the names;
 * the value lists (0: a number) */
static const uint8_t CK_FLD[CK_NE][2] = {
    {CK_O(ck_root), 0}, {CK_O(ck_scale), 0}, {CK_O(ck_snap), 0}, {CK_O(ck_chord), 0}, {CK_O(ck_inv), 0},
    {CK_O(ck_x2), 4}, {CK_O(ck_ord), 0}, {CK_O(ck_rate), 0}, {CK_O(ck_oct), 0}, {CK_O(ck_gate), 0},
    {CK_O(ck_x1), 0}, {CK_O(ck_x1), 3}, {CK_O(ck_x2), 0}, {0, 0}};
static const char CK_FLD_N[] = "ROOT\0SCALE\0SNAP\0CHORD\0INV\0STRUM\0ORDER\0RATE\0OCT\0GATE\0SWING\0HUMAN\0RHYTHM\0LATCH";
static const char *const CK_LIST[CK_NE] = {CK_ROOT_N, CK_SCALE_N, "NEAR\0FILTER", CK_CHORD_N, "ROOT\0/1\0/2\0/3", 0,
                                            CK_ORD_N, CK_RATE_N, 0, 0, 0, 0, CK_RHY_N, "OFF\0ON"};
/* the mask of a field's bits (its highest value rounded up to 2^n - 1) */
CK_FN static uint32_t ck_fmask(uint32_t f)
{
    return (2u << (31 - __builtin_clz(CK_FMAX[f]))) - 1u;
}
CK_FN static uint32_t ck_get(uint32_t f)
{
    if (f == CK_E_LATCH)
        return ck.fl & CK_F_LATCH;
    return (uint32_t)((const uint8_t *)&proj.set)[CK_FLD[f][0]] >> CK_FLD[f][1] & ck_fmask(f);
}
/* set field f to v (0..its highest value). A field that changes the notes ends the old notes and plays the
 * new ones. The page remembers the field (< > and the panel) and the edit (the press is no tap). */
CK_FN static void ck_set(uint32_t f, uint32_t v)
{
    CK_BASE;
    uint8_t *b = (uint8_t *)&proj.set + CK_FLD[f][0];
    uint32_t m = ck_fmask(f) << CK_FLD[f][1], nt = f <= CK_E_INV;
    cp->pg = (uint8_t)((cp->pg & 7u) | CK_P_EDIT | f << 4);
    ui.act_t = plat_ms();                                   /* (the autosave waits for quiet) */
    if (f == CK_E_LATCH) {
        if (v != (cp->fl & CK_F_LATCH)) {
            cp->fl ^= CK_F_LATCH;
            if (!v)
                ck_all_off();
        }
        return;
    }
    if (!(cp->on & CK_M_ARP))
        cp->nk = 0;                                         /* (a strum: its key plays all its notes below) */
    if (nt)
        ck_voice(0x80u);
    *b = (uint8_t)((*b & ~m) | (v << CK_FLD[f][1] & m));
    if (nt)
        ck_voice(0x90u);
}
/* the text of value v of field f: a list name, or a number in buf */
CK_FN static const char *ck_val_txt(uint32_t f, uint32_t v, char *buf)
{
    const char *l = CK_LIST[f];
    if (l)
        return ck_nm(l, v);
    if (!v && (f == CK_E_STRUM || f == CK_E_HUMAN))
        return "OFF";
    v = f == CK_E_STRUM ? v * 10u : f == CK_E_OCT ? v + 1u : f == CK_E_GATE ? v * 10u + 10u
        : f == CK_E_SWING ? 50u + 3u * v : v;
    put_s(put_i(buf, (int)v), f == CK_E_STRUM ? "MS" : f == CK_E_GATE || f == CK_E_SWING ? "%" : "");
    return buf;
}
/* 1: the page is on the screen: a stage button held for CK_TAP_MS (or an edit), or the page is pinned */
static uint32_t ck_page_on(void)
{
    return ck_page_vis(ck.pg, fm1_ms - ck_t0);
}
/* 1: the keys edit the page (its stage button is held, no menu) */
CK_FN static uint32_t ck_keys_edit(void)
{
    uint32_t pg = ck.pg & 3u;
    return pg && (ck.btn_prev >> (pg - 1u) & 1u) && smk_ck_knobs_owned();
}
/* key k on the page: the field of its zone gets the value of the key (a one-key zone: the next value) */
CK_FN static void ck_key_edit(uint32_t k)
{
    uint32_t z = ck_zone((ck.pg & 3u) - 1u, k), f = z >> 12;
    if (z)
        ck_set(f, ck_zone_val(z, k, ck_get(f), CK_FMAX[f]));
}
/* SCALE, CHORD, ARP: a tap (shorter than CK_TAP_MS, no edit) turns the stage on or off. A hold shows its page,
 * and the keys edit the page. The release closes the page. FADER BANK pins the page (the keys play), a second
 * press closes it; < and > step the last field. NOTE REPEAT + ARP: LATCH on or off. NOTE REPEAT alone: panic.
 * Knobs 2-8: CC 48-54 (no stage takes the knobs). */
CK_FN static void ck_controls(void)
{
    CK_BASE;
    static const uint8_t ROLE[4] = {B_SEL, B_ENV, B_ARP, B_LFO};   /* SCALE, CHORD, ARP, NOTE REPEAT */
    uint32_t raw = fm1_in.buttons, i, cur = 0, press, rel, f, tog;
    for (i = 0; i < 4u; i++)
        cur |= ((raw >> panel.btn[ROLE[i]]) & 1u) << i;
    cur |= (raw >> SMK_BTN_FADER_BANK & 7u) << 4;            /* FADER BANK, <, > (bits 1-3) */
    press = cur & ~(uint32_t)cp->btn_prev;
    rel = (uint32_t)cp->btn_prev & ~cur;
    cp->btn_prev = (uint8_t)cur;
    if (!smk_ck_knobs_owned())
        press &= ~0x70u;                                    /* a menu: FADER BANK, < and > do nothing */
    if (press & 8u)
        cp->fl &= (uint8_t)~CK_F_NRUSED;
    if ((press & 4u) && (cur & 8u)) {                       /* NOTE REPEAT + ARP: LATCH */
        ck_set(CK_E_LATCH, !(cp->fl & CK_F_LATCH));
        cp->fl |= CK_F_NRUSED;
        ui_say("LATCH", cp->fl & CK_F_LATCH ? " ON" : " OFF");
    }
    cp->pg = (uint8_t)ck_page_btn(cp->pg, press, rel, cur, fm1_ms, &ck_t0, &tog);
    for (i = 0; i < 3u; i++)
        if (tog >> i & 1u) {                                /* a tap: the stage on or off */
            ck_all_off();
            cp->on ^= (uint8_t)(1u << i);
            ui_say(ck_nm(CK_MODE_N, i), cp->on >> i & 1u ? " ON" : " OFF");
        }
    f = cp->pg >> 4;
    if ((press & 0x60u) && ck_page_on()) {                  /* < >: the last field one step down / up */
        uint32_t v = ck_get(f);
        if (press & 0x40u)
            ck_set(f, v < CK_FMAX[f] ? v + 1u : v);
        else
            ck_set(f, v ? v - 1u : 0u);
    }
    if ((rel & 8u) && !(cp->fl & CK_F_NRUSED)) {             /* NOTE REPEAT alone: panic */
        ck_all_off();
        ck_send(0xB0u | cp->ch, 123u, 0u);                   /* All Notes Off */
        ui_say("PANIC", 0);
    }
    if (smk_ck_knobs_owned())                                /* knobs 2-8 send CC 48-54 for MIDI-learn */
        for (i = 0; i < NE; i++) {                          /* (relative: 1-63 up, 65-127 down) */
            int32_t d = fm1_enc_take(panel.enc[i]) * panel.dir[i];
            if (d)
                ck_send(0xB0u, 48u + i, (uint32_t)(d < -63 ? -63 : d > 63 ? 63 : d) & 127u);
        }
}
/* ------- clip mode screen: the KEYS panel (the bottom strip) and the SCALE / CHORD / ARP page (main area) --- */
static const uint8_t CK_WI[12] = {0, 0, 1, 1, 2, 3, 3, 4, 4, 5, 5, 6};   /* the white key of each pitch class */
#define CK_BLK(k) (0x54Au >> (k) % 12u & 1u)                             /* 1: a black key */
/* the left x of key k on a keyboard drawing with white keys ww wide from x0 */
CK_FN static int32_t ck_key_x(uint32_t k, int32_t x0, int32_t ww)
{
    return x0 + (int32_t)(k / 12u * 7u + CK_WI[k % 12u]) * ww + (int32_t)CK_BLK(k) * (ww - ww * 3 / 10);
}
/* a keyboard of n keys from key 0 (C), the white keys ww wide and h high, key k in col[k] */
CK_FN static void ck_kbd(int32_t x0, int32_t y, int32_t ww, int32_t h, uint32_t n, const uint16_t *col)
{
    uint32_t k, pass;
    for (pass = 0; pass < 2u; pass++)
        for (k = 0; k < n; k++) {
            int32_t x = ck_key_x(k, x0, ww), bw = ww * 3 / 5;
            if (CK_BLK(k) != pass)
                continue;
            if (pass) {
                cv_rect(x - 1, y, bw + 2, h * 3 / 5 + 1, C_BLACK);
                cv_rect(x, y, bw, h * 3 / 5, col[k]);
            } else {
                cv_rect(x, y, ww - 1, h, col[k]);
            }
        }
}
/* 1: the arp follows the MIDI clock (a tick in the last 0.5 s), 0: the internal tempo (for the header) */
uint8_t smk_ck_ext(void)
{
    return (uint8_t)(apc_clock_seen && fm1_ms - apc_clock_last_ms <= 500u);
}
/* The KEYS panel (ui.c draw_knobs, clip mode, no menu): the stage chips, one octave with the scale (the root
 * marked, the notes that sound lit), the scale and chord names, the arp line and its steps. *last: the last
 * drawing's signature; force: draw anyway. Return 1 when it drew on the canvas. */
uint8_t smk_ck_panel(uint32_t *last, uint32_t force)
{
    CK_BASE;
    CK_SET;
    uint8_t out[CK_MAXN];
    uint16_t col[12];
    char t[64], *q;
    uint32_t k, i, n, snd = 0, root = 99u, sig = 2166136261u, m = ck_mask(sp->ck_scale, sp->ck_root);
    uint32_t arp = cp->on & CK_M_ARP, rh = sp->ck_x2 & 15u, len = rh ? (CK_RHY_T[rh] & 15u) + 1u : 8u;
    uint32_t pos = arp && cp->nk && cp->step ? (cp->step - 1u) % len + 1u : 0u;
    for (k = 0; k < CK_NKEY; k++)
        if (CK_BIT(cp->snd, k) || (cp->nk && cp->korder[0] == k)) {
            n = ck_key_notes(k, out);
            if (root > 11u && n)
                root = out[0] % 12u;                        /* the chord root: the first key's first note */
            for (i = 0; i < n && !arp; i++)
                snd |= 1u << out[i] % 12u;
        }
    if (cp->arp_note)
        snd |= 1u << (cp->arp_note - 1u) % 12u;
    if (root > 11u)
        root = sp->ck_root;
    for (i = 0; i < 12u; i++)
        sig = (sig ^ (&sp->ck_ch)[i]) * 16777619u;
    sig = (sig ^ (cp->on | (cp->fl & CK_F_LATCH) << 3 | snd << 4 | root << 16 | (uint32_t)cp->pg << 20)) * 16777619u;
    sig = (sig ^ pos) * 16777619u;
    if (!force && sig == *last)
        return 0u;
    *last = sig;
    cv_begin(240, 82, C_BLACK);
    cv_rect(0, 0, 240, 1, C_LINE);
    for (i = 0; i < 4u; i++) {                              /* SCALE CHORD ARP LATCH */
        uint32_t on = i < 3u ? cp->on >> i & 1u : cp->fl & CK_F_LATCH;
        uint16_t c = i < 3u ? CK_MODE_C[i] : C_AMB;
        int32_t x = 3 + (int32_t)i * 59;
        if (on)
            rbox(x, 4, 55, 13, 4, c);
        else
            rframe(x, 4, 55, 13, 4, dim(c, 7));
        text_c(x + 27, vc(&FONT_T, 4, 13), &FONT_T, i < 3u ? ck_nm(CK_MODE_N, i) : "LATCH", on ? C_BLACK : dim(c, 11));
        if ((cp->pg & 3u) == i + 1u)                        /* the page on the screen */
            cv_rect(x + 8, 18, 39, 2, C_WHITE);
    }
    for (i = 0; i < 12u; i++)                               /* one octave: the scale, the notes that sound */
        col[i] = snd >> i & 1u ? C_AMB : ck_in(m, i) ? (CK_BLK(i) ? RGB(140, 140, 140) : RGB(220, 220, 220))
                                                    : (CK_BLK(i) ? RGB(30, 30, 30) : RGB(70, 70, 70));
    ck_kbd(4, 23, 15, 36, 12u, col);
    cv_rect(ck_key_x(sp->ck_root, 4, 15) + 2, 23 + (CK_BLK(sp->ck_root) ? 15 : 31), 5, 3, CK_MODE_C[0]);
    q = put_s(put_s(put_s(t, ck_nm(CK_ROOT_N, sp->ck_root)), " "), ck_nm(CK_SCALE_N, sp->ck_scale));
    if (sp->ck_snap)
        put_s(q, " FILTER");
    cv_text(114, vc(&FONT_XS, 22, 12), &FONT_XS, t, cp->on & CK_M_SCALE ? CK_MODE_C[0] : C_DIM);
    if (cp->on & CK_M_CHORD) {
        q = put_s(put_s(put_s(t, ck_nm(CK_ROOT_N, root)), " "), ck_nm(CK_CHORD_N, sp->ck_chord));
        if (sp->ck_inv)
            put_s(put_s(q, " "), ck_nm(CK_LIST[CK_E_INV], sp->ck_inv));
        cv_text(114, vc(&FONT_B, 35, 15), &FONT_B, t, CK_MODE_C[1]);
        if (sp->ck_x2 >> 4)
            cv_text(114, vc(&FONT_T, 51, 10), &FONT_T, (ck_val_txt(CK_E_STRUM, sp->ck_x2 >> 4, put_s(t, "STRUM ")), t),
                    C_GRAY);
    } else {
        cv_text(114, vc(&FONT_B, 35, 15), &FONT_B, "NO CHORD", C_DIM);
    }
    q = put_s(put_s(put_s(put_s(t, ck_nm(CK_ORD_N, sp->ck_ord)), " "), ck_nm(CK_RATE_N, sp->ck_rate)), " X");
    q = put_s(put_s(put_i(q, sp->ck_oct + 1), " "), ck_val_txt(CK_E_GATE, sp->ck_gate, t + 52));
    if (sp->ck_x1 & 7u)
        q = put_s(put_s(q, " SW"), ck_val_txt(CK_E_SWING, sp->ck_x1 & 7u, t + 52));
    if (sp->ck_x1 >> 3 & 7u)
        q = put_i(put_s(q, " HUM"), sp->ck_x1 >> 3 & 7u);
    if (rh)
        put_s(put_s(q, " "), ck_nm(CK_RHY_N, rh));
    cv_text(4, vc(&FONT_XS, 61, 12), &FONT_XS, t, arp ? CK_MODE_C[2] : C_DIM);
    for (i = 0; i < len && arp; i++)                        /* the steps: hits, rests, the step now */
        cv_rect(4 + (int32_t)i * 8, 76, 5, 4, i + 1u == pos ? C_WHITE : ck_rhy_hit(rh, i) ? dim(CK_MODE_C[2], 9) : C_LINE);
    return 1u;
}
/* The page of SCALE, CHORD or ARP over the main area (smk_clip_lcd_draw, the canvas is ready): the fields and
 * their values (the last field lit), then the 37 keys with the zone of each field and the key of its value */
static void ck_page_draw(void)
{
    CK_BASE;
    char t[12];
    uint16_t col[CK_NKEY];
    uint32_t pg = (cp->pg & 3u) - 1u, k, n;
    uint16_t c = CK_MODE_C[pg];
    const uint16_t *z = ck_zones(pg);
    int32_t x = cv_text(4, vc(&FONT_B, 0, 16), &FONT_B, ck_nm(CK_MODE_N, pg), c);
    cv_text(x + 6, vc(&FONT_XS, 0, 16), &FONT_XS, cp->on >> pg & 1u ? "ON" : "OFF", cp->on >> pg & 1u ? C_WHITE : C_DIM);
    text_r(236, vc(&FONT_T, 0, 16), &FONT_T, cp->pg & CK_P_PIN ? "PINNED  < > STEP" : "KEYS EDIT  FADER BANK: PIN",
           C_GRAY);
    cv_rect(0, 17, 240, 1, C_LINE);
    for (k = 0; k < CK_NKEY; k++)
        col[k] = CK_BLK(k) ? RGB(25, 25, 25) : RGB(60, 60, 60);
    for (n = 0; z[n]; n++) {
        uint32_t f = z[n] >> 12, lo = z[n] >> 6 & 63u, hi = z[n] & 63u, v = ck_get(f);
        int32_t fx = 4 + (int32_t)(n / 4u) * 120, fy = 20 + (int32_t)(n % 4u) * 12, w;
        const char *nm = ck_nm(CK_FLD_N, f);
        uint16_t zc = n & 1u ? dim(c, 7) : dim(c, 12);
        if (f == (uint32_t)cp->pg >> 4)
            rbox(fx - 2, fy, 116, 12, 3, dim(c, 4));
        cv_text(fx, vc(&FONT_T, fy, 12), &FONT_T, nm, C_GRAY);
        cv_text(fx + 44, vc(&FONT_XS, fy, 12), &FONT_XS, ck_val_txt(f, v, t), C_WHITE);
        for (k = lo; k <= hi; k++)                          /* the zone; the key of the value (one key: lit if on) */
            col[k] = (lo == hi ? v != 0u : k - lo == v) ? C_WHITE : CK_BLK(k) ? dim(zc, 9) : zc;
        x = (ck_key_x(lo, 10, 10) + ck_key_x(hi, 10, 10) + 9) / 2;
        w = tw(&FONT_T, nm) / 2 + 1;
        x = x < w ? w : x > 239 - w ? 239 - w : x;
        text_c(x, vc(&FONT_T, 69 + (int32_t)(n & 1u) * 10, 10), &FONT_T, nm, n & 1u ? dim(c, 11) : c);
    }
    ck_kbd(10, 91, 10, 47, CK_NKEY, col);
}
/* every main loop pass (also between frames, for the arp timing) */
CK_FN void smk_ck_poll(void)
{
    CK_SET;
    CK_BASE;
    uint32_t now = fm1_ms, k, on = clip_grid_mode, ble = plat_ble_state() == PLAT_BLE_CONNECTED ? CK_F_BLE : 0u;
    uint32_t dt = (uint16_t)(now - cp->last_ms);
    cp->last_ms = (uint16_t)now;
    if (cp->off_req || (cp->fl & ~ble & CK_F_BLE)) {          /* MIDI Stop, USB reset, Bluetooth drop: all notes off */
        cp->off_req = 0;
        ck_all_off();
    }
    cp->fl = (uint8_t)((cp->fl & ~CK_F_BLE) | ble);
    if (!on) {
        if (cp->fl & CK_F_ACTIVE) {                          /* clip mode exit: all notes off, the modes off */
            ck_all_off();
            cp->on = cp->pg = 0;
        }
        cp->fl = (uint8_t)(ble | CK_F_NRUSED);               /* (LATCH off) */
        cp->ext_seen = (uint8_t)apc_clock_count;
        cp->btn_prev = 0x7Fu;                                /* a button held at entry acts only after its release */
    } else {
        if (!(cp->fl & CK_F_ACTIVE)) {
            cp->fl |= CK_F_ACTIVE;
            was_on = 0u;
        }
        if (cp->ch != sp->ck_ch - 1u) {                     /* a new CLIP KEYS CH: all notes off */
            ck_all_off();                                   /* (on the old channel) */
            cp->ch = (uint8_t)(sp->ck_ch - 1u);
        }
        ck_controls();
    }
    for (k = 0; k < CK_NKEY; k++) {                         /* (out of clip mode: keys held at entry do not play) */
        uint32_t bit = 1u << (k & 31u), *pk = &cp->keys[k >> 5];
        uint32_t d = ((const volatile uint32_t *)&fm1_in.keys)[k >> 5] & bit;
        int w;
        if (d == (*pk & bit))
            continue;
        *pk ^= bit;
        if (!on)
            continue;
        if (!d) {
            ck_release(k);
            continue;
        }
        w = k >= SMK_KEY_F3 && k < SMK_KEY_F3 + NKEYS ? KEY_WHITE[k - SMK_KEY_F3] : -1;
        if (w >= 0 && ((fm1_in.pnotes >> (k - SMK_KEY_F3)) & 1u))   /* a held pad blocks its white key */
            continue;
        if (ck_keys_edit())                                 /* a stage button held: the key edits its page */
            ck_key_edit(k);
        else
            ck_press(k, fm1_in.key_vel[k]);
    }
    if (on) {
        ck_arp(dt);
        ck_q_run(0);                                        /* HUMAN: the note-ons that are due */
    }
}

/* every frame, after ui_frame; the wheels also between frames. The 10 outer keys (C3..E3, G#5..C6) have no
 * X0X role: their events are dropped. */
/* pads as a step grid (steps, patterns: 1-8 top row) unless they play notes (a 303's REC keyboard) */
static void smk_grid(void)
{
    smk_pad_grid = (uint8_t)(clip_grid_mode || ui.view == V_HOME || ui.view == V_SONG ||
                             (ui.btn & (1u << B_HOME | 1u << B_SAVE)) || !(is_303() && ui.rec));
    if (clip_grid_mode)
        smk_pad_piano = 0;
}

static void smk_frame(void)
{
    while (smk_extkey_take() >= 0)
        ;
    smk_arrows();
    (void)smk_clip_map_refresh();
    smk_faders();
    smk_wheels();
    /* long KNOB BANK hold toggles clip-grid mode once per hold */
    if (fm1_in.buttons & (1u << SMK_BTN_KNOB_BANK)) {
        if (!knob_bank_hold_start) {
            knob_bank_hold_start = fm1_ms ? fm1_ms : 1u;
        } else if (!knob_bank_toggled && fm1_ms - knob_bank_hold_start > 1000u) {
            clip_grid_mode ^= 1u;
            knob_bank_toggled = 1u;
            if (clip_grid_mode) {
                extern void smk_clip_enter_cleanup(void);
                smk_clip_enter_cleanup();
                smk_apc_clip_entered();
            }
            if (clip_grid_mode)
                say("LOOPY CLIPS", 0);
            else
                say("LP OFF", 0);
        }
    } else {
        /* A short KNOB BANK tap does nothing in clip mode. */
        knob_bank_hold_start = 0;
        knob_bank_toggled = 0;
        /* OCT- / OCT+ move the local view down / up by one 8x2 view. The move stays
         * in the latched clip footprint. plat_buttons hides these buttons from the
         * X0X UI in clip mode, so they have no other effect. */
        {
            static uint32_t oct_prev;
            uint32_t oct_now = fm1_in.buttons &
                               ((1u << panel.btn[B_OCTDN]) | (1u << panel.btn[B_OCTUP]));
            uint32_t oct_pressed = oct_now & ~oct_prev;
            oct_prev = oct_now;
            if (clip_grid_mode && (oct_pressed & (1u << panel.btn[B_OCTDN])))
                apc_view = smk_apc40_view_after_move(apc_cols, apc_rows, apc_view, -1);
            if (clip_grid_mode && (oct_pressed & (1u << panel.btn[B_OCTUP])))
                apc_view = smk_apc40_view_after_move(apc_cols, apc_rows, apc_view, 1);
        }
    }
    /* FADER BANK does nothing in clip mode (smk_faders returns early). */
    static uint32_t last_notes;
    static uint8_t sent_note[SMK_APC40_PAD_COUNT];
    static uint8_t was_on;
    if (clip_grid_mode) {
        smk_pad_piano = 0;   /* force direct white map, ignore piano layout */
        /* simple pad press: send note on rising edge, note-off on falling */
        if (!was_on) {
            last_notes = fm1_in.pnotes;   /* suppress false rising edges from pads held at entry */
            for (uint32_t i = 0; i < SMK_APC40_PAD_COUNT; i++)
                sent_note[i] = 0xFFu;
            was_on = 1u;
        }
        uint32_t now = fm1_in.pnotes, edge = now & ~last_notes, fall = last_notes & ~now;
        last_notes = now;
        for (uint32_t i = 0; i < SMK_APC40_PAD_COUNT; i++) {
            uint32_t bit = 1u << SMK_WHITE_NOTE[i];
            if (edge & bit) {
                extern void midi_send_note(uint32_t note, uint32_t vel);
                uint8_t logical = smk_apc40_logical_pad_for_physical((uint8_t)i);
                uint8_t note = smk_apc40_note_for_pad_view(apc_cols, apc_rows, apc_origin_col,
                                                            apc_origin_row, apc_view, logical);
                if (note != 0xFFu && apc_led_velocity[note] != 0u) {
                    sent_note[i] = note;
                    apc_note(note, 127u);
                }
            }
            if ((fall & bit) && sent_note[i] != 0xFFu) {
                extern void midi_send_note(uint32_t note, uint32_t vel);
                apc_note(sent_note[i], 0u);
                sent_note[i] = 0xFFu;
            }
        }
    } else {
        if (was_on) {
            /* mode exit: send note-off only for pads that received a note-on */
            for (uint32_t i = 0; i < SMK_APC40_PAD_COUNT; i++) {
                if (sent_note[i] != 0xFFu) {
                    extern void midi_send_note(uint32_t note, uint32_t vel);
                    apc_note(sent_note[i], 0u);
                    sent_note[i] = 0xFFu;
                }
            }
            was_on = 0;
            last_notes = 0;
        }
    }
    smk_grid();
    smk_pads();
}
#endif

/* ------------------------------------------------------------- boot --- */
#define BOOTGUARD_MAGIC 0x42475244u
struct bootguard_s bootguard __attribute__((section(".noinit")));   /* declared in x0x.c (ota_commit) */

static void hexs(char *b, uint32_t v)
{
    uint32_t i;
    for (i = 0; i < 8u; i++)
        b[i] = "0123456789ABCDEF"[(v >> (28u - 4u * i)) & 15u];
    b[8] = 0;
}

static void fm1_fault(const fm1_crash_t *c)
{
    char b[12];
    uint32_t t0;
    fm1_audio_stop();
    lcd_fill(0, 0, 240, 240, RGB(160, 0, 0));
    draw_text_box(0, 8, 240, &FONT_S, "X0X CRASH", C_WHITE, 1);
    hexs(b, c->vec);
    draw_text_box(10, 40, 220, &FONT_S, b, C_WHITE, 0);
    hexs(b, c->pc);
    draw_text_box(10, 60, 220, &FONT_S, b, C_WHITE, 0);
    hexs(b, c->emu);
    draw_text_box(10, 84, 220, &FONT_S, b, C_WHITE, 0);
    hexs(b, c->dbg);
    draw_text_box(10, 102, 220, &FONT_S, b, C_WHITE, 0);
    hexs(b, c->rets);
    draw_text_box(10, 120, 220, &FONT_S, b, C_WHITE, 0);
    t0 = fm1_ticks();
    while ((uint32_t)(fm1_ticks() - t0) < 4000u * 1000u * FM1_TICKS_PER_US)
        ;
    fm1_reboot();
}

static void enter_uboot(const char *why)
{
    fm1_audio_stop();
    lcd_fill(0, 0, 240, 240, C_BLACK);
    draw_text_box(0, 110, 240, &FONT_S, why, C_WHITE, 1);
    fm1_delay_ms(20);
    usb_detach();
    fm1_delay_ms(30);
    bootguard.pending = 0;                          /* intentional reset: not a failed boot */
    fm1_enter_uboot();
}

/* SAFE MODE: X0X crashed (or hung) twice while starting. Nothing that makes sound runs: no audio,
 * no engine, no UI; USB is on, so the web installer (X0X, or the stock firmware) and M-UPGRADE can
 * reach it. PLAY tries X0X again. The chip's own update mode stays the last resort (fm1_cstart). */
static void safe_main(void)
{
    char b[24];
    uint32_t t0, play;
    lcd_fill(0, 0, 240, 240, C_BLACK);
    draw_text_box(0, 18, 240, &FONT_B, "X0X SAFE MODE", C_HI, 1);
    draw_text_box(0, 52, 240, &FONT_S, "IT CRASHED TWICE", C_WHITE, 1);
    draw_text_box(0, 72, 240, &FONT_S, "WHILE STARTING.", C_WHITE, 1);
    draw_text_box(0, 102, 240, &FONT_S, "NO SOUND. USB IS ON:", C_WHITE, 1);
    draw_text_box(0, 122, 240, &FONT_S, "REINSTALL FROM THE", C_WHITE, 1);
    draw_text_box(0, 142, 240, &FONT_S, "WEB INSTALLER.", C_WHITE, 1);
    draw_text_box(0, 172, 240, &FONT_S, "PLAY: TRY AGAIN", C_HI, 1);
    if (fm1_crash.magic == FM1_CRASH_MAGIC) {
        b[0] = 'P';
        b[1] = 'C';
        b[2] = ' ';
        hexs(b + 3, fm1_crash.pc);
        draw_text_box(0, 200, 240, &FONT_S, b, C_GRAY, 1);
        b[0] = fm1_crash.core == 1 ? '2' : '1';     /* the core, the vector, DBG_MSG: "1 V01 D00001000" */
        b[1] = ' ';
        b[2] = 'V';
        hexs(b + 3, fm1_crash.vec);
        b[3] = b[9];
        b[4] = b[10];
        b[5] = ' ';
        b[6] = 'D';
        hexs(b + 7, fm1_crash.dbg);
        draw_text_box(0, 220, 240, &FONT_S, b, C_GRAY, 1);
    }
    fm1_input_init();
    panel_init();
    usb_start();
    fm1_timer5_start(isr_timer5, 1);
    fm1_guard_lock_top();
    fm1_irq_enable_all();
    play = 1u << panel.btn[B_PLAY];
    t0 = fm1_ms;
    for (;;) {
        fm1_wdt_feed();
        usb_retry(fm1_ms);
        if (bootguard.pending && fm1_ms - t0 > 10000u)
            bootguard.pending = 0;                  /* safe mode itself is up: not another failed boot */
        if ((fm1_in.buttons & play) && fm1_ms - t0 > 500u) {
            bootguard.failed = 0;                   /* try X0X again, from scratch */
            bootguard.pending = 0;
            draw_text_box(0, 172, 240, &FONT_S, "STARTING X0X...", C_HI, 1);
            usb_detach();
            fm1_delay_ms(30);
            fm1_reboot();
        }
        ota_service();
        if (usb.ota_req) {
            usb.ota_req = 0;
            if (flash_ok)
                ota_session();                      /* an install commits and resets; else back here */
        }
        if (usb.uboot_req)
            enter_uboot("UBOOT (USB)");
    }
}

static void fm1_main(void)
{
    int32_t knob = 512 * 16;
    uint32_t last_frame = 0, k;
    uint8_t last_pat = 0xFF;
    {
        uint32_t f = irq_save();
        flash_ok = FL_JEDEC_OK(FL_FAR(fl_jedec_ram)());   /* the expected 1 MiB part, else RAM only */
        irq_restore(f);
        if (flash_ok) {
            fl_plain_window_init();                 /* user sample slots read as plaintext through XIP */
#if BOARD_SMK37
            st_owner_check();                       /* SMK: never parse another ported firmware's store (storage.c) */
#endif
            for (k = 0; k < SMP_USER_SLOTS; k++)
                smp_user_scan(k);
            ota_boot_cleanup();
        }
    }
    lcd_init();
    palette_set(1);
    draw_text_box(0, 96, 240, &FONT_M, "X0X", C_HI, 1);
    draw_text_box(0, 134, 240, &FONT_S, "909 808 303 303 BREAK", C_GRAY, 1);
    if (x0x_dbg.magic != DBG_MAGIC) {
        memset(&x0x_dbg, 0, sizeof x0x_dbg);
        x0x_dbg.magic = DBG_MAGIC;
    }
    if (safe_mode)
        safe_main();                                /* never returns */
    if (fm1_crash.magic == FM1_CRASH_MAGIC && fm1_crash.count != x0x_dbg.crash_seen) {
        char b[16];                                 /* a crash since the last boot: say so, a moment */
        x0x_dbg.crash_seen = fm1_crash.count;
        b[0] = 'P';
        b[1] = 'C';
        b[2] = ' ';
        hexs(b + 3, fm1_crash.pc);
        draw_text_box(0, 176, 240, &FONT_S, fm1_crash.core == 1 ? "THE SECOND CORE CRASHED"
                      : "RESTARTED AFTER A CRASH", RGB(255, 80, 60), 1);
        draw_text_box(0, 196, 240, &FONT_S, b, C_GRAY, 1);
        fm1_wdt_feed();
        fm1_delay_ms(2500);
        fm1_wdt_feed();
    }
    x0x_dbg.boots++;
    x0x_dbg.max_us = 0;
    fm1_input_init();
    fm1_adc_init();
#if BOARD_SMK37
    if (flash_ok)                                   /* the factory key / pad velocity curves (USRTRIM, read only) */
        smk_trim_load(fm1_xip_ptr(SMK_TRIM_FLASH));
#endif
    panel_init();
    led_pos_init();
    plat_perf_init();
    engine_init(proj.pat, &proj.arr.song, proj.arr.lane, &proj.sound);
    if (project_load() != 0)
        project_defaults();                         /* nothing saved yet (or another format) */
    engine_apply_sound(&proj.sound);
    seq.bpm = (float)proj.set.bpm_x10 / 10.0f;
    seq.accent_q7 = proj.set.accent_q7;
    project_apply_io();                             /* clock and notes out, the MIDI channels, the output */
    engine_brk_loops();
#if X0X_BLE
    ble_prepare(proj.set.bt & 1u);                  /* a controller image, BLUETOOTH ON: core 1 starts ready for it */
    ble_start();                                    /* its init on core 0, before core 1 starts (about 0.6 s) */
#endif
    plat_cpu2_set(1);                               /* the second core: at power-on, before the audio and the
                                                     * timers, as Melodee starts it (started later, it does not
                                                     * reach its entry); if it does not answer, one core */
#if X0X_BLE
    ble_attach();                                   /* core 1 runs the controller from now on */
#endif
    audio_init();
    usb_start();
#if X0X_TRS
    uart_midi_init();                               /* before TIMER5: it read-modify-writes port H too */
#endif
    /* above ALNK0 (3): the key / encoder / LED matrix is scanned one column per tick and must keep
     * its rhythm. Below the audio (as Felucca has it), a render that takes most of its 5.8 ms half
     * buffer stopped the scan for milliseconds: fast encoder turns lost their steps, and the column
     * lit when the render began stayed lit through it, so the LEDs flickered with the audio load. */
    fm1_timer5_start(isr_timer5, 4);
    fm1_guard_lock_top();
    fm1_irq_enable_all();
    fm1_delay_ms(30);
#if BOARD_SMK37
    if ((fm1_in.buttons & FM1_OCT_BOTH) == FM1_OCT_BOTH)
        panel_setup();                              /* OCTAVE- + OCTAVE+ held at power-on */
#else
    if ((fm1_in.buttons & 3u) == 3u)
        panel_setup();                              /* OCT- + OCT+ held at power-on */
#endif
    fm1_delay_ms(300);
    lcd_fill(0, 0, 240, 240, C_BLACK);
    ui_init();
#if BOARD_SMK37
    if (st_owner_cleared)
        ui_say("OLD DATA CLEARED", 0);          /* another firmware's store (storage.c st_owner_check); say() has 27 chars */
#endif

    for (;;) {
        fm1_wdt_feed();
        usb_retry(fm1_ms);
        if (fm1_ms > 30000u && bootguard.pending) {     /* a crash or hang in the first 30 s counts */
            bootguard.pending = 0;
            bootguard.failed = 0;
        }
        {
            int32_t b = fm1_adc_read(FM1_ADC_BATT);     /* the battery: a slow average (plat_battery) */
            if (b > 0)
                batt_raw = batt_raw ? batt_raw + (b - batt_raw) / 32 : b;
        }
        {
            int32_t a = fm1_adc_read(FM1_ADC_MASTER);
            if (a >= 0) {
                uint32_t k10;
                knob += (a * 16 - knob) / 8;
                k10 = (uint32_t)(knob / 16);
                master_q12 = (k10 * k10) >> 8;           /* 0 .. ~4096 */
            }
        }
        {   /* OCT- + OCT+ held 5 s: update mode (Felucca's, unchanged) */
            static uint32_t t0, shown;
            uint32_t both = (1u << panel.btn[B_OCTDN]) | (1u << panel.btn[B_OCTUP]);
#if BOARD_SMK37
            if (smk_clip_mode_on()) {
                t0 = fm1_ms;
                shown = 0;
            } else
#endif
            if ((fm1_in.buttons & both) != both) {
                if (shown)
                    ui_say("UPDATE MODE CANCELLED", 0);
                shown = 0;
                t0 = fm1_ms;
            } else if (fm1_ms - t0 > 2000u && fm1_ms - t0 <= 5000u) {
                uint32_t left = (5000u - (fm1_ms - t0) + 999u) / 1000u;
                if (left != shown) {
                    char d[4] = {(char)('0' + left), '.', '.', 0};
                    ui_say("UPDATE MODE IN ", d);
                    shown = left;
                }
            } else if (fm1_ms - t0 > 5000u) {
                enter_uboot("UBOOT");
            }
        }
        ed_service();                               /* sample upload SysEx */
        if (ed_reboot_at && fm1_ms - ed_reboot_at > 200u) {
            bootguard.pending = 0;                  /* intentional reset: not a failed boot */
            usb_detach();                           /* ED_BT_TEST restart: the reply had 200 ms */
            fm1_delay_ms(30);
            fm1_reboot();
        }
        ota_service();                              /* M-UPGRADE handshake */
        if (usb.ota_req) {
            usb.ota_req = 0;
            seq_stop(&seq);
            if (flash_ok)
                ota_session();                      /* returns only if nothing was committed */
            lcd_fill(0, 0, 240, 240, C_BLACK);
            ui_init();
        }
        if (usb.uboot_req)
            enter_uboot("UBOOT (USB)");
#if X0X_CDC
        cdc_task();
#endif
        if (seq.ppat[TRK_BRK] != last_pat) {        /* the break's new pattern may name other loops */
            last_pat = seq.ppat[TRK_BRK];
            engine_brk_loops();
        }
        if (fm1_ms - last_frame >= 16u) {           /* ~60 frames/s at most; input in between */
            last_frame = fm1_ms;
            x0x_dbg.ui_frames++;
            ui_frame();
#if BOARD_SMK37
            smk_frame();                            /* faders, wheels, pads */
            smk_ck_poll();                          /* clip mode keybed */
#endif
        } else {
            ui_input_only();
#if BOARD_SMK37
            smk_wheels();                           /* (the wheels follow the hand, not the screen) */
            smk_grid();
            smk_ck_poll();                          /* (the arp keeps time between frames) */
#endif
        }
    }
}

void fm1_cstart(void)
{
    uint32_t *s, *d, p3, src, wdt;
    fm1_cpu1_halt();                                /* a second core left running by the reset would read
                                                     * the RAM cleared below (Jangada, fm1-x0x#10) */
    fm1_time_init();
    fm1_reset_reason();
    p3 = fm1_boot.p3_rst;
    src = fm1_boot.rst_src;
    wdt = fm1_boot.wdt_con;
    fm1_wdt_arm(0x0D);
    if (bootguard.magic != BOOTGUARD_MAGIC) {
        bootguard.magic = BOOTGUARD_MAGIC;
        bootguard.failed = 0;
        bootguard.pending = 0;
    }
    if (bootguard.pending)
        bootguard.failed++;
    bootguard.pending = 1;
    if (bootguard.failed >= 4u) {                   /* safe mode failed too: the chip's own update mode */
        bootguard.failed = 0;
        bootguard.pending = 0;
        fm1_enter_uboot();
    }
    p3 |= (bootguard.failed >= 2u) << 8;            /* two failed boots: safe mode (kept past the .bss clear) */
    fm1_irq_init();
    irq_stack_mark();                               /* before any interrupt: PERF's IRQ STACK */
    for (d = _bss_start; d < _bss_end; d++)
        *d = 0;
    for (d = _pool_start; d < _pool_end; d++)
        *d = 0;
    for (s = _data_load, d = _data_start; d < _data_end; s++, d++)
        *d = *s;
    for (s = _rt_load, d = _rt_start; d < _rt_end; s++, d++)
        *d = *s;
    for (s = _c1_load, d = _c1_start; d < _c1_end; s++, d++)
        *d = *s;                                    /* the second core's loop (fm1_cpu1.h) */
    fm1_mailbox_clear();
    fm1_guard_enable(FM1_GUARD_STACK | FM1_GUARD_WRITE | FM1_GUARD_BUS | FM1_GUARD_PC);
    safe_mode = (uint8_t)(p3 >> 8);
    fm1_boot.p3_rst = (uint8_t)p3;
    fm1_boot.rst_src = src;
    fm1_boot.wdt_con = (uint8_t)wdt;
    fm1_main();
    for (;;)
        ;
}
