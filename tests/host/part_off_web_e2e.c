/* SPDX-License-Identifier: GPL-3.0-only */
/* M7a end to end, firmware -> web: the whole host app exports the .x0x inner project file (header from the
 * real project_file_header(), word 28 = PROJ_FILE_OFF_WORD from x0x.h, payload = project_stage) for every
 * part OFF mask 0..31, into <dir>/mask<m>.x0xp. tests/host/part_off_web_e2e.mjs feeds those bytes to the
 * web parseProject(). Also cross-checks web-built word 28 (CRC-8 poly 0x17, same as web/x0x_projects.js)
 * through firmware project_file_validate, and records accept/reject for the node side to agree on.
 * Test only; no production code. Build/run: tests/host/run_part_off_web_e2e.sh. */
#define main x0x_host_main
#include "../../host/x0x_host.c"
#undef main
#include <stdio.h>
#include <string.h>

static int fails;

static void verdict(const char *name, int ok, const char *why)
{
    if (ok)
        printf("  PASS %s\n", name);
    else if (why && why[0])
        printf("  FAIL %s  (%s)\n", name, why);
    else
        printf("  FAIL %s\n", name);
    if (!ok) fails++;
}

/* Web CRC-8 of the mask (poly 0x17), independent of PROJ_FILE_OFF_CHK — must match web/x0x_projects.js offChk. */
static uint8_t web_off_chk(unsigned m)
{
    return (uint8_t)(((m & 1u) ? 0x17u : 0u) ^ ((m & 2u) ? 0x2Eu : 0u) ^ ((m & 4u) ? 0x5Cu : 0u) ^
                     ((m & 8u) ? 0xB8u : 0u) ^ ((m & 16u) ? 0x67u : 0u));
}
static uint32_t web_off_word(unsigned m)
{
    return m ? (0x4F460000u | (uint32_t)web_off_chk(m) << 8 | (uint32_t)m) : 0u;
}

static void record(FILE *fw, uint32_t w, int accept)
{
    fprintf(fw, "%08x %s\n", (unsigned)w, accept ? "accept" : "reject");
}

int main(int argc, char **argv)
{
    FILE *fw;
    char path[1024];
    unsigned m, i, x;
    uint8_t h[PROJECT_FILE_HDR];
    unsigned bad, export_bad = 0;

    if (argc != 2) { fprintf(stderr, "usage: %s <outdir>\n", argv[0]); return 2; }
    boot();
    run_ms(300);

    snprintf(path, sizeof path, "%s/fw_validate.txt", argv[1]);
    fw = fopen(path, "w");
    if (!fw) { perror(path); return 2; }

    for (m = 0; m < 32; m++) {
        part_off_live = (uint8_t)m;
        project_apply_io();
        project_capture_stage();
        project_file_header(h);
        snprintf(path, sizeof path, "%s/mask%u.x0xp", argv[1], m);
        {
            FILE *f = fopen(path, "wb");
            if (!f) { perror(path); fclose(fw); return 2; }
            if (fwrite(h, 1, sizeof h, f) != sizeof h ||
                fwrite(&project_stage, 1, sizeof project_stage, f) != sizeof project_stage) {
                fclose(f); fclose(fw); return 2;
            }
            fclose(f);
        }
        printf("mask %2u word28 %08x staged %02x\n", m, (unsigned)get32(h + 28), part_off_staged);

        part_off_staged = 0xFFu;
        {
            int rc = project_file_validate(h);
            if (rc != 0 || part_off_staged != (uint8_t)m) export_bad++;
            record(fw, get32(h + 28), rc == 0);
        }
    }
    verdict("fw_export_project_file_validate", export_bad == 0,
            export_bad ? "exported header failed project_file_validate" : "");

    /* Cross-check: web-built word 28 (poly 0x17) must be accepted by project_file_validate. */
    bad = 0;
    for (m = 0; m < 32; m++) {
        uint32_t ww = web_off_word(m);
        part_off_live = (uint8_t)m;
        project_apply_io();
        project_capture_stage();
        project_file_header(h);
        put32(h + 28, ww);
        part_off_staged = 0xFFu;
        {
            int rc = project_file_validate(h);
            if (rc != 0 || part_off_staged != (uint8_t)m) bad++;
            record(fw, ww, rc == 0);
        }
        if (ww != PROJ_FILE_OFF_WORD(m)) bad++;
    }
    verdict("web_built_word28_project_file_validate", bad == 0, bad ? "mismatch or validate fail" : "");

    /* Reject corpus: wrong check byte, legacy 0x4F460000|mask, mask>31, 1-bit flips — validate must refuse.
     * fw_validate.txt records the QA agreement set (one wrong check per mask, every legacy, every mask>31,
     * every 1-bit flip); C still exhausts all 255 wrong checks per mask. */
    bad = 0;
    part_off_live = 0;
    project_apply_io();
    project_capture_stage();
    project_file_header(h);
    for (m = 1; m < 32; m++) {
        uint32_t w = web_off_word(m);
        for (x = 1; x < 256; x++) {
            uint32_t badw = w ^ ((uint32_t)x << 8);
            int rc;
            put32(h + 28, badw);
            part_off_staged = 0x0Au;
            rc = project_file_validate(h);
            if (rc != -1) bad++;
            if (x == 1) record(fw, badw, rc == 0);  /* agreement: actual FW outcome */
        }
        {
            uint32_t leg = 0x4F460000u | m;
            int rc;
            put32(h + 28, leg);
            part_off_staged = 0x0Au;
            rc = project_file_validate(h);
            if (rc != -1) bad++;
            record(fw, leg, rc == 0);
        }
    }
    for (m = 32; m < 256; m++) {
        uint32_t w = 0x4F460000u | (uint32_t)web_off_chk(m & 31u) << 8 | m;
        int rc;
        put32(h + 28, w);
        part_off_staged = 0x0Au;
        rc = project_file_validate(h);
        if (rc != -1) bad++;
        record(fw, w, rc == 0);
    }
    for (m = 0; m < 32; m++) {
        uint32_t w = web_off_word(m);
        for (i = 0; i < 32; i++) {
            uint32_t fl = w ^ (1u << i);
            int rc;
            put32(h + 28, fl);
            part_off_staged = 0x0Au;
            rc = project_file_validate(h);
            if (rc != -1) bad++;
            record(fw, fl, rc == 0);
        }
    }
    verdict("fw_validate_rejects_bad_word28_corpus", bad == 0, bad ? "accepted a bad word" : "");

    fclose(fw);
    printf("part_off_web_e2e_fw: %s (%d failing cases)\n", fails ? "FAIL" : "ok", fails);
    return fails ? 1 : 0;
}
