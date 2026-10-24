/* SPDX-License-Identifier: GPL-3.0-only
 * Host-only future protocol prototype test; synthetic IDs, no actual I/O.
 */
#include <stdio.h>
#include <string.h>
#include "../../tools/prototypes/ota_identity_v1.h"

static unsigned checks, failures;
#define CHECK(expr, label) do { checks++; if (!(expr)) { failures++; printf("FAIL %s\n", label); } } while (0)

int main(void)
{
    uint8_t nonce[32], next[32], tag_a[16], tag_b[16], zero[32] = {0}, ff[16];
    uint8_t query[37], reply[56], changed[56], bad[38];
    ota_uid_read_plan plan;
    unsigned i, n, model;
    for (i = 0; i < 32; i++) { nonce[i] = (uint8_t)(i + 1u); next[i] = (uint8_t)(i + 2u); }
    for (i = 0; i < 16; i++) { tag_a[i] = (uint8_t)(i + 30u); tag_b[i] = (uint8_t)(i + 60u); ff[i] = 0xff; }
    CHECK(!ota_uid_plan(OTA_UID_UNKNOWN, 0x856014u, &plan) && !plan.tx_bytes && !plan.rx_bytes,
          "JEDEC alone never selects hardware provenance");
    CHECK(!ota_uid_plan(OTA_UID_P25Q80H_V17, 0xEB6014u, &plan) && !plan.tx_bytes,
          "unknown EB6014 never gets guessed Puya UID plan");
    for (i = OTA_UID_P25Q80H_V17; i <= OTA_UID_P25Q80SH_V14; i++) {
        CHECK(ota_uid_plan(i, 0x856014u, &plan), "explicit documented part profile");
        CHECK(plan.tx[0] == 0x4b && plan.tx_bytes == 5 && plan.rx_bytes == 16,
              "4B four dummy bytes and sixteen output bytes");
        CHECK(!(plan.tx[1] | plan.tx[2] | plan.tx[3] | plan.tx[4]), "dummy bytes only, no address/write");
    }
    CHECK(!ota_uid_plan(99, 0x856014u, &plan), "unknown part profile");
    CHECK(ota_uid_reads_valid(tag_a, tag_a, 16, 0, 0), "matching successful synthetic reads");
    CHECK(!ota_uid_reads_valid(tag_a, tag_b, 16, 0, 0), "inconsistent reads");
    CHECK(!ota_uid_reads_valid(zero, zero, 16, 0, 0), "all-zero bus data");
    CHECK(!ota_uid_reads_valid(ff, ff, 16, 0, 0), "all-FF bus data");
    CHECK(!ota_uid_reads_valid(tag_a, tag_a, 8, 0, 0), "truncated eight-byte pseudo UID");
    CHECK(!ota_uid_reads_valid(tag_a, tag_a, 16, -1, 0), "first SPI failure propagates");
    CHECK(!ota_uid_reads_valid(tag_a, tag_a, 16, 0, -1), "second SPI failure propagates");
    CHECK(ota_id_query(nonce, query, sizeof query) == 37, "canonical query");
    CHECK(ota_id_reply(query, 37, OTA_ID_UNQUALIFIED, tag_a, OTA_ID_APP, OTA_ID_PRO, reply, 56) == 56,
          "unavailable provider still answers read-only capability query");
    CHECK(reply[5] == OTA_ID_UNAVAILABLE && !ota_id_nonzero(reply + 40, 16), "no fabricated tag when unavailable");
    CHECK(!ota_id_matches(reply, 56, nonce, tag_a, OTA_ID_APP, OTA_ID_PRO), "unavailable is never identity success");
    CHECK(ota_id_reply(query, 37, 99, tag_a, OTA_ID_APP, OTA_ID_PRO, reply, 56) == 56 &&
          reply[5] == OTA_ID_UNAVAILABLE && !ota_id_nonzero(reply + 40, 16), "unknown qualification stays unavailable");
    CHECK(ota_id_reply(query, 37, OTA_ID_FACTORY_TAG, NULL, OTA_ID_APP, OTA_ID_PRO, reply, 56) == 56 &&
          reply[5] == OTA_ID_UNAVAILABLE, "missing public tag never fabricated");
    CHECK(ota_id_reply(query, 37, OTA_ID_FACTORY_TAG, zero, OTA_ID_APP, OTA_ID_PRO, reply, 56) == 56 &&
          reply[5] == OTA_ID_UNAVAILABLE, "zero public tag stays unavailable");
    CHECK(ota_id_reply(query, 37, 1, tag_a, 0, 1, reply, 56) < 0, "unknown responder phase");
    CHECK(ota_id_reply(query, 37, 1, tag_a, 1, 0, reply, 56) < 0, "unknown responder model");
    for (i = 0; i < 5; i++) {
        memcpy(bad, query, 37); bad[i] ^= 0x40;
        CHECK(ota_id_reply(bad, 37, 1, tag_a, 1, 1, reply, 56) < 0, "bad query version/header/kind");
    }
    for (model = OTA_ID_ELITE; model <= OTA_ID_PRO; model++) {
        CHECK(ota_id_reply(query, 37, OTA_ID_FACTORY_TAG, tag_a, OTA_ID_APP, model, reply, 56) == 56,
              "synthetic app reply");
        CHECK(ota_id_matches(reply, 56, nonce, tag_a, OTA_ID_APP, model), "synthetic correct app");
        CHECK(!ota_id_matches(reply, 56, nonce, tag_b, OTA_ID_APP, model), "foreign same-model unit");
        CHECK(!ota_id_matches(reply, 56, next, tag_a, OTA_ID_APP, model), "old reply wrong fresh challenge");
        CHECK(!ota_id_matches(reply, 56, nonce, tag_a, OTA_ID_LOADER, model), "wrong phase");
        CHECK(!ota_id_matches(reply, 56, nonce, tag_a, OTA_ID_APP, 3u - model), "wrong exact model");
    }
    CHECK(ota_id_query(next, query, 37) == 37, "new loader challenge");
    CHECK(ota_id_reply(query, 37, OTA_ID_FACTORY_TAG, tag_a, OTA_ID_LOADER, OTA_ID_PRO, reply, 56) == 56,
          "synthetic destination Pro loader after Elite app");
    CHECK(ota_id_matches(reply, 56, next, tag_a, OTA_ID_LOADER, OTA_ID_PRO), "same-tag cross-model destination");
    CHECK(!ota_id_matches(reply, 56, nonce, tag_a, OTA_ID_LOADER, OTA_ID_PRO), "replayed app-era nonce");
    for (n = 0; n < 56; n++) CHECK(!ota_id_matches(reply, n, next, tag_a, OTA_ID_LOADER, OTA_ID_PRO), "short reply");
    CHECK(!ota_id_matches(reply, 57, next, tag_a, OTA_ID_LOADER, OTA_ID_PRO), "trailing reply data");
    for (i = 0; i < 56; i++) {
        memcpy(changed, reply, 56); changed[i] ^= 0x80;
        CHECK(!ota_id_matches(changed, 56, next, tag_a, OTA_ID_LOADER, OTA_ID_PRO), "single-byte reply mutation");
    }
    for (n = 0; n < 37; n++) CHECK(ota_id_reply(query, n, 1, tag_a, 1, 1, reply, 56) < 0,
                                  "short query refuses before provider capability");
    memcpy(bad, query, 37); bad[37] = 0;
    CHECK(ota_id_reply(bad, 38, 1, tag_a, 1, 1, reply, 56) < 0, "trailing query data");
    CHECK(ota_id_query(zero, query, 37) < 0, "all-zero challenge refuses");
    CHECK(ota_id_reply(query, 37, 1, tag_a, 1, 1, reply, 56) < 0, "cleared query cannot replay prior success");
    CHECK(!ota_id_matches(NULL, 56, nonce, tag_a, 1, 1), "null reply");
    CHECK(!ota_id_matches(reply, 56, nonce, NULL, 1, 1), "no trusted initial identity");
    printf("%u/%u prototype checks passed; no hardware, SPI/USB or destructive I/O\n", checks - failures, checks);
    return failures ? 1 : 0;
}
