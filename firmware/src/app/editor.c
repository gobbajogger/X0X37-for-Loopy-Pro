/* SPDX-License-Identifier: GPL-3.0-only
 * Sample-slot commands adapted from Felucca's editor.c, Copyright (C) 2026 Leo Kuroshita
 * (@kurogedelic), Hugelton Instruments. */
/* X0X's subset of Felucca's editor protocol (web/EDITOR_PROTOCOL.md):
 *   F0 7D 46 4C cmd args.. F7
 * INFO (1), PING (25) and the sample-slot commands SMP_BEGIN .. SMP_INFO (11..15), so
 * tools/fm1_sample_upload.py loads break loops into the user slots unchanged. The
 * parameter / preset / track commands are Felucca's instrument and are not answered. */
#ifndef ED_MAINTENANCE_ADMIT
#define ED_MAINTENANCE_ADMIT() 1
#endif
#ifndef ED_MAINTENANCE_HELD
#define ED_MAINTENANCE_HELD() 0
#endif

#define ED_HDR0 0x7D
#define ED_HDR1 0x46
#define ED_HDR2 0x4C
enum { ED_INFO = 1, ED_PROJECT = 9, ED_SMP_BEGIN = 11, ED_SMP_WRITE, ED_SMP_END, ED_SMP_ERASE, ED_SMP_INFO, ED_SMP_READ, ED_PING = 25,
       ED_DIAG = 33, ED_BT_TEST = 34, ED_CTL_READ = 37, ED_INLOG = 38, ED_BOOT_INFO = 39,
       ED_BOOT_BEGIN = 43, ED_BOOT_WRITE, ED_BOOT_READ, ED_BOOT_END, ED_BOOT_ABORT,
       ED_INSTALL = 48, ED_CAPTURE = 49 };
#include "../../../tools/mboot/session_wire.h"
#include "boot_screen.h"
#if BOARD_SMK37
/* ED_INLOG (SMK-37, read only, a76): the inbound recorder (app/smk_inlog.h). No arg or arg 0: the entry
 * count, fm1_ms now, then 2 words for each entry (the oldest first), each word 5 x 7 bits, low bits
 * first. Then the 40 stored clip velocities and the 40 stored clip channels (one byte each).
 * Arg 1: 1, padled_dirty, then 16 GRB words of padled_rgb (the colour to send) and 16 GRB words of the
 * last frame sent to the pads (padled_buf), each word 4 x 7 bits, low bits first.
 * Arg 6: versioned read-only pad sensor snapshot, or BUSY; see web/EDITOR_PROTOCOL.md. */
static void smk_inlog_reply(uint32_t arg);   /* main_fm1.c */
#endif
/* ED_CTL_READ (SMK-37 with Bluetooth, for tools/fm1_install.py; read only): the Bluetooth block
 * (x0x_ble_abi.h XB_FLASH_OFF, XB_FLASH_SIZE = fm1_flash_map.h FL_BLE_LO..FL_BLE_HI). Offset (3 x 7 bits,
 * from the block start), count (pairs, 7 bits) -> offset, the bytes packed 7 to 8 (as ED_SMP_READ).
 * An offset outside the block gets no reply; the count stops at the block end. 36 (ABI 3 CTL2_READ,
 * piece 2) is not used, so an ABI 3 tool gets no reply. */
/* ED_BT_TEST (SMK-37 with Bluetooth, for tools/ble_autotest.py): op 0 = query, 1 = clear the start
 * block (as BLUETOOTH OFF), 2 = restart the unit (a chip reset, as fm1_reboot; never the update
 * loader). Reply: op, BLUETOOTH setting, guard blocked, plat_ble_state(). */
static uint32_t ed_reboot_at;                /* fm1_ms + 1 when a restart was asked for (0: none) */
#define ED_READ_MAX 224u                     /* X0X: SMP_READ's bytes a frame (the break page draws the slots) */

static uint8_t ed_out[732];                  /* ED_INLOG with 64 entries: 732 B (tests/diag_size_test.py) */
static uint32_t ed_n;

static void ed_begin(uint32_t cmd)
{
    ed_out[0] = 0xF0;
    ed_out[1] = ED_HDR0;
    ed_out[2] = ED_HDR1;
    ed_out[3] = ED_HDR2;
    ed_out[4] = (uint8_t)cmd;
    ed_n = 5;
}
static void ed_b(uint32_t v)
{
    if (ed_n < sizeof ed_out - 1u)
        ed_out[ed_n++] = (uint8_t)(v & 0x7Fu);
}
static void ed_str(const char *s, uint32_t max)
{
    uint32_t i;
    for (i = 0; s && s[i] && i < max; i++)
        ed_b((uint8_t)s[i] & 0x7Fu);
    ed_b(0);
}
static int ed_send(void)
{
    ed_out[ed_n++] = 0xF7;
    return ota_wire_send(ed_out, ed_n);
}

/* EC-026: a volatile mutation watch, not a lock or authentication mechanism.
 * No project/OTA scratch is retained. A START retry cannot clear dirty state.
 * The per-boot ticket defeats A -> B -> delayed A / old COMMIT interleaving.
 * Full hostile START+COMMIT replay across reboot is not authenticated here. */
#define ED_INSTALL_TOKEN 16u
#define ED_INSTALL_TICKET_MAX 0x0FFFFFFFu
static struct {
    uint32_t serial, ticket;
    uint8_t token[ED_INSTALL_TOKEN], state; /* 0 none, 1 watching, 2 dirty, 3 consumed */
} ed_install;
static uint8_t ed_install_pending; /* 1 authorize, 2 discard legacy requests after failed COMMIT */
static void ed_install_dirty(void)
{
    if (ed_install.state == 1u)
        ed_install.state = 2u;
}
static void ed_install_forget(void)
{
    if (ed_install.state)
        ed_install.state = 3u;
    ed_install_pending = 0;
}

