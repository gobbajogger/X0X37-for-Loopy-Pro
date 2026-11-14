/* SPDX-License-Identifier: GPL-3.0-only */
/* Real file-boundary checks; the host executable is included, not mocked. */
#define _POSIX_C_SOURCE 200809L
#define main preset_librarian_entry
#include "../../tools/preset_librarian.c"
#undef main
#include "../../firmware/src/crc32_nibble.h"

static unsigned checks, failed;
static void check(int ok, const char *msg)
{
    checks++;
    if (!ok) {
        failed++;
        fprintf(stderr, "FAIL %s\n", msg);
    }
}
static void rewrite_crc(uint8_t *p, unsigned n)
{
    uint32_t crc;
    memset(p + 16, 0, 4);
    crc = x0x_crc32(p, n);
    p[16] = (uint8_t)crc;
    p[17] = (uint8_t)(crc >> 8);
    p[18] = (uint8_t)(crc >> 16);
    p[19] = (uint8_t)(crc >> 24);
}
static int put_file(const char *path, const uint8_t *data, unsigned n)
{
    FILE *f = fopen(path, "wb");
    int ok;
    if (!f) return 0;
    ok = fwrite(data, 1, n, f) == n;
    return fclose(f) == 0 && ok;
}
static int exists(const char *path)
{
    return access(path, F_OK) == 0;
}
static void execute_files(void)
{
    char dir[] = "/tmp/dnbox-librarian-XXXXXX";
    char in[PATH_MAX], out[PATH_MAX];
    preset_record_t r;
    uint8_t blob[PRESET_MAX_BYTES], exported[PRESET_MAX_BYTES];
    uint32_t n = 0;
    FILE *f;
    char *read_argv[] = { "lib", "inspect", "--target", "chicago", in };
    char *write_argv[] = { "lib", "export", "--target", "chicago", in, out };
    char *wrong_argv[] = { "lib", "export", "--target", "miami", in, out };
    unsigned i;
    check(mkdtemp(dir) != NULL, "create private temp dir");
    if (!exists(dir))
        return;
    snprintf(in, sizeof in, "%s/renamed.data", dir);
    snprintf(out, sizeof out, "%s/out.dnpr", dir);
    preset_record_clear(&r);
    r.engine_id = PRESET_ENGINE_CHICAGO;
    r.original_engine = PRESET_ENGINE_CHICAGO;
    r.schema_version = PRESET_SCHEMA_V1;
    r.origin_tag = PRESET_ORIGIN_USER;
    r.nparams = PRESET_CHICAGO_NPARAMS_V1;
    check(preset_record_set_name(&r, "A-1") == PRESET_OK, "set name");
    for (i = 0; i < r.nparams; i++)
        r.params[i] = (uint8_t)(7u * i);
    check(preset_record_encode(blob, sizeof blob, &n, &r) == PRESET_OK,
          "encode real source record");
    check(put_file(in, blob, n), "write renamed user source");
    check(preset_librarian_entry(5, read_argv) == 0,
          "inspect renamed raw file without extension");
    check(preset_librarian_entry(6, write_argv) == 0,
          "export byte-exact user to new file");
    f = fopen(out, "rb");
    check(f != NULL, "new exported file exists");
    if (f) {
        check(fread(exported, 1, sizeof exported, f) == n,
              "new exported length");
        check(memcmp(blob, exported, n) == 0,
              "A1 raw bytes preserved");
        check(fclose(f) == 0, "close verified export");
    }
    check(preset_librarian_entry(6, write_argv) != 0,
          "no overwrite of existing destination");
    f = fopen(out, "rb");
    check(f != NULL, "existing export remains present");
    if (f) {
        check(fread(exported, 1, sizeof exported, f) == n &&
              memcmp(blob, exported, n) == 0,
              "existing export remains byte-identical after refusal");
        check(fclose(f) == 0, "close preserved export");
    }
    check(unlink(out) == 0, "clear destination for negative cases");
    check(preset_librarian_entry(6, wrong_argv) != 0 &&
          !exists(out), "wrong target engine refused with no output");
    check(put_file(in, blob, n), "restore pristine source");

    blob[20] = PRESET_ORIGIN_FACTORY;
    blob[22] = 1;
    rewrite_crc(blob, n);
    check(put_file(in, blob, n), "factory test fixture");
    check((preset_librarian_entry(6, write_argv) == 0) ==
          (DNBOX_FACTORY_PRESET_EXPORT != 0),
          "factory export precisely follows switch");
    check(exists(out) == (DNBOX_FACTORY_PRESET_EXPORT != 0),
          "factory export output presence follows switch");
    if (exists(out))
        check(unlink(out) == 0, "clear factory ON output");

    blob[20] = PRESET_ORIGIN_USER; /* still factory-derived: index 1 */
    rewrite_crc(blob, n);
    check(put_file(in, blob, n), "derived fixture");
    check((preset_librarian_entry(6, write_argv) == 0) ==
          (DNBOX_FACTORY_PRESET_EXPORT != 0),
          "factory-derived export precisely follows switch");
    check(exists(out) == (DNBOX_FACTORY_PRESET_EXPORT != 0),
          "factory-derived output presence follows switch");
    if (exists(out))
        check(unlink(out) == 0, "clear derived ON output");

    blob[22] = 0;
    rewrite_crc(blob, n);
    blob[100] ^= 1u; /* CRC is now wrong */
    check(put_file(in, blob, n), "CRC-bad fixture");
    check(preset_librarian_entry(6, write_argv) != 0 && !exists(out),
          "corrupt file refused before output");
    blob[100] ^= 1u;
    blob[8] = 2u;
    rewrite_crc(blob, n);
    check(put_file(in, blob, n), "newer schema fixture");
    check(preset_librarian_entry(6, write_argv) != 0 && !exists(out),
          "newer schema refused before output");
    blob[8] = 1u;
    rewrite_crc(blob, n);
    blob[n] = 0u;
    check(put_file(in, blob, n + 1u), "trailing byte fixture");
    check(preset_librarian_entry(6, write_argv) != 0 && !exists(out),
          "trailing byte refused before output");
    {
        uint8_t huge[PRESET_MAX_BYTES + 1u];
        memset(huge, 0, sizeof huge);
        check(put_file(in, huge, sizeof huge), "oversize fixture");
        check(preset_librarian_entry(6, write_argv) != 0 && !exists(out),
              "oversize file refused before output");
    }
    check(!exists(out), "all negative cases left destination absent");
    (void)unlink(in);
    (void)unlink(out);
    check(rmdir(dir) == 0, "temp fixture cleaned");
}
int main(void)
{
    execute_files();
    if (failed) {
        fprintf(stderr, "FAIL preset-librarian %u/%u\n", failed, checks);
        return 1;
    }
    printf("PASS preset-librarian checks=%u factory_export=%d\n",
           checks, DNBOX_FACTORY_PRESET_EXPORT);
    return 0;
}
