/* SPDX-License-Identifier: GPL-3.0-only */
/* Reuse the production-linked panic fixture; verify mute keeps internal notes
 * while MIDI stays silent, and OFF/panic still release those internal notes. */
#define main panic_fixture_main
#include "panic_notes_test.c"
#undef main

int main(void)
{
    for (int b = 0; b < NBASS; b++) {
        int t = TRK_BASS0 + b;
        setup();
        seq_start(&seq); run(64);
        CHECK(seq.t[t].sounding);
        nevents = 0;
        seq.mute |= 1u << (MUTE_BASS0 + b);
        run(4410);
        CHECK(seq.t[t].pos == 1);
        CHECK(seq.t[t].sounding);
        CHECK(count_kind(EV_OFF, b) == 0);
        CHECK(saw_status(0x80 | seq.ch[t]));
        CHECK(!saw_status(0x90 | seq.ch[t]));
        CHECK(!seq.bass_out_valid[b]);
        nevents = 0;
        run(3u * 4410u);
        CHECK(seq.t[t].pos == 4);
        CHECK(seq.t[t].sounding);
        CHECK(count_kind(EV_ON, b) == 1); /* muted NOTE still progresses */
        CHECK(!saw_status(0x90 | seq.ch[t]));
        seq.mute &= ~(1u << (MUTE_BASS0 + b));
        nevents = 0; run(4410);
        CHECK(seq.t[t].sounding);
        CHECK(count_kind(EV_ON, b) == 0); /* unmute TIE needs no new onset */
        CHECK(!saw_status(0x90 | seq.ch[t])); /* donor MIDI policy */
        seq.mute |= 1u << (MUTE_BASS0 + b);
        seq_notes_off(&seq, &sink);
        CHECK(!seq.t[t].sounding);
        CHECK(seq.playing);
        nevents = 0; run(4410);
        CHECK(!seq.t[t].sounding); /* panic makes the next TIE an orphan */
        CHECK(count_kind(EV_ON, b) == 0);

        setup(); seq_start(&seq); run(64);
        seq.mute |= 1u << (MUTE_BASS0 + b);
        seq.part_off |= 1u << t;
        run(4410);
        CHECK(!seq.t[t].sounding);
        CHECK(count_kind(EV_OFF, b) > 0);
        CHECK(seq.playing);
    }
    printf("seq mute carry: %u checks, %u failures\n", checks, failures);
    return failures != 0;
}
