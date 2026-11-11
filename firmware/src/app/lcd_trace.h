/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef X0X_LCD_TRACE_HEADER_H
#define X0X_LCD_TRACE_HEADER_H
#ifndef X0X_LCD_TRACE
#define X0X_LCD_TRACE 0
#endif

/* Opt-in bench only: X0X_LCD_TRACE=1. All symbols disappear when disabled. */
#if X0X_LCD_TRACE
#define LCD_TRACE_SAMPLES 32u
enum { LCD_T_KEY, LCD_T_MIDI, LCD_T_DMA, LCD_T_FRAME, LCD_T_TIMER5, LCD_T_AUDIO, LCD_T_COUNT };
typedef struct {
    volatile uint32_t samples[LCD_TRACE_SAMPLES];
    volatile uint32_t total, worst;
} lcd_trace_hist_t;
/* The opt-in ring needs 816 bytes of .bss (6 x (32 samples + counters)).
 * Do not allocate it from .pool: the SMK production pool has only 136 B above
 * the mandatory 8,192-B minimum. 32 samples keep the trace build linkable. */
static lcd_trace_hist_t lcd_trace_hist[LCD_T_COUNT];
static volatile uint32_t lcd_trace_key_at, lcd_trace_midi_at;
static volatile uint8_t lcd_trace_key_pending, lcd_trace_midi_pending;
static volatile uint64_t lcd_trace_seen_keys;
static volatile uint32_t lcd_trace_seen_notes, lcd_trace_seen_midi;
static volatile uint32_t lcd_trace_dma_at, lcd_trace_dma_size;
static volatile uint32_t lcd_trace_frame_at, lcd_trace_frame_bytes, lcd_trace_last_frame_bytes;
static volatile uint32_t lcd_trace_max_frame_bytes;

/* O(1) interrupt-safe telemetry. No interrupt masking or queue changes. */
static void lcd_trace_record(unsigned kind, uint32_t microseconds)
{
    lcd_trace_hist_t *h = &lcd_trace_hist[kind];
    uint32_t n = h->total;
    h->samples[n % LCD_TRACE_SAMPLES] = microseconds;
    if (microseconds > h->worst) h->worst = microseconds;
    h->total = n + 1u;
}
static void lcd_trace_input_tick(uint64_t keys, uint32_t notes, uint32_t midi_write)
{
    uint32_t now = fm1_ticks();
    if (keys != lcd_trace_seen_keys || notes != lcd_trace_seen_notes) {
        if (!lcd_trace_key_pending) lcd_trace_key_at = now;
        lcd_trace_key_pending = 1u;
        lcd_trace_seen_keys = keys;
        lcd_trace_seen_notes = notes;
    }
    if (midi_write != lcd_trace_seen_midi) {
        if (!lcd_trace_midi_pending) lcd_trace_midi_at = now;
        lcd_trace_midi_pending = 1u;
        lcd_trace_seen_midi = midi_write;
    }
}
static void lcd_trace_dispatch(void)
{
    uint32_t now = fm1_ticks();
    if (lcd_trace_key_pending) {
        uint32_t since = lcd_trace_key_at;
        lcd_trace_key_pending = 0u;
        lcd_trace_record(LCD_T_KEY, (now - since) / FM1_TICKS_PER_US);
    }
    if (lcd_trace_midi_pending) {
        uint32_t since = lcd_trace_midi_at;
        lcd_trace_midi_pending = 0u;
        lcd_trace_record(LCD_T_MIDI, (now - since) / FM1_TICKS_PER_US);
    }
}
static void lcd_trace_dma_begin(uint32_t bytes)
{
    lcd_trace_dma_size = bytes;
    lcd_trace_dma_at = fm1_ticks();
}
static void lcd_trace_dma_done(void)
{
    if (lcd_trace_dma_size > 8u)
        lcd_trace_record(LCD_T_DMA, (fm1_ticks() - lcd_trace_dma_at) / FM1_TICKS_PER_US);
}
static void lcd_trace_pixels(uint32_t bytes) { lcd_trace_frame_bytes += bytes; }
static void lcd_trace_frame(void)
{
    uint32_t now = fm1_ticks();
    if (lcd_trace_frame_at)
        lcd_trace_record(LCD_T_FRAME, (now - lcd_trace_frame_at) / FM1_TICKS_PER_US);
    lcd_trace_frame_at = now;
    lcd_trace_last_frame_bytes = lcd_trace_frame_bytes;
    if (lcd_trace_frame_bytes > lcd_trace_max_frame_bytes)
        lcd_trace_max_frame_bytes = lcd_trace_frame_bytes;
    lcd_trace_frame_bytes = 0u;
}
/* Exact nearest-rank P50/P95 of the last 32 completed samples, in microseconds. */
static uint32_t lcd_trace_percentile(unsigned kind, unsigned percent)
{
    const lcd_trace_hist_t *h = &lcd_trace_hist[kind];
    uint32_t sorted[LCD_TRACE_SAMPLES];
    uint32_t n = h->total < LCD_TRACE_SAMPLES ? h->total : LCD_TRACE_SAMPLES;
    for (uint32_t i = 0; i < n; ++i) {
        uint32_t v = h->samples[i], j = i;
        while (j && sorted[j - 1u] > v) { sorted[j] = sorted[j - 1u]; --j; }
        sorted[j] = v;
    }
    if (!n) return 0u;
    uint32_t rank = (percent * n + 99u) / 100u;
    return sorted[rank ? rank - 1u : 0u];
}
#endif
#endif
