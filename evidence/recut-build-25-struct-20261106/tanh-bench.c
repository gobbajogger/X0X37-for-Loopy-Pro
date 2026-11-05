#define _POSIX_C_SOURCE 200809L
#define main exhaustive_main
#include "../../tests/host/drum909_tanh_pair_test.c"
#undef main
#include <time.h>

static float inputs[4096];
static volatile float sink;
static double now(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return (double)t.tv_sec + (double)t.tv_nsec * 1e-9; }

int main(void)
{
    unsigned rng = 37;
    for (unsigned i = 0; i < 4096; ++i) {
        rng = rng * 1664525u + 1013904223u;
        inputs[i] = ((float)(rng >> 8) * (1.0f / 16777216.0f) - 0.5f) * 16.0f;
    }
    for (unsigned run = 0; run < 7; ++run) {
        double seconds[2];
        for (unsigned pass = 0; pass < 2; ++pass) {
            unsigned which = (pass + run) & 1;
            float sum = 0.0f;
            double start = now();
            if (which) {
                for (unsigned i = 0; i < 8388608; ++i) sum += d9_tanh(inputs[i & 4095]);
            } else {
                for (unsigned i = 0; i < 8388608; ++i) sum += ref_tanh(inputs[i & 4095]);
            }
            seconds[which] = now() - start;
            sink = sum;
        }
        printf("%u %.9f %.9f\n", run, seconds[0], seconds[1]);
    }
    return 0;
}
