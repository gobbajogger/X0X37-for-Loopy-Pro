#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../../firmware/src/app/lcd_tiles.h"

#define W 240u
#define H 138u
#define NC (W/X0X_LCD_TILE_W)
#define NR ((H+X0X_LCD_TILE_H-1u)/X0X_LCD_TILE_H)
static uint16_t canvas[W*H], panel[W*H], dma[X0X_LCD_MAX_DMA_BYTES/2u];
static uint32_t hashes[NC*NR];
static unsigned sent, max_bytes, dispatches, operations;

static void emit(void *ctx,const uint16_t *src,uint32_t stride,uint32_t x,uint32_t y,uint32_t w,uint32_t h)
{
    (void)ctx;
    const unsigned bytes=w*h*2u;
    assert(bytes<=X0X_LCD_MAX_DMA_BYTES);
    x0x_lcd_tile_pack(dma,src,stride,x,y,w,h);
    for(unsigned yy=0;yy<h;++yy) for(unsigned xx=0;xx<w;++xx)
        panel[(y+yy)*W+x+xx]=dma[yy*w+xx];
    sent+=bytes;
    if(bytes>max_bytes)max_bytes=bytes;
    ++dispatches;++operations;
}
static void commit(int force)
{
    x0x_lcd_tile_commit(canvas,H,hashes,force,emit,0);
}
static void reset_counts(void){sent=max_bytes=dispatches=operations=0;}
static void assert_pixels(void){ assert(!memcmp(canvas,panel,sizeof(canvas))); }
int main(void) {
    for (unsigned y=0;y<H;y++) for(unsigned x=0;x<W;x++) canvas[y*W+x]=(uint16_t)((y*151u+x*7u)&65535u);
    commit(1);assert_pixels();assert(sent==W*H*2);assert(max_bytes<=1536);assert(dispatches==operations);
    reset_counts();commit(0);assert(!sent);assert(!dispatches);
    /* Narrow playhead updates cost only touched tiles, not a width-spanning rectangle. */
    for(unsigned y=17;y<76;y++) canvas[y*W+42]=0xffff;
    reset_counts();commit(0);assert_pixels();assert(sent<=X0X_LCD_TILE_H*X0X_LCD_TILE_W*2u*5u);assert(max_bytes<=X0X_LCD_TILE_H*X0X_LCD_TILE_W*2u);assert(dispatches>0);
    /* Two far apart dots must not inflate into the huge bounding rectangle. */
    canvas[W*3+1]^=1;canvas[W*130+237]^=1;
    reset_counts();commit(0);assert_pixels();assert(sent<=2u*X0X_LCD_TILE_H*X0X_LCD_TILE_W*2u);
    /* Knob-like scattering across width may use multiple chunks but stays bounded. */
    for(unsigned x=0;x<W;x+=31)canvas[W*101+x]^=(uint16_t)(x+1);
    reset_counts();commit(0);assert_pixels();assert(max_bytes<=X0X_LCD_MAX_DMA_BYTES);
    reset_counts();commit(0);assert(!sent);
    puts("ok lcd-bounded-tiles: exact pixels, idle no DMA, playhead sparse, non-inflating corners, bounded DMA and dispatch");
}
