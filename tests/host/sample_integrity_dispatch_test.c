/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual editor service/dispatcher and scanner, fake NOR with one-way programming.
 * Platform/UI/project validator are sentinels; no device or native build proof. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define X0X_HOST 1
#define BOARD_SMK37 0
#define X0X_BLE 0
#define X0X_DRUM_TABLES_DEFINE              /* x0x.h pulls drum909 tanh helpers */
#include "../../firmware/src/app/x0x.h"

static uint8_t flash_image[0x100000];
#define SMP_USER_XIP(k) (flash_image + SMP_USER_OFF(k))
#include "../../firmware/src/app/slots.c"
project_t proj, project_stage;
static uint32_t fm1_ms, fm1_crash[2];
static struct { uint32_t failed, pending; } bootguard;
static struct { unsigned ota_req, uboot_req; } usb;
static int flash_ok = 1, flash_erases, flash_writes, fail_write;
static uint8_t frame[640], reply[800];
static uint32_t frame_n, reply_n;
static void fm1_irq_off(void) {}
static void fm1_irq_on(void) {}
static void fm1_wdt_feed(void) {}
static void audio_silence(void) {}
static void fl_inval(uint32_t off, uint32_t n) { (void)off; (void)n; }
static int fl_write(uint32_t off, const void *p, uint32_t n)
{
    assert(off + n <= sizeof flash_image);
    uint32_t i;
    if (fail_write && off == SMP_USER_OFF(0)+512) { fail_write = 0; return 1; }
    for (i = 0; i < n; i++) flash_image[off+i] &= ((const uint8_t *)p)[i];
    flash_writes++;
    return 0;
}
static int fl_erase4k(uint32_t off, uint32_t *took)
{
    assert(off + 4096 <= sizeof flash_image);
    memset(flash_image + off, 255, 4096);
    *took = 1;
    flash_erases++;
    return 0;
}
static int fl_read(uint32_t off, void *dst, uint32_t n)
{
    assert(off + n <= sizeof flash_image);
    memcpy(dst, flash_image + off, n);
    return 0;
}
static uint8_t *fm1_xip_ptr(uint32_t off)
{
    assert(off < sizeof flash_image);
    return flash_image + off;
}
static int store_stage_held(void) { return 0; }
static uint8_t *store_stage_lock(uint32_t *cap)
{
    static uint8_t buf[64];
    if (cap)
        *cap = sizeof buf;
    return buf;
}
static void store_stage_unlock(void) {}
static uint32_t st_crc32(const uint8_t *p, uint32_t n)
{
    uint32_t c = ~0u, b;
    while (n--) {
        c ^= *p++;
        for (b = 0; b < 8; b++)
            c = c & 1 ? (c >> 1) ^ 0xEDB88320u : c >> 1;
    }
    return ~c;
}
void engine_brk_loops(void) {}
void engine_brk_drop_range(const uint8_t *base, uint32_t nbytes) { (void)base; (void)nbytes; }
void ui_say(const char *a, const char *b) { (void)a; (void)b; }
int ui_boot_flash_busy(void) { return 0; }
int ui_project_busy(void) { return 0; }
int ui_project_begin(void) { return 0; }
void ui_project_end(int apply) { (void)apply; }
int ui_project_save(void) { return 0; }
int ui_project_load(void) { return 0; }
int ui_project_reset(void) { return 0; }
void project_capture_stage(void) {}
/* The sample-loader scenario does not exercise project export. Supplying
 * the real editor's new compatibility/preset admission dependencies keeps
 * the fixture linked without bypassing those checks in production. */
int project_storage_unsupported(void) { return 0; }
int engine_bass_sound_validate(const sound_t *sound) { (void)sound; return 0; }
int engine_bass_preset_pending(int voice) { (void)voice; return 0; }
void project_file_header(uint8_t *h) { memset(h, 0, PROJECT_FILE_HDR); }
int project_file_validate(const uint8_t *h) { (void)h; return 0; }
static int ota_wire_send(const uint8_t *p, uint32_t n)
{
    assert(n <= sizeof reply);
    memcpy(reply, p, n);
    reply_n = n;
    return 0;
}
static int ota_frame_get(const uint8_t **p, uint32_t *n)
{
    *p = frame;
    *n = frame_n;
    return frame_n != 0;
}
static void ota_frame_done(void) { frame_n = 0; }
static uint8_t host_main_scratch[4096] __attribute__((aligned(4)));
#define ED_SMP_SCRATCH (host_main_scratch + 0xD00)
#include "../../firmware/src/app/editor.c"

