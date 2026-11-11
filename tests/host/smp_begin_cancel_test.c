/* SPDX-License-Identifier: GPL-3.0-only */
/* Firmware editor.c: SMP_BEGIN must keep last-good until the first WRITE. */
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
static int flash_ok = 1, flash_erases;
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
    memcpy(flash_image + off, p, n);
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

static void load_good(void)
{
    uint8_t header[512], data[32], a[600];
    smp_user_hdr_t *h = (smp_user_hdr_t *)header;
    uint32_t n, off, i;
    memset(header, 0, sizeof header);
    memset(data, 0x5A, sizeof data);
    h->magic = SMP_USER_MAGIC;
    h->version = 1;
    h->nz = 1;
    memcpy(h->name, "GOODLOOP", 8);
    h->data_len = sizeof data;
    h->zone[0].n = 64;
    h->zone[0].rate = 65536;
    h->crc = st_crc32(data, sizeof data);
    a[0] = 0;
    request(ED_SMP_BEGIN, a, 1);
    assert(reply[6] == 0);
    for (off = 512; off < 512 + sizeof data; off += 256) {
        uint32_t chunk = sizeof data - (off - 512);
        if (chunk > 256)
            chunk = 256;
        a[0] = 0;
        u21(a + 1, off);
        n = pack(data + (off - 512), chunk, a + 4);
        request(ED_SMP_WRITE, a, n + 4);
        assert(reply[9] == 0);
    }
    a[0] = 0;
    n = pack(header, 512, a + 1);
    request(ED_SMP_END, a, n + 1);
    assert(reply[6] == 0 && usr_nz[0] == 1);
    for (i = 0; i < 8; i++)
        assert(((const smp_user_hdr_t *)(flash_image + SMP_USER_OFF(0)))->name[i] == "GOODLOOP"[i]);
}

