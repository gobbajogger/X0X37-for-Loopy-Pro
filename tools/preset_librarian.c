/* SPDX-License-Identifier: GPL-3.0-only */
/* Host-only DNPR librarian bridge. Uses the firmware C codec as the sole
 * serializer and validator. No plugin formats, device transport or flash I/O.
 * Compatible with POSIX host systems (Linux/macOS). */
#define _POSIX_C_SOURCE 200809L
#include "../firmware/src/app/preset_record.h"
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

static int engine_token(const char *s)
{
    if (!strcmp(s, "chicago")) return PRESET_ENGINE_CHICAGO;
    if (!strcmp(s, "miami")) return PRESET_ENGINE_MIAMI;
    if (!strcmp(s, "juno")) return PRESET_ENGINE_JUNO;
    if (!strcmp(s, "drone")) return PRESET_ENGINE_DRONE;
    if (!strcmp(s, "909")) return PRESET_ENGINE_909;
    if (!strcmp(s, "808")) return PRESET_ENGINE_808;
    if (!strcmp(s, "break")) return PRESET_ENGINE_BREAK;
    return 0;
}

static int read_dnpr(const char *path, uint8_t out[PRESET_MAX_BYTES],
                     uint32_t *len)
{
    uint8_t data[PRESET_MAX_BYTES + 1u];
    FILE *f = fopen(path, "rb");
    size_t n;
    int good;
    if (!f) {
        fprintf(stderr, "BAD FILE: cannot open input: %s\n", strerror(errno));
        return 1;
    }
    n = fread(data, 1, sizeof data, f);
    good = n >= PRESET_HEADER_BYTES && n <= PRESET_MAX_BYTES &&
           !ferror(f) && feof(f);
    if (fclose(f) != 0)
        good = 0;
    if (!good) {
        fputs("BAD FILE: truncated, oversized or unreadable\n", stderr);
        return 1;
    }
    memcpy(out, data, n);
    *len = (uint32_t)n;
    return 0;
}

static int validate_file(const uint8_t *bytes, uint32_t len,
                         int requested_engine, preset_record_t *out)
{
    /* The target comes from the authoritative header when the user has not
     * specified one. It is untrusted until preset_record_import_file has
     * validated the CRC, version, engine, origin, sample refs and counts. */
    uint16_t target;
    int err;
    if (len < PRESET_HEADER_BYTES)
        return PRESET_ERR_TRUNCATED;
    target = (uint16_t)requested_engine;
    if (!target)
        target = bytes[21] ? bytes[21] :
                 (uint16_t)((uint16_t)bytes[6] | ((uint16_t)bytes[7] << 8));
    err = preset_record_import_file(out, bytes, len, target);
    return err;
}

static void print_escaped(const uint8_t *src, unsigned n)
{
    unsigned i;
    putchar('"');
    for (i = 0; i < n && src[i]; i++) {
        unsigned c = src[i];
        if (c >= 32u && c <= 126u && c != '"' && c != '\\')
            putchar((int)c);
        else
            printf("\\x%02X", c);
    }
    putchar('"');
}

static int write_no_replace(const char *dest, const uint8_t *data, uint32_t n)
{
    char tmp[PATH_MAX];
    const char suffix[] = ".tmp.XXXXXX";
    size_t dest_len = strlen(dest);
    unsigned off = 0;
    int fd;
    if (dest_len < 5 || strcmp(dest + dest_len - 5, ".dnpr") != 0) {
        fputs("BAD NAME: destination must end in .dnpr\n", stderr);
        return 1;
    }
    if (dest_len + sizeof suffix > sizeof tmp) {
        fputs("BAD NAME: output path too long\n", stderr);
        return 1;
    }
    memcpy(tmp, dest, dest_len);
    memcpy(tmp + dest_len, suffix, sizeof suffix);
    fd = mkstemp(tmp);
    if (fd < 0) {
        fprintf(stderr, "WRITE FAILED: %s\n", strerror(errno));
        return 1;
    }
    while (off < n) {
        ssize_t wrote = write(fd, data + off, (size_t)(n - off));
        if (wrote < 0 && errno == EINTR)
            continue;
        if (wrote <= 0)
            break;
        off += (unsigned)wrote;
    }
    if (off != n || fsync(fd) != 0) {
        fputs("WRITE FAILED: incomplete host file\n", stderr);
        (void)close(fd);
        (void)unlink(tmp);
        return 1;
    }
    if (close(fd) != 0) {
        fputs("WRITE FAILED: close failed\n", stderr);
        (void)unlink(tmp);
        return 1;
    }
    /* Same-directory hard-link publish is atomic and fails if dest exists.
     * Existing user files are never opened, truncated or overwritten. */
    if (link(tmp, dest) != 0) {
        fprintf(stderr, "NO OVERWRITE: destination not created: %s\n",
                strerror(errno));
        (void)unlink(tmp);
        return 1;
    }
    (void)unlink(tmp);
    return 0;
}

