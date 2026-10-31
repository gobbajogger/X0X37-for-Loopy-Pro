/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef ORIGINAL_MIAMI_SELECTED_VOICE_H
#define ORIGINAL_MIAMI_SELECTED_VOICE_H
#include "miami_program.h"
#include "miami_pitch.h"
#include "voice_gain.h"
#include "control_math.h"

/* Selected 30-program native 44100 context, one explicitly assigned voice.
 * No allocator, held-key list, global Analog/tuning generator, outer FX/mute,
 * sample-rate bridge or production engine activation is implied by this API.
 * Structs are caller-owned; only these functions may mutate live voice state.
 * A compiled control/table snapshot stays immutable until all readers finish. */
typedef struct {
 float a[129],b[129],r[129],cap[129];
 int32_t pitch[128];
} original_miami_voice_table_storage;
typedef struct {
 original_ms20_tables filter;
 const int32_t *pitch;
 uint32_t ready;
} original_miami_voice_tables;
/* Copies verified finite tables into an immutable owned snapshot. Tables must
 * be authentic 44100-derived inputs; finite validation is not provenance proof.
 * No references to mutable source arrays survive this call. */
int original_miami_voice_tables_init(original_miami_voice_tables *out,
 original_miami_voice_table_storage *storage,const original_ms20_tables *filter,
 const int32_t pitch[128]);
/* Target path: validate once, borrow qualified immutable flash arrays. Calling
 * this function explicitly asserts verified provenance, lifetime across all
 * voice readers, and exclusive immutable ownership of the underlying bytes.
 * A const pointer alone does not enforce that contract. Mutable/shared-writer
 * arrays must use the defensive owned copy above. No tables reside in voice
 * state and no table-sized RAM copy is needed for immutable target flash. */
int original_miami_voice_tables_bind_immutable(original_miami_voice_tables *out,
 const original_ms20_tables *filter,const int32_t pitch[128]);

typedef struct {
 original_miami_pitch_controls pitch;
 original_voice_gain_controls gain;
 float cutoff,resonance,env_depth,noise,pw_intensity,pw_width,xmod,vco_gain;
 /* Source/MI/MW are zero in this selected profile: widths change only with
  * the program. Compile once instead of repeating four checked conversions
  * at every voice start and 690 Hz control tick. */
 float wave_width[4];
 float patch_slave,patch_cutoff,patch_gain;
 float mg_base[2],mg_times[2];
 float bend_normalized;
 /* The enclosing engine consumes these endpoints: macro14 master and
  * macro10 Filter/Crush's two Decimator controls. Never apply master per voice. */
 float master_volume,decimator_normalized[2];
 uint32_t revision,ready;
 uint8_t shape[4],mg_shape[2],damper;
} original_miami_voice_controls;
/* Cold path only: compile after program snapshot/revision changes. The audio
 * path never calls program getters, scans109 parameters or reconverts macros.
 * All18 macros have explicit voice/outer endpoints in this immutable view. */
int original_miami_voice_compile(original_miami_voice_controls *out,
 const original_miami_program *program);

typedef struct {
 float phase,unipolar,bipolar,hold;
 uint32_t noise;
 uint8_t wrapped;
} original_miami_voice_mg;
typedef struct {
 original_miami_pitch pitch;
 original_voice_gain gain;
 original_miami_ms20 filter;
 original_prepared_oscillators prepared;
 original_vca vcf,vca;
 original_miami_voice_mg mg[2];
 /* Last immutable VPatch sources consumed, and their selected destinations.
  * Kept as state witnesses; no current-tick feedback is substituted. */
 float previous_vca,previous_mg1,mod_slave,mod_cutoff,mod_gain;
 float vcf_bipolar,vca_bipolar,velocity,unison_coordinate;
 uint32_t raw_velocity;
 uint32_t counter,last_duration,control_updates,wave_preparations;
 uint32_t pending_tail_duration;
 int32_t activation[4];
 uint8_t unison_flag,note_key,key_down,damper_deferred,active,ready,awaiting_restart;
} original_miami_selected_voice;
typedef struct {
 unsigned note_key,velocity,last_note;
 /* Native d8f0 target after global scale/Analog/tuning generation. This
  * unresolved producer is explicit: passing MIDI note asserts ET-only input. */
 float tuned_pitch;
 float unison_coordinate;
 unsigned unison_flag;
 /* Full-note copied head MG phase/hold/noise state, in native voice order.
  * Caller provides {-1,...} for first key or an ordered head snapshot. */
 original_miami_voice_mg mg[2];
} original_miami_voice_note;

int original_miami_selected_voice_init(original_miami_selected_voice *s,
 const original_miami_voice_tables *tables,unsigned voice_index,unsigned last_note);
/* Inactive MG reset matches native B0738: phase-1, cached outputs/hold0,
 * seed0x6d70, wrapped1. This is component initialization, not a replacement
 * for the complete global reset/allocator. First full note still requires
 * its explicit native ordered-head MG input and tuned-pitch producer. */
/* Full note requires inactive state. Reusing an audible tail requires the
 * outer pending-replacement policy; rejection leaves this voice unchanged.
 * Preserves all oscillator phases/current gains and filter noise/gain history.
 * Both note functions refresh MG outputs THEN invoke the extra helper entry. */
int original_miami_selected_voice_full_note(original_miami_selected_voice *s,
 const original_miami_voice_controls *c,const original_miami_voice_tables *t,
 const original_miami_voice_note *note,float bpm);
