/* SPDX-License-Identifier: GPL-3.0-only */
#define main forward_test_unused
#include "test.c"
#undef main
static uint8_t backup[sizeof src];
static uint32_t rb_key;
static void rb_reset(void)
{
    fixture();memcpy(src,backup,sizeof src);torn=record_mode=0;
}
int main(int argc,char **argv)
{
    assert(argc==2);FILE *f=fopen(argv[1],"rb");assert(f);
    uint8_t key[4];assert(fread(key,1,4,f)==4);rb_key=ota_rd32(key);
    assert(fread(backup,1,sizeof backup,f)==sizeof backup);assert(fgetc(f)==EOF);fclose(f);
    rb_reset();assert(!bp_install(0,sizeof src,rb_key,hdr,sec,cur));
    for(unsigned i=0;i<sizeof flash;i++)assert(flash[i]==(allowed(i)?src[i]:before[i]));
    /* Same pinned plaintext encrypted for a different chip key must work. */
    rb_reset();ldr_sfc(src+0x4000,BP_RB_CRYPT_SIZE,0,rb_key);
    ldr_sfc(src+0x4000,BP_RB_CRYPT_SIZE,0,rb_key^123);reseal();
    assert(!bp_install(0,sizeof src,rb_key^123,hdr,sec,cur));
    rb_reset();src[0x5000]^=1;reseal();assert(bp_install(0,sizeof src,rb_key,hdr,sec,cur)==-76&&!erases);
    rb_reset();src[0x8B100]^=1;reseal();assert(bp_install(0,sizeof src,rb_key,hdr,sec,cur)==-76&&!erases);
    rb_reset();((bp_header *)(void *)(src+0x100000))->policy=1;reseal();assert(bp_install(0,sizeof src,rb_key,hdr,sec,cur)==-60&&!erases);
    fixture();((bp_header *)(void *)(src+0x100000))->policy=2;reseal();assert(bp_install(0,sizeof src,123,hdr,sec,cur)==-60&&!erases);
    rb_reset();src[0xAB000]=0;reseal();assert(bp_install(0,sizeof src,rb_key,hdr,sec,cur)==-60&&!erases);
    rb_reset();finish_bad=1;assert(bp_install(0,sizeof src,rb_key,hdr,sec,cur)==-68);
    assert(!memcmp(flash+0xBFF00,before+0xBFF00,112));
    rb_reset();inject=1;cut_at=~0u;assert(!bp_install(0,sizeof src,rb_key,hdr,sec,cur));unsigned total=mutation,checked=0;
    if(getenv("BP_FAST"))total=0;
    for(unsigned point=1;point<=total;point++) {
        /* Every sector's half erase and first half page, plus the final prefix.
         * Other full boundaries use the same writer proven by forward sweep. */
        if(point%34!=1&&point%34!=3&&point!=total-1)continue;
        rb_reset();inject=1;torn=1;cut_at=point;
        if(!setjmp(power_loss)){(void)bp_install(0,sizeof src,rb_key,hdr,sec,cur);assert(0);}
        inject=0;assert(!memcmp(flash+0xBFF00,before+0xBFF00,112));
        assert(!bp_install(0,sizeof src,rb_key,hdr,sec,cur));
        for(unsigned i=0;i<sizeof flash;i++)assert(flash[i]==(allowed(i)?src[i]:before[i]));
        checked++;
    }
    printf("logical baseline rollback: pinned pair, alternate key, altered pair/spares rejected, %u torn resume cuts PASS\n",checked);
    return 0;
}
