// SPDX-License-Identifier: MPL-2.0
// Copyright (C) 2026 M. Geisler (genkigenki/makkuro)
/* The SDK dispatcher tries classic fallback handlers even in this LE-only
 * image. Report not handled so its original unknown-command completion stays
 * in charge. Strong definitions keep the unused classic parser archive member
 * out of the image; generic and LE handlers remain the SDK implementations. */
#include <stdint.h>

extern const int config_btctler_modules;
/* An unsupported module selection must fail the link, not silently lose classic
 * commands. LTO removes this unresolved call only for the LE-only value (2). */
extern void ctl_classic_fallback_requires_le_only_module(void);
#define CTL_CLASSIC_HANDLER(name) \
    int name(uint16_t opcode, uint8_t *data, int len) \
    { \
        (void)opcode; (void)data; (void)len; \
        if (config_btctler_modules != 2) ctl_classic_fallback_requires_le_only_module(); \
        return 0; \
    }

CTL_CLASSIC_HANDLER(hci_lmp_link_control_cmd_handler)
CTL_CLASSIC_HANDLER(hci_lmp_link_policy_cmd_handler)
CTL_CLASSIC_HANDLER(hci_lmp_controller_baseband_cmd_handler)
CTL_CLASSIC_HANDLER(hci_lmp_informational_parameters_cmd_handler)
CTL_CLASSIC_HANDLER(hci_lmp_status_parameters_cmd_handler)
CTL_CLASSIC_HANDLER(hci_lmp_vendor_cmd_handler)
CTL_CLASSIC_HANDLER(hci_lmp_testing_cmd_handler)
