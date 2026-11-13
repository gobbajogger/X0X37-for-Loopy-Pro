static uint8_t knob_bank_prev;
static uint32_t clip_pad_keys;
static uint8_t clip_pad_note[SMK_APC40_PAD_COUNT], clip_pad_active;
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
/* The pad colour of each APC40 velocity: the owner-calibrated table (tools/apc_pad_colors.py --pad-name
 * APC40_PAD_OWNER, from the owner export of 2026-10-17). Dev and release builds use it (BEHAVIOR.md CLP-13). */
#include "app/apc40_pad_owner.h"
/* CLP-18: the link that wrote each clip cell last. Bit n set: Bluetooth wrote cell n last; bit n clear: USB.
 * Bits 40 and 41: the same for the PLAY and REC LED feedback.
 * The last writer owns the cell: when its link ends, the cell clears, and the cells of the other link stay.
 * smk_apc_ble_event sets apc_in_ble while it stores a Bluetooth message (the same interrupt as usb.c). */
#define APC_OWN_PLAY 40u
static uint64_t apc_ble_owned;
static uint8_t apc_in_ble;
static volatile uint8_t apc_link_host;    /* CLP-80: bit 0 USB, bit 1 Bluetooth: its host sent clip feedback */
static void apc_own(uint32_t bit)
{
    apc_link_host |= (uint8_t)(1u << apc_in_ble);   /* CLP-80: this link has a Loopy host */
    apc_ble_owned = (apc_ble_owned & ~((uint64_t)1u << bit)) | (uint64_t)apc_in_ble << bit;
}
void smk_apc_led_set(uint8_t note, uint8_t type, uint8_t velocity)
{
    if (note >= SMK_APC40_CLIP_COUNT)
        return;
    apc_own(note);
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
    /* Store feedback on either link. Outside clip mode, only a link that completed
     * the APC handshake owns these messages; the other link still plays notes. */
    uint8_t owned = (uint8_t)(clip_grid_mode || (apc_host_on & (1u << apc_in_ble)));
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
        apc_own(APC_OWN_PLAY + (d1 == SMK_APC40_RECORD_NOTE));   /* CLP-18: bit 40 PLAY, bit 41 REC */
        smk_apc_transport_led_set(d1, d2);
        return owned;
    }
    /* attempt 35: the other APC40 mkII LED notes (track, scene, device and bank buttons, notes 0-98,
     * any channel: blink and pulse rates come on channels 2-16): kept from the sound engine in clip mode */
    if (d1 <= 98u)
        return owned;
    return 0u;
}
#if BOARD_SMK37
/* a76: the inbound recorder (app/smk_inlog.h). usb.c and ble.c give it each APC40 LED message that
 * came in. ble.c also gives it the link and notify edges. */
#include "app/smk_inlog.h"
static smk_inlog_t smk_inlog_g;   /* in .bss: the Elite pool keeps its 8 KiB headroom (the gate) */
void smk_inlog(uint32_t src, uint32_t pkt)
{
    smk_inlog_put(&smk_inlog_g, fm1_ms, src, pkt);
}
static void smk_inlog_w(uint32_t v, uint32_t nb)   /* one word, nb x 7 bits, low bits first */
{
    while (nb--) {
        ed_b(v);
        v >>= 7;
    }
}
/* INLOG 6: the CPU0 main loop copies existing input state without masking IRQs.
 * Only its TIMER5 ISR writes these samples. That ISR completes before the main
 * loop resumes and increments adc_ok or adc_miss before updating inputs. Reject
 * a copy crossed by a tick, at most three times. Copy 176 bytes to the existing
 * call-scoped editor scratch; serialize later, outside the short coherence window.
 * The scratch is not shared with the ISR (x0x.c), and curves are boot-immutable.
 * Latest mux samples are not simultaneous analog measurements. No scan-path work
 * or persistent RAM is added. The 521-byte payload fits the existing ed_out. */