static uint32_t ed_unpack7(const uint8_t *a, uint32_t na, uint8_t *out, uint32_t max)
{
    uint32_t n = 0;
    while (na && n < max) {
        uint32_t m = *a++, j;
        na--;
        for (j = 0; j < 7u && na && n < max; j++, na--)
            out[n++] = (uint8_t)(*a++ | ((m >> j) & 1u) << 7);
    }
    return n;
}
/* n bytes packed 7 to 8: a byte of high bits, then up to 7 low 7-bit bytes (ED_SMP_READ, ED_CTL_READ) */
static void ed_put7(const uint8_t *src, uint32_t n)
{
    uint32_t k, j;
    for (k = 0; k < n; k += 7) {
        uint32_t m = 0;
        for (j = 0; j < 7u && k + j < n; j++)
            m |= (uint32_t)(src[k + j] >> 7) << j;
        ed_b(m);
        for (j = 0; j < 7u && k + j < n; j++)
            ed_b(src[k + j]);
    }
}
/* Sample/project chunk staging: 512 B, live only for the duration of one ed_handle
 * (fill then consume; no cross-message retention). On the device this aliases the
 * main-loop scratch at OTA_DEC_OFF (x0x.c ED_SMP_SCRATCH), the same call-scoped
 * ownership class as ota_dec / PROJECT_IO_BUFFER. Host builds keep a private buffer. */
#define ED_SMP_BUF_SIZE 512u
#ifdef ED_SMP_SCRATCH
_Static_assert(ED_SMP_BUF_SIZE <= 640u, "ed_smp_buf fits the ota_dec region");
#define ed_smp_buf ED_SMP_SCRATCH
#else
static uint8_t ed_smp_buf[ED_SMP_BUF_SIZE] __attribute__((aligned(4)));
#endif

/* Unlike the legacy decoder, reject truncated groups, excess bytes and noncanonical
 * high-bit masks. A malformed frame must never be accepted as a shorter write. */
static uint32_t ed_unpack_exact(const uint8_t *a, uint32_t na, uint8_t *out, uint32_t max)
{
    uint32_t n = na - (na + 7u) / 8u, i, pos = 0;
    if (!n || n > max || na % 8u == 1u)
        return 0;
    while (na) {
        uint32_t mask = *a++, take = --na > 7u ? 7u : na;
        if (mask >> take)
            return 0;
        for (i = 0; i < take; i++, na--) {
            if (*a & 0x80u)
                return 0;
            out[pos++] = (uint8_t)(*a++ | ((mask >> i) & 1u) << 7);
        }
    }
    return n;
}

/* PROJECT version 1. The project-stage allocation doubles as UI undo storage;
 * ui_project_begin freezes input/audio, and end restores that storage on every exit.
 * A browser restores sample slots only while mode 2 holds this lock, after VALIDATE. */
static struct {
    uint32_t at, next;
    uint32_t sample_len[SMP_USER_SLOTS], sample_crc[SMP_USER_SLOTS];
    uint8_t mode, valid, changed, recovery, header[PROJECT_FILE_HDR];
} ed_project;
/* Revision 13 is the only admitted project representation. */
#define ED_PROJECT_SIZE PROJECT_FILE_SIZE

static uint32_t ed_project_sample_len(uint32_t k)
{
    const smp_user_hdr_t *h = (const smp_user_hdr_t *)smp_user_xip(k);
    return usr_nz[k] ? SMP_USER_DATA + h->data_len : 0;
}
static void ed_project_sample_snapshot(void)
{
    uint32_t k;
    for (k = 0; k < SMP_USER_SLOTS; k++) {
        uint32_t n = ed_project_sample_len(k);
        ed_project.sample_len[k] = n;
        ed_project.sample_crc[k] = n ? st_crc32(smp_user_xip(k), n) : 0;
    }
}
static int ed_project_samples_restored(void)
{
    uint32_t k;
    if (!ed_project.changed)
        return 1;
    for (k = 0; k < SMP_USER_SLOTS; k++) {
        uint32_t n = ed_project_sample_len(k);
        if (n != ed_project.sample_len[k] || (n && st_crc32(smp_user_xip(k), n) != ed_project.sample_crc[k]))
            return 0;
    }
    return 1;
}

