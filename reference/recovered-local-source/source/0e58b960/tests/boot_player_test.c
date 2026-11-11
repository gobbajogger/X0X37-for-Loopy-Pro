/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "app/boot_player.h"
static unsigned count;
#define C(x) do{count++;if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);return 1;}}while(0)
int main(int argc,char **argv){
 C(argc==3);FILE*f=fopen(argv[1],"rb");C(f);uint8_t data[32768];size_t n=fread(data,1,sizeof data,f);fclose(f);bm_reader_t r=bm_memory(data,n);bm_info_t m;C(bm_validate(&r,sizeof data,&m)==1);bm_player_t p;uint16_t row[80];unsigned y;int draw;
 C(bm_player_begin(&p,r,&m,0));unsigned rendered=0;char name[1024];FILE*out=0;
 for(unsigned now=0;now<=2743;now++){
  unsigned frame=p.frame;
  if(bm_player_step(&p,now,0,row,&y,&draw)&&draw){
   frame=p.frame;if(!y){if(out)fclose(out);snprintf(name,sizeof name,"%s/frame-%02u.rgb565",argv[2],frame);out=fopen(name,"wb");C(out);}
   C(fwrite(row,2,80,out)==80);rendered++;
  }
  if(now==2742)C(p.active);if(now==2743)C(!p.active);
 }
 if(out)fclose(out);C(rendered==27*80);C(!p.failed);C(bm_finish(&p.decoder,&p.info));
 C(bm_player_begin(&p,r,&m,UINT32_MAX-100));C(bm_player_step(&p,UINT32_MAX-100,0,row,&y,&draw));C(bm_player_step(&p,5,1,row,&y,&draw)==0&&!p.active);
 C(bm_player_begin(&p,r,&m,0));C(bm_player_step(&p,2500,0,row,&y,&draw)&&!draw);C(p.decoder.produced==40);C(bm_player_step(&p,2743,0,row,&y,&draw)==0&&!p.active);
 C(bm_player_begin(&p,r,&m,0));C(bm_player_step(&p,0,1,row,&y,&draw)==0&&!p.active&&p.decoder.produced==0);
 printf("boot player %u checks PASS; actual decoder frame outputs; mocked clock, no LCD/hardware timing claim\n",count);return 0;
}