static void smk_pad_inputs_reply(void)
{
    uint8_t *s = ed_smp_buf;
    uint32_t attempt, i, j, ok = 0, miss = 0, frames = 0, now = 0, notes = 0;
    _Static_assert(SMK_NPAD == 16u, "pad snapshot v1 requires 16 pads");
    _Static_assert(176u <= ED_SMP_BUF_SIZE, "pad snapshot fits the call-scoped editor scratch");
    for (attempt = 0; attempt < 3u; attempt++) {
        ok = fm1_in.adc_ok;
        miss = fm1_in.adc_miss;
        frames = fm1_in.frames;
        RING_PUBLISH();
        now = fm1_ms;
        notes = fm1_in.pnotes;
        for (i = 0; i < SMK_NPAD; i++) {
            uint32_t at = i * 10u;
            uint16_t raw = fm1_in.pad_raw[i], base = fm1_in.pad_base[i], peak = fm1_in.pad_peak[i];
            s[at] = (uint8_t)raw;
            s[at + 1u] = (uint8_t)(raw >> 8);
            s[at + 2u] = (uint8_t)base;
            s[at + 3u] = (uint8_t)(base >> 8);
            s[at + 4u] = (uint8_t)peak;
            s[at + 5u] = (uint8_t)(peak >> 8);
            s[at + 6u] = fm1_in.pad_st[i];
            s[at + 7u] = fm1_in.pad_vel[i];
            s[at + 8u] = fm1_in.pad_press[i];
            s[at + 9u] = smk_pad_note[i];
        }
        for (i = 0; i < 8u; i++) {
            uint16_t v = i < 4u ? fm1_in.fad_raw[i] : i < 6u ? fm1_in.wheel[i - 4u]
                                                      : i == 6u ? fm1_in.pedal : fm1_in.batt;
            s[160u + 2u * i] = (uint8_t)v;
            s[161u + 2u * i] = (uint8_t)(v >> 8);
        }
        RING_PUBLISH();
        if (ok == fm1_in.adc_ok && miss == fm1_in.adc_miss && frames == fm1_in.frames)
            break;
    }
    ed_b(6u);
    ed_b(1u);                                      /* schema version */
    ed_b(attempt == 3u);                            /* status: snapshot / busy */
    if (attempt == 3u)
        return;                                    /* no torn/partial measurements */
    ed_b(SMK_NPAD);
    ed_b(smk_pad_grid | (uint32_t)smk_pad_piano << 1);
    ed_b(SMK_PAD_ON);
    ed_b(SMK_PAD_OFF);
    ed_b(SMK_PAD_PEAKN);
    smk_inlog_w(now, 5u);
    smk_inlog_w(frames, 5u);
    smk_inlog_w(ok, 5u);
    smk_inlog_w(miss, 5u);
    smk_inlog_w(notes, 5u);
    for (i = 0; i < 8u; i++)
        smk_inlog_w(s[160u + 2u * i] | (uint32_t)s[161u + 2u * i] << 8, 3u);
    for (i = 0; i < SMK_NPAD; i++) {
        const uint8_t *p = s + i * 10u;
        for (j = 0; j < 3u; j++)                   /* raw, base, peak */
            smk_inlog_w(p[2u * j] | (uint32_t)p[2u * j + 1u] << 8, 3u);
        smk_inlog_w(p[6], 2u);                     /* state */
        ed_b(p[7]);                               /* last velocity */
        ed_b(p[8]);                               /* pressure */
        ed_b(p[9]);                               /* mapped note + 1, or 0 */
        for (j = 0; j < 5u; j++)
            smk_inlog_w(smk_trim_pad[i][j], 3u);    /* active curves, not a flash read */
    }
}
/* The reply of editor command 38 INLOG (editor.c, web/EDITOR_PROTOCOL.md). Arg 1: the pad LED output
 * (lcd.c): the colour to send and the last frame sent. Args 2-5: the pad calibration export (CLP-79).
 * Arg 6: the read-only pad input snapshot. Else: the entries and the stored clip cells. */
