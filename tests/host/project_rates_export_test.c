/* SPDX-License-Identifier: GPL-3.0-only */
/* Real firmware project capture/header/validation. Files contain synthetic data. */
#define SMC_UI_MAIN retained_fixture_main
#include "smc_ui_test.c"
int main(void){
 const char *dir=getenv("PROJECT_RATE_FIXTURES");CHECK(dir!=NULL);
 for(int stream=13;stream<=15;stream++)for(int rate=RATE_8;rate<=RATE_4;rate++)for(int part=0;part<3;part++){
  setup(V_PART,PART_303A);proj.set.bt|=PROJ_BT_CLK_IN;
  memset(proj.arr.lane,0,sizeof proj.arr.lane);
  for(int p=0;p<NPAT;p++)proj.pat[p].rsv[0]=proj.pat[p].rsv[1]=0;
  if(stream==14)proj.pat[0].rsv[0]=1; /* Chicago preset 1, not a forged stream header. */
  if(stream==15){uint8_t notes[]={60,64,67};proj.pat[0].bass[0].step[0]=(bstep_t){60,G_NOTE,100};CHECK(held_chord_set(proj.arr.lane,0,0,0,notes,3));}
  /* Matching format-1 peer fixture: do not mislabel extended rates as old data. */
  if(rate==RATE_8 && part==0){
   project_capture_stage();CHECK(project_validate_stage()==0);
   uint8_t old_header[PROJECT_FILE_HDR];project_file_header(old_header);
   CHECK(old_header[16]==1);CHECK(old_header[20]==stream);CHECK(project_file_validate(old_header)==0);
   char old_path[1024];CHECK(snprintf(old_path,sizeof old_path,"%s/old-s%d.bin",dir,stream)>0);
   FILE *old=fopen(old_path,"wb");CHECK(old!=NULL);
   CHECK(fwrite(old_header,1,sizeof old_header,old)==sizeof old_header);
   CHECK(fwrite(&project_stage,1,sizeof project_stage,old)==sizeof project_stage);CHECK(fclose(old)==0);
  }
  if(part==0)proj.pat[0].drum[0].rate=(uint8_t)rate;
  if(part==1)proj.pat[0].bass[0].rate=(uint8_t)rate;
  if(part==2)proj.pat[0].brk.rate=(uint8_t)rate;
  project_capture_stage();CHECK(project_validate_stage()==0);
  uint8_t header[PROJECT_FILE_HDR];project_file_header(header);
  CHECK(header[16]==2);CHECK(header[20]==stream);CHECK(project_file_validate(header)==0);
  char path[1024];CHECK(snprintf(path,sizeof path,"%s/s%d-r%d-p%d.bin",dir,stream,rate,part)>0);
  FILE *f=fopen(path,"wb");CHECK(f!=NULL);
  CHECK(fwrite(header,1,sizeof header,f)==sizeof header);
  CHECK(fwrite(&project_stage,1,sizeof project_stage,f)==sizeof project_stage);CHECK(fclose(f)==0);
 }
 printf("Firmware format2 exports: %u checks PASS\n",checks);return 0;
}
