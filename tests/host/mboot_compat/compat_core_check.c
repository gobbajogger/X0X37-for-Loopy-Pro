/* SPDX-License-Identifier: GPL-3.0-only
 * Offline test adapter. No platform.c, device backend, or device-write path.
 * core_nor callbacks below operate exclusively on a synthetic memory array. */
#include "core.h"
#include "contract.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FLASH_SIZE 0x100000u
static unsigned char flash[FLASH_SIZE], before[FLASH_SIZE];
static unsigned writes, erases, fail_program;
static unsigned char work[CORE_WORK_SIZE];

static size_t read_file(const char *path, unsigned char *dst, size_t cap) {
    FILE *f = fopen(path, "rb");
    if (!f) { perror(path); exit(2); }
    size_t n = fread(dst, 1, cap, f);
    int extra = fgetc(f), error = ferror(f);
    fclose(f);
    if (extra != EOF || error) { fputs("invalid fixture size/read\n", stderr); exit(2); }
    return n;
}
static bool read_mem(void *ctx, uint32_t addr, void *dst, uint32_t n) {
    (void)ctx;
    if (!core_range(addr, n, FLASH_SIZE)) return false;
    memcpy(dst, flash + addr, n); return true;
}
static bool program_mem(void *ctx, uint32_t addr, const void *src, uint32_t n) {
    (void)ctx;
    ++writes;
    if (fail_program && writes == fail_program) return false;
    if (!core_range(addr, n, FLASH_SIZE) || n > 256 || (addr & 255u) + n > 256)
        return false;
    const unsigned char *p = src;
    for (uint32_t i = 0; i < n; ++i) {
        if ((flash[addr+i] & p[i]) != p[i]) return false;
        flash[addr+i] &= p[i];
    }
    return true;
}
static bool erase_mem(void *ctx, uint32_t addr, uint32_t n) {
    (void)ctx;
    ++erases;
    if (addr != 0xc1000u || n != 4096) return false;
    memset(flash + addr, 255, n); return true;
}
static struct core_nor nor = {{NULL, FLASH_SIZE, read_mem}, program_mem, erase_mem};
static void reset_memory(void) {
    memset(flash, 255, sizeof flash);
    writes = erases = fail_program = 0;
}
static void snapshot(void) { memcpy(before, flash, sizeof flash); }
static void unchanged_except(uint32_t lo, uint32_t hi) {
    assert(!memcmp(flash, before, lo));
    assert(!memcmp(flash + hi, before + hi, FLASH_SIZE - hi));
}
static void guard_tests(void) {
    reset_memory();
    memset(flash + 0xc1000, 0x53, 0x22c);
    memcpy(flash + 0xc1224, "trimdat", 8);
    /* Real unit bytes are never used. Sentinel MAC/efuse bytes must survive. */
    memset(flash + 0xfff00, 0xa7, 256);
    snapshot();
    assert(core_guard(&nor, true, work) == CORE_GUARD_BACKED_UP);
    assert(erases == 0 && writes > 0);
    unchanged_except(0xff000, 0xff23c);
    unsigned char backup[0x23c], original[0x22c];
    memcpy(backup, flash + 0xff000, sizeof backup);
    memcpy(original, flash + 0xc1000, sizeof original);
    assert(!memcmp(backup, "TRIMCOPY", 8));
    /* Valid deliberately changed calibration is not overwritten. */
    flash[0xc1000] ^= 1;
    writes = erases = 0; snapshot();
    assert(core_guard(&nor, true, work) == CORE_GUARD_UNCHANGED);
    assert(writes == 0 && erases == 0 && !memcmp(flash, before, sizeof flash));
    /* Failure at each restore page preserves backup; retry restores source. */
    for (unsigned fail = 1; fail <= 3; ++fail) {
        memset(flash + 0xc1000, 0x11, 4096);
        writes = erases = 0; fail_program = fail; snapshot();
        assert(core_guard(&nor, true, work) == CORE_GUARD_ERROR);
        unchanged_except(0xc1000, 0xc2000);
        assert(!memcmp(flash + 0xff000, backup, sizeof backup));
        writes = erases = fail_program = 0;
        assert(core_guard(&nor, true, work) == CORE_GUARD_RESTORED);
        assert(!memcmp(flash + 0xc1000, original, sizeof original));
        assert(!memcmp(flash + 0xff000, backup, sizeof backup));
    }
    /* Bad backup CRC and missing source marker do not trigger restoration. */
    flash[0xff00e] ^= 1; flash[0xc1224] = 0;
    writes = erases = 0; snapshot();
    assert(core_guard(&nor, true, work) == CORE_GUARD_UNCHANGED);
    assert(writes == 0 && erases == 0 && !memcmp(flash, before, sizeof flash));
    /* A partial first backup is left alone, never erased to make room. */
    reset_memory(); flash[0xff010] = 0;
    memcpy(flash + 0xc1224, "trimdat", 8); snapshot();
    assert(core_guard(&nor, true, work) == CORE_GUARD_UNCHANGED);
    assert(writes == 0 && erases == 0 && !memcmp(flash, before, sizeof flash));
    puts("PASS synthetic calibration: backup, boundaries, differing source, restore interruption, invalid/partial backup");
}
static void contract_tests(void) {
    unsigned char out[24], mac[8] = {1,2,3,4,5,6,7,8};
    contract_app(out, 0x4000, 0x980f, 0x1357, mac);
    assert(core_le32(out) == 0x01c7fe20 && core_le32(out+4) == 0x4000);
    assert(core_le32(out+8) == 0x02000000 && core_le16(out+12) == 0x980f);
    assert(core_le16(out+14) == 0x1357 && !memcmp(out+16, mac, 8));
    struct core_app app = {0}; app.entry = 0x02000120; app.base = 0x4000;
    contract_updater(out, &app, 0x980f, 0x01c03000, 0x01c04000);
    assert(core_le32(out) == app.entry && core_le32(out+4) == app.base);
    assert(core_le32(out+8) == 0x01c03000 && core_le16(out+12) == 0x980f);
    assert(core_le32(out+16) == 0 && core_le32(out+20) == 0x01c04000);
    unsigned char request[80] = {0};
    assert(contract_update_choice(request, true) == 0);
    assert(contract_update_choice(request, false) == 2);
    core_put_le16(request+2, 0x5a0d);
    core_put_le16(request, core_crc16(0, request+2, 78));
    assert(contract_update_choice(request, true) == 1);
    request[10] ^= 1;
    assert(contract_update_choice(request, true) == 0);
    puts("PASS app/updater ABI and valid/invalid request selection");
}
int main(int argc, char **argv) {
    if (argc == 2 && !strcmp(argv[1], "synthetic")) {
        guard_tests(); contract_tests(); return 0;
    }
    if (argc == 4 && !strcmp(argv[1], "config")) {
        unsigned char blob[4096]; struct core_config cfg;
        size_t n = read_file(argv[2], blob, sizeof blob);
        bool ok = core_config_parse(&cfg, blob, (uint32_t)n);
        assert(ok == (atoi(argv[3]) != 0));
        if (ok) assert(cfg.width == 4 && cfg.system_hz == 360000000);
        puts(ok ? "PASS valid configuration" : "PASS invalid configuration refused"); return 0;
    }
    if (argc == 5 && !strcmp(argv[1], "app")) {
        assert(read_file(argv[2], flash, sizeof flash) == sizeof flash);
        struct core_app app;
        bool mounted = core_app_mount(&nor.flash, (uint16_t)strtoul(argv[3], NULL, 0), &app);
        enum core_app_state state = mounted ? core_app_check(&nor.flash, &app, work) : CORE_APP_NOT_FOUND;
        assert((int)state == atoi(argv[4]));
        if (state == CORE_APP_OK) {
            assert(app.base == 0x4000 && app.entry == 0x02000120 && app.file_count == 2);
            assert(app.region_count == 6 && app.app_length == 0x97400);
        }
        printf("PASS app state=%d\n", state); return 0;
    }
    if (argc == 6 && !strcmp(argv[1], "head")) {
        unsigned char container[0x3830];
        assert(read_file(argv[2], flash, sizeof flash) == sizeof flash);
        size_t n = read_file(argv[3], container, sizeof container);
        assert(read_file(argv[4], before, sizeof before) == sizeof before);
        bool ok = core_boot_install(flash, container, (uint32_t)n, FLASH_SIZE);
        assert(ok == (atoi(argv[5]) != 0));
        assert(!memcmp(flash, before, sizeof flash));
        puts(ok ? "PASS native synthetic head transform matches exact preservation fixture"
                : "PASS native head refusal leaves every fixture byte unchanged");
        return 0;
    }
    if (argc == 6 && !strcmp(argv[1], "updater")) {
        unsigned char request[80], scanned[80], output[CORE_UPDATE_MAX], expected[CORE_UPDATE_MAX];
        assert(read_file(argv[2], flash, sizeof flash) == sizeof flash);
        assert(read_file(argv[3], request, sizeof request) == sizeof request);
        size_t n = read_file(argv[4], expected, sizeof expected);
        struct core_updater updater;
        bool ok = core_updater_decode(&nor.flash, request, output, sizeof output, work, &updater);
        assert(ok == (atoi(argv[5]) != 0));
        if (ok) {
            assert(updater.load == 0x01c0a800 && updater.size == n && !memcmp(output, expected, n));
            assert(core_request_scan(&nor.flash, 16, scanned, NULL));
            assert(!memcmp(scanned, request, 80));
        }
        puts(ok ? "PASS exact staged OTA decode and flash request scan" : "PASS malformed staged OTA refused");
        return 0;
    }
    fputs("Invalid offline-check invocation\n", stderr); return 2;
}