static void smk_inlog_reply(uint32_t arg)
{
    uint32_t i;
    if (arg == 6u) {
        smk_pad_inputs_reply();
        return;
    }
#if X0X_DEV_PADCAL
    if (arg - 2u < 128u / SMK_PADCAL_CHUNK) {     /* args 2-5: the pad calibration, 32 entries each (CLP-79) */
        const uint8_t *c = padcal.rgb[(arg - 2u) * SMK_PADCAL_CHUNK];
        ed_b(arg);
        for (i = 0; i < 3u * SMK_PADCAL_CHUNK; i++)     /* R, G, B of each entry, 2 x 7 bits */
            smk_inlog_w(c[i], 2u);
        for (i = 0; i < SMK_PADCAL_CHUNK / 8u; i++)     /* then the ok bits of the 32 entries */
            smk_inlog_w(padcal.ok[(arg - 2u) * (SMK_PADCAL_CHUNK / 8u) + i], 2u);
        smk_inlog_w(padcal.tl, 2u);                  /* the orientation correction */
        return;
    }
#endif
    if (arg == 1u) {
        ed_b(1u);
        ed_b(padled_dirty);
        for (i = 0; i < 2u * PADLED_N; i++)
            smk_inlog_w(i < PADLED_N ? padled_rgb[i] : padled_sent(i - PADLED_N), 4u);
        return;
    }
    ed_b(smk_inlog_g.n);
    smk_inlog_w(fm1_ms, 5u);
    for (i = 0; i < 2u * smk_inlog_g.n; i++)
        smk_inlog_w(smk_inlog_at(&smk_inlog_g, i), 5u);
    for (i = 0; i < 2u * SMK_APC40_CLIP_COUNT; i++)
        ed_b(i < SMK_APC40_CLIP_COUNT ? apc_led_velocity[i] : apc_led_channel[i - SMK_APC40_CLIP_COUNT]);
}
#endif
/* a76: ble.c gives each Bluetooth note and CC here. While notify is off, a note for a clip cell
 * (the pad colours) does not change the cell (BEHAVIOR.md CLP-18). It gets the same answer as
 * before, so X0X does not drop the note. All other messages go to smk_apc_usb_event as before. */