static void inspect(const preset_record_t *rec, uint32_t n)
{
    printf("DnBOX DNPR v1: engine=%s, length=%u, schema=%u, origin=%s\n",
           preset_record_engine_name(rec->engine_id), n,
           rec->schema_version, preset_record_origin_name(rec->origin_tag));
    fputs("name=", stdout);
    print_escaped(rec->name, PRESET_NAME_MAX);
    fputs(" source=", stdout);
    print_escaped(rec->source_name, PRESET_SOURCE_MAX);
    printf(" factory_index=%u refs=%u\n",
           rec->factory_index, rec->nsamples);
    if (rec->plugin_format || rec->plugin_version)
        printf("opaque plugin metadata id=%u version=%u (no converter)\n",
               rec->plugin_format, rec->plugin_version);
    if (rec->nsamples)
        fputs("SAMPLE REFS UNVERIFIED: match CRCs against device samples before use\n",
              stdout);
}

static int usage(void)
{
    fputs("DnBOX host-only librarian (no device I/O)\n"
          "Usage: preset_librarian inspect [--target ENGINE] INPUT\n"
          "       preset_librarian export  [--target ENGINE] INPUT OUTPUT.dnpr\n"
          "Engines: chicago miami juno drone 909 808 break\n", stderr);
    return 2;
}

int main(int argc, char **argv)
{
    uint8_t wire[PRESET_MAX_BYTES], serialized[PRESET_MAX_BYTES];
    uint32_t len = 0, written = 0;
    preset_record_t rec;
    int target = 0, err, i = 2;
    int export_mode;
    if (argc < 3)
        return usage();
    export_mode = strcmp(argv[1], "export") == 0;
    if (!export_mode && strcmp(argv[1], "inspect") != 0)
        return usage();
    if (i < argc && strcmp(argv[i], "--target") == 0) {
        if (i + 1 >= argc)
            return usage();
        target = engine_token(argv[i + 1]);
        if (!target) {
            fputs("WRONG ENGINE: unsupported target engine\n", stderr);
            return 2;
        }
        i += 2;
    }
    if (argc - i != (export_mode ? 2 : 1))
        return usage();
    if (read_dnpr(argv[i], wire, &len))
        return 2;
    err = validate_file(wire, len, target, &rec);
    if (err != PRESET_OK) {
        fprintf(stderr, "%s: %s\n", preset_import_message(err),
                preset_record_err_str(err));
        return 2;
    }
    if (!export_mode) {
        inspect(&rec, len);
        return 0;
    }
    err = preset_record_export(serialized, sizeof serialized, &written, &rec);
    if (err == PRESET_ERR_EXPORT_FACTORY) {
        fputs("FACTORY EXPORT OFF\n", stderr);
        return 2;
    }
    if (err != PRESET_OK) {
        fprintf(stderr, "BAD FILE: native export %s\n",
                preset_record_err_str(err));
        return 2;
    }
    if (written != len || memcmp(wire, serialized, len) != 0) {
        fputs("BAD FILE: non-canonical record; no output written\n", stderr);
        return 2;
    }
    if (write_no_replace(argv[i + 1], serialized, written))
        return 2;
    printf("SAVED %u bytes to %s\n", written, argv[i + 1]);
    return 0;
}
