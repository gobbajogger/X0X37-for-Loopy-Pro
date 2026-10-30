/* SPDX-License-Identifier: GPL-3.0-only */
#include "factory_banks.h"
#include "original_sound.h"

static const factory_bank_t factory_banks[FACTORY_BANK_COUNT] = {
    { "chicago", "Chicago", FACTORY_BANK_CHICAGO, FACTORY_CHICAGO_NPARAMS,
      FACTORY_CHICAGO_NPRESETS, 0, 0 },
    { "miami", "Miami", FACTORY_BANK_MIAMI, FACTORY_MIAMI_NPARAMS,
      FACTORY_MIAMI_NPRESETS, 0, 0 }
};

const factory_bank_t *factory_bank(int bank_id)
{
    if (bank_id < 0 || bank_id >= FACTORY_BANK_COUNT) return 0;
    return &factory_banks[bank_id];
}

int factory_bank_count(void) { return FACTORY_BANK_COUNT; }

const factory_preset_t *factory_preset(int bank_id, int index)
{
    (void)bank_id; (void)index;
    return 0;
}

const char *factory_preset_name(int bank_id, int index)
{
    return bank_id < 0 || index < 0 ? 0 : original_sound_name((unsigned)bank_id, (unsigned)index);
}

void user_bank_clear(user_bank_t *ub)
{
    if (ub) {
        uint8_t *z = (uint8_t *)ub;
        uint32_t i;
        for (i = 0; i < sizeof *ub; i++)
            z[i] = 0;
    }
}

int user_bank_import(user_bank_t *ub, uint8_t bank_id, const factory_preset_t *preset)
{
    /* This API accepts the retired rounded-byte record, not an OS patch cell. */
    (void)ub; (void)bank_id; (void)preset;
    return -1;
}

int user_bank_used_count(const user_bank_t *ub)
{
    int i, n = 0;
    if (!ub) return 0;
    for (i = 0; i < USER_BANK_SLOTS; i++) if (ub->slot[i].used) n++;
    return n;
}

int factory_banks_intact(void)
{
    for (unsigned bank = 0; bank < FACTORY_BANK_COUNT; ++bank) {
        if (factory_banks[bank].npresets != original_sound_factories(bank) ||
            factory_banks[bank].nparams != original_sound_count(bank)) return 0;
        for (unsigned preset = 0; preset < original_sound_factories(bank); ++preset) {
            const char *name = original_sound_name(bank, preset);
            if (!name || !*name) return 0;
        }
    }
    return 1;
}
