#define PADLED_N 16u
#define PADLED_LATCH 250u
static uint8_t padled_buf[PADLED_LATCH + PADLED_N * 24u] SMK_BIG;   /* (zeroed at boot: the latch stays 0) */
static uint32_t padled_rgb[PADLED_N];
static uint8_t padled_dirty = 1;
/* hardware pad (0..7 top row, 8..15 bottom row, left to right; smk_input.h SMK_PAD_OF_WHITE) -> position in
 * the chain (stock table) */
static const uint8_t PADLED_SLOT[PADLED_N] = {0, 15, 14, 13, 12, 11, 10, 9, 1, 2, 3, 4, 5, 6, 7, 8};

static void padled_set(uint32_t pad, uint8_t r, uint8_t g, uint8_t b)
{
    uint32_t c = (uint32_t)g << 16 | (uint32_t)r << 8 | b;
    if (pad < PADLED_N && padled_rgb[pad] != c) {
        padled_rgb[pad] = c;
        padled_dirty = 1;
    }
}

/* a76: the GRB colour of a hardware pad in the last frame sent (padled_buf), for ED_INLOG */
static uint32_t padled_sent(uint32_t pad)
{
    const uint8_t *o = padled_buf + PADLED_LATCH + PADLED_SLOT[pad % PADLED_N] * 24u;
    uint32_t v = 0, i;
    for (i = 0; i < 24u; i++)
        v = v << 1 | (o[i] == 0xF0u);
    return v;
}

static void padled_flush(void)
{
    uint32_t p, i;
    if (!padled_dirty)
        return;
    padled_dirty = 0;
    for (p = 0; p < PADLED_N; p++) {
        uint8_t *o = padled_buf + PADLED_LATCH + PADLED_SLOT[p] * 24u;
        for (i = 0; i < 24u; i++)
            o[i] = (padled_rgb[p] >> (23u - i)) & 1u ? 0xF0u : 0xC0u;
    }
    lcd_sync();
    fm1_padled_send(padled_buf, sizeof padled_buf);
    fm1_lcd_wait();
    fm1_padled_done();
}