static uint32_t ed_u21(const uint8_t *a)
{
    return (uint32_t)a[0] | (uint32_t)a[1] << 7 | (uint32_t)a[2] << 14;
}
static void ed_w21(uint32_t v) { ed_b(v); ed_b(v >> 7); ed_b(v >> 14); }
static uint8_t *ed_project_byte(uint32_t off)
{
    return off < PROJECT_FILE_HDR ? &ed_project.header[off] : (uint8_t *)&project_stage + off - PROJECT_FILE_HDR;
}
static int ed_project_samples_locked(void)
{
    return ed_project.mode == 1 || (ed_project.mode == 2 && !ed_project.valid);
}
static int ed_project_end(int apply)
{
    if (ed_project.mode) {
        if (!apply && !ed_project_samples_restored()) {
            ed_project.recovery = 1;
            ui_say("SAMPLE RECOVERY NEEDED", "");
            return 2;                               /* never resume old music against partial samples */
        }
        ed_project.mode = 0;
        ed_project.recovery = 0;
        ui_project_end(apply);
    }
    return 0;
}
static uint32_t ed_project_rc(int rc)
{
    return rc == 0 ? 0u : rc == UI_PROJECT_BUSY ? 2u : rc == -4 ? 4u : rc == -7 ? 6u : rc == -10 ? 9u : 5u;
}
static void ed_project_handle(const uint8_t *a, uint32_t na)
{
    uint32_t op, rc = 0, off = 0, n = 0, i;
    if (!na)
        return;
    op = *a++; na--;
    ed_b(op);
    if (ed_project.mode)
        ed_project.at = fm1_ms;
    switch (op) {
    case 64:                                        /* QUERY / keepalive */
        if (na) { rc = 1; break; }
        ed_b(0); ed_b(X0X_BASS_TAGGED_SOUND?2:1); ed_w21(PROJECT_FILE_SIZE); ed_b(ed_project.recovery ? 3 : ed_project.mode);
        return;
    case 65:                                        /* EXPORT BEGIN */
    case 67:                                        /* IMPORT BEGIN */
        if (op == 65 && (project_storage_unsupported() || engine_bass_sound_validate(&proj.sound) ||
            engine_bass_preset_pending(0) || engine_bass_preset_pending(1))) { rc=2; break; }
        if ((op == 65 && na) || (op == 67 && (na != 3 || ed_u21(a) != PROJECT_FILE_SIZE))) {
            rc = 1; break;
        }
        if (op == 67 && ed_project.recovery) {
            ed_project.recovery = 0;                 /* recover under the existing audio/input hold */
        } else {
            if (ed_project.mode || ui_project_begin()) { rc = 2; break; }
            ed_project.changed = 0;
            if (op == 67)
                ed_project_sample_snapshot();
        }
        ed_project.mode = (uint8_t)(op == 65 ? 1 : 2);
        ed_project.at = fm1_ms;
        ed_project.next = 0;
        ed_project.valid = 0;
        if (op == 65) {
            project_capture_stage();
            project_file_header(ed_project.header);
            ed_b(0); ed_w21(PROJECT_FILE_SIZE);
            return;
        }
        break;
    case 66:                                        /* READ */
        if (ed_project.mode != 1) { rc = 2; break; }
        if (na != 5) { rc = 1; break; }
        off = ed_u21(a); n = (uint32_t)a[3] | (uint32_t)a[4] << 7;
        if (!n || n > ED_READ_MAX || off > ED_PROJECT_SIZE || n > ED_PROJECT_SIZE - off) {
            rc = 1; break;
        }
        for (i = 0; i < n; i++)
            ed_smp_buf[i] = *ed_project_byte(off + i);
        ed_b(0); ed_w21(off); ed_put7(ed_smp_buf, n);
        return;
    case 68:                                        /* sequential WRITE, exact retries are idempotent */
        if (ed_project.mode != 2) { rc = 2; break; }
        if (na < 5) { rc = 1; break; }
        off = ed_u21(a);
        n = ed_unpack_exact(a + 3, na - 3, ed_smp_buf, ED_READ_MAX);
        if (!n || off > ED_PROJECT_SIZE || n > ED_PROJECT_SIZE - off) { rc = 1; break; }
        if (off > ed_project.next || (off < ed_project.next && n > ed_project.next - off)) {
            rc = 3; break;
        }
        if (off < ed_project.next) {
            for (i = 0; i < n; i++)
                if (*ed_project_byte(off + i) != ed_smp_buf[i]) { rc = 3; break; }
            if (rc) break;
        } else {
            ed_project.valid = 0;
            for (i = 0; i < n; i++)
                *ed_project_byte(off + i) = ed_smp_buf[i];
            ed_project.next += n;
        }
        ed_b(0); ed_w21(ed_project.next);
        return;
    case 69:                                        /* COMMIT, deliberately never saved implicitly */
    case 74:                                        /* VALIDATE before any sample write */
        if (ed_project.mode != 2) { rc = 2; break; }
        if (op == 69 && (na != 2 || a[0] != 0x58 || a[1] != 0x30)) { rc = 8; break; }
        if (op == 74 && na) { rc = 1; break; }
        if (ed_project.next != ED_PROJECT_SIZE) { rc = 3; break; }
        rc = ed_project_rc(project_file_validate(ed_project.header));
        ed_project.valid = rc == 0;
        /* Recompute the staged header after validation so subsequent
         * VALIDATE/COMMIT operations check the current bytes' CRC. */
        if (!rc) project_file_header(ed_project.header);
        if (!rc && op == 69)
            ed_project_end(1);
        break;
    case 70:                                        /* ABORT, including an already expired session */
        if (na) { rc = 1; break; }
        rc = (uint32_t)ed_project_end(0);
        break;
    case 71:                                        /* SAVE */
    case 72:                                        /* LOAD SAVED */
    case 73: {                                      /* RESET */
        int result;
        if (ed_project.mode) { rc = 2; break; }
        if (op == 71 ? na != 0 : na != 2 || a[0] != 0x58 || a[1] != 0x30) {
            rc = op == 71 ? 1 : 8; break;
        }
        result = op == 71 ? ui_project_save() : op == 72 ? ui_project_load() : ui_project_reset();
        rc = result == UI_PROJECT_BUSY ? 2u : result == -7 ? 6u : result ? 7u : 0u;
        break;
    }
    default: rc = 1; break;
    }
    ed_b(rc);
}

static uint32_t ed_smp_slot(uint32_t k) { return SMP_USER_OFF(k); }
/* Armed for rewrite: WRITE may touch the slot. Flash stays intact until the first
 * sector erase, so cancel after BEGIN alone keeps the last good sample. */
