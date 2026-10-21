/* SPDX-License-Identifier: GPL-3.0-only */
/* TEST-only: prove the persistent record belongs to the package's staged
 * bundle loader before any payload write. Snapshot occupies hdr[384..495].
 * This is integrity/ownership binding, not cryptographic authentication. */
static int bp_record_admit(uint32_t ota_off, uint32_t ota_len,
                           uint8_t *saved, uint8_t *a, uint8_t *b)
{
    uint32_t n, len, cap=0;
    if(ota_len<544 || ota_off>0xFFFFFFFFu-ota_len)return -72;
    if (ldr_fread(OTA_RES,saved,112)) return -71;
    for (unsigned i=0;i<112;i++) b[i]=0;
    ota_wr16(b+2,0x5A0D); ota_wr16(b+4,0x5A01); ota_wr16(b+6,0x5441);
    for (unsigned i=0;i<sizeof OTA_PARM_ID-1;i++) b[8+i]=OTA_PARM_ID[i];
    ota_wr32(b+72,OTA_AREA); ota_wr16(b,ota_crc16(b+2,78,0));
    if (!ota_memeq(saved,b,112)) return -72;
    if (ota_read(ota_off,a,512)||ldr_fread(OTA_AREA,b,32)) return -71;
    len=ota_rd32(a+8);
    if (len<128 || len>((OTA_RES&~4095u)-OTA_AREA-32u) || ota_len!=len+32 ||
        ota_rd32(a+4)!=32 || ota_crc16(a+2,30,0)!=ota_rd16(a) ||
        ota_crc16(a+34,30,0)!=ota_rd16(a+32) ||
        ota_rd32(a+40)!=0x01C0A800 || ota_rd32(a+36)>0x14000 ||
        ota_rd32(b+4)!=OTA_AREA || ota_rd32(b+8)!=len+32 ||
        b[12]!=0x41 || b[13]!=255 || ota_rd16(b+14)!=65535 ||
        ota_crc16(b+2,30,0)!=ota_rd16(b) ||
        !ota_memeq(b+16,(const uint8_t *)"LOADER.BIN\0\0\0\0\0\0",16)) return -72;
    for(unsigned i=0x48;i+16<=0x88;i++)
        cap|=ota_memeq(a+i,(const uint8_t *)"X0X-BUNDLE-LDR1",16);
    if(!cap)return -72;
    for(uint32_t k=0;k<len;k+=n) {
        n=len-k;if(n>512)n=512;
        if(ota_read(ota_off+32+k,a,n)||ldr_fread(OTA_AREA+32+k,b,n))return -71;
        if(!ota_memeq(a,b,n))return -72;
        ldr_progress(k,len);
    }
    return 0;
}
static int bp_record_finish(const uint8_t *saved,uint8_t *buf)
{
    if(ldr_fread(OTA_RES,buf,112))return -71;
    if(!ota_memeq(saved,buf,112))return -72;
    /* All payloads and commit were verified before entry. After this erase,
     * a torn/failed read is an uncertain committed outcome, NOT rollback.
     * RAM record remains on error; cold boot may enter the verified app. */
    if(ldr_erase(OTA_RES&~4095u))return -73;
    if(ldr_fread(OTA_RES,buf,112))return -71;
    for(unsigned i=0;i<112;i++)if(buf[i]!=255)return -73;
    ldr_record_clear();
    return 0;
}
