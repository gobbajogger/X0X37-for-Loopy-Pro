/* SPDX-License-Identifier: GPL-3.0-only */
/* Included by ble.c after the two role services. TIMER5 is the sole owner of
 * the host union and the mailbox consumer. Never restart the RF/core-1 image. */
static uint8_t ble_role_changing;
static uint32_t ble_role_at;

static void ble_service(void)
{
    uint8_t next = (uint8_t)(smc_ble_role_get() != 0);
    uint8_t addr[6];
    unsigned i;
    int stopped;
    if (ble_up && next != ble_central_role && !ble_role_changing) {
        ble_role_changing = 1;
        ble_role_at = fm1_ms;
    }
    if (ble_central_role) ble_central_service();
    else ble_device_service();
    if (!ble_up) return;
    if (next == ble_central_role) { ble_role_changing = 0; return; }
    if (ble_central_role)
        stopped = blec_pair_stopped(&ble_r.host.c, &ble_r.host.c2);
    else
        stopped = !ble_link_st && !ble_r.host.h.cmd_wait &&
                  (ble_r.host.h.state == BLEH_IDLE ||
                   (ble_r.host.h.state == BLEH_ADVERTISING && !ble_r.host.h.want_adv));
    if (!stopped || xb_ring_used(&ble_r.mb.c2h) || xb_ring_used(&ble_r.mb.h2c)) {
        /* A controller that never acknowledges stop is not safe to reuse. */
        if (fm1_ms - ble_role_at >= 3000u) ble_fail();
        return;
    }
    for (i = 0; i < sizeof addr; i++)
        addr[i] = ble_central_role ? ble_r.host.c.addr[i] : ble_r.host.h.addr[i];
    ble_central_link_notify(0);
    smk_apc_ble_link(0, 0);
    ble_r.orr = ble_r.ow;
    ble_sx_n = ble_sx_over = ble_sx_reply = ble_sx_ch = 0;
    if (next) {
        blec_init(&ble_r.host.c, ble_send, ble_central_midi, 0, addr);
        blec_init(&ble_r.host.c2, ble_send, ble_central_midi, (void *)1, addr);
        blec_pair_bind(&ble_r.host.c, &ble_r.host.c2);
    } else {
        blec_pair_bind(0, 0);
        bleh_init(&ble_r.host.h, ble_send, 0, ble_r.nm, addr);
        bleh_set_sysex(&ble_r.host.h, ble_sysex_in);
    }
    ble_host_on = 0;  /* next tick starts the new role with its HCI Reset */
    ble_central_role = next;
    ble_role_changing = 0;
}