static uint8_t smp_importing[SMP_USER_SLOTS];
static uint8_t smp_sec0_erased[SMP_USER_SLOTS];
static void ed_smp_inval(uint32_t k)
{
    fm1_irq_off();
    fl_inval(ed_smp_slot(k), SMP_USER_SIZE(k));
    fm1_irq_on();
}
/* Drop live use of slot k before flash rewrite. Does not erase. */
static void ed_smp_retire(uint32_t k)
{
    if (!usr_nz[k])
        return;
    usr_nz[k] = 0;
    engine_brk_loops();                               /* the next trigger reads the replacement, not this slot */
    engine_brk_drop_range(smp_user_xip(k), SMP_USER_SIZE(k));
}
/* Silence audio then erase one 4 KiB sector (import / boot flash paths). */
static int ed_silence_erase4k(uint32_t off, uint32_t *took)
{
    uint32_t local;
    if (!took)
        took = &local;
    audio_silence();
    return fl_erase4k(off, took);
}
#if SMP_SEC0_BACKUP
/* Erase 4 KiB at dst and program from src through ed_smp_buf (backup and restore). */
static int ed_smp_copy_4k(uint32_t dst, const uint8_t *src)
{
    uint32_t i;
    if (ed_silence_erase4k(dst, 0))
        return 1;
    for (i = 0; i < 0x1000u; i += 256u) {
        uint32_t n = 0x1000u - i;
        if (n > 256u)
            n = 256u;
        memcpy(ed_smp_buf, src + i, n);
        if (fl_write(dst + i, ed_smp_buf, n))
            return 1;
        fm1_wdt_feed();
    }
    return 0;
}
/* Copy live sector 0 into FL_SMPV before the first destructive erase. */
static int ed_smp_backup_sec0(uint32_t k)
{
    return ed_smp_copy_4k(SMP_SEC0_BACKUP_OFF(k), smp_user_xip(k));
}
static int ed_smp_backup_present(uint32_t k)
{
    const smp_user_hdr_t *h = (const smp_user_hdr_t *)fm1_xip_ptr(SMP_SEC0_BACKUP_OFF(k));
    return h->magic == SMP_USER_MAGIC && h->version == 1 && h->nz && h->nz <= 16u;
}
static void ed_smp_clear_backup(uint32_t k)
{
    if (!ed_smp_backup_present(k))
        return;
    (void)ed_silence_erase4k(SMP_SEC0_BACKUP_OFF(k), 0);
}
/* Put sector 0 back from FL_SMPV when an import aborted after erasing it. */
static int ed_smp_restore_sec0(uint32_t k)
{
    uint32_t dst = ed_smp_slot(k);
    const uint8_t *src;
    if (!ed_smp_backup_present(k))
        return 1;
    src = fm1_xip_ptr(SMP_SEC0_BACKUP_OFF(k));
    if (ed_smp_copy_4k(dst, src))
        return 1;
    ed_smp_inval(k);
    smp_user_scan(k);
    smp_sec0_erased[k] = 0;
    smp_importing[k] = 0;
    ed_smp_clear_backup(k);
    engine_brk_loops();
    return usr_nz[k] ? 0 : 1;
}
/* Recover last-good after a failed END, a new BEGIN, or boot (power loss). */
static void ed_smp_try_restore(uint32_t k)
{
    if (usr_nz[k]) {
        ed_smp_clear_backup(k);
        return;
    }
    if (ed_smp_backup_present(k))
        (void)ed_smp_restore_sec0(k);
}
void ed_smp_recover(void)
{
    uint32_t k;
    for (k = 0; k < SMP_USER_SLOTS; k++)
        ed_smp_try_restore(k);
}
#else
static int ed_smp_backup_sec0(uint32_t k) { (void)k; return 0; }
static void ed_smp_clear_backup(uint32_t k) { (void)k; }
static void ed_smp_try_restore(uint32_t k) { (void)k; }
void ed_smp_recover(void) {}
#endif
static int ed_smp_begin(uint32_t k)
{
    ed_smp_try_restore(k);                            /* prior abandon / power-loss mid-WRITE */
    if (ed_project.mode == 2)
        ed_project.changed = 1;
    smp_importing[k] = 1;
    smp_sec0_erased[k] = 0;
    return 0;
}
static int ed_smp_erase(uint32_t k, uint32_t all)
{
    uint32_t i;
    int rc = 0;
    if (ed_project.mode == 2)
        ed_project.changed = 1;
    ed_smp_retire(k);
    smp_importing[k] = 0;
    smp_sec0_erased[k] = 0;
    ed_smp_clear_backup(k);
    for (i = 0; i < (all ? SMP_USER_SIZE(k) / 0x1000u : 1u) && !rc; i++) {
        rc = ed_silence_erase4k(ed_smp_slot(k) + i * 0x1000u, 0);
        fm1_wdt_feed();
    }
    ed_smp_inval(k);
    return rc;
}
/* Erase the 4 KiB sector that holds off. Sector 0 is deferred until the first
 * WRITE into it (BEGIN no longer wipes the header). Before that erase, shadow
 * sector 0 into FL_SMPV so cancel / failed END can restore last-good.
 * Higher sectors: FL_SMPV only holds sector 0, so wiping past 0x1000 already
 * destroys unrecoverable last-good. Clear any sec0 shadow (do not restore a
 * header over wiped data) and, if sector 0 is still live, erase it too so a
 * valid header cannot outlive its higher-sector payload. */
static int ed_smp_erase_for_write(uint32_t k, uint32_t off, uint32_t *took)
{
    if (off < 0x1000u) {
        if (smp_sec0_erased[k])
            return 0;
        if (usr_nz[k] && ed_smp_backup_sec0(k))
            return 1;                                 /* refuse wipe if shadow cannot be stored */
        ed_smp_retire(k);
        if (ed_silence_erase4k(ed_smp_slot(k), took))
            return 1;
        smp_sec0_erased[k] = 1;
        ed_smp_inval(k);
        return 0;
    }
    if (off & 0xFFFu)
        return 0;
    if (!smp_sec0_erased[k]) {
        /* First destructive touch is past sector 0: last-good cannot be kept. */
        ed_smp_retire(k);
        if (ed_silence_erase4k(ed_smp_slot(k), took))
            return 1;
        smp_sec0_erased[k] = 1;
        ed_smp_inval(k);
    } else {
        ed_smp_retire(k);
    }
    if (ed_silence_erase4k(ed_smp_slot(k) + off, took))
        return 1;
    ed_smp_clear_backup(k);                           /* sec0-only shadow would lie after this */
    return 0;
}
static int ed_smp_end(uint32_t k, const uint8_t *a, uint32_t na)
{
    const smp_user_hdr_t *h = (const smp_user_hdr_t *)ed_smp_buf;
    uint32_t n = ed_unpack_exact(a, na, ed_smp_buf, ED_SMP_BUF_SIZE);
    if (n != sizeof(smp_user_hdr_t) && n != SMP_USER_DATA) {
        ed_smp_try_restore(k);
        return 1;
    }
    if (h->magic != SMP_USER_MAGIC || h->version != 1 || !h->nz || h->nz > 16u ||
        h->data_len > SMP_USER_SIZE(k) - SMP_USER_DATA) {
        ed_smp_try_restore(k);
        return 2;
    }
    /* NOR can only clear bits. Programming a new header onto a live sector 0
     * AND-corrupts the previous one (BEGIN no longer erases). Require a WRITE
     * that erased sector 0 this import; otherwise keep last-good and refuse. */
    if (!smp_sec0_erased[k]) {
        smp_importing[k] = 0;
        return 6;
    }
    ed_smp_inval(k);
    if (st_crc32(smp_user_xip(k) + SMP_USER_DATA, h->data_len) != h->crc) {
        ed_smp_try_restore(k);
        return 3;
    }
    if (ed_project.mode == 2)
        ed_project.changed = 1;
    if (fl_write(ed_smp_slot(k), ed_smp_buf, n)) {
        ed_smp_try_restore(k);
        return 4;
    }
    ed_smp_inval(k);
    smp_user_scan(k);
    smp_importing[k] = 0;
    smp_sec0_erased[k] = 0;
    ed_smp_clear_backup(k);
    engine_brk_loops();
    return usr_nz[k] ? 0 : 5;
}

