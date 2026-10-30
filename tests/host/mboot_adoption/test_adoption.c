/* SPDX-License-Identifier: GPL-3.0-only */
#include "adoption.h"
#include "core.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint8_t image[X0X_MBOOT_FLASH_SIZE], approved[sizeof image], out[sizeof image];
static uint8_t container[0x3830], pinned[sizeof container];
static size_t container_size;
static unsigned checks;
static void record(unsigned offset, uint32_t address, uint32_t length,
                   uint16_t crc, const char *name) {
    uint8_t raw[32] = {0};
    core_put_le16(raw + 2, crc); core_put_le32(raw + 4, address);
    core_put_le32(raw + 8, length); memcpy(raw + 16, name, strlen(name));
    core_put_le16(raw, core_crc16(0, raw + 2, 30));
    core_enc(0xffff, raw, sizeof raw); memcpy(image + offset, raw, sizeof raw);
}
static unsigned tlv(unsigned pos, const char *name, const uint8_t *value, unsigned n) {
    image[pos++] = (uint8_t)n;
    memcpy(image + pos, name, strlen(name) + 1); pos += strlen(name) + 1;
    memcpy(image + pos, value, n); return pos + n;
}
static void fixture(void) {
    memset(image, 0xff, sizeof image);
    uint8_t raw[32] = {0}, value[4]; unsigned p = 0x38d0 + 34;
    core_put_le32(raw + 8, sizeof image); raw[13] = 16;
    core_put_le16(raw, core_crc16(0, raw + 2, 30));
    core_enc(0xffff, raw, 32); memcpy(image, raw, 32);
    memset(image + 0x38d0, 0, 34);
    p = tlv(p, "OSC", (const uint8_t *)"OSC0", 4);
    core_put_le32(value, 24000000); p = tlv(p, "OSC_FREQ", value, 4);
    core_put_le32(value, 360000000); p = tlv(p, "SYS_CLK", value, 4);
    core_put_le32(value, 1); p = tlv(p, "HSB_DIV", value, 4);
    core_put_le32(value, 2); p = tlv(p, "LSB_DIV", value, 4); image[p++] = 0;
    record(32, 0xa0, 0x3830, 0xffff, "uboot.boot");
    record(64, 0x38d0, p - 0x38d0, core_crc16(0, image + 0x38d0, p - 0x38d0), "isd_config.ini");
    record(96, 0x4000, 0xffffffffu, 0xffff, "app_dir_head");
    record(128, 0xfff00, 256, 0xffff, "key_mac");
    /* Unique synthetic application/data/calibration/key sentinels. */
    for (unsigned i = 0x4000; i < sizeof image; ++i) image[i] = (uint8_t)(i * 31u + i / 256);
    memset(image + 0xff000, 0xff, 0x23c);
    memcpy(image + 0xc1224, "trimdat", 8);
    memcpy(approved, image, sizeof image);
    memcpy(pinned, container, container_size);
    memset(out, 0x5a, sizeof out);
}
static int prepare(unsigned model) {
    return x0x_mboot_prepare(model, image, sizeof image, approved, sizeof approved,
        container, container_size, pinned, container_size, out, sizeof out);
}
static void refused(int expected) {
    assert(prepare(X0X_MBOOT_ELITE) == expected);
    for (unsigned i = 0; i < sizeof out; ++i) assert(out[i] == 0x5a);
    ++checks;
}
int main(int argc, char **argv) {
    assert(argc == 2); FILE *f = fopen(argv[1], "rb"); assert(f);
    container_size = fread(container, 1, sizeof container, f);
    assert(fgetc(f) == EOF && !ferror(f)); fclose(f);
    assert(container_size >= 0x80);
    for (unsigned model = X0X_MBOOT_PRO; model <= X0X_MBOOT_ELITE; ++model) {
        fixture(); int status = prepare(model); if(status) fprintf(stderr, "prepare status=%d\n", status); assert(status == X0X_MBOOT_OK);
        assert(!memcmp(image, approved, sizeof image));
        assert(!memcmp(out, image, 32)); assert(!memcmp(out + 64, image + 64, 0x60));
        assert(!memcmp(out + 0x38d0, image + 0x38d0, sizeof image - 0x38d0));
        uint8_t expected[0x4000]; memcpy(expected, image, sizeof expected);
        assert(core_boot_install(expected, container, container_size, sizeof image));
        assert(!memcmp(out, expected, sizeof expected));
        assert(x0x_mboot_same_head(model, out, 0x4000, expected, sizeof expected));
        for (unsigned i = 0; i < sizeof expected; ++i) {
            expected[i] ^= 1;
            assert(!x0x_mboot_same_head(model, out, 0x4000, expected, sizeof expected));
            expected[i] ^= 1;
        }
        ++checks;
    }
    for (unsigned offset = 96; offset <= 128; offset += 32) {
        fixture(); uint8_t swap[32]; memcpy(swap, image + 64, 32);
        memcpy(image + 64, image + offset, 32); memcpy(image + offset, swap, 32);
        memcpy(approved, image, sizeof image); refused(X0X_MBOOT_CONFIG);
    }
    fixture(); assert(prepare(99) == X0X_MBOOT_ARGUMENT); ++checks;
    fixture(); image[0x70000] ^= 1; refused(X0X_MBOOT_STALE_INPUT);
    fixture(); pinned[0x81] ^= 1; refused(X0X_MBOOT_CONTAINER);
    fixture(); container[0x58] = 'f'; refused(X0X_MBOOT_CONTAINER); container[0x58] = 's';
    fixture(); container[container_size-1] ^= 1; memcpy(pinned, container, container_size);
    refused(X0X_MBOOT_LAYOUT); container[container_size-1] ^= 1;
    fixture(); image[0x38d0 + 35] ^= 1; memcpy(approved, image, sizeof image); refused(X0X_MBOOT_CONFIG);
    fixture(); image[0xc1224] = 0; memcpy(approved, image, sizeof image); refused(X0X_MBOOT_CALIBRATION);
    fixture(); image[0xff000] = 0; memcpy(approved, image, sizeof image); refused(X0X_MBOOT_CALIBRATION);
    fixture(); image[0xff23b] = 0; memcpy(approved, image, sizeof image); refused(X0X_MBOOT_CALIBRATION);
    fixture(); record(32, 0xa0, 0x3900, 0, "uboot.boot"); memcpy(approved, image, sizeof image); refused(X0X_MBOOT_LAYOUT);
    fixture(); record(96, 0x100, 32, 0, "app_dir_head"); memcpy(approved, image, sizeof image); refused(X0X_MBOOT_LAYOUT);
    fixture(); image[64] ^= 1; memcpy(approved, image, sizeof image); refused(X0X_MBOOT_LAYOUT);
    fixture(); assert(x0x_mboot_prepare(1, image, sizeof image-1, approved, sizeof approved,
        container, container_size, pinned, container_size, out, sizeof out) == X0X_MBOOT_ARGUMENT); ++checks;
    assert(!x0x_mboot_same_head(99, out, 0x4000, image, 0x4000));
    assert(!x0x_mboot_same_head(1, out, 0x3fff, image, 0x4000));
    assert(!x0x_mboot_same_head(1, NULL, 0x4000, image, 0x4000));
    printf("PASS %u adoption groups; both models; all 16384 head-byte mismatch positions; no device backend\n", checks);
    return 0;
}
