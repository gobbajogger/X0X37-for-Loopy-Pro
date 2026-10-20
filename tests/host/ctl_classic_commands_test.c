// SPDX-License-Identifier: MPL-2.0
// Copyright (C) 2026 M. Geisler (genkigenki/makkuro)
/* Exhaustively check fallback results and the LE-only module requirement. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#ifndef TEST_MODULES
#define TEST_MODULES 2
#endif
const int config_btctler_modules = TEST_MODULES;
#include "../../ble/ctl/src/ctl_classic_commands.c"

typedef int (*handler_t)(uint16_t, uint8_t *, int);
static handler_t volatile handlers[] = {
    hci_lmp_link_control_cmd_handler,
    hci_lmp_link_policy_cmd_handler,
    hci_lmp_controller_baseband_cmd_handler,
    hci_lmp_informational_parameters_cmd_handler,
    hci_lmp_status_parameters_cmd_handler,
    hci_lmp_vendor_cmd_handler,
    hci_lmp_testing_cmd_handler,
};
int main(void)
{
    uint8_t data[17] = {0};
    for (unsigned i = 0; i < sizeof handlers / sizeof handlers[0]; i++) {
        for (unsigned op = 0; op < 65536; op++) {
            assert(handlers[i]((uint16_t)op, data, sizeof data) == 0);
            assert(handlers[i]((uint16_t)op, NULL, -1) == 0);
        }
    }
    puts("PASS: all 65536 opcodes in all seven classic fallback families");
    return 0;
}