#ifndef X0X_DEBUG
#define X0X_DEBUG 0
#endif
#if X0X_DEBUG
/* development builds only (X0X_DEBUG=1; tools/build.py refuses it in a release): the clock work
 * reads and writes the chip's registers live over USB.
 *   PEEK  40 addr n        -> addr, n words      (n <= 16)
 *   POKE  41 addr value    -> addr, old, new
 *   CLOCK 42 iterations    -> 24 MHz ticks a fixed loop took (IRQs off), C0_TL_CKCNT before, after
 * A number is 5 x 7 bits, least significant first. Only these ranges, word aligned: an address
 * outside them gets no reply (a stray read of a gated block could fault). */
enum { ED_PEEK = 40, ED_POKE, ED_CLOCK };
#include "fm1_debug.h"
static uint32_t ed_u35(const uint8_t *a)
{
    return (uint32_t)a[0] | (uint32_t)a[1] << 7 | (uint32_t)a[2] << 14 | (uint32_t)a[3] << 21 | (uint32_t)a[4] << 28;
}
static void ed_w35(uint32_t v)
{
    uint32_t k;
    for (k = 0; k < 5u; k++)
        ed_b(v >> (7u * k));
}
static int ed_debug(uint32_t cmd, const uint8_t *a, uint32_t na)
{
    uint32_t addr, i;
#if X0X_DEBUG > 1                                     /* the clock work's timing loop: level 2 (room) */
    if (cmd == ED_CLOCK) {
        uint32_t it, c0, c1;
        if (na < 5u)
            return 0;
        it = ed_u35(a);
        if (!it || it > 50000000u)
            return 0;
        ed_w35(fm1_dbg_clock(it, &c0, &c1));
        ed_w35(c0);
        ed_w35(c1);
        return 1;
    }
#endif
    if (na < 10u)
        return 0;
    addr = ed_u35(a);
    if (cmd == ED_PEEK) {
        uint32_t n = ed_u35(a + 5);
        if (!n || n > 16u || !fm1_dbg_ok(addr, n))
            return 0;
        ed_w35(addr);
        for (i = 0; i < n; i++)
            ed_w35(fm1_dbg_rd(addr + 4u * i));
        return 1;
    }
    if (!fm1_dbg_ok(addr, 1))
        return 0;
    ed_w35(addr);
    ed_w35(fm1_dbg_rd(addr));
    fm1_dbg_wr(addr, ed_u35(a + 5));
    ed_w35(fm1_dbg_rd(addr));
    return 1;
}
#endif

/* Boot screen upload. Chunks sit in the existing store buffer. Flash is
 * touched only by END, and only with a whole valid record, written to the
 * inactive copy with the header last. */
static boot_stage_t bst;
static boot_io_t bio;

static int boot_dev_erase(void *ctx, uint32_t off)
{
    (void)ctx;
    fm1_wdt_feed();
    return ed_silence_erase4k(off, 0);
}
static int boot_dev_prog(void *ctx, uint32_t off, const uint8_t *src, uint32_t n)
{
    (void)ctx;
    return fl_write(off, src, n);
}
static int boot_dev_read(void *ctx, uint32_t off, uint8_t *dst, uint32_t n)
{
    (void)ctx;
    return fl_read(off, dst, n);
}
static int boot_cmd(uint32_t cmd)
{
    return cmd == ED_BOOT_INFO || (cmd >= ED_BOOT_BEGIN && cmd <= ED_BOOT_ABORT);
}
static void boot_housekeep(void)
{
    if (boot_poll(&bst, fm1_ms) == BOOT_LATE)
        store_stage_unlock();
}
static const uint8_t *boot_shown(void)
{
    const uint8_t *a, *b;
    int w;
    if (!flash_ok)
        return 0;
    a = fm1_xip_ptr(BOOT_SEC0);
    b = fm1_xip_ptr(BOOT_SEC1);
    w = boot_pick(a, b);
    return w < 0 ? 0 : (w ? b : a);
}
static void boot_finish(int rc)
{
    if (!bst.armed)
        store_stage_unlock();
    ed_b((uint32_t)rc);
}
static void ed_boot(uint32_t cmd, const uint8_t *a, uint32_t na)
{
    int rc;
    if (cmd == ED_BOOT_INFO) {
        const uint8_t *sec = boot_shown();
        const uint8_t *pay = sec ? sec + BOOT_PAY_OFF : 0;
        char l0[13], l1[22];
        boot_lines(!flash_ok ? 0 : pay, l0, l1);
        ed_b(flash_ok ? (pay && pay[0] == BOOT_KIND_CUSTOM ? 0u : 1u) : (uint32_t)BOOT_NOFLASH);
        ed_b(pay && (pay[1] & BOOT_F_PIC) ? 1u : 0u);
        ed_str(l0, 12);
        ed_str(l1, 21);
        return;
    }
    if (cmd == ED_BOOT_ABORT) {
        boot_abort(&bst);
        store_stage_unlock();
        ed_b(0);
        return;
    }
    if (cmd == ED_BOOT_READ) {
        const uint8_t *sec = boot_shown();
        uint32_t off, n, len;
        if (na < 4u || !sec) {
            ed_b(1);
            return;
        }
        off = (uint32_t)a[0] | (uint32_t)a[1] << 7 | (uint32_t)a[2] << 14;
        len = boot_u32(sec + 12);
        n = a[3] > ED_READ_MAX / 2u ? ED_READ_MAX : a[3] * 2u;
        if (off >= len) {
            ed_b(1);
            return;
        }
        if (n > len - off)
            n = len - off;
        ed_b(0);
        ed_b(off);
        ed_b(off >> 7);
        ed_b(off >> 14);
        ed_put7(sec + BOOT_PAY_OFF + off, n);
        return;
    }
    if (cmd == ED_BOOT_BEGIN) {
        uint32_t cap = 0;
        uint8_t *b;
        if (!flash_ok) {
            ed_b(BOOT_NOFLASH);
            return;
        }
        if (ui_boot_flash_busy()) {
            ed_b(BOOT_BUSY);
            return;
        }
        if (!store_stage_held()) {
            b = store_stage_lock(&cap);
            if (!b) {
                ed_b(BOOT_BUSY);
                return;
            }
            bst.buf = b;
            bst.cap = cap;
        }
        bio.erase = boot_dev_erase;
        bio.prog = boot_dev_prog;
        bio.read = boot_dev_read;
        bio.ctx = 0;
        rc = boot_begin(&bst, fm1_ms);
        if (rc != BOOT_OK) {
            bst.armed = 0;
            store_stage_unlock();
        }
        ed_b((uint32_t)rc);
        return;
    }
    if (cmd == ED_BOOT_WRITE) {
        uint32_t off = 0, n;
        if (na >= 3u)
            off = (uint32_t)a[0] | (uint32_t)a[1] << 7 | (uint32_t)a[2] << 14;
        n = na >= 3u ? ed_unpack_exact(a + 3, na - 3u, ed_smp_buf, 256u) : 0u;
        rc = boot_write(&bst, off, n ? ed_smp_buf : 0, n, fm1_ms);
        ed_b(off);
        ed_b(off >> 7);
        ed_b(off >> 14);
        boot_finish(rc);
        return;
    }
    if (cmd == ED_BOOT_END) {
        if (ui_boot_flash_busy()) {
            ed_b(BOOT_BUSY);
            return;
        }
        rc = boot_end(&bst, &bio, BOOT_SEC0, BOOT_SEC1, fm1_ms);
        boot_finish(rc);
        return;
    }
    ed_b(BOOT_BAD);
}

