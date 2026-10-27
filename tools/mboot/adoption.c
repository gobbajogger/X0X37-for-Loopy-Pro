/* SPDX-License-Identifier: GPL-3.0-only */
#include "adoption.h"
#include "core.h"
#include <string.h>

static int model_valid(unsigned model)
{
    return model == X0X_MBOOT_PRO || model == X0X_MBOOT_ELITE;
}

static int terminated_version(const uint8_t *version)
{
    for (unsigned i = 0; i < 32; ++i)
        if (!version[i]) return 1;
    return 0;
}

int x0x_mboot_same_head(unsigned model, const uint8_t *candidate, size_t size,
    const uint8_t *approved_head, size_t approved_size)
{
    return model_valid(model) && candidate && approved_head &&
        size == X0X_MBOOT_HEAD_SIZE && approved_size == size &&
        memcmp(candidate, approved_head, size) == 0;
}

int x0x_mboot_prepare(unsigned model,
    const uint8_t *image, size_t image_size,
    const uint8_t *approved_image, size_t approved_image_size,
    const uint8_t *container, size_t container_size,
    const uint8_t *approved_container, size_t approved_container_size,
    uint8_t *output, size_t output_size)
{
    uint8_t head[X0X_MBOOT_HEAD_SIZE];
    struct core_config config;
    struct core_record record;
    unsigned configurations = 0;
    if (!model_valid(model) || !image || !approved_image || !container ||
        !approved_container || !output || image_size != X0X_MBOOT_FLASH_SIZE ||
        approved_image_size != image_size || output_size != image_size)
        return X0X_MBOOT_ARGUMENT;
    if (memcmp(image, approved_image, image_size))
        return X0X_MBOOT_STALE_INPUT;
    if (container_size < 0x80 || container_size > 0x3830 ||
        approved_container_size != container_size ||
        memcmp(container, approved_container, container_size) ||
        memcmp(container + 0x50, "AC79BLIDsmk37\0\0\0", 16) ||
        !terminated_version(container + 0x60))
        return X0X_MBOOT_CONTAINER;
    /* Validate actual retained TLVs, not the generated SDK [FELUCCA] stub.
     * The frozen upstream parser owns supported clock/SPI/reset semantics. */
    for (unsigned offset = 64; offset < 0xa0; offset += 32) {
        uint8_t raw[32];
        memcpy(raw, image + offset, sizeof raw);
        core_enc(0xffff, raw, sizeof raw);
        if (!core_record_parse(&record, raw))
            return X0X_MBOOT_LAYOUT;
        if (!memcmp(record.name, "isd_config.ini", sizeof "isd_config.ini")) {
            if (offset != 64 || ++configurations != 1 || record.length > 4096 ||
                !core_range(record.address, record.length, image_size) ||
                core_crc16(0, image + record.address, record.length) != record.data_crc ||
                !core_config_parse(&config, image + record.address, record.length))
                return X0X_MBOOT_CONFIG;
        }
    }
    if (configurations != 1)
        return X0X_MBOOT_CONFIG;
    /* Future first boot owns creation of TRIMCOPY. Do not overwrite or silently
     * adopt an existing reservation. No restoration runs during preparation. */
    if (memcmp(image + 0xc1224, "trimdat", 8))
        return X0X_MBOOT_CALIBRATION;
    for (unsigned i = 0xff000; i < 0xff23c; ++i)
        if (image[i] != 0xff)
            return X0X_MBOOT_CALIBRATION;
    memcpy(head, image, sizeof head);
    if (!core_boot_install(head, container, (uint32_t)container_size, image_size))
        return X0X_MBOOT_LAYOUT;
    /* Defence in depth: only the boot record and its fixed slot may differ. */
    if (memcmp(head, image, 32) || memcmp(head + 64, image + 64, 0x60) ||
        memcmp(head + 0x38d0, image + 0x38d0, sizeof head - 0x38d0))
        return X0X_MBOOT_LAYOUT;
    memcpy(output, image, image_size);
    memcpy(output, head, sizeof head);
    return X0X_MBOOT_OK;
}
