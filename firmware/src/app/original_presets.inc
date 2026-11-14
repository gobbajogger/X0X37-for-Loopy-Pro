/* SPDX-License-Identifier: GPL-3.0-only */
/* Textually included by engine.c: replacement-only patch publication. */
#include "bass_pattern_preset.h"
static int16_t bass_named_preset[NBASS] = {-1, -1};
static volatile uint8_t bass_preview_dirty, bass_wheel_mask, bass_edit_depth;
static volatile uint32_t bass_request[NBASS], bass_applied[NBASS];
static uint32_t bass_request_serial;
static volatile uint32_t bass_publish_seq[NBASS], bass_midi_mark[NBASS], bass_cq_mark[NBASS];
enum { BASS_REQUEST_FACTORY, BASS_REQUEST_UNAVAILABLE, BASS_REQUEST_INHERIT };
static volatile uint8_t bass_request_mode[NBASS], bass_unavailable_recall[NBASS];
static volatile uint32_t bass_pattern_epoch[NBASS];
typedef struct { uint32_t token, midi, cq; uint8_t mode, pat; } bass_request_t;
/* Per-pattern preset recall (owner 2026-11-12 02:42): a request may wait for
 * its pattern. 0xFF = no gate (every ordinary preset choice). */
static volatile uint8_t bass_gate_pat[NBASS] = {0xFF, 0xFF};
static void bass_preset_boundary_voice(int v);
static const bass_identity_entry_t BASS_SOUND_REGISTRY[4] = {
    {ORIGINAL_SOUND_CHICAGO_ID, ORIGINAL_SOUND_SCHEMA, 22, 1},
    {ORIGINAL_SOUND_MIAMI_ID, ORIGINAL_SOUND_SCHEMA, 18, 1},
    {ORIGINAL_SOUND_DRONE_ID, ORIGINAL_SOUND_SCHEMA, ORIGINAL_DRONE_NPARAMS, X0X_DRONE_RUNTIME_ENABLED},
    {ORIGINAL_SOUND_JUNO_ID, ORIGINAL_SOUND_SCHEMA, ORIGINAL_JUNO_NPARAMS, 1}
};

