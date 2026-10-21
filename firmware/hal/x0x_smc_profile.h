/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
/* Study outputs remain non-installable. Only the separately validated, named
 * Elite DEV package route may select the diagnostic runtime for installation. */
#ifndef X0X_SMC_VENDOR_STUDY_ONLY
#define X0X_SMC_VENDOR_STUDY_ONLY 0
#endif
#ifndef X0X_SMC_VENDOR_DEV_PACKAGE
#define X0X_SMC_VENDOR_DEV_PACKAGE 0
#endif
#if (X0X_SMC_VENDOR_STUDY_ONLY != 0 && X0X_SMC_VENDOR_STUDY_ONLY != 1) || \
    (X0X_SMC_VENDOR_DEV_PACKAGE != 0 && X0X_SMC_VENDOR_DEV_PACKAGE != 1)
#error "SMC profile selectors must be boolean"
#endif
#if (X0X_SMC_VENDOR_STUDY_ONLY || X0X_SMC_VENDOR_DEV_PACKAGE) && !X0X_SMC_VENDOR_TEST
#error "SMC profile selector requires the SMC TEST runtime"
#endif
#if X0X_SMC_VENDOR_TEST
#if !X0X_BLE
#error "SMC vendor TEST requires Bluetooth support"
#endif
#if (X0X_SMC_VENDOR_STUDY_ONLY + X0X_SMC_VENDOR_DEV_PACKAGE) != 1
#error "Select exactly one packaging-disabled study or named Elite DEV package profile"
#endif
#if !BOARD_SMK37
#error "SMC vendor TEST requires SMK-37"
#endif
#if X0X_SMC_VENDOR_DEV_PACKAGE
#if !defined(X0X_DEV_PADCAL) || X0X_DEV_PADCAL != 1
#error "SMC vendor package requires development pad calibration"
#endif
#if !defined(X0X_SMC_VENDOR_MODEL_ELITE) || X0X_SMC_VENDOR_MODEL_ELITE != 1
#error "SMC vendor package requires explicit Elite model selection"
#endif
/* The pinned target compiler does not accept strcmp as a constant expression.
 * Exact model text is bound by the named build and runtime manifest model CRC. */
_Static_assert(sizeof(SMK_MODEL) == sizeof("SMK-37 Elite"),
               "SMC vendor DEV package is Elite-only");
#endif
#endif
