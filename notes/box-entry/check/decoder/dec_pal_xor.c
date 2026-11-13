/* Representative 16-color index XOR-RLE decoder for size measurement.
 * Palette: 16 x u16 BE. Each frame: byte RLE of XOR'd 8-bit indices against the
 * previous index buffer (first frame XOR of 0). RLE: c<128 => c+1 repeats of
 * next byte; c>=128 => (c-127) literal bytes. Indices live in a 57600-byte
 * previous buffer (fits in existing cv_px during boot). Each line is expanded
 * through the palette into a 240-pixel RAM line for lcd_blit. */
#include <stdint.h>

static uint16_t pal[16];
static uint8_t idx[240 * 240];
static uint16_t line[240];
static volatile uint32_t sink;

static uint8_t rd8(const uint8_t **p, const uint8_t *e)
{
    if (*p >= e) return 0;
    return *(*p)++;
}

uint32_t boxent_decode_pal_xor(const uint8_t *src, uint32_t n, uint16_t *out_lines, uint32_t stride)
{
    const uint8_t *p = src, *e = src + n;
    uint32_t i, written = 0;
    for (i = 0; i < 16u && p + 1u < e; i++) {
        uint32_t hi = rd8(&p, e);
        pal[i] = (uint16_t)((hi << 8) | rd8(&p, e));
    }
    i = 0;
    while (i < 240u * 240u && p < e) {
        uint32_t c = rd8(&p, e), k, cnt;
        uint8_t v;
        if (c < 128u) {
            cnt = c + 1u;
            v = rd8(&p, e);
            for (k = 0; k < cnt && i < 240u * 240u; k++, i++)
                idx[i] ^= v;
        } else {
            cnt = c - 127u;
            for (k = 0; k < cnt && i < 240u * 240u; k++, i++)
                idx[i] ^= rd8(&p, e);
        }
    }
    for (i = 0; i < 240u; i++) {
        uint32_t x;
        for (x = 0; x < 240u; x++) {
            uint8_t t = idx[i * 240u + x] & 15u;
            line[x] = pal[t];
        }
        for (x = 0; x < 240u; x++)
            out_lines[i * stride + x] = line[x];
        written += 240u;
    }
    sink = written;
    return written;
}
