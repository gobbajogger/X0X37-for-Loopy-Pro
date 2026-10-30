/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef X0X_MBOOT_ADOPTION_H
#define X0X_MBOOT_ADOPTION_H
#include <stddef.h>
#include <stdint.h>
#define X0X_MBOOT_FLASH_SIZE 0x100000u
#define X0X_MBOOT_HEAD_SIZE 0x4000u
enum x0x_mboot_model { X0X_MBOOT_PRO = 1, X0X_MBOOT_ELITE = 2 };
enum x0x_mboot_result {
    X0X_MBOOT_OK = 0, X0X_MBOOT_ARGUMENT, X0X_MBOOT_STALE_INPUT,
    X0X_MBOOT_CONTAINER, X0X_MBOOT_CONFIG, X0X_MBOOT_CALIBRATION,
    X0X_MBOOT_LAYOUT
};
/* Offline, in-memory candidate construction. No NOR/backend callbacks exist.
 * approved_image and approved_container are the caller's frozen reviewed inputs,
 * not data read a second time from a device. All buffers must be disjoint.
 * Output is untouched on refusal. Neither success nor these pins grant a write.
 * This initial contract refuses an occupied calibration-backup allocation.
 */
int x0x_mboot_prepare(unsigned model,
    const uint8_t *image, size_t image_size,
    const uint8_t *approved_image, size_t approved_image_size,
    const uint8_t *container, size_t container_size,
    const uint8_t *approved_container, size_t approved_container_size,
    uint8_t *output, size_t output_size);
/* Strict complete-head equality for a later ordinary app update on an adopted
 * unit. This is NOT a replacement for existing model/range/update guards. */
int x0x_mboot_same_head(unsigned model, const uint8_t *candidate, size_t size,
    const uint8_t *approved_head, size_t approved_size);
#endif
