/* SPDX-License-Identifier: GPL-3.0-only */
/* Independent production replay: a rejected feedback write must retry, and a
 * dropped unrelated release must never turn a held Pause repeat into a press. */
#include <stdint.h>
#include <string.h>
static unsigned attempted, accepted, fail_note, failure_enabled;
static uint8_t last_ok[3];
int plat_ble_smc_send(const uint8_t *m, uint8_t n)
{
    attempted++;
    if (failure_enabled && n == 3 && m[0] == 0x90 && m[1] == fail_note) return 0;
    accepted++;
    if (n == 3) memcpy(last_ok, m, 3);
    return 1;
}
#define SMC_UI_SEND_IMPLEMENTED 1
#define SMC_UI_MAIN smc_ui_replay_main
#include "smc_ui_test.c"

static void send_note(int host, unsigned note, unsigned status, unsigned velocity)
{
    if (host) {
        unsigned locked=smc.locked, down=smc.lock_down;
        CHECK(!smc_midi_host((status == 0x80 ? 8u : 9u) |
            (status | 15u) << 8 | note << 16 | velocity << 24));
        smc_poll(); CHECK(smc.locked==locked && smc.lock_down==down);
    }
    message(status, note, velocity);
}
static void queue_lost_release(int host, unsigned note, unsigned status)
{
    if (host) {
        for (unsigned i = 0; i < SMC_Q; i++)
            CHECK(!smc_midi_host(11u | 0xBFu << 8 | 16u << 16));
        CHECK(!smc_midi_host((status == 0x80 ? 8u : 9u) | (status | 15u) << 8 | note << 16));
    }
    {
        const uint8_t filler[3] = {0xB0, 16, 0};
        const uint8_t release[3] = {(uint8_t)status, (uint8_t)note, 0};
        for (unsigned i = 0; i < SMC_Q; i++) smc_ble_midi(filler, 3);
        smc_ble_midi(release, 3);
    }
    smc_poll();
}
static void feedback_retry(void)
{
    setup(V_SEQ, PART_808); smc_frame();
    seq.ext_now = smc.beat_at + smc.beat_period / 2u; smc_frame();
    CHECK(smc.known[0x5D] && !smc.lit[0x5D]);
    unsigned before = accepted;
    fail_note = 0x5D; failure_enabled = 1; seq.ext_now = smc.beat_at + smc.beat_period; smc_frame();
    CHECK(accepted == before && !smc.lit[0x5D]);
    smc_frame(); CHECK(accepted == before && !smc.lit[0x5D]);
    failure_enabled = 0; smc_frame();
    CHECK(accepted == before + 1 && smc.lit[0x5D]);
    CHECK(last_ok[0] == 0x90 && last_ok[1] == 0x5D && last_ok[2] == 127);
    send_note(0, 0x5D, 0x90, 127); send_note(0, 0x5D, 0x90, 0);
    CHECK(smc.locked); seq.ext_now += smc.beat_period / 2u; smc_frame(); CHECK(smc.lit[0x5D]);
    send_note(0, 0x5D, 0x90, 127); CHECK(smc.locked);
    now_ms += 900; smc_poll(); send_note(0, 0x5D, 0x90, 0);
    CHECK(!smc.locked); smc_frame(); CHECK(!smc.lit[0x5D]);
    puts("PASS independent LED backpressure: rejected write retries; cache tracks accepted bytes; lock/unlock phases");
}
static void independent_release_loss(void)
{
    const unsigned unrelated[3] = {8, 24, 0x5E};
    for (int host = 0; host < 2; host++) for (int type = 0; type < 2; type++) {
        unsigned status = type ? 0x80 : 0x90;
        for (unsigned n = 0; n < 3; n++) {
            setup(V_SEQ, PART_909); proj.set.bt |= PROJ_BT_SMC_USER;
            send_note(host, 0x5D, 0x90, 127); CHECK(smc.locked && smc.lock_down);
            pref_t captured = smc_target(0);
            queue_lost_release(host, unrelated[n], status);
            CHECK(smc.locked && smc.lock_down);
            send_note(host, 0x5D, 0x90, 127);
            CHECK(smc.locked && smc_same(captured, smc_target(0)));
            send_note(host, 0x5D, status, 0); CHECK(smc.locked && !smc.lock_down);
            send_note(host, 0x5D, 0x90, 127); CHECK(smc.locked);
            now_ms += 900; smc_poll(); CHECK(!smc.locked);
            send_note(host, 0x5D, status, 0); CHECK(!smc.locked && !smc.lock_down);
        }
        setup(V_SEQ, PART_808); proj.set.bt |= PROJ_BT_SMC_USER;
        send_note(host, 0x5D, 0x90, 127); CHECK(smc.locked);
        /* Publication rollover is ordinary modulo arithmetic, not a stale loss. */
        smc_pause_release_lost[0] = smc_pause_release_seen[0] = UINT32_MAX;
        queue_lost_release(host, 0x5D, status);
        CHECK(smc.locked && !smc.lock_down);
        CHECK(!smc_pause_release_lost[0] && !smc_pause_release_seen[0]);
        send_note(host, 0x5D, 0x90, 127); CHECK(smc.locked);
        now_ms += 900; smc_poll(); CHECK(!smc.locked);
        send_note(host, 0x5D, status, 0);
    }
    puts("PASS independent Pause release ownership: DAW with/without retired-host noise, NoteOff/zero, unrelated Solo/Select/Play losses, actual Pause loss and epoch wrap");
}
int main(void)
{
    feedback_retry(); independent_release_loss();
    printf("smc_pause_loss: PASS (%u independent production checks)\n", checks);
    return 0;
}
