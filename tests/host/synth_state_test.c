/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../../firmware/src/app/synth_state.h"
#include "../../firmware/src/app/x0x.h"
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); exit(1); } checks++; } while (0)
/* Changing this number requires an explicit new legacy ABI/version decision. */
typedef char legacy_v1_abi_is_35052[(sizeof(project_t) == 35052u) ? 1 : -1];
static unsigned checks;
static project_t legacy;
static uint8_t envelope[sizeof(project_t) + 512], again[sizeof envelope], damaged[sizeof envelope];
static void repair_crc(uint8_t *p, size_t n)
{
    synth_state_put32(p + 28, synth_state_envelope_crc(p,n));
}
int main(void)
{
    synth_state_view_t view, decoded, before;
    uint8_t settings[128];
    size_t i, n = 0, n2 = 0;
    /* Pinned raw ABI fixture with distinct data in every byte, including padding,
     * unused sound cells and legacy motion lanes. Codec never normalizes them. */
    for (i = 0; i < sizeof legacy; i++) ((uint8_t *)&legacy)[i] = (uint8_t)(i * 37u + 19u);
    for (i = 0; i < sizeof settings; i++) settings[i] = (uint8_t)(i ^ 0xa5u);
    CHECK(synth_state_migrate_legacy(&legacy,sizeof legacy,sizeof legacy,&view) == 0);
    CHECK(view.legacy_lock_engine == SYNTH_STATE_303_ID);
    for (i = 0; i < 2; i++) {
        CHECK(view.part[i].selected_engine_id == SYNTH_STATE_303_ID);
        CHECK(view.part[i].engine_id == SYNTH_STATE_303_ID);
        CHECK(view.part[i].schema == SYNTH_STATE_303_SCHEMA && view.part[i].size == 0);
    }
    CHECK(synth_state_migrate_legacy(&legacy,sizeof legacy - 1,sizeof legacy,&decoded) == SYNTH_STATE_INVALID);
    CHECK(synth_state_migrate_legacy(&legacy,sizeof legacy,0,&decoded) == SYNTH_STATE_INVALID);
    CHECK(synth_state_encode(envelope,sizeof envelope,&view,&n) == 0);
    CHECK(synth_state_decode(envelope,n,sizeof legacy,&decoded) == 0);
    CHECK(!memcmp(&legacy,decoded.legacy,sizeof legacy));
    CHECK(synth_state_encode(again,sizeof again,&decoded,&n2) == 0 && n == n2 && !memcmp(envelope,again,n));
    /* Unknown engine and future engine setting schema are retained opaquely. */
    view.part[1].engine_id = 0x87654321u;
    view.part[1].schema = 65535;
    view.part[1].size = sizeof settings;
    view.part[1].bytes = settings;
    CHECK(synth_state_encode(envelope,sizeof envelope,&view,&n) == 0);
    CHECK(synth_state_decode(envelope,n,sizeof legacy,&decoded) == 0);
    CHECK(decoded.part[1].selected_engine_id == SYNTH_STATE_303_ID);
    CHECK(decoded.part[1].engine_id == 0x87654321u && decoded.part[1].schema == 65535);
    CHECK(decoded.part[1].size == sizeof settings && !memcmp(decoded.part[1].bytes,settings,sizeof settings));
    CHECK(!memcmp(decoded.legacy,&legacy,sizeof legacy));
    CHECK(synth_state_encode(again,sizeof again,&decoded,&n2) == 0 && n == n2 && !memcmp(envelope,again,n));
    /* Select the retained alternate while the original 303 pots remain untouched. */
    view.part[1].selected_engine_id = 0x87654321u;
    CHECK(synth_state_encode(envelope,sizeof envelope,&view,&n) == 0);
    CHECK(synth_state_decode(envelope,n,sizeof legacy,&decoded) == 0);
    CHECK(decoded.part[1].selected_engine_id == 0x87654321u);
    CHECK(!memcmp(decoded.legacy,&legacy,sizeof legacy));
    CHECK(synth_state_encode(again,sizeof again,&decoded,&n2) == 0 && n == n2 && !memcmp(envelope,again,n));
    /* Every truncated byte length fails, with caller output unchanged. */
    before = decoded;
    for (i = 0; i < n; i++) {
        CHECK(synth_state_decode(envelope,i,sizeof legacy,&decoded) != 0);
        CHECK(!memcmp(&decoded,&before,sizeof decoded));
    }
    CHECK(synth_state_encode(again,n - 1,&view,&n2) == SYNTH_STATE_SPACE);
    CHECK(synth_state_decode(envelope,n,sizeof legacy - 1,&decoded) == SYNTH_STATE_INVALID);
    memcpy(damaged,envelope,n); damaged[4] = 2;
    CHECK(synth_state_decode(damaged,n,sizeof legacy,&decoded) == SYNTH_STATE_VERSION_ERROR);
    memcpy(damaged,envelope,n); damaged[SYNTH_STATE_HEADER + 1] ^= 1;
    CHECK(synth_state_decode(damaged,n,sizeof legacy,&decoded) == SYNTH_STATE_CRC_ERROR);
    memcpy(damaged,envelope,n); damaged[16] = 2; repair_crc(damaged,n);
    CHECK(synth_state_decode(damaged,n,sizeof legacy,&decoded) == SYNTH_STATE_INVALID);
    memcpy(damaged,envelope,n); damaged[24] = 1; repair_crc(damaged,n);
    CHECK(synth_state_decode(damaged,n,sizeof legacy,&decoded) == SYNTH_STATE_INVALID);
    memcpy(damaged,envelope,n); synth_state_put16(damaged + SYNTH_STATE_HEADER + sizeof legacy + 10,129); repair_crc(damaged,n);
    CHECK(synth_state_decode(damaged,n,sizeof legacy,&decoded) == SYNTH_STATE_INVALID);
    memcpy(damaged,envelope,n); synth_state_put32(damaged + SYNTH_STATE_HEADER + sizeof legacy,0); repair_crc(damaged,n);
    CHECK(synth_state_decode(damaged,n,sizeof legacy,&decoded) == SYNTH_STATE_INVALID);
    memcpy(damaged,envelope,n); damaged[n] = 0; synth_state_put32(damaged+8,(uint32_t)(n+1)); repair_crc(damaged,n+1);
    CHECK(synth_state_decode(damaged,n+1,sizeof legacy,&decoded) == SYNTH_STATE_INVALID);
    view.part[1].size = 129;
    CHECK(synth_state_encode(again,sizeof again,&view,&n2) == SYNTH_STATE_INVALID);
    CHECK(!memcmp(&decoded,&before,sizeof decoded));
    printf("synth state: %u checks PASS; raw project ABI %lu bytes, envelope overhead %u bytes\n",checks,(unsigned long)sizeof legacy,SYNTH_STATE_HEADER+2*SYNTH_STATE_RECORD);
    return 0;
}
