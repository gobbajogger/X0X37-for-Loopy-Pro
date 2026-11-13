/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#include <stdint.h>
#include "app/version.h"

/* USB display identity only; OTA model admission and USB VID/PID stay unchanged. */
#if !BOARD_SMK37 || (defined(X0X_USB_37PRO) && defined(X0X_USB_37ELITE))
#error USB product name requires an SMK board and unambiguous model
#endif
#if defined(FELUCCA_LOADER)
#ifndef SMK_MODEL
#error SMK_MODEL is required for an SMK loader USB descriptor
#endif
#define X0X_USB_PRODUCT_TEXT u"" SMK_MODEL " Update"
#elif defined(X0X_USB_37ELITE)
#define X0X_USB_PRODUCT_TEXT u"DnBOX " X0X_VERSION " SMK-37 Elite"
#elif defined(X0X_USB_37PRO)
#define X0X_USB_PRODUCT_TEXT u"DnBOX " X0X_VERSION " SMK-37 Pro"
#else
#ifndef SMK_MODEL
#error SMK_MODEL is required for an SMK USB descriptor
#endif
/* Direct/MKE builds must not fall back to the legacy non-keyboard name. */
#define X0X_USB_PRODUCT_TEXT u"DnBOX " X0X_VERSION " " SMK_MODEL
#endif

enum { X0X_USB_PRODUCT_CHARS = sizeof(X0X_USB_PRODUCT_TEXT) / sizeof(uint16_t) - 1u };
_Static_assert(sizeof(uint16_t) == 2u, "USB strings use 16-bit UTF-16 code units");
_Static_assert(2u + 2u * X0X_USB_PRODUCT_CHARS <= 255u,
               "USB product name exceeds the USB string descriptor length limit");
static const struct {
    uint8_t length;
    uint8_t type;
    uint16_t text[X0X_USB_PRODUCT_CHARS];
} STR2 = {2u + 2u * X0X_USB_PRODUCT_CHARS, 3u, X0X_USB_PRODUCT_TEXT};
_Static_assert(sizeof STR2 == 2u + 2u * X0X_USB_PRODUCT_CHARS,
               "USB product descriptor must not contain padding");
