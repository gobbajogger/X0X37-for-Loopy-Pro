/* SPDX-License-Identifier: GPL-3.0-only */
/* Exercise the real editor parser and sample finalizer with observable platform
 * boundaries. Project validation/runtime have separate integration regressions. */
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
/* Drive the real editor's revision-13 export preflight without a DSP/storage link. */
static int host_storage_unsupported, host_sound_invalid, host_preset_pending;
int project_storage_unsupported(void) { return host_storage_unsupported; }
int engine_bass_sound_validate(const sound_t *sound) { (void)sound; return host_sound_invalid; }
int engine_bass_preset_pending(int voice) { (void)voice; return host_preset_pending; }
static uint32_t fm1_ms, fm1_crash[2];
static struct { uint32_t failed, pending; } bootguard;
static struct { unsigned ota_req, uboot_req; } usb;
static int flash_ok = 1, flash_writes, flash_erases, held, adopts, aborts, validation, action, wire_fail, stage_held, headers;
static uint8_t frame[640], reply[800];
static uint32_t frame_n, reply_n;
static void (*wire_after_send)(void);
static void fm1_irq_off(void) {}
static void fm1_irq_on(void) {}
static void fm1_wdt_feed(void) {}
static void audio_silence(void) {}
static void fl_inval(uint32_t off, uint32_t n) { (void)off; (void)n; }
static int fl_write(uint32_t off, const void *p, uint32_t n)
{
    assert(off + n <= sizeof flash_image);
    memcpy(flash_image + off, p, n); flash_writes++; return 0;
}
static int fl_erase4k(uint32_t off, uint32_t *took)
{
    assert(off + 4096 <= sizeof flash_image);
    memset(flash_image + off, 255, 4096); *took = 1; flash_erases++; return 0;
}
/* Boot-customize paths in editor.c (ef38e221+) need the same host stubs as
 * smp_begin_cancel_test.c so this TU links under -Werror. */
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
static int store_stage_held(void) { return stage_held; }
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
        for (b = 0; b < 8; b++) c = c & 1 ? (c >> 1) ^ 0xEDB88320u : c >> 1;
    }
    return ~c;
}
void engine_brk_loops(void) {}
void engine_brk_drop_range(const uint8_t *base, uint32_t nbytes) { (void)base; (void)nbytes; }
void ui_say(const char *a, const char *b) { (void)a; (void)b; }
int ui_boot_flash_busy(void) { return 0; }
int ui_project_busy(void) { return held; }
int ui_project_begin(void) { if (held) return UI_PROJECT_BUSY; held = 1; return 0; }
void ui_project_end(int apply)
{
    assert(held); held = 0;
    if (apply) { adopts++; memcpy(&proj, &project_stage, sizeof proj); }
    else aborts++;
}
int ui_project_save(void) { return action; }
int ui_project_load(void) { return action; }
int ui_project_reset(void) { return action; }
void project_capture_stage(void) { memcpy(&project_stage, &proj, sizeof proj); }
void project_file_header(uint8_t *h) { headers++; memset(h, 0x55, PROJECT_FILE_HDR); }
int project_file_validate(const uint8_t *h) { (void)h; return validation; }
static int ota_wire_send(const uint8_t *p, uint32_t n)
{
    assert(n <= sizeof reply); memcpy(reply, p, n); reply_n = n;
    if (wire_after_send) wire_after_send();
    return wire_fail;
}
static int ota_frame_get(const uint8_t **p, uint32_t *n)
{
    *p = frame; *n = frame_n; return frame_n != 0;
}
static void ota_frame_done(void) { frame_n = 0; }
/* Prove the device overlay path: editor chunk staging aliases the same 0xD00
 * region class as ota_dec (call-scoped; no cross-message retention). */
static uint8_t host_main_scratch[4096] __attribute__((aligned(4)));
#define ED_SMP_SCRATCH (host_main_scratch + 0xD00)
#include "../../firmware/src/app/editor.c"