uint8_t smk_apc_ble_event(uint32_t pkt, uint8_t notifying)
{
    uint8_t r;
    if (smk_apc40_ble_colour_unheard(pkt, notifying)) {
        smk_inlog(3u, pkt);
        return (uint8_t)(clip_grid_mode || (apc_host_on & 2u));
    }
    smk_inlog(1u, pkt);
    apc_in_ble = 1u;
    r = smk_apc_usb_event(pkt);
    apc_in_ble = 0u;
    return r;
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
/* Track the handshake by input link. A USB Loopy session must not consume notes
 * arriving from a Bluetooth instrument, or vice versa. */
void smk_apc_host_seen(void)
{
    apc_host_on |= 1u;
}
void smk_apc_ble_host_seen(void) { apc_host_on |= 2u; }
/* CLP-18: clear the clip cells and the PLAY and REC LED feedback of one link (ble 1: those that Bluetooth
 * wrote last, 0: those of USB), and the feedback burst. If a cell stays lit, the latched layout stays, so the pads of the other link do not move,
 * and the next frame draws them. If no cell stays lit, new feedback latches the layout again, and the pad
 * colour store (lcd.c padled_rgb) goes dark. Return 1 if a cell stays lit. */
static uint8_t smk_apc_link_clear(uint8_t ble)
{
    uint32_t i, left = 0u;
    uint64_t m = ble ? apc_ble_owned : ~apc_ble_owned;
    apc_link_host &= (uint8_t)~(1u << ble);       /* CLP-80: this link has no Loopy host now */
    smk_apc40_cells_clear_mask(apc_led_velocity, apc_led_channel, apc_cell_flags, apc_cell_tick, m);
    if ((m >> APC_OWN_PLAY) & 1u)
        apc_play_feedback = 0u;
    if ((m >> (APC_OWN_PLAY + 1u)) & 1u)
        apc_record_feedback = 0u;
    apc_burst_mask = 0u;
    apc_burst_count = 0u;
    for (i = 0; i < SMK_APC40_CLIP_COUNT; i++)
        left |= apc_led_velocity[i];
    if (!left) {
        apc_layout_locked = apc_layout_pending = 0u;   /* new feedback latches the layout again */
        for (i = 0; i < PADLED_N; i++)
            padled_rgb[i] = 0u;                    /* the next frame sends dark pads */
        padled_dirty = 1u;
    }
    return (uint8_t)(left != 0u);
}
void smk_apc_host_lost(void)
{
    apc_host_on &= (uint8_t)~1u;
    smk_ck_midi_stop();                              /* clip keys: all notes off (smk_ck_poll) */
    apc_clock_count = apc_clock_last_ms = apc_clock_period_ms = 0u;
    apc_clock_seen = 0u;
    if (!smk_apc_link_clear(0u)) {                   /* CLP-18: only the USB cells, PLAY and REC */
        apc_cols = SMK_APC40_SOURCE_COLS;
        apc_rows = 2u;
        apc_origin_col = apc_origin_row = 0u;
        apc_view_has_clips = 0u;
        apc_view = 0u;
    }
}
/* usb.c usb_poll calls this each poll with the USB link state (configured, not suspended). When the link
 * ends (cable out, the host sleeps), the cells that USB wrote clear (CLP-18). apc_host_on and the notes do
 * not change here. tests/host/smk_clip_usb_clear_test.sh */
void smk_apc_usb_link(uint8_t up)
{
    static uint8_t was;
    if (was && !up)
        (void)smk_apc_link_clear(0u);
    was = up;
}
/* CLP-18: the APC40 mkII introduction with mode 0x40 (generic mode, F0 47 7F 29 60 00 04 40 .. F7) gives the
 * controller back to its own LED control: the LED feedback of that host ends. usb.c (ble 0) and ble.c (ble 1)
 * call this; the cells of that link clear. Feedback after it shows as before. */
void smk_apc_host_generic(uint8_t ble)
{
    (void)smk_apc_link_clear(ble);
}
/* ble.c ble_service calls this after each event poll (the same interrupt context as the cell
 * stores). If the Bluetooth link ended or the central turned notify off, clear the clip feedback that
 * Bluetooth wrote: those pads and LCD cells go dark and no such clip blinks. The USB cells stay (CLP-18).
 * Clip mode and the sequencer do not change.
 * a76: the clear runs again 300 ms later (smk_apc40_lost_step). tests/smk_clip_dark_test.sh */
void smk_apc_ble_link(uint8_t connected, uint8_t notifying)
{
    static smk_apc40_lost_t lost;
    if (!smk_apc40_lost_step(&lost, connected, notifying, fm1_ms))
        return;
    apc_host_on &= (uint8_t)~2u;
    (void)smk_apc_link_clear(1u);                    /* CLP-18: only the Bluetooth cells, PLAY and REC */
}
/* Clip grid on the LCD (main loop only, from ui.c draw_main). It shows the 8x5 APC
 * cells: lit cells in the LCD colour (APC40_LCD565: the Akai colour as plain RGB565), unlit cells dim, queued cells with a white
 * dot. A white frame marks each cell that is on the 16 pads now (the OCT view, or the
 * full 4x4). It redraws when the cells or the view change, at most every 64 ms.
 * It uses the existing canvas (gfx.c cv_px): no new RAM buffer. */
#define SMK_CLIP_LCD_MIN_MS 64u
static uint8_t was_on;   /* 0: the next smk_clip_lcd_draw draws at once */
static uint32_t ck_rpt_st;   /* < >: the next repeat and the arm bit (ck_rpt) */
void smk_clip_lcd_force(void) { was_on = 0u; }
static uint32_t ck_page_on(void);
/* CLP-80: 1 = no link has a Loopy host and no clip keys page shows: ui.c draw_main shows the waiting
 * screen, not the clip grid. A link has a host from the first clip or PLAY / REC feedback message on it
 * (apc_own) until smk_apc_link_clear for that link (link end, generic mode, USB host lost). */
uint8_t smk_clip_wait(void)
{
    return (uint8_t)(!apc_link_host && !ck_page_on());
}
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
                /* The Akai colour as plain RGB565. An idle shade: half of the full shade (CLP-05). */
                uint16_t c = smk_clip_lcd565(v);
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
