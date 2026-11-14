/* SPDX-License-Identifier: GPL-3.0-only */
/* QA #900: engine_id owns schema; original_engine is provenance only.
 * Native codec and real host librarian, no device, plugin or flash access. */
#define _POSIX_C_SOURCE 200809L
#define main preset_librarian_entry
#include "../../tools/preset_librarian.c"
#undef main
static int failed;
static int checks;
static void check(int yes, const char *msg)
{
    checks++;
    if (!yes) { failed++; fprintf(stderr, "FAIL %s\n", msg); }
}
static int put_file(const char *path, const uint8_t *data, size_t len)
{
    FILE *f = fopen(path, "wb");
    int ok;
    if (!f) return 0;
    ok = fwrite(data, 1, len, f) == len;
    if (fclose(f) != 0) ok = 0;
    return ok;
}
static int file_is(const char *path, const uint8_t *data, size_t len)
{
    uint8_t got[PRESET_MAX_BYTES + 1u];
    FILE *f = fopen(path, "rb");
    size_t n;
    int eof, err;
    if (!f) return 0;
    n = fread(got, 1, sizeof got, f);
    eof = feof(f); err = ferror(f);
    if (fclose(f) != 0) return 0;
    return n == len && eof && !err && memcmp(data, got, len) == 0;
}
static void orientation(uint16_t actual, uint8_t provenance,
                        const char *right, const char *wrong)
{
    preset_record_t src, dest, untouched;
    uint8_t bytes[PRESET_MAX_BYTES];
    uint32_t n = 0;
    char dir[] = "/tmp/dnpr-schema-qa-XXXXXX";
    char input[PATH_MAX], output[PATH_MAX];
    char *wrong_args[] = { "lib", "export", "--target", (char *)wrong, input, output };
    char *right_args[] = { "lib", "export", "--target", (char *)right, input, output };
    char *default_args[] = { "lib", "export", input, output };
    char *inspect_args[] = { "lib", "inspect", input };
    int rc;
    unsigned i;

    preset_record_clear(&src);
    src.engine_id = actual;
    src.original_engine = provenance;
    src.schema_version = PRESET_SCHEMA_V1;
    src.origin_tag = PRESET_ORIGIN_USER;
    src.nparams = (uint8_t)preset_record_expected_nparams(actual, PRESET_SCHEMA_V1);
    for (i = 0; i < src.nparams; i++) src.params[i] = (uint8_t)(i * 5u + 1u);
    check(preset_record_set_name(&src, "cross-origin") == PRESET_OK, "set name");
    check(preset_record_encode(bytes, sizeof bytes, &n, &src) == PRESET_OK,
          "encode valid CRC with cross-engine provenance");
    check(n == (uint32_t)(100u + src.nparams), "wire schema size is correct");
    memset(&dest, 0xA5, sizeof dest);
    untouched = dest;
    check(preset_record_decode(&dest, bytes, n, provenance) == PRESET_ERR_ENGINE,
          "decode rejects provenance-as-target");
    check(memcmp(&dest, &untouched, sizeof dest) == 0, "decode failure no mutation");
    rc = preset_record_import(&dest, bytes, n, provenance);
    check(rc == PRESET_ERR_ENGINE, "native import rejects schema mismatch");
    check(memcmp(&dest, &untouched, sizeof dest) == 0, "native failure no mutation");
    rc = preset_record_import_file(&dest, bytes, n, provenance);
    check(rc == PRESET_ERR_ENGINE, "file import rejects schema mismatch");
    check(memcmp(&dest, &untouched, sizeof dest) == 0, "file failure no mutation");
    preset_record_clear(&dest);
    untouched = dest;
    check(preset_record_import(&dest, bytes, n, provenance) == PRESET_ERR_ENGINE,
          "empty slot schema mismatch refused");
    check(memcmp(&dest, &untouched, sizeof dest) == 0, "empty slot remains empty");
    check(preset_import_class(PRESET_ERR_ENGINE) == PRESET_IMPORT_WRONG_ENGINE,
          "wrong engine class");
    check(preset_record_decode(&dest, bytes, n, actual) == PRESET_OK,
          "decode accepts actual engine");
    check(preset_record_equal(&dest, &src), "decode retains provenance");
    preset_record_clear(&dest);
    check(preset_record_import(&dest, bytes, n, actual) == PRESET_OK,
          "native import accepts actual schema");
    check(preset_record_equal(&dest, &src), "native preserves original_engine");
    preset_record_clear(&dest);
    check(preset_record_import_file(&dest, bytes, n, actual) == PRESET_OK,
          "file import accepts actual schema");
    check(preset_record_equal(&dest, &src), "file import retains provenance");

    check(mkdtemp(dir) != NULL, "make isolated host fixture");
    if (access(dir, F_OK) != 0) return;
    snprintf(input, sizeof input, "%s/rename.bin", dir);
    snprintf(output, sizeof output, "%s/export.dnpr", dir);
    check(put_file(input, bytes, n), "save actual DNPR input");
    check(preset_librarian_entry(6, wrong_args) != 0,
          "host --target rejects provenance-only match");
    check(access(output, F_OK) != 0, "host rejection creates no output");
    check(preset_librarian_entry(6, right_args) == 0,
          "host --target actual engine accepted");
    check(file_is(output, bytes, n), "host explicit target bytes unchanged");
    check(unlink(output) == 0, "clear explicit export");
    check(preset_librarian_entry(4, default_args) == 0,
          "host implicit target follows engine_id");
    check(file_is(output, bytes, n), "host default provenance preserved");
    check(preset_librarian_entry(3, inspect_args) == 0,
          "host implicit inspection follows engine_id");
    check(unlink(input) == 0 && unlink(output) == 0, "clean host fixture files");
    check(rmdir(dir) == 0, "clean host fixture directory");
}
int main(void)
{
    orientation(PRESET_ENGINE_CHICAGO, PRESET_ENGINE_MIAMI, "chicago", "miami");
    orientation(PRESET_ENGINE_MIAMI, PRESET_ENGINE_CHICAGO, "miami", "chicago");
    if (failed) {
        fprintf(stderr, "FAIL engine-identity %d/%d\n", failed, checks);
        return 1;
    }
    printf("PASS engine-identity checks=%d factory_export=%d\n",
           checks, DNBOX_FACTORY_PRESET_EXPORT);
    return 0;
}
