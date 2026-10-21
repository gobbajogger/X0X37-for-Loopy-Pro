/* SPDX-License-Identifier: GPL-3.0-only */
/* NON-INSTALLABLE loader sizing prototype. Not a wire protocol release.
 * Requires existing ota_read/ldr_fread/erase/prog/poll, preserved chip-key check,
 * RAM loader ownership, held core1, stopped DMA and no flash/XIP readers.
 * No standalone piece writes. Header lives after a 1 MiB logical flash entry. */
#include "rollback_pins.h"
#define BP_MAGIC 0x31425058u
#define BP_VERSION 1u
#define BP_COMMIT_OFF 0x84000u /* sizing contract only; final layout must bind this */
typedef struct { uint32_t off,size,crc; } bp_piece;
typedef struct { uint32_t magic,version,model,policy,commit_off,commit_size,bundle_crc,header_crc; uint32_t appdesc_off,image_id,reserved[2]; bp_piece p[4]; } bp_header;
_Static_assert(sizeof(bp_header)==96,"study metadata bytes");
static uint32_t bp_crc(uint32_t c,const uint8_t *p,uint32_t n)
{
    while(n--){c^=*p++;for(unsigned b=0;b<8;b++)c=(c>>1)^(0xEDB88320u & (0u-(c&1)));}return c;
}
static int bp_scan(const bp_header *h,uint32_t base,uint8_t *buf,int installed)
{
    uint32_t all=~0u;
    for(unsigned i=0;i<4;i++) {
        uint32_t c=~0u;
        for(uint32_t k=0;k<h->p[i].size;k+=512) {
            if(installed ? ldr_fread(h->p[i].off+k,buf,512) : ota_read(base+h->p[i].off+k,buf,512))return -61;
            if(installed==2 && h->p[i].off+k==h->commit_off)
                for(unsigned j=0;j<128;j++)buf[j]=((const uint8_t *)h)[128+j];
            if(h->policy==2 && i)for(unsigned j=0;j<512;j++)if(buf[j]!=255)return -77;
            c=bp_crc(c,buf,512);all=bp_crc(all,buf,512);ldr_progress(k,h->p[i].size);
        }
        if(~c!=h->p[i].crc)return -62;
    }
    return ~all==h->bundle_crc ? 0 : -63;
}
static int bp_sector(uint32_t off,uint8_t *sec,uint8_t *cur)
{
    if(ldr_fread(off,cur,4096))return -64;
    if(ota_memeq(sec,cur,4096))return 0;
    for(unsigned tries=0;tries<2;tries++) {
        if(ldr_erase(off))return -65;
        for(unsigned k=0;k<4096;k+=256)if(ldr_prog(off+k,sec+k,256))return -66;
        if(ldr_fread(off,cur,4096))return -64;
        if(ota_memeq(sec,cur,4096))return 0;
    }
    return -67;
}
/* Logical rollback is the exact approved baseline main, not arbitrary ABI2.
 * Its 68-byte ABI2 header is inside a withheld 128-byte commit PREFIX. */
static int bp_rollback_bind(uint32_t base,uint32_t key,uint8_t *buf)
{
    uint32_t c=~0u;
    for(uint32_t k=0;k<0x99000u;k+=512) {
        if(ota_read(base+0x4000u+k,buf,512))return -61;
        if(k<BP_RB_CRYPT_SIZE) {
            uint32_t n=BP_RB_CRYPT_SIZE-k;if(n>512)n=512;
            ldr_sfc(buf,n,k,key);
        }
        c=bp_crc(c,buf,512);ldr_progress(k,0x99000u);
    }
    return ~c==BP_RB_MAIN_CRC?0:-76;
}
/* Decrypt only existing SFC-encrypted main; external immutable data is plain.
 * bp_manifest_structure is generated from the existing XB4 validator's exact
 * structural prefix; streaming payload validation replaces its XIP callback. */