static int bass_request_snapshot(int voice, bass_request_t *r)
{
    uint32_t serial = bass_publish_seq[voice];
    if (serial & 1u) return 0;
    __asm__ volatile("" ::: "memory");
    r->token = bass_request[voice]; r->midi = bass_midi_mark[voice]; r->cq = bass_cq_mark[voice];
    r->mode = bass_request_mode[voice]; r->pat = bass_gate_pat[voice];
    __asm__ volatile("" ::: "memory");
    return serial == bass_publish_seq[voice];
}
static void bass_publish_request(int voice, uint32_t token, unsigned mode, unsigned pat)
{
    ++bass_publish_seq[voice];
    __asm__ volatile("" ::: "memory");
    bass_midi_mark[voice] = plat_midi_mark(); bass_cq_mark[voice] = cq_w;
    bass_request_mode[voice] = (uint8_t)mode; bass_gate_pat[voice] = (uint8_t)pat;
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
    (void)bass_sound_tag_encode(&sound->bass[voice], &info, BASS_SOUND_REGISTRY, 4);
}
static int bass_tagged_cell_validate(const uint8_t *cell, const bass_sound_tag_t *tag, int *bank)
{
    /* Archived sounds remain valid when their recognized backend is absent.
     * Live selection has a separate availability gate below. */
    return bass_sound_cell_validate(cell, tag, BASS_SOUND_REGISTRY, 4, bank);
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
#if X0X_BASS_NOTE_SOUND
/* Per-note bass sound lock (owner 2026-11-12 02:43, 03:12). Capture is ported
 * from 279df014c/76f9f88fd (deleted with bass303 in 18481a63, last tree c4ecd248),
 * adapted to the original Chicago/Miami cell. Playback is new: the locked cell
 * and part mixer are applied at that note's onset, before the key. Capture runs
 * only while NOTE LOCK is on (runtime, not saved); with no lock records the
 * onset path does one flag read. */
static volatile uint8_t note_lock_capture, note_lock_any, note_lock_applied[NBASS];
void engine_note_lock_capture(int on) { note_lock_capture = (uint8_t)(on != 0); }
int engine_note_lock_capture_on(void) { return note_lock_capture; }
int engine_bass_note_validate(const lane_t *l)
{
    int bank;
    if(l->used!=1 || l->t!=BASS_NOTE_TARGET || l->pat>=NPAT ||
       l->part<PART_303A || l->part>PART_303B || l->v>=NSTEPS ||
       l->i!=BASS_NOTE_VERSION || l->rsv)return -1;
    for(int i=BASS_NOTE_USED;i<NSTEPS;i++)if(l->val[i])return -1;
    int rc=bass_tagged_cell_validate(l->val,(const bass_sound_tag_t *)(l->val+BASS_NOTE_TAG),&bank);
    if(rc)return rc;
    for(int i=0;i<MX_NPARAMS;i++)if(l->val[BASS_NOTE_MIX+i]>engine_param(T_MIX,l->part,i)->max)return -1;
    return 0;
}
static int bass_note_capture(lane_t *lanes,int pat,int voice,int step,const sound_t *sound,int effective)
{
    if(!lanes || sound!=mot_base_snd || (unsigned)pat>=NPAT ||
       (unsigned)voice>=NBASS || (unsigned)step>=NSTEPS || engine_bass_sound_bank(sound,voice)<0 ||
       original_bass[voice].status!=ORIGINAL_RUNTIME_OK ||
       !original_runtime_kind_available(original_bass[voice].kind))return -1;
    int k=bass_note_find(lanes,pat,voice,step);
    if(k<0)for(int j=0;j<NLANE;j++)if(!lanes[j].used){k=j;break;}
    if(k<0)return -7;
    lane_t next={0};
    next.used=1;next.gen=(uint8_t)(lanes[k].gen+1);next.t=BASS_NOTE_TARGET;
    next.pat=(uint8_t)pat;next.part=(uint8_t)(PART_303A+voice);next.v=(uint8_t)step;next.i=BASS_NOTE_VERSION;
    __builtin_memcpy(next.val,sound->v[T_303][voice],NPARAMS_MAX);
    __builtin_memcpy(next.val+BASS_NOTE_MIX,sound->v[T_MIX][PART_303A+voice],MX_NPARAMS);
    if(effective) {
        /* Audio owns motion runtime. Capture what this accepted edge hears,
         * including an in-flight ramp, without rewriting the selected base. */
        for(int j=0;j<NLANE;j++) {
            const lane_t *l=&mot.lane[j];
            int value=motion_value(&mot,j);
            if(value<0 || l->part!=PART_303A+voice)continue;
            if(l->t==T_303 && l->v==voice && l->i<NPARAMS_MAX)
                (void)original_sound_set(next.val,l->i,(unsigned)value);
            if(l->t==T_MIX && l->v==PART_303A+voice && l->i<MX_NPARAMS)
                next.val[BASS_NOTE_MIX+l->i]=(uint8_t)value;
        }
    }
    {   /* the tag describes the captured cell (bass_tag_set, into the record) */
        const uint8_t *cell=next.val;
        bass_sound_info_t info = {{original_sound_engine_id((unsigned)cell[29]), ORIGINAL_SOUND_SCHEMA,
            (uint8_t)original_sound_count((unsigned)cell[29]), BASS_IDENTITY_KNOWN},
            (uint16_t)cell[30], (uint8_t)(original_sound_edit_mask(cell) ? BASS_SOUND_EDITED : 0)};
        if(bass_sound_tag_encode((bass_sound_tag_t *)(next.val+BASS_NOTE_TAG),&info,
                                BASS_SOUND_REGISTRY,4)!=BASS_IDENTITY_READY)return -1;
    }
    if(engine_bass_note_validate(&next))return -1;
    lanes[k].used=0;
    __asm__ volatile("" ::: "memory");
    __builtin_memcpy((uint8_t *)&lanes[k]+1,(const uint8_t *)&next+1,sizeof next-1);
    __asm__ volatile("" ::: "memory");
    lanes[k].used=1;
    return 0;
}
/* 0 captured, 1 NOTE LOCK is off (nothing stored), <0 refused (pool full / sound). */
int engine_bass_note_capture(lane_t *lanes,int pat,int voice,int step,const sound_t *sound)
{return note_lock_capture?bass_note_capture(lanes,pat,voice,step,sound,0):1;}
/* Sequencer calls these only for actual recorder writes, before changing notes.
 * A refusal affects storage only: the accepted live note still auditions. */
int seq_bass_sound_prepare(seq_t *s,int voice,int pat,int step)
{
    if(!note_lock_capture)return 1;
    if(s!=&seq || !mot.lane)return -1;
    return bass_note_capture(mot.lane,pat,voice,step,mot_base_snd,1);
}
void seq_bass_sound_clear(seq_t *s,int voice,int pat,int step)
{if(s==&seq && mot.lane)bass_note_clear(mot.lane,pat,voice,step);}
/* Main loop: whether any lock record exists (the onset path reads only this). */
void engine_note_lock_refresh(void)
{
    uint8_t any=0;
    if(mot.lane)for(int k=0;k<NLANE;k++)if(mot.lane[k].used && mot.lane[k].t==BASS_NOTE_TARGET){any=1;break;}
    note_lock_any=any;
}
static void note_lock_restore(int v)
{
    unsigned n=original_sound_count(original_bass[v].kind);
    note_lock_applied[v]=0;
    for(unsigned i=0;i<n;i++)if(!original_sound_is_performance(original_bass[v].kind,i))apply_base(T_303,v,(int)i);
    for(int i=0;i<MX_NPARAMS;i++)apply_base(T_MIX,PART_303A+v,i);
}
/* ISR, a bass onset (s_bon), before the key: the onset's locked sound, or the
 * knobs back after a locked onset. A lock for the other engine is skipped. */
static void note_lock_onset(int v)
{
    /* A preserved archive is valid storage, not an audible lock target. */
    if(original_bass[v].status!=ORIGINAL_RUNTIME_OK ||
       !original_runtime_kind_available(original_bass[v].kind))return;
    const lane_t *l=0;
    if(note_lock_any && mot.lane) {
        int k=bass_note_find(mot.lane,seq.ppat[TRK_BASS0+v],v,eng_step[TRK_BASS0+v]);
        if(k>=0 && mot.lane[k].val[29]==(uint8_t)original_bass[v].kind)l=&mot.lane[k];
    }
    if(!l) { if(note_lock_applied[v])note_lock_restore(v); return; }
    unsigned n=original_sound_count(original_bass[v].kind);
    for(unsigned i=0;i<n;i++) {
        float value;
        if(original_sound_is_performance(original_bass[v].kind,i))continue;
        if(original_sound_value(l->val,i,&value))(void)original_runtime_param(&original_bass[v],i,value);
    }
    for(int i=0;i<MX_NPARAMS;i++)apply_param(T_MIX,PART_303A+v,i,l->val[BASS_NOTE_MIX+i]);
    note_lock_applied[v]=1;
}
/* ISR, transport stop: no lock outlives playback. */
static void note_lock_stop(void)
{ for(int v=0;v<NBASS;v++)if(note_lock_applied[v])note_lock_restore(v); }
#endif
/* ISR, every sequenced bass onset (s_bon), before the key. */
static void bass_onset(int v)
{
    if (bass_gate_pat[v] < NPAT && !bass_edit_depth) bass_preset_boundary_voice(v);
#if X0X_BASS_NOTE_SOUND
    note_lock_onset(v);
#endif
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
    /* This is initialized project/history adoption. Cold init and explicit
     * panic remain separate APIs; neither may erase debt between replacements. */
    int rc=original_runtime_replace(&original_bass[v],sound->v[T_303][v],seq_tempo(&seq),
        &original_tuning,&original_fx_owner[v]);
    if(rc==ORIGINAL_RUNTIME_OK || rc==ORIGINAL_RUNTIME_UNAVAILABLE) {
        bass_unavailable_recall[v]=0;
        __asm__ volatile("" ::: "memory");
        bass_pattern_epoch[v]++;
    }
    original_switch_gain[v] = 512u;
    /* An unavailable placeholder owns no DSP or residual audio. It must be
     * possible to explicitly select an available engine without waiting for
     * a render-success quiet scan that this placeholder will never run. */
    original_quiet_samples[v] = rc == ORIGINAL_RUNTIME_UNAVAILABLE ? 512u : 0u;
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
        engine_bass_cancel_preset(v); /* Old-project requests cannot outlive adoption. */
        bass_unavailable_recall[v]=0;
        live_key_context_retire(v, 0);
        (void)bass_restore_part(sound, v);
    }
}
int engine_bass_bank(int v) { return (unsigned)v < NBASS ? original_bass[v].kind : -1; }
uint32_t engine_bass_pattern_epoch(int v)
{
    uint32_t epoch=(unsigned)v<NBASS?bass_pattern_epoch[v]:0;
    __asm__ volatile("" ::: "memory");
    return epoch;
}
int engine_bass_bank_available(int v, int bank)
{
    return (unsigned)v < NBASS && original_runtime_kind_available(bank) &&
        (bank != FACTORY_BANK_CHICAGO || original_fx_ready);
}
const x0x_param_t *engine_bass_bank_param(int bank, int parameter)
{ return original_sound_param((unsigned)bank, (unsigned)parameter); }
int engine_bass_preview_changed(void) { return bass_preview_dirty; }
void engine_bass_wheel_owners(uint8_t mask) { bass_wheel_mask = mask; }
int engine_bass_wheel_active(void) { return bass_wheel_mask != 0; }
int engine_bass_preset_index(int v) { return (unsigned)v < NBASS ? bass_named_preset[v] : -1; }
int engine_bass_preset_edited(int v)
{
    if ((unsigned)v < NBASS && bass_unavailable_recall[v]) return 0;
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
    /* An archived unavailable recall is a terminal silent pattern outcome,
     * not a new playable preset choice waiting for the old note forever. */
    if (request.mode != BASS_REQUEST_FACTORY) return 1;
    if (bass_has_locks(v)) return 4;
    if (bass_wheel_mask & (1u << v)) return 3;
    if (original_runtime_owned(&original_bass[v]) || original_quiet_samples[v] < 512u) return 2;
    return 1;
}
int engine_bass_requested_bank(int v)
{ bass_request_t r; return (unsigned)v < NBASS && bass_request_snapshot(v, &r) && engine_bass_preset_pending(v) ? (int)((r.token >> 7) & 3u) : engine_bass_bank(v); }
int engine_bass_requested_preset(int v)
{ bass_request_t r; return (unsigned)v < NBASS && bass_request_snapshot(v, &r) && engine_bass_preset_pending(v) ? (int)(r.token & 127) : engine_bass_preset_index(v); }
void engine_bass_cancel_preset(int v)
{ if ((unsigned)v < NBASS) bass_publish_request(v, 0x40000000u | ((++bass_request_serial & 0x3fffffu) << 8), BASS_REQUEST_FACTORY, 0xFFu); }
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
int engine_bass_select_preset_at(int v, int bank, int preset, sound_t *sound, int pat)
{
    if ((unsigned)v >= NBASS || !sound || sound != mot_base_snd ||
        (unsigned)preset >= original_sound_factories((unsigned)bank)) return -1;
    if (!engine_bass_bank_available(v, bank)) return -2;
    if (bass_has_locks(v)) return -4;
    uint32_t token = 0x80000000u | ((++bass_request_serial & 0x3fffffu) << 9) |
        ((uint32_t)bank << 7) | (uint32_t)preset;
    bass_preview_dirty = 1;
    bass_publish_request(v, token, BASS_REQUEST_FACTORY, (unsigned)pat < NPAT ? (unsigned)pat : 0xFFu);
    return 0;
}
int engine_bass_select_preset(int v, int bank, int preset, sound_t *sound)
{ return engine_bass_select_preset_at(v, bank, preset, sound, -1); }
/* Only a recognized archived pattern request can ask for silent unavailability.
 * The ordinary user-choice API continues to refuse disabled engines. */
