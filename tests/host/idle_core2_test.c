/* SPDX-License-Identifier: GPL-3.0-only */
/* Production engine fixture: idle 808 must not dispatch or wait on CPU2. */
#define plat_cpu2_run fixture_host_cpu2_run
#define plat_cpu2_wait fixture_host_cpu2_wait
#define AUDIO_PATH_IDENTITY_LIBRARY
#include "audio_path_identity.c"
#undef plat_cpu2_run
#undef plat_cpu2_wait

static unsigned dispatches, waits;
int plat_cpu2_run(void (*fn)(uint32_t), uint32_t arg)
{
    ++dispatches;
    return fixture_host_cpu2_run(fn, arg);
}
int plat_cpu2_wait(void)
{
    ++waits;
    return fixture_host_cpu2_wait();
}

int main(int argc, char **argv)
{
    assert(argc == 2); /* synthetic WAV prepared by audio_path_identity.py */
    boot();
    assert(plat_cpu2_set(1) == 0);
    engine_guard_enable(0);
    assert(load_slot(0, 0, argv[1]) == 0);
    engine_brk_loops();
    int32_t lr[512];
    for (unsigned i = 0; i < 128; ++i) engine_render(lr, 256);
    assert(dispatches == 0 && waits == 0);
    /* BRK still renders through the local shared-job path. */
    engine_brk_live(0, 1);
    for (unsigned i = 0; i < 16; ++i) engine_render(lr, 256);
    assert(dispatches == 0 && waits == 0);
    assert(eng_peak[PART_BRK] != 0);
    engine_brk_live(0, 0);
    engine_drum(1, 0, 1.0f);
    engine_render(lr, 256);
    assert(dispatches != 0 && waits != 0);
    /* The existing disabled-core fallback must still render active 808. */
    assert(plat_cpu2_set(0) == 0);
    dispatches = waits = 0;
    memset(lr, 0, sizeof lr);
    eng_peak[PART_808] = 0;
    engine_render(lr, 256);
    assert(dispatches != 0 && waits == 0);
    unsigned heard = 0;
    for (unsigned i = 0; i < 512; ++i) heard |= lr[i] != 0;
    assert(heard && eng_peak[PART_808] != 0);
    return 0;
}
