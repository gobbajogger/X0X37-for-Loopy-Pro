/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#include <stdint.h>
/* Built-in Chicago / Miami factory banks (private/dev).
 * Owner-paid personal-use packs; owner strips before public redistribution.
 * User-imported presets stay allowed in a separate writable bank. */

#define FACTORY_BANK_CHICAGO 0
#define FACTORY_BANK_MIAMI 1
#define FACTORY_BANK_COUNT 2

#define FACTORY_CHICAGO_NPARAMS 22
#define FACTORY_MIAMI_NPARAMS 18
#define FACTORY_CHICAGO_NPRESETS 55
#define FACTORY_MIAMI_NPRESETS 30
#define FACTORY_NAME_MAX 24
#define FACTORY_PARAMS_MAX 22

typedef struct {
    char name[FACTORY_NAME_MAX];
    uint8_t nparams;
    uint8_t pots[FACTORY_PARAMS_MAX]; /* 0..255 = AU 0..1 */
} factory_preset_t;

typedef struct {
    const char *brand;          /* "chicago" / "miami" */
    const char *label;          /* UI label */
    uint8_t bank_id;
    uint8_t nparams;
    uint16_t npresets;
    const factory_preset_t *presets; /* NULL in the original profile: use original_sound_init */
    const char *const *param_names;  /* NULL in the original profile: use original_sound_param */
} factory_bank_t;

/* Original profile names/counts describe the admitted program selection, without
 * duplicate rounded byte records. factory_preset() returns NULL in that profile;
 * callers load exact programs through bass_preset_decode/original_sound_init. */
const factory_bank_t *factory_bank(int bank_id);
int factory_bank_count(void);
const factory_preset_t *factory_preset(int bank_id, int index);
const char *factory_preset_name(int bank_id, int index);

/* Writable user bank beside factory. Import/save must not overwrite factory ROM. */
#define USER_BANK_SLOTS 64
typedef struct {
    uint8_t used;               /* 0 empty */
    uint8_t bank_id;            /* which brand schema the pots follow */
    factory_preset_t preset;
} user_preset_slot_t;

typedef struct {
    user_preset_slot_t slot[USER_BANK_SLOTS];
} user_bank_t;

void user_bank_clear(user_bank_t *ub);
int user_bank_import(user_bank_t *ub, uint8_t bank_id, const factory_preset_t *preset);
int user_bank_used_count(const user_bank_t *ub);
/* Returns 1 if every factory ROM entry is unchanged vs the compiled table. */
int factory_banks_intact(void);
