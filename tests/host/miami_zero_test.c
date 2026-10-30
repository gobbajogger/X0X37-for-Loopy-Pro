/* SPDX-License-Identifier: GPL-3.0-only */
/* Standalone Miami idle-zero regression. No private synth inputs or devices. */
#include "miami_zero.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_FRAMES 256u
#define GUARD 8u
#define CHECK(x) do { if (!(x)) { \
    fprintf(stderr, "miami_zero: failed at line %d: %s\n", __LINE__, #x); \
    return 1; \
} } while (0)

static uint32_t random_state = UINT32_C(0x5a17fbd3);
static uint32_t next_random(void)
{
    random_state ^= random_state << 13;
    random_state ^= random_state >> 17;
    random_state ^= random_state << 5;
    return random_state;
}

static void fill(void *destination, size_t bytes)
{
    unsigned char *p = (unsigned char *)destination;
    for (size_t i = 0; i < bytes; ++i) p[i] = (unsigned char)next_random();
}

static int test_words(uint32_t n, unsigned offset)
{
    uint32_t actual[MAX_FRAMES + 2u * GUARD];
    uint32_t expected[MAX_FRAMES + 2u * GUARD];
    fill(actual, sizeof actual);
    memcpy(expected, actual, sizeof actual);
    for (uint32_t i = 0; i < n; ++i) expected[GUARD + offset + i] = 0;
    original_miami_zero_words(actual + GUARD + offset, n);
    CHECK(memcmp(actual, expected, sizeof actual) == 0);
    return 0;
}

static int test_halves(uint32_t n, unsigned offset)
{
    uint16_t actual[MAX_FRAMES + 2u * GUARD];
    uint16_t expected[MAX_FRAMES + 2u * GUARD];
    fill(actual, sizeof actual);
    memcpy(expected, actual, sizeof actual);
    for (uint32_t i = 0; i < n; ++i) expected[GUARD + offset + i] = 0;
    original_miami_zero_halves(actual + GUARD + offset, n);
    CHECK(memcmp(actual, expected, sizeof actual) == 0);
    return 0;
}

static int test_floats(uint32_t n, unsigned offset)
{
    float actual[MAX_FRAMES + 2u * GUARD];
    float expected[MAX_FRAMES + 2u * GUARD];
    fill(actual, sizeof actual);
    memcpy(expected, actual, sizeof actual);
    for (uint32_t i = 0; i < n; ++i) expected[GUARD + offset + i] = 0.0f;
    original_miami_zero_floats(actual + GUARD + offset, n);
    CHECK(memcmp(actual, expected, sizeof actual) == 0);
    return 0;
}

int main(void)
{
    const uint32_t fzero = 0;
    const float positive_zero = 0.0f;
    CHECK(sizeof(float) == sizeof(uint32_t));
    CHECK(memcmp(&fzero, &positive_zero, sizeof fzero) == 0);
    /* An empty span must remain legal even with a null pointer. */
    original_miami_zero_words(NULL, 0);
    original_miami_zero_halves(NULL, 0);
    original_miami_zero_floats(NULL, 0);
    /* Shifted spans, irregular lengths, and random surrounding state must
     * preserve both guard regions and every unrequested output/work byte. */
    for (unsigned repetition = 0; repetition < 4; ++repetition) {
        for (uint32_t n = 0; n <= MAX_FRAMES; ++n) {
            for (unsigned offset = 0; offset <= GUARD; ++offset) {
                CHECK(test_words(n, offset) == 0);
                CHECK(test_halves(n, offset) == 0);
                CHECK(test_floats(n, offset) == 0);
            }
        }
    }
    printf("miami_zero: PASS %u byte-exact guarded spans for all three outputs\n",
           4u * (MAX_FRAMES + 1u) * (GUARD + 1u));
    return 0;
}
