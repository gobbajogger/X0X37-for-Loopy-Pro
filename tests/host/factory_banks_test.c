/* SPDX-License-Identifier: GPL-3.0-only */
/* Host proof: Chicago/Miami factory banks load without user import;
 * a user import leaves factory ROM unchanged. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../../firmware/src/app/factory_banks.c"

static int failures;

static void expect(int cond, const char *msg)
{
    if (!cond) {
        fprintf(stderr, "FAIL %s\n", msg);
        failures++;
    }
}

int main(void)
{
    const factory_bank_t *chi = factory_bank(FACTORY_BANK_CHICAGO);
    const factory_bank_t *mia = factory_bank(FACTORY_BANK_MIAMI);
    user_bank_t user;
    factory_preset_t custom;
    const factory_preset_t *before;
    int slot;

    expect(factory_bank_count() == 2, "two factory brands");
    expect(chi && mia, "both banks present");
    expect(chi->npresets == FACTORY_CHICAGO_NPRESETS, "chicago count");
    expect(mia->npresets == FACTORY_MIAMI_NPRESETS, "miami count");
    expect(chi->npresets == 81, "chicago 81");
    expect(mia->npresets == 30, "miami 30");
    expect(factory_banks_intact(), "factory intact at boot");
    expect(factory_preset_name(FACTORY_BANK_CHICAGO, 0) != 0, "chicago name0");
    expect(factory_preset_name(FACTORY_BANK_MIAMI, 0) != 0, "miami name0");

    /* Snapshot first Chicago factory preset, then user-import a custom slot. */
    before = factory_preset(FACTORY_BANK_CHICAGO, 0);
    expect(before != 0, "chicago preset0");
    user_bank_clear(&user);
    memset(&custom, 0, sizeof custom);
    snprintf(custom.name, sizeof custom.name, "User Test");
    custom.nparams = FACTORY_CHICAGO_NPARAMS;
    custom.pots[0] = 11;
    custom.pots[1] = 22;
    slot = user_bank_import(&user, FACTORY_BANK_CHICAGO, &custom);
    expect(slot >= 0, "user import ok");
    expect(user_bank_used_count(&user) == 1, "one user slot");
    expect(factory_banks_intact(), "factory intact after user import");
    expect(memcmp(before, factory_preset(FACTORY_BANK_CHICAGO, 0), sizeof *before) == 0,
           "factory ROM unchanged by user import");
    expect(strcmp(factory_preset_name(FACTORY_BANK_CHICAGO, 0), before->name) == 0,
           "factory name unchanged");

    if (failures) {
        fprintf(stderr, "%d failure(s)\n", failures);
        return 1;
    }
    printf("PASS chicago=%u miami=%u user_slots=%d\n",
           (unsigned)chi->npresets, (unsigned)mia->npresets, user_bank_used_count(&user));
    return 0;
}
