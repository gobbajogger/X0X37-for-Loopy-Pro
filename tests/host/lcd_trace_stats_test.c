#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#define BOARD_SMK37 0
#define X0X_LCD_TRACE 1
#define FM1_TICKS_PER_US 24u
static uint32_t tick;
static uint32_t fm1_ticks(void) { return tick; }
#include "../../firmware/src/app/lcd_trace.h"
int main(void)
{
    for(uint32_t i=0;i<64;i++)lcd_trace_record(LCD_T_AUDIO,i);
    assert(lcd_trace_percentile(LCD_T_AUDIO,50)==31u);
    assert(lcd_trace_percentile(LCD_T_AUDIO,95)==60u);
    assert(lcd_trace_hist[LCD_T_AUDIO].worst==63u);
    tick=24u*10u;
    lcd_trace_input_tick(1, 2u, 4u);
    tick=24u*12u;
    lcd_trace_dispatch();
    assert(lcd_trace_percentile(LCD_T_KEY,50)==2u);
    assert(lcd_trace_percentile(LCD_T_MIDI,95)==2u);
    lcd_trace_pixels(480u);
    tick=24u*20u;
    lcd_trace_frame();
    assert(lcd_trace_last_frame_bytes==480u);
    lcd_trace_dma_begin(1536u);
    tick+=24u*1024u;
    lcd_trace_dma_done();
    assert(lcd_trace_percentile(LCD_T_DMA,50)==1024u);
    puts("ok lcd-trace-stats: P50/P95/worst, key/MIDI arrival-dispatch, frame bytes and DMA duration");
    return 0;
}