static int bp_bind(const bp_header *h,uint32_t base,uint32_t key,uint8_t *vmem,uint8_t *buf)
{
    xb4_manifest_t *v=(xb4_manifest_t *)(void *)vmem;
    uint32_t a=h->appdesc_off&~31u,delta=h->appdesc_off-a;
    uint32_t app_crc=~0u;
    for(uint32_t k=0;k<BP_APP_SIZE;) {
        uint32_t n=BP_APP_SIZE-k;if(n>512)n=512;
        if(ota_read(base+BP_APP_OFF+k,buf,n))return -61;
        ldr_sfc(buf,n,BP_APP_OFF+k-0x4000u,key);
        app_crc=bp_crc(app_crc,buf,n);k+=n;ldr_progress(k,BP_APP_SIZE);
    }
    if(~app_crc!=BP_APP_CRC)return -75;

    if(ota_read(base+a,buf,64))return -61;
    ldr_sfc(buf,64,a-0x4000u,key);
    if(ota_rd32(buf+delta)!=0x55493058u||ota_rd32(buf+delta+4)!=1||
       ota_rd32(buf+delta+8)!=0xAB000||ota_rd32(buf+delta+12)!=8192||
       ota_rd32(buf+delta+16)!=h->p[1].crc)return -69;
    if(ota_read(base+h->commit_off,vmem,128))return -61;
    ldr_sfc(vmem,128,h->commit_off-0x4000u,key);
    if(bp_manifest_structure(v,sizeof *v,h->model)||v->image_id!=h->image_id)return -70;
    for(unsigned i=0;i<3;i++) {
        uint32_t c=~0u,k=i==0?128:0;
        for(;k<v->segment[i].size;) {
            uint32_t n=v->segment[i].size-k;if(n>512)n=512;
            uint32_t off=v->segment[i].off+k;
            if(ota_read(base+off,buf,n))return -61;
            if(i==0)ldr_sfc(buf,n,off-0x4000u,key);
            c=bp_crc(c,buf,n);k+=n;ldr_progress(k,v->segment[i].size);
        }
        if(~c!=v->segment[i].crc32)return -62;
    }
    return 0;
}
/* Reuses existing hdr1024/sec4096/cur4096. No extra static buffer or allocator.
 * Recovery record clearing stays caller-owned, and ONLY after return0. */
static int bp_install(uint32_t base,uint32_t fl_size,uint32_t key,uint8_t *hdr,uint8_t *sec,uint8_t *cur)
{
    static const uint32_t off[4]={0x4000,0xAB000,0xAD000,0xFE000};
    static const uint32_t size[4]={0x99000,8192,8192,4096};
    bp_header *h=(bp_header *)(void *)hdr;
    uint8_t *commit=hdr+128;
    uint32_t hc;int rc;
    if(fl_size!=0x100000u+sizeof *h || base>0xFFFFFFFFu-fl_size)return -60;
    if(ota_read(base+0x100000,hdr,sizeof *h))return -61;
    hc=h->header_crc;h->header_crc=0;
    if(~bp_crc(~0u,hdr,sizeof *h)!=hc)return -62;
    h->header_crc=hc;
    if(h->magic!=BP_MAGIC||h->version!=BP_VERSION||(h->policy!=1&&h->policy!=2)||
       h->model!=~bp_crc(~0u,(const uint8_t *)SMK_MODEL,sizeof(SMK_MODEL)-1)||
       h->reserved[0]||h->reserved[1]||!h->image_id||h->commit_size!=128)return -60;
    for(unsigned i=0;i<4;i++)if(h->p[i].off!=off[i]||h->p[i].size!=size[i])return -60;
    if(h->policy==1) {
        if(h->appdesc_off!=BP_APPDESC_OFF||h->commit_off!=BP_COMMIT_OFF)return -60;
    } else {
        if(h->appdesc_off||h->commit_off!=BP_RB_COMMIT||h->image_id!=BP_RB_IMAGE_ID||
           h->p[1].crc!=BP_FF_8K_CRC||h->p[2].crc!=BP_FF_8K_CRC||h->p[3].crc!=BP_FF_4K_CRC)return -60;
    }

    if((rc=bp_scan(h,base,sec,0)))return rc;
    if(ota_read(base+h->commit_off,commit,128))return -61;
    if((rc=h->policy==1?bp_bind(h,base,key,hdr+256,sec):bp_rollback_bind(base,key,sec)))return rc;
    /* Stage app data + secondary vendor loads before whole main rewrite. */
    for(unsigned pass=0;pass<4;pass++) {
        unsigned i=pass==3?0:pass+1;
        for(uint32_t s=h->p[i].off;s<h->p[i].off+h->p[i].size;s+=4096) {
            for(unsigned k=0;k<4096;k+=512)if(ota_read(base+s+k,sec+k,512))return -61;
            if(s==h->commit_off)for(unsigned k=0;k<128;k++)sec[k]=255;
            if((rc=bp_sector(s,sec,cur)))return rc;
            ldr_progress(s,h->p[i].off+h->p[i].size);
        }
    }
    if((rc=bp_scan(h,base,sec,2)))return rc; /* all final CRCs before manifest publication */
    if(ldr_prog(h->commit_off,commit,128))return -66;
    if(ldr_fread(h->commit_off,sec,128)||!ota_memeq(sec,commit,128))return -64;
    for(unsigned i=0;i<4;i++)
        if(!ota_read(0xF0000000u,sec,8)&&ota_memeq(sec,(const uint8_t *)"success",8))return 0;
    return -68; /* caller MUST retain record on finish timeout/wrong response */
}
