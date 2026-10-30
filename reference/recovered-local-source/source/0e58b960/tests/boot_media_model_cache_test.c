/* SPDX-License-Identifier: GPL-3.0-only */
static unsigned progress_calls;
#define BM_PROGRESS() (++progress_calls)
#define main legacy_main
#include "boot_screen_test.c"
#undef main
static unsigned reads,max_read;
static int counted(void*ctx,uint32_t off,uint8_t*dst,uint32_t n){reads++;if(n>max_read)max_read=n;return tio_read(ctx,off,dst,n);}
int main(int argc,char**argv){if(argc!=2)return 2;uint8_t media[3840],old[3840];FILE*f=fopen(argv[1],"rb");if(!f)return 2;unsigned n=fread(media,1,sizeof media,f);fclose(f);wipe();unsigned v1=build(old,"OLD","KEPT",0);check("v1 storage remains accepted",commit_buf(old,v1)==BOOT_OK);unsigned before=erases;
#if BOARD_SMK37
check("SMK media accepted",commit_buf(media,n)==BOOT_OK);boot_io_t counted_io={tio_erase,tio_prog,counted,0};reads=max_read=progress_calls=0;check("cached copy validates",boot_copy_state(&counted_io,BOOT_SMK_SEC1,0)==1);check("read transactions bounded by two chunk passes plus header",reads<=1+2*((n+255)/256));check("max physical read256",max_read<=256);check("decode progress and flash yield present",progress_calls>=3200/256+2*((n+255)/256));
#else
check("FM1 rejects valid media before erase",!boot_payload_ok(media,n)&&commit_buf(media,n)==BOOT_BAD&&erases==before);check("old FM1 v1 remains selectable",boot_pick(nor[0],nor[1])==0);
#endif
printf("model%d cache/model %s; reads=%u yields=%u\n",BOARD_SMK37,bad?"FAILED":"PASS",reads,progress_calls);return !!bad;}
