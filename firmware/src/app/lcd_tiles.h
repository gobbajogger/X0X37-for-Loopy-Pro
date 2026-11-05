/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef X0X_LCD_TILES_H
#define X0X_LCD_TILES_H
#include <stdint.h>

/* SPI1 is 12 MHz on SMK-37; 1536 pixel bytes occupy 1.024 ms on the wire. */
#define X0X_LCD_TILE_W 16u
#define X0X_LCD_TILE_H 16u
#define X0X_LCD_WIDTH 240u
#define X0X_LCD_TILE_COLS (X0X_LCD_WIDTH / X0X_LCD_TILE_W)
#define X0X_LCD_MAX_COLS_PER_DMA 3u
#define X0X_LCD_MAX_DMA_BYTES (X0X_LCD_TILE_W * X0X_LCD_TILE_H * X0X_LCD_MAX_COLS_PER_DMA * 2u)

typedef void (*x0x_lcd_tile_emit_fn)(void *context, const uint16_t *pixels,
                                      uint32_t stride, uint32_t x, uint32_t y,
                                      uint32_t w, uint32_t h);

/* Hash the actual pixels of one tile. Last strip may be shorter than 16 rows. */
static uint32_t x0x_lcd_tile_hash(const uint16_t *px, uint32_t stride,
                                  uint32_t x, uint32_t y, uint32_t h)
{
    uint32_t hash = 2166136261u;
    for (uint32_t row = 0; row < h; ++row) {
        const uint16_t *p = px + (y + row) * stride + x;
        for (uint32_t col = 0; col < X0X_LCD_TILE_W; ++col)
            hash = (hash ^ p[col]) * 16777619u;
    }
    return hash;
}

/* Pack adjacent dirty tiles into one DMA buffer without overwriting the canvas. */
static void x0x_lcd_tile_pack(uint16_t *out, const uint16_t *px, uint32_t stride,
                              uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
    for (uint32_t row = 0; row < h; ++row) {
        const uint16_t *src = px + (y + row) * stride + x;
        for (uint32_t col = 0; col < w; ++col)
            out[row * w + col] = src[col];
    }
}

/* Emit each changed 16x16 tile exactly once; merge only adjacent changed columns.
 * A transfer contains at most 3 tiles or 1536 bytes. The emitter owns DMA
 * buffer lifetime and foreground input service between transfers. */
static void x0x_lcd_tile_commit(const uint16_t *px, uint32_t rows,
                                 uint32_t *last_hashes, int force,
                                 x0x_lcd_tile_emit_fn emit, void *context)
{
    for (uint32_t row = 0; row < rows; row += X0X_LCD_TILE_H) {
        const uint32_t h = rows - row < X0X_LCD_TILE_H ? rows - row : X0X_LCD_TILE_H;
        uint32_t dirty = 0u;
        for (uint32_t col = 0; col < X0X_LCD_TILE_COLS; ++col) {
            const uint32_t hash = x0x_lcd_tile_hash(px, X0X_LCD_WIDTH,
                                                     col * X0X_LCD_TILE_W, row, h);
            uint32_t *old = &last_hashes[(row / X0X_LCD_TILE_H) * X0X_LCD_TILE_COLS + col];
            if (force || hash != *old)
                dirty |= 1u << col;
            *old = hash;
        }
        for (uint32_t col = 0; col < X0X_LCD_TILE_COLS;) {
            if (!(dirty & (1u << col))) { ++col; continue; }
            const uint32_t begin = col++;
            while (col < X0X_LCD_TILE_COLS && col - begin < X0X_LCD_MAX_COLS_PER_DMA &&
                   (dirty & (1u << col)))
                ++col;
            emit(context, px, X0X_LCD_WIDTH, begin * X0X_LCD_TILE_W, row,
                 (col - begin) * X0X_LCD_TILE_W, h);
        }
    }
}
#endif
