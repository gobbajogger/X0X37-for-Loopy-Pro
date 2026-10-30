/* SPDX-License-Identifier: GPL-3.0-only */
/* Compare the production packed renderer against the original nibble reader. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static void lcd_sync(void) {}
static void lcd_blit(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint16_t *p)
{ (void)x; (void)y; (void)w; (void)h; (void)p; }
#include "../../firmware/src/gfx.c"
#include "font_raw.h"

static int32_t raw_text(int32_t x, int32_t y, const felucca_font_t *f, const char *s, uint16_t colour)
{
    uint32_t red = colour >> 11, green = (colour >> 5) & 63u, blue = colour & 31u;
    for (; *s; s++) {
        uint32_t gi = glyph(f, (uint8_t)*s), w = f->bw[gi], stride = (w + 1) / 2, gy, gx;
        const uint8_t *p = f->data + f->off[gi];
        for (gy = 0; gy < f->h; gy++)
            for (gx = 0; gx < w; gx++) {
                uint32_t alpha = p[gy * stride + gx / 2];
                int32_t px = x - f->pad + (int32_t)gx, py = y + (int32_t)gy + cv_oy;
                alpha = gx & 1 ? alpha & 15 : alpha >> 4;
                if (!alpha || (uint32_t)px >= cv_w || py < cv_y0 || py >= cv_y1)
                    continue;
                if (alpha == 15) cv_px[py * cv_w + px] = swap16(colour);
                else {
                    uint32_t bg = swap16(cv_px[py * cv_w + px]);
                    uint32_t r = (red * alpha + (bg >> 11) * (15 - alpha)) / 15;
                    uint32_t g = (green * alpha + ((bg >> 5) & 63) * (15 - alpha)) / 15;
                    uint32_t b = (blue * alpha + (bg & 31) * (15 - alpha)) / 15;
                    cv_px[py * cv_w + px] = swap16((r << 11) | (g << 5) | b);
                }
            }
        x += f->adv[gi];
    }
    return x;
}

int main(void)
{
    static const felucca_font_t *const packed[] = {&FONT_T, &FONT_XS, &FONT_S, &FONT_B, &FONT_M, &FONT_L};
    static const felucca_font_t *const raw[] = {&RAW_FONT_T, &RAW_FONT_XS, &RAW_FONT_S, &RAW_FONT_B, &RAW_FONT_M, &RAW_FONT_L};
    static uint16_t reference[240 * 80];
    uint32_t role, ch, variant, cases = 0;
    for (role = 0; role < sizeof packed / sizeof *packed; role++) {
        const felucca_font_t *f = packed[role], *r = raw[role];
        assert(f->h == r->h && f->pad == r->pad && f->first == r->first && f->last == r->last);
        assert(!memcmp(f->bw, r->bw, f->last - f->first + 1));
        assert(!memcmp(f->adv, r->adv, f->last - f->first + 1));
        for (ch = 1; ch < 256; ch++) {
            char s[4] = {(char)ch, '8', (char)ch, 0};
            /* The number-only L face has no '?' fallback; its callers use its
             * declared range. Other faces also exercise fallback and folding. */
            if ((f->first > '?' || f->last < '?') && (ch < f->first || ch > f->last))
                continue;
            for (variant = 0; variant < 4; variant++) {
                int32_t x = variant == 1 ? -6 : variant == 2 ? 227 : 7;
                int32_t y = variant == 3 ? -9 : 2, end;
                uint16_t bg = variant & 1 ? RGB(71, 125, 218) : RGB(26, 30, 33);
                uint16_t fg = variant & 2 ? RGB(247, 133, 62) : C_WHITE;
                cv_begin(240, 80, bg);
                if (variant == 3) cv_band(3, 13);
                end = raw_text(x, y, r, s, fg);
                memcpy(reference, cv_px, sizeof reference);
                cv_begin(240, 80, bg);
                if (variant == 3) cv_band(3, 13);
                assert(cv_text(x, y, f, s, fg) == end);
                if (memcmp(reference, cv_px, sizeof reference)) {
                    fprintf(stderr, "font role %u character %u variant %u differs\n", role, ch, variant);
                    return 1;
                }
                cases++;
            }
        }
    }
    printf("font packing: %u rendered cases retain identical pixels and metrics\n", cases);
    return 0;
}