static void request(uint8_t cmd, const uint8_t *p, uint32_t n)
{
    assert(n + 4 <= sizeof frame);
    frame[0] = ED_HDR0;
    frame[1] = ED_HDR1;
    frame[2] = ED_HDR2;
    frame[3] = cmd;
    if (n)
        memcpy(frame + 4, p, n);
    frame_n = n + 4;
    reply_n = 0;
    ed_service();
    assert(!frame_n);
    assert(reply_n >= 6 && reply[0] == 0xF0 && reply[4] == cmd && reply[reply_n - 1] == 0xF7);
}
static uint32_t pack(const uint8_t *p, uint32_t n, uint8_t *out)
{
    uint32_t i, j, pos = 0;
    for (i = 0; i < n; i += 7) {
        uint32_t m = pos++;
        out[m] = 0;
        for (j = 0; j < 7 && i + j < n; j++) {
            out[m] |= (p[i + j] >> 7) << j;
            out[pos++] = p[i + j] & 127;
        }
    }
    return pos;
}
static void u21(uint8_t *p, uint32_t n)
{
    p[0] = n & 127;
    p[1] = n >> 7 & 127;
    p[2] = n >> 14 & 127;
}


static int checks, failures;
#define CHECK(label, expr) do { checks++; if (!(expr)) { failures++; printf("FAIL %s\n",label); } } while (0)
static uint8_t old_slot[0x14000];
static void reset_case(void)
{
    smp_user_hdr_t *h;
    memset(flash_image,255,sizeof flash_image);
    memset(&ed_project,0,sizeof ed_project);
    memset(smp_importing,0,sizeof smp_importing);
    memset(smp_sec0_erased,0,sizeof smp_sec0_erased);
    memset(&ed_install,0,sizeof ed_install);
    h=(smp_user_hdr_t *)(flash_image+SMP_USER_OFF(0));
    memset(h,0,sizeof *h); h->magic=SMP_USER_MAGIC; h->version=1; h->nz=1;
    h->data_len=5000; h->zone[0].n=10000; h->zone[0].rate=65536;
    memset(flash_image+SMP_USER_OFF(0)+SMP_USER_DATA,0xF0,5000);
    h->crc=st_crc32(flash_image+SMP_USER_OFF(0)+SMP_USER_DATA,5000);
    smp_user_scan(0); assert(usr_nz[0]);
    memcpy(old_slot,flash_image+SMP_USER_OFF(0),sizeof old_slot);
    flash_erases=flash_writes=fail_write=0;
}
static void begin(void) { uint8_t k=0; request(ED_SMP_BEGIN,&k,1); assert(reply[6]==0); }
static int write_chunk(unsigned off,unsigned len,uint8_t value)
{
    uint8_t a[300],data[256]; unsigned n;
    memset(data,value,len); a[0]=0; u21(a+1,off); n=pack(data,len,a+4);
    request(ED_SMP_WRITE,a,n+4); return reply[9];
}
static int finish(unsigned bad)
{
    uint8_t a[600],header[512] __attribute__((aligned(4))); smp_user_hdr_t *h=(smp_user_hdr_t *)header;
    unsigned n;
    memset(header,0,sizeof header); h->magic=SMP_USER_MAGIC; h->version=1; h->nz=1;
    h->data_len=256; h->zone[0].n=512; h->zone[0].rate=65536;
    h->crc=st_crc32(flash_image+SMP_USER_OFF(0)+512,256);
    if(bad==1)h->zone[0].rate=0;
    if(bad==2)h->zone[0].idx=89;
    if(bad==3)h->zone[0].off=257;
    if(bad==4)h->zone[0].n=514;
    a[0]=0;n=pack(header,sizeof header,a+1);request(ED_SMP_END,a,n+1);return reply[6];
}
static void project_begin_valid(void)
{
    uint8_t a[4]={67}; u21(a+1,PROJECT_FILE_SIZE);request(ED_PROJECT,a,4);assert(reply[6]==0);
    /* Project wire serialization has its own tests. Feed real sequential WRITE
     * frames, then use the sentinel validator via the actual VALIDATE command. */
    { unsigned off; uint8_t b[300],data[224]={0};
      for(off=0;off<PROJECT_FILE_SIZE;) {unsigned n=PROJECT_FILE_SIZE-off;
        if(n>224)n=224;
        b[0]=68;u21(b+1,off);unsigned z=pack(data,n,b+4);
        request(ED_PROJECT,b,z+4);assert(reply[6]==0);off+=n;}}
    a[0]=74;request(ED_PROJECT,a,1);assert(reply[6]==0);
}
int main(void)
{
    unsigned i,off;int e,w,rc;uint8_t a[3];
    reset_case();begin();rc=write_chunk(0x1100,256,0x0F);
    CHECK("out of order rejected",rc!=0); CHECK("out of order no writes",!flash_writes&&!flash_erases);
    CHECK("out of order preserves occupied slot",!memcmp(old_slot,flash_image+SMP_USER_OFF(0),sizeof old_slot));
    reset_case();begin();
    for(off=512;off<=0x1100;off+=256)assert(write_chunk(off,256,0xA5)==0);
    e=flash_erases;w=flash_writes;rc=write_chunk(0x1000,256,0xA5);
    CHECK("retry accepted",rc==0);CHECK("retry no destructive work",e==flash_erases&&w==flash_writes);
    CHECK("retry preserves sibling",flash_image[SMP_USER_OFF(0)+0x1100]==0xA5);
    rc=write_chunk(0x1000,256,0x05);CHECK("conflicting retry rejected",rc!=0);
    CHECK("conflicting retry unchanged",flash_image[SMP_USER_OFF(0)+0x1000]==0xA5);
    for(i=1;i<=4;i++) {reset_case();begin();assert(write_chunk(512,256,0xA5)==0);
      rc=finish(i);CHECK("invalid zone rejected",rc!=0);
      CHECK("invalid zone last-good restored",usr_nz[0]&&!memcmp(old_slot,flash_image+SMP_USER_OFF(0),sizeof old_slot));}
    reset_case();begin();assert(write_chunk(512,256,0xA5)==0);CHECK("valid END",finish(0)==0&&usr_nz[0]);
    for(i=0;i<2;i++){reset_case();project_begin_valid();begin();
      a[0]=i?69:70;a[1]=0x58;a[2]=0x30;request(ED_PROJECT,a,i?3:1);
      CHECK("project ends",reply[6]==0&&!ed_project.mode);
      CHECK("unused arm cleared",!smp_importing[0]);
      CHECK("installer not stuck busy",!ed_install_busy());
      rc=write_chunk(512,256,0x0F);CHECK("late write rejected",rc!=0);
      CHECK("late write keeps original",!memcmp(old_slot,flash_image+SMP_USER_OFF(0),sizeof old_slot));}
    reset_case();begin();fail_write=1;CHECK("write failure reported",write_chunk(512,256,0xA5)==3);
    CHECK("failed write retry",write_chunk(512,256,0xA5)==0);
    /* An uncommitted partial replacement must keep recovery/busy state on ABORT. */
    reset_case();project_begin_valid();begin();assert(write_chunk(512,256,0xA5)==0);
    a[0]=70;request(ED_PROJECT,a,1);
    CHECK("partial abort retains recovery hold",reply[6]==2&&ed_project.mode&&ed_project.recovery);
    CHECK("partial import stays busy",ed_install_busy()&&smp_sec0_erased[0]);
    begin(); /* Restores sector zero from shadow, preserving every original byte. */
    a[0]=70;request(ED_PROJECT,a,1);
    CHECK("restored abort clears arm",reply[6]==0&&!smp_importing[0]&&!ed_install_busy());
    CHECK("restored abort preserves data",!memcmp(old_slot,flash_image+SMP_USER_OFF(0),sizeof old_slot));
    /* Empty slots also require an explicit import before a write. */
    reset_case();memset(flash_image+SMP_USER_OFF(0),255,SMP_USER_SIZE(0));smp_user_scan(0);
    CHECK("empty slot without begin rejected",write_chunk(512,256,0xA5)!=0&&!flash_writes&&!flash_erases);
    begin();CHECK("empty slot ordered write",write_chunk(512,256,0xA5)==0);
    CHECK("empty slot valid end",finish(0)==0&&usr_nz[0]);
    /* Power loss after first-sector rewrite restores the old sample at reboot. */
    reset_case();begin();assert(write_chunk(512,256,0xA5)==0);
    memset(smp_importing,0,sizeof smp_importing);memset(smp_sec0_erased,0,sizeof smp_sec0_erased);
    smp_user_scan(0);ed_smp_recover();
    CHECK("reboot recovery restores last-good",usr_nz[0]&&!memcmp(old_slot,flash_image+SMP_USER_OFF(0),sizeof old_slot));
    printf("sample integrity: %d checks / %d failures\n",checks,failures);return failures?1:0;
}
