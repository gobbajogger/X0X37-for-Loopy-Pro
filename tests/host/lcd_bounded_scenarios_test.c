#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../../firmware/src/app/lcd_tiles.h"

#define W 240u
#define MAX_ROWS 138u
#define OLD_COLS (W/8u)
#define NEW_ROWS ((MAX_ROWS+X0X_LCD_TILE_H-1u)/X0X_LCD_TILE_H)
static uint16_t canvas[W*MAX_ROWS], dma[X0X_LCD_MAX_DMA_BYTES/2u];
static uint32_t old_rows[MAX_ROWS],old_cols[OLD_COLS],new_hash[NEW_ROWS*X0X_LCD_TILE_COLS];
typedef struct { unsigned bytes,max_bytes,ops; } stats_t;
static stats_t before,after;
static void add(stats_t *s,unsigned bytes){s->bytes+=bytes;if(bytes>s->max_bytes)s->max_bytes=bytes;s->ops++;}
static void emit(void *ctx,const uint16_t *src,uint32_t stride,uint32_t x,uint32_t y,uint32_t w,uint32_t h)
{(void)ctx;unsigned bytes=w*h*2u;assert(bytes<=X0X_LCD_MAX_DMA_BYTES);x0x_lcd_tile_pack(dma,src,stride,x,y,w,h);add(&after,bytes);}
/* Original cv_commit_rows row/column hash + one cross-product bounding rectangle. */
static void legacy(uint32_t rows,int force){
    uint32_t rowh[MAX_ROWS],colh[OLD_COLS],r0=rows,r1=0,c0=OLD_COLS,c1=0;
    for(unsigned c=0;c<OLD_COLS;c++)colh[c]=2166136261u;
    for(unsigned r=0;r<rows;r++){
      const uint16_t*q=canvas+r*W;uint32_t hr=2166136261u;
      for(unsigned c=0;c<OLD_COLS;c++){
        uint32_t hc=colh[c];for(unsigned k=0;k<8u;k++){hr=(hr^q[k])*16777619u;hc=(hc^q[k])*16777619u;}
        colh[c]=hc;q+=8u;
      }rowh[r]=hr;
    }
    if(force){r0=c0=0;r1=rows-1u;c1=OLD_COLS-1u;}
    else{
      for(unsigned r=0;r<rows;r++)if(rowh[r]!=old_rows[r]){if(r0==rows)r0=r;r1=r;}
      for(unsigned c=0;c<OLD_COLS;c++)if(colh[c]!=old_cols[c]){if(c0==OLD_COLS)c0=c;c1=c;}
      if(r0==rows||c0==OLD_COLS)return;
    }
    for(unsigned r=0;r<rows;r++)old_rows[r]=rowh[r];
    for(unsigned c=0;c<OLD_COLS;c++)old_cols[c]=colh[c];
    add(&before,(c1-c0+1u)*8u*(r1-r0+1u)*2u);
}
static void print_stats(const char *label){
    printf("%-11s %8u %8u %9u %9u %5u %5u\n",label,before.bytes,after.bytes,
      (before.max_bytes*2u+2u)/3u,(after.max_bytes*2u+2u)/3u,before.ops,after.ops);
}
static void reset(unsigned rows){
    memset(canvas,0,sizeof canvas);memset(old_rows,0,sizeof old_rows);
    memset(old_cols,0,sizeof old_cols);memset(new_hash,0,sizeof new_hash);
    memset(&before,0,sizeof before);memset(&after,0,sizeof after);
    legacy(rows,1);x0x_lcd_tile_commit(canvas,rows,new_hash,1,emit,0);
    memset(&before,0,sizeof before);memset(&after,0,sizeof after);
}
int main(void){
    puts("scenario      old bytes new bytes old max us new max us old ops new ops");
    reset(138u);
    for(unsigned y=40u;y<=75u;y++){canvas[y*W+50u]=0xffffu;canvas[y*W+165u]=0xffffu;}
    legacy(138u,0);x0x_lcd_tile_commit(canvas,138u,new_hash,0,emit,0);
    print_stats("playhead");assert(after.bytes<before.bytes);assert(after.max_bytes<=1536u);
    reset(82u);
    for(unsigned y=17u;y<=39u;y++){canvas[y*W+7u]=0x9999u;canvas[y*W+119u]=0x9999u;canvas[y*W+232u]=0x9999u;}
    legacy(82u,0);x0x_lcd_tile_commit(canvas,82u,new_hash,0,emit,0);
    print_stats("knob");assert(after.bytes<before.bytes);assert(after.max_bytes<=1536u);
    reset(82u);
    for(unsigned y=0;y<82u;y++)for(unsigned x=0;x<W;x++)canvas[y*W+x]=(uint16_t)((y+x)%65535u);
    add(&before,82u*240u*2u);
    for(unsigned y=0;y<82u;y+=16u)for(unsigned x=0;x<W;x+=48u){unsigned h=82u-y<16u?82u-y:16u;emit(0,canvas,W,x,y,48u,h);}
    print_stats("smc-popup");assert(after.bytes==before.bytes);assert(after.max_bytes<=1536u);
    reset(138u);legacy(138u,0);x0x_lcd_tile_commit(canvas,138u,new_hash,0,emit,0);
    print_stats("idle");assert(!before.bytes&&!after.bytes);
}
