/* SPDX-License-Identifier: GPL-3.0-only */
#define X0X_SMC_VENDOR_TEST 1
#define BOARD_SMK37 1
#define T_MODEL 1
#include "../../firmware/hal/x0x_bundle_gate.h"
static bp_update_gate gate;
#define X0X_BUNDLE_UPDATE_ALLOWED() bp_update_allowed(&gate)
#define main baseline_ota_test_main
#include "../ota_test.c"
#undef main
#include <assert.h>
static void denied_without_io(void)
{
    memset(nor,0xA5,sizeof nor); requests=0;committed=0;bad_erase=0;
    assert(ota_session()==-42);
    assert(!requests && !committed && !bad_erase);
    ota_unstage();
    assert(ota_boot_cleanup()==1);
    for(unsigned i=0;i<sizeof nor;i++) assert(nor[i]==0xA5);
}
int main(void)
{
    denied_without_io();
    bp_update_recovery(&gate);assert(bp_update_allowed(&gate));
    bp_update_attempt(&gate);denied_without_io();
    bp_update_recovery(&gate);denied_without_io();
    puts("actual OTA session: deny before staging/cleanup, no flash writes, irreversible attempt gate PASS");
    return 0;
}
