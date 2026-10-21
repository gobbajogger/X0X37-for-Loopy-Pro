/* SPDX-License-Identifier: GPL-3.0-only */
/* Included instead of the payload-writing ldr_core.c. First-stage export only;
 * no encrypted app input and no main/head/sample erase or program operation. */
#include "head_export.h"
#include "head_record_owner.h"
static uint8_t hx_hdr[1024],hx_a[512],hx_b[512],hx_wire[320];
static hx_state hx;
static uint32_t hx_admitted,hx_reset_at;
static int hx_read_head(uint32_t off,void*p,uint32_t n)
{if(off>=HX_SIZE||!n||n>HX_SIZE-off)return -1;return ldr_fread(off,p,n);}
static int hx_finish_owned(void){return bp_record_finish(hx_hdr+384,hx_a);}
static int hx_admit(void)
{
    uint32_t fo,fs,oo,os;int rc;
    hx_admitted=0;hx=(hx_state){0};
    if((rc=ota_ufw(hx_hdr,&fo,&fs,&oo,&os)))return rc;
    if(!fo||!oo||!fs)return -1;
    if((rc=bp_record_admit(oo,os,hx_hdr+384,hx_a,hx_b)))return rc;
    /* Admission acknowledgment only; never FINISH_WRITE/payload installation. */
    if(ota_read(0xE0000000u,hx_a,8)||!ota_memeq(hx_a,(const uint8_t*)"success",8))return -1;
    hx_admitted=1;return 0;
}
static void hx_send(const hx_reply*r,const uint8_t*data)
{
    uint32_t n=sizeof *r+r->size,sum=0,w;
    hx_a[0]=0;hx_a[1]=0x59;hx_a[2]=0x40;hx_a[3]=n;hx_a[4]=n>>8;hx_a[5]=0;
    memcpy(hx_a+6,r,sizeof *r);memcpy(hx_a+6+sizeof *r,data,r->size);
    for(unsigned i=0;i<n;i++)sum+=hx_a[6+i];
    hx_a[6+n]=(uint8_t)~sum;
    hx_wire[0]=0xF0;w=1+ota_pack7(hx_a,7+n,hx_wire+1);hx_wire[w++]=0xF7;
    (void)ota_wire_send(hx_wire,w);
}
static void hx_service(void)
{
    uint32_t d=ota_take();hx_reply r;
    if(d==7 && ota_dec[2]==0x11){ota_reply_identity();return;}
    if(d!=23 || ota_dec[2]!=0x40)return;
    uint32_t cmd=ota_rd32(ota_dec+6),nonce=ota_rd32(ota_dec+10),off=ota_rd32(ota_dec+14),n=ota_rd32(ota_dec+18);
    if(!cmd && nonce && !off && !n) {
        r=(hx_reply){HX_MAGIC,HX_MODEL,nonce,0,0,0,5};hx_send(&r,hx_b);return;
    }
    if(!hx_admitted)return;
    if(hx_request(&hx,cmd,nonce,off,n,&r,hx_b,hx_read_head,hx_finish_owned))return;
    hx_send(&r,hx_b);
    if(r.status==HX_DONE&&!hx_reset_at)hx_reset_at=ldr_ms()+250u;
}
