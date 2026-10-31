/* SPDX-License-Identifier: GPL-3.0-only */
#define EXPECT_SIGNAL 1
#define SMC_UI_MAIN unused_smc_ui_main
#include "smc_ui_test.c"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include "dsp/fxbus.h"
static fxbus_t signal_fx;static int16_t ring[79200];static float z[256],in[256],l[256],r[256];
static uint64_t hash=1469598103934665603ull;
static void add_audio_hash(const void*p,unsigned n){const unsigned char*b=p;while(n--){hash^=*b++;hash*=1099511628211ull;}}
int main(int argc,char**argv){
 FILE *raw=argc>1?fopen(argv[1],"wb"):0;
 unsigned checks=0;int lastwet=-1,lastsig=-1,idle=-1;
 for(int mode=0;mode<2;mode++)for(int ping=0;ping<2;ping++)for(int feedback=0;feedback<3;feedback++){
  fxbus_init(&signal_fx,ring,79200);fxbus_set(&signal_fx,FX_DL_TYPE,mode);fxbus_set(&signal_fx,FX_DL_PING,ping);fxbus_set(&signal_fx,FX_DL_FDBK,feedback*40);fxbus_set(&signal_fx,FX_DL_TIME,0);fxbus_set(&signal_fx,FX_DL_LEVEL,127);fxbus_set(&signal_fx,FX_RV_LEVEL,0);fxbus_set_bpm(&signal_fx,125);
  for(int b=0;b<1000;b++){
   memset(in,0,sizeof in);if(b==0)in[0]=.5f;
   fxbus_process_st(&signal_fx,z,z,z,in,l,r,256);add_audio_hash(l,sizeof l);add_audio_hash(r,sizeof r);if(raw){fwrite(l,1,sizeof l,raw);fwrite(r,1,sizeof r,raw);}
#ifdef EXPECT_SIGNAL
   int wet=0;for(int i=0;i<256;i++)wet|=l[i]!=0.0f||r[i]!=0.0f;
   if(!feedback){assert(signal_fx.dl_signal==(b==0||wet));checks++;}
   if(mode==1&&!ping&&!feedback){if(wet)lastwet=b;if(signal_fx.dl_signal)lastsig=b;if(idle<0&&signal_fx.dl_quiet>=signal_fx.dlen)idle=b;}
#endif
  }
 }
#ifdef EXPECT_SIGNAL
 assert(lastwet==lastsig);assert(idle>lastsig);checks+=2;printf("signal checks%u lastwet%d lastsignal%d ringidle%d ",checks,lastwet,lastsig,idle);
#endif
 printf("audio_hash=%016llx fxbus_size=%zu\n",(unsigned long long)hash,sizeof signal_fx);if(raw)fclose(raw);return 0;
}
