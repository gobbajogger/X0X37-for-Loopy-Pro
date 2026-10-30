/* SPDX-License-Identifier: GPL-3.0-only */
#define main legacy_main
#include "boot_screen_test.c"
#undef main
static uint32_t fault_byte,read_fault;
static int torn_prog(void *ctx,uint32_t off,const uint8_t *src,uint32_t n){if(fault_byte<n){if(fault_byte)(void)tio_prog(ctx,off,src,fault_byte);return -1;}fault_byte-=n;return tio_prog(ctx,off,src,n);}
static int error_read(void *ctx,uint32_t off,uint8_t *dst,uint32_t n){if(read_fault<n)return -1;read_fault-=n;return tio_read(ctx,off,dst,n);}
static void serial(unsigned copy,uint32_t seq){boot_p32(nor[copy]+8,seq);boot_p32(nor[copy]+20,boot_crc(nor[copy],20));}
int main(int argc,char **argv){
 if(argc!=2)return 2;FILE*f=fopen(argv[1],"rb");if(!f)return 2;uint8_t media[3840],old[3840],snapshot[2][4096];uint32_t n=fread(media,1,sizeof media,f);fclose(f);
 wipe();uint32_t v1=build(old,"OLD","KEPT",0);check("legacy commit baseline",commit_buf(old,v1)==BOOT_OK);check("complete kind3 validates",boot_payload_ok(media,n));check("kind3 commit and XIP selection",commit_buf(media,n)==BOOT_OK&&boot_pick(nor[0],nor[1])==1&&nor[1][32]==3);memcpy(snapshot,nor,sizeof nor);
 for(unsigned i=0;i<n+32;i++){
  memcpy(nor,snapshot,sizeof nor);fault_byte=i;boot_io_t torn={tio_erase,torn_prog,tio_read,0};
  int rc=boot_commit(&torn,BOOT_SMK_SEC0,BOOT_SMK_SEC1,media,n);
  if(rc!=BOOT_FLASH || boot_pick(nor[0],nor[1])!=1){printf("tear failure at%u\n",i);bad++;break;}
 }
 check("every payload/header program cutoff retains previous media",!bad);
 memcpy(nor,snapshot,sizeof nor);for(unsigned i=0;i<200;i++){uint32_t before=erases;read_fault=i;boot_io_t broken={tio_erase,tio_prog,error_read,0};if(boot_commit(&broken,BOOT_SMK_SEC0,BOOT_SMK_SEC1,media,n)!=BOOT_FLASH||erases!=before){bad++;break;}}
 check("read failure forbids erase",!bad);
 memcpy(nor,snapshot,sizeof nor);serial(0,UINT32_MAX);serial(1,UINT32_MAX-1);check("serial max selects correct old record",boot_pick(nor[0],nor[1])==0);check("wrap commit selects zero as newer",boot_commit(&io,BOOT_SMK_SEC0,BOOT_SMK_SEC1,media,n)==BOOT_OK&&boot_u32(nor[1]+8)==0&&boot_pick(nor[0],nor[1])==1);
 serial(0,10);serial(1,10);check("serial equal chooses sector0",boot_pick(nor[0],nor[1])==0);serial(1,10+0x80000000u);uint32_t before=erases;check("half-range ambiguity refuses read/update with no erase",boot_pick(nor[0],nor[1])==-1&&boot_commit(&io,BOOT_SMK_SEC0,BOOT_SMK_SEC1,media,n)==BOOT_BAD&&erases==before);
 memcpy(nor,snapshot,sizeof nor);nor[1][32+28]=1;boot_p32(nor[1]+16,boot_crc(nor[1]+32,n));boot_p32(nor[1]+20,boot_crc(nor[1],20));check("outer CRC-valid malformed media rejected by XIP and callback",!boot_image_state(nor[1],0)&&boot_copy_state(&io,BOOT_SMK_SEC1,0)==0&&boot_pick(nor[0],nor[1])==0);
 printf("boot media storage %s\n",bad?"FAILED":"PASS");return !!bad;
}