int main(void)
{
    uint8_t a[600], before[4096], data[32], header[512];
    uint32_t n, off;
    int erases_before;
    smp_user_hdr_t *h = (smp_user_hdr_t *)header;
    memset(flash_image, 0xFF, sizeof flash_image);
    load_good();
    memcpy(before, flash_image + SMP_USER_OFF(0), sizeof before);
    erases_before = flash_erases;

    a[0] = 0;
    request(ED_SMP_BEGIN, a, 1);
    assert(reply[6] == 0);
    assert(usr_nz[0] == 1);
    assert(flash_erases == erases_before);
    assert(!memcmp(before, flash_image + SMP_USER_OFF(0), sizeof before));
    puts("smp begin-cancel: BEGIN alone keeps last-good ok");

    /* BEGIN then END with no WRITE must refuse — NOR cannot rewrite a live header. */
    memset(header, 0, sizeof header);
    h->magic = SMP_USER_MAGIC;
    h->version = 1;
    h->nz = 1;
    memcpy(h->name, "NEWNAME1", 8);
    h->data_len = 32;
    h->zone[0].n = 64;
    h->zone[0].rate = 65536;
    h->crc = st_crc32((const uint8_t *)(flash_image + SMP_USER_OFF(0) + 512), 32);
    a[0] = 0;
    request(ED_SMP_BEGIN, a, 1);
    assert(reply[6] == 0);
    a[0] = 0;
    n = pack(header, 512, a + 1);
    request(ED_SMP_END, a, n + 1);
    assert(reply[6] == 6);
    assert(usr_nz[0] == 1);
    assert(!memcmp(before, flash_image + SMP_USER_OFF(0), sizeof before));
    puts("smp begin-cancel: END without WRITE refuses ok");

    /* Mid-WRITE erases sector 0; next BEGIN restores from FL_SMPV shadow. */
    memset(data, 0x5A, sizeof data);
    memset(header, 0, sizeof header);
    h->magic = SMP_USER_MAGIC;
    h->version = 1;
    h->nz = 1;
    memcpy(h->name, "GOODLOOP", 8);
    h->data_len = sizeof data;
    h->zone[0].n = 64;
    h->zone[0].rate = 65536;
    h->crc = st_crc32(data, sizeof data);
    a[0] = 0;
    request(ED_SMP_BEGIN, a, 1);
    assert(reply[6] == 0);
    off = 512;
    a[0] = 0;
    u21(a + 1, off);
    n = pack(data, sizeof data, a + 4);
    request(ED_SMP_WRITE, a, n + 4);
    assert(reply[9] == 0);
    assert(!memcmp(flash_image + SMP_USER_OFF(0) + 512, data, sizeof data));
    assert(usr_nz[0] == 0);
    a[0] = 0;
    request(ED_SMP_BEGIN, a, 1);
    assert(reply[6] == 0);
    assert(usr_nz[0] == 1);
    assert(!memcmp(before, flash_image + SMP_USER_OFF(0), sizeof before));
    puts("smp begin-cancel: mid-WRITE abandon restores last-good ok");

    /* Failed END (CRC) restores last-good. */
    a[0] = 0;
    request(ED_SMP_BEGIN, a, 1);
    off = 512;
    a[0] = 0;
    u21(a + 1, off);
    memset(data, 0xA5, sizeof data);
    n = pack(data, sizeof data, a + 4);
    request(ED_SMP_WRITE, a, n + 4);
    assert(reply[9] == 0);
    a[0] = 0;
    n = pack(header, 512, a + 1);   /* header CRC still matches old 0x5A data */
    request(ED_SMP_END, a, n + 1);
    assert(reply[6] == 3);
    assert(usr_nz[0] == 1);
    assert(!memcmp(before, flash_image + SMP_USER_OFF(0), sizeof before));
    puts("smp begin-cancel: failed END restores last-good ok");

    /* Multi-sector last-good: WRITE into 0x1000 then abandon must NOT leave
     * usr_nz set over a restored sector-0 header with wiped higher data. */
    {
        uint8_t big_hdr[512], big_data[5000], sec1_before[4096], chunk[256];
        smp_user_hdr_t *bh = (smp_user_hdr_t *)big_hdr;
        uint32_t base = SMP_USER_OFF(0);
        memset(flash_image, 0xFF, sizeof flash_image);
        memset(big_hdr, 0, sizeof big_hdr);
        memset(big_data, 0x5A, sizeof big_data);
        bh->magic = SMP_USER_MAGIC;
        bh->version = 1;
        bh->nz = 1;
        memcpy(bh->name, "BIGLOOP1", 8);
        bh->data_len = sizeof big_data;
        bh->zone[0].n = (uint32_t)sizeof big_data * 2u;
        bh->zone[0].rate = 65536;
        bh->crc = st_crc32(big_data, sizeof big_data);
        memcpy(flash_image + base, big_hdr, 512);
        memcpy(flash_image + base + 512, big_data, sizeof big_data);
        smp_user_scan(0);
        assert(usr_nz[0] == 1);
        memcpy(sec1_before, flash_image + base + 0x1000, sizeof sec1_before);

        a[0] = 0;
        request(ED_SMP_BEGIN, a, 1);
        assert(reply[6] == 0);
        for (off = 512; off <= 0x1000u; off += 256) {
            memset(chunk, 0xA5, sizeof chunk);
            a[0] = 0;
            u21(a + 1, off);
            n = pack(chunk, sizeof chunk, a + 4);
            request(ED_SMP_WRITE, a, n + 4);
            assert(reply[9] == 0);
        }
        assert(memcmp(flash_image + base + 0x1000, sec1_before, sizeof sec1_before) != 0);
        a[0] = 0;
        request(ED_SMP_BEGIN, a, 1);   /* abandon: must not resurrect corrupt sample */
        assert(reply[6] == 0);
        assert(usr_nz[0] == 0);
        /* Boot path: scan then recover — still empty, not a zombie usr_nz. */
        smp_user_scan(0);
        ed_smp_recover();
        assert(usr_nz[0] == 0);
        puts("smp begin-cancel: higher-sector mid-WRITE abandon fail-closed ok");
    }
    return 0;
}
