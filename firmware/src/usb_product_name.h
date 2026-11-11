/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#include <stdint.h>
#include "app/version.h"

/* This file is included only in the Pro or Elite application, never the loader. */
#if !BOARD_SMK37 || (defined(X0X_USB_37PRO) == defined(X0X_USB_37ELITE))
#error USB product name requires exactly one SMK-37 Pro/Elite model
#endif
#if defined(X0X_USB_37ELITE)
#define X0X_USB_PRODUCT_TEXT u"DnBOX " X0X_VERSION " 37ELITE"
#else
#define X0X_USB_PRODUCT_TEXT u"DnBOX " X0X_VERSION " 37PRO"
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