int engine_bass_recall_unavailable_at(int v, int bank, int preset, sound_t *sound, int pat)
{
    if ((unsigned)v>=NBASS || !sound || sound!=mot_base_snd || (unsigned)pat>=NPAT || !seq.pat ||
        (unsigned)preset>=original_sound_factories((unsigned)bank) ||
        original_runtime_kind_available(bank) ||
        seq.pat[pat].rsv[v]!=bass_pattern_preset_encode((unsigned)bank,(unsigned)preset+1u))return -1;
    uint32_t token=0x80000000u|((++bass_request_serial&0x3fffffu)<<9)|
        ((uint32_t)bank<<7)|(uint32_t)preset;
    bass_preview_dirty=1;
    bass_publish_request(v,token,BASS_REQUEST_UNAVAILABLE,(unsigned)pat);
    return 0;
}
/* OFF means inherit the preserved base after an unavailable recall. A future
 * OFF cue is gated just like any other pattern request; drawing it never unmutes. */
int engine_bass_pattern_inherit(int v, sound_t *sound, int pat)
{
    if ((unsigned)v>=NBASS || !sound || sound!=mot_base_snd || (unsigned)pat>=NPAT ||
        !seq.pat || seq.pat[pat].rsv[v])return -1;
    if (!bass_unavailable_recall[v])return 0;
    int bank=engine_bass_sound_bank(sound,v);
    if (bank<0)return -1;
    unsigned preset=sound->v[T_303][v][30];
    uint32_t token=0x80000000u|((++bass_request_serial&0x3fffffu)<<9)|
        ((uint32_t)bank<<7)|preset;
    bass_preview_dirty=1;
    bass_publish_request(v,token,BASS_REQUEST_INHERIT,(unsigned)pat);
    return 0;
}
int engine_bass_preset_gate(int v)
{ bass_request_t r; return (unsigned)v < NBASS && bass_request_snapshot(v,&r) && r.pat < NPAT ? r.pat : -1; }

