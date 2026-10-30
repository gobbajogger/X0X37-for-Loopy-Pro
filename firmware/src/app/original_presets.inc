/* SPDX-License-Identifier: GPL-3.0-only */
/* Textually included by engine.c: replacement-only patch publication. */
static int16_t bass_named_preset[NBASS] = {-1, -1};
static volatile uint8_t bass_preview_dirty, bass_wheel_mask, bass_edit_depth;
static volatile uint32_t bass_request[NBASS], bass_applied[NBASS];
static uint32_t bass_request_serial;
static volatile uint32_t bass_publish_seq[NBASS], bass_midi_mark[NBASS], bass_cq_mark[NBASS];
typedef struct { uint32_t token, midi, cq; } bass_request_t;
static const bass_identity_entry_t BASS_SOUND_REGISTRY[2] = {
    {ORIGINAL_SOUND_CHICAGO_ID, ORIGINAL_SOUND_SCHEMA, 22, 1},
    {ORIGINAL_SOUND_MIAMI_ID, ORIGINAL_SOUND_SCHEMA, 18, 1}
};

static int bass_request_snapshot(int voice, bass_request_t *r)
{
    uint32_t serial = bass_publish_seq[voice];
    if (serial & 1u) return 0;
    __asm__ volatile("" ::: "memory");
    r->token = bass_request[voice]; r->midi = bass_midi_mark[voice]; r->cq = bass_cq_mark[voice];
    __asm__ volatile("" ::: "memory");
    return serial == bass_publish_seq[voice];
}
static void bass_publish_request(int voice, uint32_t token)
{
    ++bass_publish_seq[voice];
    __asm__ volatile("" ::: "memory");
    bass_midi_mark[voice] = plat_midi_mark(); bass_cq_mark[voice] = cq_w;
    bass_request[voice] = token;
    __asm__ volatile("" ::: "memory");
    ++bass_publish_seq[voice];
}
static int bass_has_locks(int voice)
{
    if (mot.lane) for (int k = 0; k < NLANE; ++k)
        if (mot.lane[k].used && mot.lane[k].t == T_303 && mot.lane[k].v == voice) return 1;
    return 0;
}
static void bass_tag_set(sound_t *sound, int voice, int bank, int preset, int edited)
{
    bass_sound_info_t info = {{original_sound_engine_id((unsigned)bank), ORIGINAL_SOUND_SCHEMA,
        (uint8_t)original_sound_count((unsigned)bank), BASS_IDENTITY_KNOWN},
        (uint16_t)preset, (uint8_t)(edited ? BASS_SOUND_EDITED : 0)};
    (void)bass_sound_tag_encode(&sound->bass[voice], &info, BASS_SOUND_REGISTRY, 2);
}
static int bass_tagged_cell_validate(const uint8_t *cell, const bass_sound_tag_t *tag, int *bank)
{
    bass_sound_info_t info;
    if (!original_sound_valid(cell)) return -9;
    if (bass_sound_tag_decode(&info, tag, BASS_SOUND_REGISTRY, 2) != BASS_IDENTITY_READY) return -9;
    *bank = cell[29];
    if (info.identity.engine != original_sound_engine_id((unsigned)*bank) ||
        info.preset != cell[30] || info.flags != (original_sound_edit_mask(cell) ? BASS_SOUND_EDITED : 0)) return -9;
    return 0;
}
int engine_bass_sound_bank(const sound_t *sound, int voice)
{
    int bank;
    return sound && (unsigned)voice < NBASS &&
        !bass_tagged_cell_validate(sound->v[T_303][voice], &sound->bass[voice], &bank) ? bank : -1;
}
int engine_bass_sound_validate(const sound_t *sound)
{
    if (!sound) return -1;
    for (int v = 0; v < NBASS; ++v) if (engine_bass_sound_bank(sound, v) < 0) return -9;
    return 0;
}
int engine_bass_sound_adopt(sound_t *sound, int bank0, int bank1)
{ (void)sound; (void)bank0; (void)bank1; return -9; }
void engine_bass_sound_refresh(sound_t *sound, int v)
{
    if (!sound || (unsigned)v >= NBASS || !original_sound_valid(sound->v[T_303][v])) return;
    const uint8_t *cell = sound->v[T_303][v];
    bass_tag_set(sound, v, cell[29], cell[30], original_sound_edit_mask(cell) != 0);
}
static int bass_restore_part(const sound_t *sound, int v)
{
    if (engine_bass_sound_bank(sound, v) < 0) return 0;
    if (original_runtime_retire(&original_bass[v]) != ORIGINAL_RUNTIME_OK) return 0;
    int rc = original_runtime_init(&original_bass[v], sound->v[T_303][v], seq_tempo(&seq),
        &original_tuning, &original_fx_owner[v]);
    original_switch_gain[v] = 512u; original_quiet_samples[v] = 0;
    bass_named_preset[v] = sound->v[T_303][v][30];
    original_seq_note[v] = -1;
    __builtin_memset(original_key_pitch[v], 0, sizeof original_key_pitch[v]);
    bass_held[v] = bass_held_count[v] = 0;
    for (unsigned s = 0; s < 3; ++s) src_held[s] &= (uint16_t)~(1u << v);
    return rc;
}
static void bass_restore_sound(const sound_t *sound)
{
    for (int v = 0; v < NBASS; ++v) {
        live_key_context_retire(v, 0);
        (void)bass_restore_part(sound, v);
    }
}
int engine_bass_bank(int v) { return (unsigned)v < NBASS ? original_bass[v].kind : -1; }
int engine_bass_bank_available(int v, int bank)
{
    return (unsigned)v < NBASS && (bank == FACTORY_BANK_MIAMI ||
        (bank == FACTORY_BANK_CHICAGO && original_fx_ready));
}
const x0x_param_t *engine_bass_bank_param(int bank, int parameter)
{ return original_sound_param((unsigned)bank, (unsigned)parameter); }
int engine_bass_preview_changed(void) { return bass_preview_dirty; }
void engine_bass_wheel_owners(uint8_t mask) { bass_wheel_mask = mask; }
int engine_bass_wheel_active(void) { return bass_wheel_mask != 0; }
int engine_bass_preset_index(int v) { return (unsigned)v < NBASS ? bass_named_preset[v] : -1; }
int engine_bass_preset_edited(int v)
{
    return (unsigned)v >= NBASS || !mot_base_snd ||
        original_sound_edit_mask(mot_base_snd->v[T_303][v]) != 0;
}
uint32_t engine_bass_choice_serial(void) { return bass_request_serial; }
int engine_bass_preset_pending(int v)
{
    bass_request_t request;
    if ((unsigned)v >= NBASS) return 0;
    if (!bass_request_snapshot(v, &request)) return 6;
    if (!(request.token & 0x80000000u) || request.token == bass_applied[v]) return 0;
    if (!plat_midi_passed(request.midi) || (int32_t)(cq_r - request.cq) < 0) return 5;
    if (bass_has_locks(v)) return 4;
    if (bass_wheel_mask & (1u << v)) return 3;
    if (original_runtime_owned(&original_bass[v]) || original_quiet_samples[v] < 512u) return 2;
    return 1;
}
int engine_bass_requested_bank(int v)
{ bass_request_t r; return (unsigned)v < NBASS && bass_request_snapshot(v, &r) && engine_bass_preset_pending(v) ? (int)((r.token >> 7) & 1) : engine_bass_bank(v); }
int engine_bass_requested_preset(int v)
{ bass_request_t r; return (unsigned)v < NBASS && bass_request_snapshot(v, &r) && engine_bass_preset_pending(v) ? (int)(r.token & 127) : engine_bass_preset_index(v); }
void engine_bass_cancel_preset(int v)
{ if ((unsigned)v < NBASS) bass_publish_request(v, 0x40000000u | ((++bass_request_serial & 0x3fffffu) << 8)); }
void engine_bass_edit_hold(int hold)
{
    __asm__ volatile("" ::: "memory");
    if (hold) ++bass_edit_depth;
    else if (bass_edit_depth) --bass_edit_depth;
    __asm__ volatile("" ::: "memory");
}
int engine_bass_base_ready(unsigned mask, const bass_base_state_t *target)
{
    for (int v = 0; v < NBASS; ++v) if (mask & (1u << v)) {
        int bank;
        if (bass_tagged_cell_validate(target->cell[v], &target->tag[v], &bank)) return -5;
        if (!engine_bass_bank_available(v, bank)) return -2;
        if (bass_has_locks(v)) return -4;
        /* Typed history restore retains its existing admission contract. Only
         * factory requests own the measured fade/quiescence transition. */
        if ((bass_wheel_mask & (1u << v)) || original_runtime_active(&original_bass[v])) return 2;
    }
    return cq_r != cq_w || live_key_r != live_key_w || live_key_cancel ||
        !plat_midi_passed(plat_midi_mark()) ? 2 : 0;
}
void engine_bass_base_restore(unsigned mask)
{
    for (int v = 0; v < NBASS; ++v) if (mask & (1u << v)) {
        engine_bass_cancel_preset(v);
        live_key_context_retire(v, 0);
        (void)bass_restore_part(mot_base_snd, v);
        for (int i = 0; i < MX_NPARAMS; ++i) apply_base(T_MIX, PART_303A + v, i);
    }
}
int engine_bass_select_preset(int v, int bank, int preset, sound_t *sound)
{
    if ((unsigned)v >= NBASS || !sound || sound != mot_base_snd ||
        (unsigned)preset >= original_sound_factories((unsigned)bank)) return -1;
    if (!engine_bass_bank_available(v, bank)) return -2;
    if (bass_has_locks(v)) return -4;
    uint32_t token = 0x80000000u | ((++bass_request_serial & 0x3fffffu) << 8) |
        ((uint32_t)bank << 7) | (uint32_t)preset;
    bass_preview_dirty = 1;
    bass_publish_request(v, token);
    return 0;
}
static void bass_preset_boundary(void)
{
    if (bass_edit_depth) return;
    for (int v = 0; v < NBASS; ++v) {
        bass_request_t request;
        if (!bass_request_snapshot(v, &request)) continue;
        uint32_t token = request.token;
        if (!(token & 0x80000000u)) { bass_applied[v] = token; continue; }
        if (engine_bass_preset_pending(v) != 1) continue;
        int bank = (int)((token >> 7) & 1), preset = (int)(token & 127);
        uint8_t next[32];
        if (!engine_bass_bank_available(v, bank) || !original_sound_init(next, bank, preset)) continue;
        live_key_context_retire(v, 1);
        if (original_runtime_retire(&original_bass[v]) != ORIGINAL_RUNTIME_OK) {
            bass_applied[v] = token;
            continue;
        }
        int rc = original_runtime_init(&original_bass[v], next, seq_tempo(&seq),
            &original_tuning, &original_fx_owner[v]);
        if (rc != ORIGINAL_RUNTIME_OK) {
            /* A failed adoption remains visible and needs a fresh explicit
             * request; do not rerun expensive initialization every block. */
            bass_applied[v] = token;
            continue;
        }
        original_switch_gain[v] = 512u; original_quiet_samples[v] = 0;
        sound_t *sound = (sound_t *)mot_base_snd;
        __builtin_memcpy(sound->v[T_303][v], next, sizeof next);
        bass_tag_set(sound, v, bank, preset, 0);
        bass_named_preset[v] = (int16_t)preset;
        original_seq_note[v] = -1;
        (void)original_runtime_prepare(&original_bass[v], seq_tempo(&seq));
        __asm__ volatile("" ::: "memory");
        bass_applied[v] = token;
    }
}