int original_miami_selected_voice_partial_note(original_miami_selected_voice *s,
 const original_miami_voice_controls *c,const original_miami_voice_tables *t,
 unsigned note_key,unsigned velocity,float tuned_pitch,unsigned oscillator_mask,float bpm);
/* Called for an owner-resolved final physical-key release, after held-key
 * priority resolution. Damper defers this release. Program change must call
 * pedal() before the next sample; this mirrors native setter event timing. */
int original_miami_selected_voice_note_off(original_miami_selected_voice *s,
 const original_miami_voice_controls *c);
int original_miami_selected_voice_pedal(original_miami_selected_voice *s,
 const original_miami_voice_controls *c);
/* Extra/event helper entry is public for an explicit host dispatcher. Normal
 * sample() always performs its own additional entry before audio processing.
 * Control includes native states4/5: action1 retires; action2 pauses before
 * the old preparation tail and requires the explicit owner/resume APIs below.
 * The assigned sample convenience cannot finish an unresolved action2; use
 * the voice owner and separate render entry for that complete lifecycle. */
int original_miami_selected_voice_control(original_miami_selected_voice *s,
 const original_miami_voice_controls *c,const original_miami_voice_tables *t,
 float bpm,unsigned *action);
/* Pre-FX stereo contribution, no outer master multiplier. Writes +0 when
 * inactive. No allocation, program conversion, table scan or coefficient
 * division per sample; prepared waveform coefficients are reused across gains.
 * Dependencies retain checked scalar APIs. Failed precondition is nonmutating. */
int original_miami_selected_voice_sample(original_miami_selected_voice *s,
 const original_miami_voice_controls *c,const original_miami_voice_tables *t,
 float bpm,float *left,float *right,unsigned *action);

/* Original C6A4 decision: full+VCA6 immediate, full+otherVCA stage5;
 * partial always immediate, regardless of enabled bit/VCA5. The assigned
 * convenience APIs above keep their narrower preconditions unchanged. */
#define ORIGINAL_MIAMI_REQUEST_FULL 1u
#define ORIGINAL_MIAMI_REQUEST_STAGE 2u
#define ORIGINAL_MIAMI_REQUEST_PARTIAL 3u
unsigned original_miami_selected_voice_request_kind(const original_miami_selected_voice *,unsigned full);
int original_miami_selected_voice_stage_raw(original_miami_selected_voice *);
int original_miami_selected_voice_stop_raw(original_miami_selected_voice *);
/* Actual C6A4 immediate numeric branch in selected physical0..127 domain.
 * Velocity0 is a raw initializer value, not a canonical NoteOff encoding.
 * The owner publishes all source words and resolves original tuning/MG before
 * entry; partial preserves MG phase/hold/noise and copies record flag/coordinate.
 * All masked current/target stores precede MG preparation. */
int original_miami_selected_voice_immediate_raw(original_miami_selected_voice *,const original_miami_voice_controls *,const original_miami_voice_tables *,const original_miami_voice_note *,unsigned full,unsigned mask,float bpm,unsigned *action);
/* B3CC4..B3ECC/D8F0: action2 pauses before the old local preparation tail.
 * Dispatch mask15 once, otherwise1/2/4/8 ascending using the preserved pending
 * per-oscillator notes. Each full part runs its genuine extra helper before
 * the final old tail. Never clear the pending descriptor/mask after dispatch.
 * Ordered-head MG and separate global helper exits remain explicit owner seams;
 * no sample or unrelated callback may intervene between these continuations. */
int original_miami_selected_voice_resume_part(original_miami_selected_voice *,const original_miami_voice_controls *,const original_miami_voice_tables *,const original_miami_voice_note *,unsigned mask,unsigned final_part,float bpm);
/* Normal native preparation visits all configured voice IDs first. A caller
 * can now render each lane without adding another690 control/helper entry. */
int original_miami_selected_voice_render_frame(original_miami_selected_voice *,const original_miami_voice_controls *,float *left,float *right);
/* Internal event-free backend seams. Validate each voice at block entry, then
 * retain the same voice readiness, immutable controls/tables and BPM until
 * block exit. Prevalidated calls require nonnull, disjoint output/action
 * pointers and exclusive serial ownership; they retain dynamic restart and
 * downstream DSP checks. Event/note APIs remain checked. */
int original_miami_selected_voice_block_valid(const original_miami_selected_voice *,const original_miami_voice_controls *,const original_miami_voice_tables *,float bpm);
int original_miami_selected_voice_control_prevalidated(original_miami_selected_voice *,const original_miami_voice_controls *,const original_miami_voice_tables *,float bpm,unsigned *action);
int original_miami_selected_voice_render_prevalidated(original_miami_selected_voice *,const original_miami_voice_controls *,float *left,float *right);
/* B0738 owned-state projection. Preserve oscillator phases and the separate
 * normalized velocity source (core42c8), while resetting current raw velocity
 * (voice20), local DSP/envelopes/MGs/cache/counter. Current pitches60,target0,
 * original readonly initialA bits3a98fbd4, noiseIndex+1/epsilon. Global/list/
 * controller reset and master preservation remain with their matching owner.
 * Constructor phase zeroing is separate; call init for a new value object. */
int original_miami_selected_voice_core_reset(original_miami_selected_voice *,const original_miami_voice_tables *,unsigned voice_index);
#endif
