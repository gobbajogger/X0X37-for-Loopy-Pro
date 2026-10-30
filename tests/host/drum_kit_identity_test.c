/* SPDX-License-Identifier: GPL-3.0-only
 * Feature 23: two eleven-voice drum models, never a shared generic grid.
 * Pure host test: no hardware, SDK, samples or firmware build required.
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* This single-unit test owns the shared lookup tables (drum909.c does on the device). */
#define X0X_DRUM_TABLES_DEFINE
#include "../../firmware/src/seq/pattern.h"
#include "../../firmware/src/dsp/drum909.h"
#include "../../firmware/src/dsp/drum808.h"
#include "../../firmware/src/app/engine.h"

#define REQUIRE(x) do { if (!(x)) { \
    fprintf(stderr, "FAIL drum kit identity at line %d: %s\n", __LINE__, #x); \
    return 1; \
} } while (0)

_Static_assert(NKIT == 2 && NDRUM == 11, "909 and 808 need independent 11-voice rows");
_Static_assert(DR_NUM == 11 && D8_NUM == 11, "DSP voice counts must match the stored rows");
_Static_assert(DR_KIT == NDRUM && D8_KIT == NDRUM, "kit controls follow the voices");
_Static_assert(T_909 != T_808 && PART_909 != PART_808, "engine and part IDs must differ");
_Static_assert(DR_CH == 7 && DR_OH == 8 && DR_CR == 9 && DR_RD == 10,
               "909 hat/cymbal voice assignments must remain stable");
_Static_assert(D8_CB == 7 && D8_CY == 8 && D8_OH == 9 && D8_CH == 10,
               "808 cowbell/cymbal/hat voice assignments must remain stable");
_Static_assert(NVOICES_MAX >= 12, "sound bank needs eleven voices and a kit");

int main(void)
{
    pattern_t p = {0};
    sound_t sound = {0};
    static const int s909[] = {DR_BD, DR_SD, DR_LT, DR_MT, DR_HT, DR_RS,
                               DR_CP, DR_CH, DR_OH, DR_CR, DR_RD};
    static const int s808[] = {D8_BD, D8_SD, D8_LT, D8_MT, D8_HT, D8_RS,
                               D8_CP, D8_CB, D8_CY, D8_OH, D8_CH};
    for (int v = 0; v < NDRUM; ++v) {
        REQUIRE(s909[v] == v && s808[v] == v);
        sm_set(p.drum[0].hit[v], v);
        sm_set(p.drum[1].hit[v], v + 32);
    }
    for (int v = 0; v < NDRUM; ++v) {
        REQUIRE(sm_get(p.drum[0].hit[v], v));
        REQUIRE(!sm_get(p.drum[0].hit[v], v + 32));
        REQUIRE(sm_get(p.drum[1].hit[v], v + 32));
        REQUIRE(!sm_get(p.drum[1].hit[v], v));
    }
    sm_set(p.drum[0].accent, 1);
    sm_set(p.drum[1].accent, 34);
    REQUIRE(sm_get(p.drum[0].accent, 1));
    REQUIRE(!sm_get(p.drum[1].accent, 1));
    REQUIRE(sm_get(p.drum[1].accent, 34));
    REQUIRE(!sm_get(p.drum[0].accent, 34));

    /* Sound pots use distinct T_909/T_808 dimensions. Modifying an 808
     * voice/kit must not change the same ordinal in the 909 sound. */
    sound.v[T_909][DR_CH][0] = 33;
    sound.v[T_808][D8_CB][0] = 77;
    sound.v[T_909][DR_KIT][1] = 60;
    sound.v[T_808][D8_KIT][1] = 118;
    REQUIRE(sound.v[T_909][DR_CH][0] == 33);
    REQUIRE(sound.v[T_808][D8_CB][0] == 77);
    REQUIRE(sound.v[T_909][DR_KIT][1] == 60);
    REQUIRE(sound.v[T_808][D8_KIT][1] == 118);

    /* Whole-pattern copying is used for projects and undo. Distinct kit
     * assignments must survive without reinterpretation or accidental alias. */
    pattern_t saved;
    memcpy(&saved, &p, sizeof saved);
    REQUIRE(!memcmp(&saved, &p, sizeof p));
    memset(&p.drum[1], 0, sizeof p.drum[1]);
    REQUIRE(sm_get(saved.drum[1].hit[D8_CH], 42));
    REQUIRE(sm_get(saved.drum[0].hit[DR_CH], 7));
    REQUIRE(!sm_get(p.drum[1].hit[D8_CH], 42));
    REQUIRE(sm_get(p.drum[0].hit[DR_CH], 7));

    puts("PASS drum_kit_identity: independent 909/808 voice, accent, sound and project rows");
    return 0;
}
