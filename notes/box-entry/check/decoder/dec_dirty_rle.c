/* Representative dirty-span + pixel-RLE decoder for size measurement.
 * Stream: repeating [u16 be skip][u16 be copy][u16 be rle_len][rle payload]
 * until skip+copy consume 57600 pixels. RLE: c<128 => c+1 repeats of next BE pixel;
 * c>=128 => (c-127) literal BE pixels. Output is written line-by-line into a
 * 240-pixel RAM line (device: cv_px / lcd_fillbuf). */
#include <stdint.h>

static uint16_t line[240];
static volatile uint32_t sink;

static uint8_t rd8(const uint8_t **p, const uint8_t *e)
{
    if (*p >= e) return 0;
    return *(*p)++;
}
static uint16_t rd16(const uint8_t **p, const uint8_t *e)
{
    uint32_t hi = rd8(p, e);
    return (uint16_t)((hi << 8) | rd8(p, e));
}

/* Decode one frame into 240-wide lines; *out_lines is a stand-in for lcd_blit. */
uint32_t boxent_decode_dirty_rle(const uint8_t *src, uint32_t n, uint16_t *out_lines, uint32_t stride)
{
    const uint8_t *p = src, *e = src + n;
    uint32_t pix = 0, written = 0;
    uint32_t x = 0, y = 0;
    while (pix < 240u * 240u && p < e) {
        uint32_t skip = rd16(&p, e);
        uint32_t copy = rd16(&p, e);
        uint32_t rlen = rd16(&p, e);
        const uint8_t *re = p + rlen;
        if (re > e) re = e;
        pix += skip;
        /* advance x,y over skipped (already on screen) */
        x += skip;
        while (x >= 240u) { x -= 240u; y++; }
        while (copy && p < re) {
            uint32_t c = rd8(&p, re);
            uint32_t cnt, lit, k;
            uint16_t v;
            if (c < 128u) {
                cnt = c + 1u;
                v = rd16(&p, re);
                for (k = 0; k < cnt && copy; k++, copy--) {
                    if (x < 240u && y < 240u)
                        line[x] = v;
                    x++;
                    if (x == 240u) {
                        /* one complete dirty line segment: copy out */
                        for (k = 0; k < 240u; k++)
                            out_lines[y * stride + k] = line[k];
                        written += 240u;
                        x = 0;
                        y++;
                    }
                }
            } else {
                lit = c - 127u;
                for (k = 0; k < lit && copy; k++, copy--) {
                    v = rd16(&p, re);
                    if (x < 240u && y < 240u)
                        line[x] = v;
                    x++;
                    if (x == 240u) {
                        uint32_t t;
                        for (t = 0; t < 240u; t++)
                            out_lines[y * stride + t] = line[t];
                        written += 240u;
                        x = 0;
                        y++;
                    }
                }
            }
        }
        p = re;
    }
    if (x && y < 240u) {
        uint32_t t;
        for (t = 0; t < 240u; t++)
            out_lines[y * stride + t] = line[t];
        written += 240u;
    }
    sink = written;
    return written;
}
