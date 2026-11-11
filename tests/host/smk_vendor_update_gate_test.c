/* SPDX-License-Identifier: GPL-3.0-only */
/* Port of 35e3c046's actual OTA regression, with stage/record negatives. */
#define X0X_SMC_VENDOR_TEST 1
#include "../../firmware/hal/x0x_bundle_gate.h"
static bp_update_gate gate;
#if !T_DEFAULT_GATE
#define X0X_BUNDLE_UPDATE_ALLOWED() bp_update_allowed(&gate)
#endif
#define main baseline_ota_test_main
#include "../ota_test.c"
#undef main
#include <assert.h>
static void denied_without_io(void)
{
    memset(nor, 0xA5, sizeof nor);
    requests = 0; committed = 0; bad_erase = 0; mute_after = 0;
    assert(ota_session() == -42);
    assert(!requests && !committed && !bad_erase);
    assert(ota_stage() == -74); assert(!requests);
    ota_unstage(); assert(ota_boot_cleanup() == 1);
    for (unsigned i = 0; i < sizeof nor; i++) assert(nor[i] == 0xA5);
}
static void package(int capability)
{
    uint8_t *b;
    if (!logical) logical = malloc(2048);
    assert(logical); logical_len = 2048; memset(logical, 0, logical_len);
    ota_wr16(logical + 8, 2);
    ota_wr32(logical + 0x40 + 8, 1536); ota_wr32(logical + 0x40 + 12, 512);
    ota_wr16(logical + 0x90, 100);
    ota_wr32(logical + 0x90 + 8, 1024); ota_wr32(logical + 0x90 + 12, 160);
    ota_jl_enc(logical + 0x40, 0x50); ota_jl_enc(logical + 0x90, 0x50);
    ota_wr16(logical + 2, ota_crc16(logical + 0x40, 0xA0, 0));
    ota_wr16(logical, ota_crc16(logical + 2, 0x3E, 0)); ota_jl_enc(logical, 0x40);
    b = logical + 1024;
    ota_wr32(b + 4, 0x20); ota_wr32(b + 8, 128); b[12] = 0x41;
    ota_wr32(b + 0x24, 128); ota_wr32(b + 0x28, 0x01C0A800u);
    ota_wr16(b + 0x20, ota_crc16(b + 0x22, 30, 0));
    memcpy(b + 0x48, "FELUCCA-LOADER-1", 16);
    if (capability) memcpy(b + 0x60, "X0X-BUNDLE-LDR1", 16);
    ota_wr16(b, ota_crc16(b + 2, 30, 0));
    requests = 0; committed = 0; bad_erase = 0; mute_after = ~0u;
    rx_full = 0; ota_deadline = 0;
}
int main(void)
{
    denied_without_io();
#if T_DEFAULT_GATE
    bp_update_recovery(&gate); /* no production admission hook: still denied */
    denied_without_io();
#else
    bp_update_recovery(&gate); assert(bp_update_allowed(&gate));
    package(0); assert(ota_stage() == -7);
    for (unsigned i = 0; i < sizeof nor; i++) assert(nor[i] == 0xA5);
    package(1); assert(ota_stage() == -75);
    assert(!committed && !bad_erase);
    for (unsigned i = 0; i < sizeof nor; i++) assert(nor[i] == 0xA5);
    free(logical);
    bp_update_attempt(&gate); denied_without_io();
    bp_update_recovery(&gate); denied_without_io();
#endif
    puts("SMK TEST OTA: deny before staging/cleanup; require bundle loader; preserve pending record PASS");
    return 0;
}