static int ed_install_busy(void)
{
    uint32_t k;
    if (!flash_ok || ed_project.mode || ui_project_busy() || bst.armed ||
        store_stage_held() || ed_reboot_at || usb.ota_req || usb.uboot_req)
        return 1;
    for (k = 0; k < SMP_USER_SLOTS; k++)
        if (smp_importing[k] || smp_sec0_erased[k])
            return 1;
    return 0;
}

/* START=1 token[16]; CANCEL=2 / COMMIT=3 token[16], ticket[4 x 7 bits].
 * Reply op, rc, token[16], ticket[4]; malformed envelopes reply only op, 1.
 * rc: 0 ok, 1 format, 2 busy, 3 stale/consumed, 4 changed, 5 exhausted.
 * Return true only for a consumed, valid COMMIT; caller must also enqueue its
 * entire ACK successfully before publishing the main-loop request. */
static int ed_install_handle(const uint8_t *a, uint32_t na)
{
    uint32_t op = na ? a[0] : 0, k, ticket = 0, rc = 0, any = 0;
    int same, start = 0;
    ed_b(op);
    if ((op != 1u && op != 2u && op != 3u) ||
        na != 1u + ED_INSTALL_TOKEN + (op == 1u ? 0u : 4u)) {
        ed_b(1); return 0;
    }
    for (k = 1; k < na; k++) {
        if (a[k] & 0x80u) { ed_b(1); return 0; }
        if (k <= ED_INSTALL_TOKEN) any |= a[k];
    }
    if (!any) { ed_b(1); return 0; }
    if (op != 1u)
        for (k = 0; k < 4u; k++) ticket |= (uint32_t)a[1u + ED_INSTALL_TOKEN + k] << (7u * k);
    same = ed_install.state && !memcmp(ed_install.token, a + 1, ED_INSTALL_TOKEN);
    if (op == 1u) {
        if (same) {
            ticket = ed_install.ticket;
            rc = ed_install.state == 2u ? 4u : ed_install.state == 3u ? 3u :
                 ed_install_busy() ? 2u : 0u;
        } else if (ed_install_busy()) rc = 2;
        else if (ed_install.serial == ED_INSTALL_TICKET_MAX) rc = 5;
        else {
            memcpy(ed_install.token, a + 1, ED_INSTALL_TOKEN);
            ticket = ed_install.ticket = ++ed_install.serial;
            ed_install.state = 1;
        }
    } else if (!same || ticket != ed_install.ticket || ed_install.state == 3u) rc = 3;
    else if (op == 2u) ed_install_forget();
    else if (ed_install.state == 2u) { rc = 4; ed_install_forget(); }
    else if (ed_install_busy()) { rc = 2; ed_install_forget(); }
    else {
        ed_install_forget();                         /* consume even if ACK enqueue fails */
        start = 1;
    }
    ed_b(rc);
    for (k = 0; k < ED_INSTALL_TOKEN; k++) ed_b(a[k + 1u]);
    for (k = 0; k < 4u; k++) ed_b(ticket >> (7u * k));
    return start;
}

