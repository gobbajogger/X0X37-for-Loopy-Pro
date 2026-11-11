/* SPDX-License-Identifier: GPL-3.0-only */
/* Same production host closure as audio_path_identity.c; no device counters. */
#define main simulator_main
#include "../../host/x0x_host.c"
#undef main
#include <assert.h>
static uint64_t cycles_now(void)
{
#ifdef __APPLE__
    struct rusage_info_v4 ri;
    if (!proc_pid_rusage(getpid(), RUSAGE_INFO_V4, (rusage_info_t *)&ri))
        return ri.ri_cycles;
#endif
    return 0;
}
int main(int argc, char **argv)
{
    if (argc != 2) return 2;
    const char *mode = argv[1];
    int32_t lr[512];
    float l[256], r[256];
    master_t m;
    boot();
    master = 4096;
    engine_usb_full(1);
    master_init(&m);
    master_set(&m, MST_LIMIT, 0);
    master_set(&m, MST_MODE, 1);
    m.g = m.g_t;
    if (!strcmp(mode, "render")) {
        for (int b = 0; b < NBASS; ++b)
            engine_bass_on(b, 36 + b * 12, 0, 0);
    }
    for (int i = 0; i < 256; ++i) { lr[2*i] = i * 1234; lr[2*i+1] = -i * 432; l[i] = .25f; r[i] = -.3f; }
    const int count = !strcmp(mode, "render") ? 4000 : 50000;
    double dt;
    uint64_t instructions, cycles;
#define MEASURE(call) do { \
    uint64_t i0 = instr_now(), c0 = cycles_now(); \
    double t0 = now_ns(); \
    for (int k = 0; k < count; ++k) { call; } \
    dt = now_ns() - t0; \
    cycles = cycles_now() - c0; \
    instructions = instr_now() - i0; \
} while (0)
    if (!strcmp(mode, "volume")) MEASURE(engine_volume(lr, 256));
    else if (!strcmp(mode, "mono")) MEASURE(master_process(&m, l, 256, 1.0f));
    else if (!strcmp(mode, "stereo")) MEASURE(master_process_st(&m, l, r, 256, 1.0f));
    else if (!strcmp(mode, "render")) MEASURE(engine_render(lr, 256));
    else return 2;
#undef MEASURE
    printf("{\"workload\":\"%s\",\"frames\":%d,\"ns\":%.0f,\"instructions\":%llu,\"cycles\":%llu}\n",
           mode, count*256, dt, (unsigned long long)instructions, (unsigned long long)cycles);
    return 0;
}
