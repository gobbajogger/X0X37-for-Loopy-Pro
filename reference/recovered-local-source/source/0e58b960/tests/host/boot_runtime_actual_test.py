#!/usr/bin/env python3
from pathlib import Path
import importlib.util,os,subprocess
root=Path(__file__).resolve().parents[2]
# Reuse the same raw scanner fixture generator; its own actual-body suite runs first.
spec=importlib.util.spec_from_file_location('inputfixture',Path(__file__).with_name('boot_input_actual_test.py'));mod=importlib.util.module_from_spec(spec);spec.loader.exec_module(mod)
runtime=(root/'firmware/src/app/boot_runtime.h').read_text();editor=(root/'firmware/src/app/editor.c').read_text();main=(root/'firmware/src/app/main_fm1.c').read_text();body=mod.function(main,'fm1_main')
for name in ['ble_prepare','ble_start','ble_attach','audio_init','fm1_input_init','fm1_timer5_start','panel_setup']:
 assert body.index(name)<body.index('boot_playback()'),name
assert body.index('boot_playback()')<body.index('ui_init()');assert 'if(!boot_setup)boot_playback()' in body
extra=r'''
static void progress(void);
#define BM_PROGRESS() progress()
#include "app/boot_screen.h"
#include "app/boot_player.h"
#include "app/dnbox_boot_media.h"
static unsigned fm1_ms,feed_count,retry_count,dma_busy,dma_fault,rows,skip_at,progress_count,progress_press,text_calls;
static void progress(void){progress_count++;if(progress_press&&progress_count==progress_press){contacts[0]=3;smk__keys(1000);}}
static int flash_ok=0;static uint8_t sectors[8192];static uint16_t cv_px[240*138];
#define C_BLACK 0
#define C_WHITE 65535
static int FONT_M,FONT_XS;
static const uint8_t *fm1_xip_ptr(uint32_t off){return sectors+(off==BOOT_SEC1?4096:0);}
static void fm1_wdt_feed(void){feed_count++;fm1_ms++;if(skip_at&&feed_count==skip_at){contacts[0]=3;smk__keys(fm1_ms*1000);}}
static void usb_retry(uint32_t t){(void)t;retry_count++;}
static uint16_t swap16(uint16_t x){return (uint16_t)((x<<8)|(x>>8));}
static void lcd_sync(void){dma_busy=0;}
static void lcd_fill(unsigned x,unsigned y,unsigned w,unsigned h,unsigned c){(void)x;(void)y;(void)w;(void)h;(void)c;lcd_sync();}
static void lcd_blit(unsigned x,unsigned y,unsigned w,unsigned h,const uint16_t*p){(void)x;(void)y;(void)w;(void)p;if(dma_busy)dma_fault++;dma_busy=1;if(h==3)rows++;}
static void cv_begin(unsigned w,unsigned h,unsigned c){(void)w;(void)h;(void)c;lcd_sync();}
static void cv_text(unsigned x,unsigned y,const int*f,const char*s,unsigned c){(void)x;(void)y;(void)f;(void)s;(void)c;text_calls++;}
static void cv_blit(unsigned x,unsigned y){lcd_blit(x,y,240,60,cv_px);}
'''
# Keep reset/assert helper but replace original test main.
helpers=mod.test[:mod.test.index('int main(void)')]
test=r'''
static void runtime_reset(void){reset();fm1_ms=feed_count=retry_count=dma_busy=dma_fault=rows=skip_at=progress_count=progress_press=text_calls=0;flash_ok=0;}
static void stored(const uint8_t *p,unsigned n){memset(sectors,255,sizeof sectors);boot_p32(sectors,BOOT_MAGIC);boot_p16(sectors+4,BOOT_VERSION);boot_p16(sectors+6,0);boot_p32(sectors+8,1);boot_p32(sectors+12,n);boot_p32(sectors+16,boot_crc(p,n));boot_p32(sectors+20,boot_crc(sectors,20));boot_p32(sectors+24,0);boot_p32(sectors+28,0);memcpy(sectors+32,p,n);flash_ok=1;}
int main(void){
 runtime_reset();boot_playback();C(rows==27*80);C(fm1_ms==2743);C(!dma_fault&&!dma_busy);C(retry_count==feed_count);C(!smk_boot_input.active);
 runtime_reset();contacts[3]=1;fm1_in.ks_stab[3]=1;boot_playback();C(rows==27*80);C((smk_boot_input.keys>>3)&1);C(!fm1_in.keys);
 runtime_reset();skip_at=12;boot_playback();C(feed_count==12);C(rows==11);C(!dma_busy&&!smk_boot_input.active);C(smk_boot_input.keys&1);contacts[0]=0;for(int i=0;i<3;i++)smk__keys(4000+i);C(!smk_boot_input.keys);contacts[0]=3;smk__keys(5000);C(fm1_in.keys&1);
 runtime_reset();flash_ok=1;memset(sectors,0xA5,sizeof sectors);boot_playback();C(rows==27*80&&!dma_fault); /* invalid stored record falls back */
 runtime_reset();stored(solid,sizeof solid);boot_playback();C(rows==80&&fm1_ms==1000&&!dma_busy&&!dma_fault);C(progress_count>0);
 runtime_reset();uint8_t legacy[44]={1,0,1,1};legacy[4]='A';legacy[16]='B';stored(legacy,sizeof legacy);boot_playback();C(rows==0&&text_calls==2&&fm1_ms==1000&&!dma_busy);
 runtime_reset();stored(solid,sizeof solid);progress_press=1;boot_playback();C(rows==0&&smk_boot_input.keys&1&&!dma_busy); /* fresh press during selection, not recaptured as entry-held */
 printf("Actual startup adapter: %u checks PASS; source ordering guard PASS; mocked LCD/time/IRQ, no target timing claim\n",checks);return 0;
}
'''
extra+='\nstatic const uint8_t solid[]={'+','.join(map(str,(root/'tests/fixtures/boot_media/solid.bm').read_bytes()))+'};\n'
source=mod.pre+'\n'.join(mod.parts)+extra+mod.function(editor,'boot_shown')+'\n'.join(mod.function(runtime,n) for n in ['boot_wait_static','boot_legacy_draw','boot_playback'])+helpers+test
out=root/'build/boot-input-test';(out/'runtime.c').write_text(source)
cmd=[os.environ.get('CC','cc'),'-std=c11','-O1','-g','-DBOARD_SMK37=1','-fsanitize=address,undefined','-fno-omit-frame-pointer','-I'+str(root/'firmware/src'),str(out/'runtime.c'),'-o',str(out/'runtime')];subprocess.run(cmd,check=True);subprocess.run([str(out/'runtime')],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