static void ed_handle(const uint8_t *f, uint32_t n)
{
    uint32_t cmd = f[3], i;
    const uint8_t *a = f + 4;
    uint32_t na = n - 4u;
    if (!ED_MAINTENANCE_ADMIT())
        return; /* no synthetic success/busy payload in an unrelated command format */
    ed_begin(cmd);
    /* Before validation: BEGIN and even failed END may restore a shadow. This
     * also covers empty-slot writes and malformed mutation attempts. Startup
     * recovery runs before any watch can exist. No other runtime path writes
     * user-slot bytes. A read or INFO must never reset this state. */
    if (cmd >= ED_SMP_BEGIN && cmd <= ED_SMP_ERASE)
        ed_install_dirty();
    switch (cmd) {
    case ED_CAPTURE: {
        /* MBOOT read-only session, disabled: production has no qualified
         * provider. This only reports zero capabilities. It cannot acquire a
         * lease, read flash, or dispatch a write operation. */
        uint8_t reply[21];
        uint32_t qn = ed_unpack_exact(a, na, ed_smp_buf, MB_WIRE_REQUEST_MAX);
        size_t rn = mb_wire_disabled(ed_smp_buf, qn, reply, sizeof reply);
        ed_put7(reply, (uint32_t)rn);
        break;
    }
    case ED_INSTALL: {
        int start = ed_install_handle(a, na);
        int sent = ed_send();
        if (na && a[0] == 3u)
            ed_install_pending = sent == 0 && start ? 1u : 2u;
        /* Even a refused/uncertain COMMIT cannot inherit a legacy IRQ request
         * that arrived while the response was being queued. */
        return;
    }
    case ED_PROJECT:
        ed_project_handle(a, na);
        break;
    case ED_INFO:
        ed_str("X0X " X0X_VERSION, 24);
        ed_b(0);                                      /* no engines / parameters over this protocol */
        ed_b(0);
        ed_b(0);
        ed_b(NSTEPS);
        ed_b(0);
        ed_b(0);
        ed_b(0x58); ed_b(0x53); ed_b(0x47);           /* XSG: atomic sample snapshot admission */
        ed_b(1); ed_b(ED_INSTALL);
        break;
    case ED_PING:
        ed_b(0);
        break;
    case ED_DIAG: {                                   /* read only: the crash record and the Bluetooth start */
        uint32_t w[77], nw = 0, k;            /* 77 x 5 B fit ed_out (tests/diag_size_test.py) */
        const uint32_t *c = (const uint32_t *)&fm1_crash;
        for (k = 0; k < sizeof fm1_crash / 4u; k++)
            w[nw++] = c[k];
        w[nw++] = bootguard.failed;
        w[nw++] = bootguard.pending;
#if X0X_BLE
        nw += ble_diag(w + nw, (uint32_t)(sizeof w / 4u) - nw);
#endif
        ed_b(nw);
        for (k = 0; k < nw; k++) {
            ed_b(w[k]);
            ed_b(w[k] >> 7);
            ed_b(w[k] >> 14);
            ed_b(w[k] >> 21);
            ed_b(w[k] >> 28);
        }
        break;
    }
#if BOARD_SMK37
    case ED_INLOG:                                    /* read only: the inbound recorder (main_fm1.c) */
        smk_inlog_reply(na >= 1u ? a[0] : 0u);
        break;
#endif
    case ED_SMP_BEGIN:
    case ED_SMP_ERASE:
        if (na < 1u || a[0] >= SMP_USER_SLOTS || !flash_ok)
            return;
        ed_b(a[0]);
        if (ed_project_samples_locked())
            ed_b(4u);
        else if (cmd == ED_SMP_BEGIN)
            ed_b(ed_smp_begin(a[0]) ? 1u : 0u);
        else
            ed_b(ed_smp_erase(a[0], 1u) ? 1u : 0u);
        break;
    case ED_SMP_WRITE: {
        uint32_t off, len, rc = 0, took;
        if (na < 5u || a[0] >= SMP_USER_SLOTS || !flash_ok)
            return;
        off = (uint32_t)a[1] | (uint32_t)a[2] << 7 | (uint32_t)a[3] << 14;
        len = ed_unpack7(a + 4, na - 4u, ed_smp_buf, 256u);
        if (off < SMP_USER_DATA || (off & 0xFFu) || !len || off + len > SMP_USER_SIZE(a[0]))
            rc = 1;
        else if (ed_project_samples_locked() ||
                 (!smp_importing[a[0]] && usr_nz[a[0]]))
            rc = 4;
        else {
            if (ed_project.mode == 2)
                ed_project.changed = 1;
            if (ed_smp_erase_for_write(a[0], off, &took))
                rc = 2;
            if (!rc) {
                /* Sector-0 backup/restore reuses ed_smp_buf; re-unpack after erase. */
                len = ed_unpack7(a + 4, na - 4u, ed_smp_buf, 256u);
                if (fl_write(ed_smp_slot(a[0]) + off, ed_smp_buf, len))
                    rc = 3;
            }
        }
        ed_b(a[0]);
        ed_b(off);
        ed_b(off >> 7);
        ed_b(off >> 14);
        ed_b(rc);
        break;
    }
    case ED_SMP_END:
        if (na < 2u || a[0] >= SMP_USER_SLOTS || !flash_ok)
            return;
        ed_b(a[0]);
        ed_b(ed_project_samples_locked() ? 4u : (uint32_t)ed_smp_end(a[0], a + 1, na - 1u));
        break;
#if X0X_DEBUG
    case ED_PEEK:
    case ED_POKE:
    case ED_CLOCK:
        if (!ed_debug(cmd, a, na))
            return;
        break;
#endif
    case ED_BOOT_INFO:
    case ED_BOOT_BEGIN:
    case ED_BOOT_WRITE:
    case ED_BOOT_READ:
    case ED_BOOT_END:
    case ED_BOOT_ABORT:
        ed_boot(cmd, a, na);
        break;
    case ED_SMP_READ: {                              /* X0X: slot, offset (3 x 7 bits, from the slot's start,
                                                      * its header included), count -> slot, offset, the bytes
                                                      * packed 7 to 8 (as WRITE takes them) */
        uint32_t off, n;
        const uint8_t *src;
        if (na < 5u || a[0] >= SMP_USER_SLOTS)
            return;
        off = (uint32_t)a[1] | (uint32_t)a[2] << 7 | (uint32_t)a[3] << 14;
        n = a[4] > ED_READ_MAX / 2u ? ED_READ_MAX : a[4] * 2u;   /* the count is in pairs (7 bits) */
        if (off >= SMP_USER_SIZE(a[0]))
            return;
        if (n > SMP_USER_SIZE(a[0]) - off)
            n = SMP_USER_SIZE(a[0]) - off;
        src = smp_user_xip(a[0]) + off;
        ed_b(a[0]);
        ed_b(off);
        ed_b(off >> 7);
        ed_b(off >> 14);
        ed_put7(src, n);
        break;
    }
    case ED_SMP_INFO: {                              /* count, size (KiB, 7 bits: the SMALLEST slot, so an older
                                                      * tool that takes one size for all never overfills one),
                                                      * per slot: zones, name, used KiB (7 bits); then the X0X
                                                      * extension 'X' 1, per slot: flash offset and size in 4 KiB
                                                      * sectors and used KiB, 14 bits each (2 x 7, low first).
                                                      * An older tool stops before the extension. */
        uint32_t mn = SMP_USER_MAX;
        for (i = 0; i < SMP_USER_SLOTS; i++)
            mn = SMP_USER_SIZE(i) < mn ? SMP_USER_SIZE(i) : mn;
        ed_b(SMP_USER_SLOTS);
        ed_b(mn / 1024u > 127u ? 127u : mn / 1024u);
        for (i = 0; i < SMP_USER_SLOTS; i++) {
            const smp_user_hdr_t *h = (const smp_user_hdr_t *)smp_user_xip(i);
            char nm[9] = {0};
            uint32_t j;
            ed_b(usr_nz[i]);
            for (j = 0; usr_nz[i] && j < 8u; j++)
                nm[j] = h->name[j] >= 32 && h->name[j] < 127 ? h->name[j] : 0;
            ed_str(nm, 8);
            j = usr_nz[i] ? (h->data_len + 1023u) / 1024u : 0u;
            ed_b(j > 127u ? 127u : j);
        }
        ed_b('X');
        ed_b(1);
        for (i = 0; i < SMP_USER_SLOTS; i++) {
            const smp_user_hdr_t *h = (const smp_user_hdr_t *)smp_user_xip(i);
            uint32_t v[3], j;
            v[0] = SMP_USER_OFF(i) / 0x1000u;
            v[1] = SMP_USER_SIZE(i) / 0x1000u;
            v[2] = usr_nz[i] ? (h->data_len + 1023u) / 1024u : 0u;
            for (j = 0; j < 3u; j++) {
                ed_b(v[j]);
                ed_b(v[j] >> 7);
            }
        }
        break;
    }
#if X0X_BLE
    case ED_BT_TEST: {
        uint32_t op = na > 0u ? a[0] : 0u;
        if (op > 4u)
            return;
        if (ui_project_busy() && (op == 1u || op == 2u))
            return;                                 /* radio reset/reboot cannot interrupt sample recovery */
        if (op >= 3u) {     /* 3: the timer 5 frame watch (fm1_guard.h), last boot's, then hits now;
                             * 4: core 1's records (fm1_cpu1.h): core 0 seen stopping, the RAM canary */
            const uint32_t *t = op == 3u ? (const uint32_t *)&fm1_t5w_prev : (const uint32_t *)&fm1_c0_dead;
            const uint32_t *g = (const uint32_t *)&fm1_can;
            const uint32_t nt = op == 3u ? sizeof fm1_t5w_prev / 4u : sizeof fm1_c0_dead / 4u;
            const uint32_t n = nt + (op == 3u ? 1u : sizeof fm1_can / 4u);
            uint32_t k;
            ed_b(op);
            ed_b(n);
            for (k = 0; k < n; k++) {
                uint32_t v = k < nt ? t[k] : op == 3u ? fm1_t5w.hits : g[k - nt];
                ed_b(v);
                ed_b(v >> 7);
                ed_b(v >> 14);
                ed_b(v >> 21);
                ed_b(v >> 28);
            }
            break;
        }
        if (op == 1u) {
            plat_ble_set(0);                          /* as BLUETOOTH OFF: the next power-on tries again */
            plat_ble_set((proj.set.bt & 1u) != 0u);   /* the setting itself does not change */
        }
        if (op == 2u)
            ed_reboot_at = fm1_ms + 1u;               /* after this reply is out (main loop) */
        ed_b(op);
        ed_b(proj.set.bt & 1u);
        ed_b(ble_guard.blocked & 1u);
        ed_b((uint32_t)plat_ble_state());
        break;
    }
    case ED_CTL_READ: {
        uint32_t off, n;
        const uint8_t *src;
        _Static_assert(XB_FLASH_OFF == FL_BLE_LO && XB_FLASH_SIZE == FL_BLE_HI - FL_BLE_LO,
                       "ED_CTL_READ: the Bluetooth block is FL_BLE");
        if (na < 4u)
            return;
        off = (uint32_t)a[0] | (uint32_t)a[1] << 7 | (uint32_t)a[2] << 14;
        n = a[3] > ED_READ_MAX / 2u ? ED_READ_MAX : a[3] * 2u;
        if (off >= XB_FLASH_SIZE)
            return;
        if (n > XB_FLASH_SIZE - off)
            n = XB_FLASH_SIZE - off;
        src = fm1_xip_ptr(XB_FLASH_OFF) + off;     /* the app area: reads decrypted, as core 1 runs it */
        ed_b(off);
        ed_b(off >> 7);
        ed_b(off >> 14);
        ed_put7(src, n);
        break;
    }
#endif
    default:
        return;                                       /* not ours: no reply */
    }
    ed_send();
}

