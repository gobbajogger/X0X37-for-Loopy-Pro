# SPDX-License-Identifier: GPL-3.0-only
from pathlib import Path
import os, shlex, subprocess
root=Path(__file__).resolve().parents[2]
src=(root/'firmware/hal/smk_input.h').read_text()
def function(name):
 start=src.index('static void '+name+'(')
 end=src.index('\n}',start)+2
 return src[start:end]
out=root/'build/host/keyboard-scanner'
out.mkdir(parents=True,exist_ok=True)
c=r'''
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { checks++; if(!(x)){fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x); exit(1);} }while(0)
static unsigned checks;
static struct {uint16_t fad_hist[4][2],fad_raw[4],fad_acc[4],wheel[2],pedal,batt; uint8_t fader[4],raw[16]; uint32_t adc_ok,adc_miss;} fm1_in;
static const uint8_t SMK_FADMUX[4]={1,2,3,0},SMK_PADMUX[8]={0,1,2,3,4,5,6,7};
static uint32_t smk__slot_addr(uint32_t s){return s/3;}
static void smk__pad(uint32_t p,uint32_t raw){(void)p;(void)raw;}
'''+function('smk__analog')+r'''
static uint32_t reg_con,reg_res,smk_col,smk_grp,smk_slot,smk_adc_slot=255,smk_muxw;
#define SMK_ADC_CON reg_con
#define SMK_ADC_RES reg_res
#define SMK_ADC_PADA 1
#define SMK_ADC_PADB 8
#define SMK_ADC_MUXC 2
static unsigned shifted_mux,latched_mux,started_slot;
static unsigned slot_raw(unsigned s){return 80+s*37;}
static void smk__adc_start(unsigned ch){ unsigned c=ch==1?0:ch==8?1:2; started_slot=latched_mux*3+c; reg_con=0; }
static unsigned smk__rows(void){return 0;}
static void smk__common(unsigned g){(void)g;}
static void smk__latch(void){latched_mux=shifted_mux;}
static void fm1__frame(void){}
static void smk__shift(unsigned col){(void)col;shifted_mux=(smk_muxw>>3)&7;}
'''+function('fm1_input_tick')+r'''
static void scanner_ticks(void){
 memset(&fm1_in,0,sizeof fm1_in);
 unsigned delivered[24]={0};
 for(unsigned n=0;n<24000;n++){
  if(smk_adc_slot!=255){
   CHECK(started_slot==smk_adc_slot);
   if(n%7){reg_con=128;reg_res=slot_raw(started_slot);delivered[started_slot]++;}else reg_con=0;
  }
  fm1_input_tick();
  if(n>48) for(unsigned a=0;a<4;a++) if(delivered[a*3+2]) CHECK(fm1_in.fad_raw[SMK_FADMUX[a]]==slot_raw(a*3+2));
 }
 CHECK(fm1_in.adc_miss>0);CHECK(fm1_in.adc_ok+fm1_in.adc_miss==23999);
 puts("PASS 24000 production ticks: mux/channel sample attribution, missed conversions dropped, next valid slot recovery");
}
static unsigned slot(unsigned f){for(unsigned a=0;a<4;a++)if(SMK_FADMUX[a]==f)return a*3+2;abort();}
static void ramps(void){
 for(unsigned f=0;f<4;f++)for(unsigned dwell=1;dwell<=16;dwell*=2)for(unsigned down=0;down<2;down++){
  memset(&fm1_in,0,sizeof fm1_in);
  unsigned first=down?1023:0,s=slot(f);
  for(unsigned n=0;n<128;n++)smk__analog(s,first);
  unsigned prev=fm1_in.fader[f];
  for(unsigned n=0;n<1024;n++)for(unsigned d=0;d<dwell;d++){
   unsigned raw=down?1023-n:n; smk__analog(s,raw);
   unsigned v=fm1_in.fader[f];CHECK(v<=127);CHECK(down?v<=prev:v>=prev);CHECK(v+2>=prev&&v<=prev+2);prev=v;
  }
  for(unsigned n=0;n<128;n++)smk__analog(s,down?0:1023);
  CHECK(fm1_in.fader[f]==(down?0:127));
 }
 puts("PASS all four faders, both directions, 1/2/4/8/16-sample raw ramps: monotonic decoded values, <=2-count transitions, both endpoints");
}
static void park(void){
 for(unsigned f=0;f<4;f++)for(unsigned raw=0;raw<1024;raw++){
  memset(&fm1_in,0,sizeof fm1_in); unsigned s=slot(f);
  for(unsigned n=0;n<128;n++)smk__analog(s,raw);
  unsigned parked=fm1_in.fader[f],acc=fm1_in.fad_acc[f];
  for(unsigned n=0;n<8;n++){smk__analog(s,raw);CHECK(fm1_in.fader[f]==parked);CHECK(fm1_in.fad_acc[f]==acc);}
  // One full-range spike must be rejected by the actual median.
  smk__analog(s,1023-raw);CHECK(fm1_in.fader[f]==parked);
  smk__analog(s,raw);CHECK(fm1_in.fader[f]==parked);
  smk__analog(s,raw);CHECK(fm1_in.fader[f]==parked);
 }
 puts("PASS every raw parked position, four channels: stable decoded output, one isolated full-range spike rejected");
}
int main(void){scanner_ticks();ramps();park();printf("PASS %u scanner assertions\n",checks);}
'''
(out/'scanner.c').write_text(c)
subprocess.run(['cc','-std=c99','-O1','-Wall','-Wextra','-Werror',*shlex.split(os.environ.get('SAN_FLAGS','')),str(out/'scanner.c'),'-o',str(out/'test')],check=True)
subprocess.run([str(out/'test')],check=True)
