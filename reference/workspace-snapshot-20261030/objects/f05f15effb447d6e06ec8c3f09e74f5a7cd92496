/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "fm1_capture_gate.h"
#define BOARD_SMK37 1
#define SMK_MODEL "SMK-37 Elite"
#define FELUCCA_ID "SMK-37 Elite_900"
#define FELUCCA_OTA_DRYRUN 0
#define ST_MAINTENANCE_ADMIT() fm1_capture_pre_admit()
#define OTA_MAINTENANCE_ADMIT() fm1_capture_pre_admit()
#include "app/plat.h"
static uint8_t nor[0x100000], before[0x100000];
static unsigned reads, erases, programs, wire, ui, commits, consumed;
static int pending;
static int st_read(uint32_t off,void *p,uint32_t n){reads++;memcpy(p,nor+off,n);return 0;}
static int st_erase(uint32_t off){erases++;memset(nor+off,255,4096);return 0;}
static int st_prog(uint32_t off,const void *p,uint32_t n)
{const uint8_t *src=p;programs++;for(uint32_t i=0;i<n;i++)nor[off+i]&=src[i];return 0;}
#include "storage.c"
static int ota_wire_send(const uint8_t *p,uint32_t n){(void)p;(void)n;wire++;return 0;}
static int ota_frame_get(const uint8_t **p,uint32_t *n)
{static const uint8_t packet[1]={0};*p=packet;*n=1;return pending;}
static void ota_frame_done(void){pending=0;consumed++;}
static uint32_t ota_now_ms(void){static uint32_t clock;clock+=6000;return clock;}
static void ota_idle(void){}
static int ota_erase(uint32_t off){return st_erase(off);}
static int ota_prog(uint32_t off,const void *p,uint32_t n){return st_prog(off,p,n);}
static int ota_fread(uint32_t off,void *p,uint32_t n){return st_read(off,p,n);}
static void ota_show(uint32_t step,int32_t code){(void)step;(void)code;ui++;}
static void ota_commit(const uint8_t *p){(void)p;commits++;}
#include "ota.c"
int main(void)
{
    uint8_t data[32],dst[32],scratch[sizeof st_buf];uint32_t cap=123;
    memset(nor,255,sizeof nor);memset(data,0x55,sizeof data);memset(dst,0x66,sizeof dst);
    memcpy(before,nor,sizeof nor);memset(st_buf,0x77,sizeof st_buf);memcpy(scratch,st_buf,sizeof scratch);
    assert(!fm1_capture_gate_acquire(&fm1_capture_barrier,1,1,1));
    assert(st_save(OBJ_SOUND,data,sizeof data)==-8);
    assert(st_load(OBJ_SOUND,dst,sizeof dst)==-1);
    assert(store_stage_lock(&cap)==NULL && cap==123 && !store_stage_held());
    assert(!reads && !erases && !programs);
    assert(!memcmp(nor,before,sizeof nor) && !memcmp(st_buf,scratch,sizeof scratch));
    for(unsigned i=0;i<sizeof dst;i++)assert(dst[i]==0x66);
    assert(!fm1_capture_gate_current(&fm1_capture_barrier,1));
    assert(ota_session()==-20 && !wire && !ui && !commits && ota_deadline==0);
    pending=1;ota_service();assert(!pending && consumed==1 && !wire);
    assert(!fm1_capture_gate_release(&fm1_capture_barrier,1));
    assert(st_save(OBJ_SOUND,data,sizeof data)==0);
    assert(st_load(OBJ_SOUND,dst,sizeof dst)==sizeof data && !memcmp(data,dst,sizeof data));
    assert(erases && programs && reads);
    assert(ota_session()!=-20 && wire && ui && !commits); /* no host package, expected refusal later */
    puts("PASS: held storage/OTA refuse before scratch, deadline, flash or UI mutation; inactive paths admitted");
    return 0;
}
