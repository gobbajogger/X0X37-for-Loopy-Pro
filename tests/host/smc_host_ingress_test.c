/* SPDX-License-Identifier: GPL-3.0-only */
/* Exercise real USB ingress and the shared peripheral-BLE APC hook. */
#define main clip_fixture_main
#include "smk_clip_knob_test.c"
#undef main

static uint32_t packet(unsigned status, unsigned data1, unsigned value)
{
    return (status >> 4) | status << 8 | data1 << 16 | value << 24;
}

static void smc_input(unsigned status, unsigned data1)
{
    uint8_t before[SMK_APC40_CLIP_COUNT];
    uint32_t p = packet(status, data1, 96);
    memcpy(before, apc_led_velocity, sizeof before);
    mi_r = mi_w;
    usb_packet(&p, 1);
    check("opted-in SMC USB event reaches the engine queue", (uint8_t)(mi_w - mi_r) == 1u);
    check("SMC USB input cannot write APC clip feedback", memcmp(before, apc_led_velocity, sizeof before) == 0);
    for (unsigned notifying = 0; notifying < 2; notifying++) {
        check("opted-in SMC peripheral BLE event bypasses APC", smk_apc_ble_event(p, notifying) == 0);
        check("SMC peripheral BLE input cannot write APC clip feedback", memcmp(before, apc_led_velocity, sizeof before) == 0);
    }
}

int main(void)
{
    for (unsigned i = 0; i < NE; i++) { panel.enc[i] = i; panel.dir[i] = 1; }
    for (unsigned i = 0; i < NB; i++) panel.btn[i] = 4u + i;
    proj.set.ck_ch = 2; seq.bpm = 120;
    host = 1; usb.up = usb.config = 1; ep1_config(); run(10);
    for (unsigned clip = 0; clip < 2; clip++) {
        for (unsigned handshake = 0; handshake < 2; handshake++) {
            clip_grid_mode = clip;
            apc_host_on = handshake ? 3u : 0u;
            proj.set.bt = PROJ_BT_SMC_USER;
            for (unsigned cc = 16; cc < 24; cc++) smc_input(0xbf, cc);
            for (unsigned cc = 40; cc < 48; cc++) smc_input(0xbf, cc);
            for (unsigned note = 0; note < 32; note++) {
                smc_input(0x9f, note); smc_input(0x8f, note);
            }
            const unsigned controls[] = {0x2e, 0x2f, 0x30, 0x31, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f, 0x60, 0x61, 0x62, 0x63};
            for (unsigned i = 0; i < sizeof controls / sizeof *controls; i++) smc_input(0x9f, controls[i]);
        }
    }
    clip_grid_mode = 1; apc_host_on = 3;
    for (unsigned user = 0; user < 2; user++) {
        proj.set.bt = user ? PROJ_BT_SMC_USER : 0;
        check("ordinary APC knob feedback stays consumed", smk_apc_usb_event(packet(0xb0, 16, 20)) == 1);
        check("ordinary APC clip feedback stays consumed", smk_apc_usb_event(packet(0x90, 24, 12)) == 1);
        check("ordinary APC clip feedback still updates colour", apc_led_velocity[24] == 12);
        check("unreserved channel-16 APC CC still consumed", smk_apc_usb_event(packet(0xbf, 48, 20)) == 1);
        if (!user) {
            check("SMC ranges remain APC feedback when User mode is off", smk_apc_usb_event(packet(0xbf, 16, 20)) == 1);
            check("channel-16 APC note remains active when User mode is off", smk_apc_usb_event(packet(0x9f, 24, 7)) == 1 && apc_led_velocity[24] == 7);
        }
    }
    printf("SMC host ingress: %s\n", fails ? "FAIL" : "PASS");
    return fails != 0;
}