static void request(uint8_t cmd, const uint8_t *p, uint32_t n)
{
    assert(n + 4 <= sizeof frame);
    frame[0] = ED_HDR0; frame[1] = ED_HDR1; frame[2] = ED_HDR2; frame[3] = cmd;
    if (n) memcpy(frame + 4, p, n);
    frame_n = n + 4; reply_n = 0; ed_service(); assert(!frame_n);
    assert(reply_n >= 6 && reply[0] == 0xF0 && reply[4] == cmd && reply[reply_n - 1] == 0xF7);
}
static void op(uint8_t code, const uint8_t *p, uint32_t n, int rc)
{
    uint8_t a[600]; a[0] = code; if (n) memcpy(a + 1, p, n);
    request(ED_PROJECT, a, n + 1);
    if (reply[5] != code || reply[6] != rc)
        fprintf(stderr, "op %u: expected rc %d, got %u (next %u)\n", code, rc, reply[6], ed_project.next);
    assert(reply[5] == code && reply[6] == rc);
}
static void u21(uint8_t *p, uint32_t n) { p[0] = n & 127; p[1] = n >> 7 & 127; p[2] = n >> 14 & 127; }
static uint32_t pack(const uint8_t *p, uint32_t n, uint8_t *out)
{
    uint32_t i, j, pos = 0;
    for (i = 0; i < n; i += 7) {
        uint32_t m = pos++; out[m] = 0;
        for (j = 0; j < 7 && i + j < n; j++) {
            out[m] |= (p[i + j] >> 7) << j; out[pos++] = p[i + j] & 127;
        }
    }
    return pos;
}
static uint8_t data[PROJECT_FILE_SIZE];
static void begin_import(void)
{
    uint8_t a[3]; u21(a, sizeof data); op(67, a, 3, 0); assert(held && ed_project.mode == 2);
}
static void write_chunk(uint32_t off, uint32_t n, int rc)
{
    uint8_t a[270]; u21(a, off); n = pack(data + off, n, a + 3); op(68, a, n + 3, rc);
}
int main(void)
{
    uint8_t a[600], confirm[2] = {0x58, 0x30}, decoded[224];
    uint32_t i, n;
    for (i = 0; i < sizeof data; i++) data[i] = (uint8_t)(i * 37 + 91);
    memset(&proj, 0x42, sizeof proj);
    op(64, 0, 0, 0); assert(reply_n == 13 && reply[7] == (X0X_BASS_TAGGED_SOUND ? 2 : 1) && ed_u21(reply + 8) == sizeof data && !reply[11]);
    u21(a, sizeof data - 1); op(67, a, 3, 1); assert(!held);
    held = 1; u21(a, sizeof data); op(67, a, 3, 2); held = 0;
    host_storage_unsupported = 1; op(65, 0, 0, 2); assert(!held);
    host_storage_unsupported = 0;
    host_sound_invalid = 1; op(65, 0, 0, 2); assert(!held);
    host_sound_invalid = 0;
    host_preset_pending = 1; op(65, 0, 0, 2); assert(!held);
    host_preset_pending = 0;
    op(65, 0, 0, 0); assert(ed_project.mode == 1 && held);
    u21(a, 20); a[3] = 100; a[4] = 0; op(66, a, 5, 0);
    assert(ed_u21(reply + 7) == 20 && ed_unpack_exact(reply + 10, reply_n - 11, decoded, sizeof decoded) == 100);
    for (i = 0; i < 100; i++) assert(decoded[i] == (i < 12 ? 0x55 : 0x42));
    a[0] = 0; request(ED_SMP_ERASE, a, 1); assert(reply[6] == 4 && !flash_erases);
    op(71, 0, 0, 2); op(70, 0, 0, 0); assert(!held && aborts == 1);
    op(70, 0, 0, 0); assert(aborts == 1);
    begin_import(); op(74, 0, 0, 3); write_chunk(224, 224, 3);
    a[0] = 0; request(ED_SMP_ERASE, a, 1); assert(reply[6] == 4 && !flash_erases);
    write_chunk(0, 224, 0); write_chunk(0, 224, 0); assert(ed_project.next == 224);
    data[0] ^= 1; write_chunk(0, 224, 3); data[0] ^= 1;
    u21(a, 224); n = pack(data + 224, 21, a + 3); a[3 + n] = 0;
    op(68, a, n + 4, 1); assert(ed_project.next == 224); /* trailing mask */
    for (i = 224; i < sizeof data; i += 224)
        write_chunk(i, sizeof data - i < 224 ? sizeof data - i : 224, 0);
    assert(!memcmp(ed_project.header, data, PROJECT_FILE_HDR));
    assert(!memcmp(&project_stage, data + PROJECT_FILE_HDR, sizeof project_stage));
    op(69, 0, 0, 8); assert(!adopts && held);
    validation = -4; op(69, confirm, 2, 4); assert(!adopts && held);
    validation = -7; op(74, 0, 0, 6);
    validation = -1; op(74, 0, 0, 5);
    int headers_before_validate = headers;
    validation = 0; op(74, 0, 0, 0); assert(!adopts && held && !ed_project_samples_locked());
    assert(headers == headers_before_validate + 1); /* migrated bytes need a new CRC header */
    op(69, confirm, 2, 0); assert(adopts == 1 && !held && !ed_project.mode);
    assert(!memcmp(&proj, data + PROJECT_FILE_HDR, sizeof proj));
    assert(!flash_writes && !flash_erases);
    action = -2; op(71, 0, 0, 7); action = UI_PROJECT_BUSY; op(71, 0, 0, 2);
    action = -7; op(71, 0, 0, 6); action = 0; op(71, 0, 0, 0);
    op(72, 0, 0, 8); op(73, confirm, 2, 0);
    begin_import(); fm1_ms += 14999; op(64, 0, 0, 0); assert(reply[11] == 2);
    fm1_ms += 14999; ed_service(); assert(held);
    fm1_ms++; ed_service(); assert(!held && aborts == 2); ed_service(); assert(aborts == 2);
    /* Sample headers after #432: END needs WRITE that erased sector 0. Also
     * proves rewrite-after-live-sample WRITE still programs payload (backup
     * must not clobber ed_smp_buf). */
    {
        uint8_t header[512], original[544], smp_data[32];
        smp_user_hdr_t *h = (smp_user_hdr_t *)header;
        int end_writes = 0;

        memset(flash_image + SMP_USER_OFF(0), 0xFF, 4096);
        for (i = 0; i < sizeof smp_data; i++) smp_data[i] = (uint8_t)(i * 7 + 3);
        memset(header, 0, sizeof header); h->magic = SMP_USER_MAGIC; h->version = 1; h->nz = 1;
        h->data_len = 32; h->zone[0].n = 64; h->zone[0].rate = 65536;
        for (i = 480; i < 512; i++) header[i] = (uint8_t)(i * 13);
        h->crc = st_crc32(smp_data, sizeof smp_data);

        a[0] = 0; n = pack(header, sizeof header, a + 1); request(ED_SMP_END, a, n + 1);
        assert(reply[6] == 6);                      /* no sector-0 erase this import */

        a[0] = 0; request(ED_SMP_BEGIN, a, 1); assert(reply[6] == 0);
        a[0] = 0; u21(a + 1, 512); n = pack(smp_data, sizeof smp_data, a + 4);
        request(ED_SMP_WRITE, a, n + 4); assert(reply[9] == 0);
        assert(!memcmp(flash_image + SMP_USER_OFF(0) + 512, smp_data, sizeof smp_data));
        a[0] = 0; n = pack(header, sizeof header, a + 1); request(ED_SMP_END, a, n + 1);
        assert(reply[6] == 0 && !memcmp(flash_image + SMP_USER_OFF(0), header, sizeof header));
        end_writes++;

        /* Second import: backup shadows live sector 0 then WRITE must still land. */
        a[0] = 0; request(ED_SMP_BEGIN, a, 1); assert(reply[6] == 0);
        a[0] = 0; u21(a + 1, 512); n = pack(smp_data, sizeof smp_data, a + 4);
        request(ED_SMP_WRITE, a, n + 4); assert(reply[9] == 0);
        assert(!memcmp(flash_image + SMP_USER_OFF(0) + 512, smp_data, sizeof smp_data));
        a[0] = 0; n = pack(header, 480, a + 1); request(ED_SMP_END, a, n + 1);
        assert(reply[6] == 0);
        end_writes++;

        n = pack(header, 511, a + 1); request(ED_SMP_END, a, n + 1); assert(reply[6] == 1);
        n = pack(header, 512, a + 1); a[n + 1] = 0; request(ED_SMP_END, a, n + 2); assert(reply[6] == 1);
        assert(end_writes == 2);
        /* Capture live flash (480-byte END leaves reserved tail erased). */
        memcpy(original, flash_image + SMP_USER_OFF(0), sizeof original);

        begin_import();
        for (i = 0; i < sizeof data; i += 224)
            write_chunk(i, sizeof data - i < 224 ? sizeof data - i : 224, 0);
        op(74, 0, 0, 0);
        a[0] = 0; request(ED_SMP_BEGIN, a, 1); assert(reply[6] == 0);
        /* BEGIN alone keeps last-good (#427); WRITE must erase before recovery. */
        a[0] = 0; u21(a + 1, 512); n = pack(original + 512, 32, a + 4);
        request(ED_SMP_WRITE, a, n + 4); assert(reply[9] == 0);
        fm1_ms += 15000; ed_service();
        assert(held && ed_project.recovery);
        op(64, 0, 0, 0); assert(reply[11] == 3);
        op(70, 0, 0, 2); assert(held);              /* partial samples must never resume old playback */
        begin_import();                           /* reconnect recovery keeps the existing hold */
        for (i = 0; i < sizeof data; i += 224)
            write_chunk(i, sizeof data - i < 224 ? sizeof data - i : 224, 0);
        op(74, 0, 0, 0);
        a[0] = 0; u21(a + 1, 512); n = pack(original + 512, 32, a + 4);
        request(ED_SMP_WRITE, a, n + 4); assert(reply[9] == 0 && held);
        assert(!memcmp(flash_image + SMP_USER_OFF(0) + 512, original + 512, 32));
        a[0] = 0; n = pack(original, 512, a + 1); request(ED_SMP_END, a, n + 1);
        assert(reply[6] == 0 && held && !memcmp(original, flash_image + SMP_USER_OFF(0), sizeof original));
        op(70, 0, 0, 0); assert(!held && adopts == 1);
    }
    puts("project protocol: staging, exact chunks, CRC/capacity, confirmation, timeout and full sample headers ok");
    return 0;
}