static void bass_finish_hard_replace(int v, int status, int preset)
{
    original_switch_gain[v]=512u;
    original_quiet_samples[v]=status==ORIGINAL_RUNTIME_UNAVAILABLE?512u:0u;
    bass_named_preset[v]=(int16_t)preset;
    original_seq_note[v]=-1;
    __builtin_memset(original_key_pitch[v],0,sizeof original_key_pitch[v]);
    bass_held[v]=bass_held_count[v]=bass_held_src[v]=0;
    bass_accent[v]=bass_slide[v]=0;
    for(unsigned s=0;s<3;++s)src_held[s]&=(uint16_t)~(1u<<v);
    perf_ctl[v][0]=64;perf_ctl[v][1]=0;mod_ctl[v]=-1;ext_touch[v]=0;
#if X0X_BASS_NOTE_SOUND
    note_lock_applied[v]=0;
#endif
}
static void bass_preset_boundary_voice(int v)
{
    {
        bass_request_t request;
        if (!bass_request_snapshot(v, &request)) return;
        uint32_t token = request.token;
        if (!(token & 0x80000000u)) { bass_applied[v] = token; return; }
        if (engine_bass_preset_pending(v) != 1) return;
        /* A pattern preset waits until its pattern plays (the bar switch). */
        if (request.pat < NPAT && seq.ppat[TRK_BASS0 + v] != request.pat) return;
        int bank = (int)((token >> 7) & 3u), preset = (int)(token & 127);
        uint8_t next[32];
        if (request.mode!=BASS_REQUEST_FACTORY) {
            /* Revalidate the actual pattern at adoption: an edit/cancel cannot
             * turn an old inspected cue into a new silent transition. */
            if (request.pat>=NPAT || !seq.pat) { bass_applied[v]=token;return; }
            if (request.mode==BASS_REQUEST_UNAVAILABLE) {
                if (seq.pat[request.pat].rsv[v]!=bass_pattern_preset_encode((unsigned)bank,(unsigned)preset+1u) ||
                    original_runtime_kind_available(bank) || !original_sound_init(next,bank,preset)) {
                    bass_applied[v]=token;return;
                }
            } else if (request.mode==BASS_REQUEST_INHERIT) {
                if (seq.pat[request.pat].rsv[v] || engine_bass_sound_bank(mot_base_snd,v)<0) {
                    bass_applied[v]=token;return;
                }
                __builtin_memcpy(next,mot_base_snd->v[T_303][v],sizeof next);
                preset=next[30];
            } else { bass_applied[v]=token;return; }
#if X0X_BASS_NOTE_SOUND
            if(note_lock_applied[v])note_lock_restore(v);
#endif
            live_key_context_retire(v,1);
            seq_rec_cancel(&seq,v);
            int rc=original_runtime_replace(&original_bass[v],next,seq_tempo(&seq),
                &original_tuning,&original_fx_owner[v]);
            bass_finish_hard_replace(v,rc,preset);
            bass_unavailable_recall[v]=(uint8_t)(request.mode==BASS_REQUEST_UNAVAILABLE && rc==ORIGINAL_RUNTIME_UNAVAILABLE);
            __asm__ volatile("" ::: "memory");
            bass_pattern_epoch[v]++;
            if(request.mode==BASS_REQUEST_INHERIT)
                for(int i=0;i<MX_NPARAMS;++i)apply_base(T_MIX,PART_303A+v,i);
            bass_applied[v]=token;
            return;
        }
        if (!engine_bass_bank_available(v, bank) || !original_sound_init(next, bank, preset)) return;
        live_key_context_retire(v, 1);
        int was_unavailable=original_bass[v].status==ORIGINAL_RUNTIME_UNAVAILABLE;
        if(was_unavailable)
            seq_rec_cancel(&seq,v);
        /* Debt can survive the first escape. Every later initialized adoption
         * must preserve it too, even when the current backend is available. */
        int rc=original_runtime_replace(&original_bass[v],next,seq_tempo(&seq),
            &original_tuning,&original_fx_owner[v]);
        if(was_unavailable)
            bass_finish_hard_replace(v,rc,preset);
        if (rc != ORIGINAL_RUNTIME_OK) {
            /* A failed adoption remains visible and needs a fresh explicit
             * request; do not rerun expensive initialization every block. */
            bass_applied[v] = token;
            return;
        }
        original_switch_gain[v] = 512u; original_quiet_samples[v] = 0;
        bass_unavailable_recall[v]=0;
        __asm__ volatile("" ::: "memory");
        bass_pattern_epoch[v]++;
        sound_t *sound = (sound_t *)mot_base_snd;
        __builtin_memcpy(sound->v[T_303][v], next, sizeof next);
        bass_tag_set(sound, v, bank, preset, 0);
        bass_named_preset[v] = (int16_t)preset;
        /* A committed bank/preset replacement, never merely a pending request.
         * parameter -1 identifies the bank; parameter -2 the factory index. */
        PARAM_TRACE(PARAM_PRESET, T_303, v, -1, bank);
        PARAM_TRACE(PARAM_PRESET, T_303, v, -2, preset);
        original_seq_note[v] = -1;
        (void)original_runtime_prepare(&original_bass[v], seq_tempo(&seq));
        __asm__ volatile("" ::: "memory");
        bass_applied[v] = token;
    }
}
static void bass_preset_boundary(void)
{
    if (bass_edit_depth) return;
    for (int v = 0; v < NBASS; ++v) bass_preset_boundary_voice(v);
}
/* A rest has no s_bon callback. Follow actual in-buffer pattern changes before
 * rendering the next stretch, including a REST at the new pattern's step0. */
static void bass_pattern_boundary(unsigned before_a, unsigned before_b)
{
    if(bass_edit_depth)return;
    if(before_a!=seq.ppat[TRK_BASS0])bass_preset_boundary_voice(0);
    if(before_b!=seq.ppat[TRK_BASS0+1])bass_preset_boundary_voice(1);
}