/* Return 1 only when this check left a non-editor frame pending for OTA. */
static int ed_service(void)
{
    const uint8_t *p;
    uint32_t n;
    if (ED_MAINTENANCE_HELD()) {
        /* No project timeout, boot housekeeping or editor dispatch while held.
         * Future capture transport must have its own qualified dispatcher; all
         * currently supported frames are refused, never replayed on release. */
        if (ota_frame_get(&p, &n)) {
            (void)ED_MAINTENANCE_ADMIT();
            ota_frame_done();
        }
        return 0;
    }
    if (ed_project.mode && !ed_project.recovery && fm1_ms - ed_project.at >= 15000u)
        ed_project_end(0);
    boot_housekeep();
    if (!ota_frame_get(&p, &n))
        return 0;
    if (n < 4u || p[0] != ED_HDR0 || p[1] != ED_HDR1 || p[2] != ED_HDR2) {
        if (ed_project.mode) {
            ota_frame_done();                        /* OTA and project operations share scratch */
            return 0;
        }
        return 1;                                    /* this same pending frame belongs to ota_service() */
    }
    /* Boot writes are longer than the 64-byte stack copy below. A truncated
     * frame would fail the check and must not be stored. */
    if (p[3] == ED_CAPTURE || p[3] == ED_PROJECT || (p[3] >= ED_SMP_BEGIN && p[3] <= ED_SMP_READ) || boot_cmd(p[3])) {
        ed_handle(p, n);
        ota_frame_done();
        return 0;
    }
    {
        uint8_t f[64];                                /* (on the stack: ed_handle keeps no pointer to it) */
        uint32_t k = n > sizeof f ? sizeof f : n, i;
        for (i = 0; i < k; i++)
            f[i] = p[i];
        ota_frame_done();
        ed_handle(f, k);
    }
    return 0;
}
