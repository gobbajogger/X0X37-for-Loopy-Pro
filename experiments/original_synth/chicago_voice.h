/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef ORIGINAL_CHICAGO_ASSIGNED_VOICE_H
#define ORIGINAL_CHICAGO_ASSIGNED_VOICE_H
#include "chicago_program.h"
#include "chicago_mp4.h"
#include "miami_pitch.h"
#include "voice_gain.h"
#include "control_math.h"
/* Fixed selected44100 DOUBLE SYNC/XMOD + MP4 profile, assigned voice only.
 * No allocator, selector, global tuning, original cold constructor or full
 * instrument claim. All original resource limits remain with the owner. */
typedef struct {
 original_mp4_reader read;
 const void *context;
 const int32_t *pitch;
 float a0,e0;
 uint32_t ready;
} original_chicago_voice_tables;
/* Bind a proven immutable packed A/E provider and verified128-entry pitch
 * table once. Lifetime/immutability/provenance are explicit caller contracts,
 * not enforced by const alone. No table rewrite or table-sized RAM copy. */
int original_chicago_voice_tables_bind(original_chicago_voice_tables *out,
 original_mp4_reader reader,const void *context,const int32_t pitch[128]);
/* Only the closed selected Chicago pitch controls are retained here. The
 * unused Miami mode/modulation fields do not consume shared target RAM. */
typedef struct {
 float master_tune,unison_detune,bend_range,detune,portamento;
 int transpose,semitone[4];
 float fine_tune[4];
} original_chicago_voice_pitch_controls;
typedef struct {
 float level[4],output,pan;
 uint8_t curve,unison_flag,ms20;
} original_chicago_voice_gain_controls;
typedef struct {
 original_chicago_voice_pitch_controls pitch;
 original_chicago_voice_gain_controls gain;
 original_chicago_routing routing;
 float vcf[4],vca[4],mg_frequency[2],mg_base[2],mg_times[2];
 float cutoff,resonance,env_depth,keyboard_track,noise,pw_intensity,pw_width,xmod,vco_gain;
 float wave_width[4]; /* Derived once per immutable control snapshot. */
 float patch_cutoff,patch_gain,patch_slave;
 uint32_t revision,ready;
 uint8_t shape[4],mg_shape[2],mg_tempo[2];
} original_chicago_voice_controls;
/* Compile all22 public controls from initialized immutable program snapshot.
 * Outer ARP/FX/master endpoints remain explicit in routing; they are not
 * silently applied per voice. No getters/conversions in the audio sample path. */
int original_chicago_voice_compile(original_chicago_voice_controls *out,const original_chicago_program *program);
typedef struct {float phase,unipolar,bipolar,hold;uint32_t noise;uint8_t wrapped;} original_chicago_voice_mg;
typedef struct {
 uint32_t oscillator_phase[4],noise;
 original_ramp noise_gain;
 original_chicago_voice_mg mg[2];
 float last_pitch;
} original_chicago_voice_seed;
typedef struct {
 int32_t raw_note;
 unsigned velocity,unison_flag;
 float tuned_pitch,last_pitch,unison_coordinate;
 original_chicago_voice_mg mg[2];
} original_chicago_voice_note;
typedef struct {
 original_miami_pitch pitch;
 original_voice_gain gain;
 original_chicago_mp4 filter;
 original_prepared_oscillators prepared;
 original_vca vcf,vca;
 original_chicago_voice_mg mg[2];
 original_ramp noise_gain;
 uint32_t noise,counter,last_duration,control_updates,wave_preparations,raw_velocity;
 int32_t activation[4],raw_note;
 float velocity,keyboard_source,unison_coordinate,vcf_bipolar,vca_bipolar;
 float previous_vcf,previous_vca,previous_mg[2],mod_cutoff,mod_gain,mod_slave,filter_postgain;
 uint32_t pending_tail_duration;
 uint8_t pending_tail_kick,awaiting_restart,unison_flag,key_down,damper_deferred,active,ready;
} original_chicago_voice;
/* Explicit cold producer input avoids inventing native phase/Analog history.
 * All phase bits/noise/MG/anchor come from caller; filter/EG/gain init is owned
 * here. Full notes preserve oscillator phases/current gains and noise history.
 * Raw generated0..187 notes remain int32; do not clip them to127. The original
 * bidirectional selector can assign velocity0; preserve it through /127 and
 * do not reinterpret this producer call as an external MIDI NoteOff. Tuned pitch
 * and anchors accept finite values outside MIDI127; final table clamps0..120. */
int original_chicago_voice_init(original_chicago_voice *s,const original_chicago_voice_tables *t,const original_chicago_voice_seed *seed);
int original_chicago_voice_full_note(original_chicago_voice *s,const original_chicago_voice_controls *c,const original_chicago_voice_tables *t,const original_chicago_voice_note *note,float bpm);
int original_chicago_voice_partial_note(original_chicago_voice *s,const original_chicago_voice_controls *c,const original_chicago_voice_tables *t,int32_t raw_note,unsigned velocity,float tuned_pitch,unsigned mask,float bpm);
int original_chicago_voice_note_off(original_chicago_voice *s,const original_chicago_voice_controls *c);
int original_chicago_voice_pedal(original_chicago_voice *s,const original_chicago_voice_controls *c);
/* Actual state4 retirement and state5 pending fade controls; pending note
 * records/priority live in the allocator. Staging makes no helper entry. */
int original_chicago_voice_stop(original_chicago_voice *s);
int original_chicago_voice_defer_restart(original_chicago_voice *s);
/* One helper entry, not one wall-clock tick. init_kick is the originalw2:
 * forced full/partial note helper1, normal sample helper0. action1 retires.
 * action2 pauses BEFORE the native pending initializer and common tail.
 * The owner resolves the latest pending raw note/tuned pitch/MG input, calls
 * resume_restart(), then render_frame() for the SAME audio frame, without
 * another ordinary helper. No guessed pending producer or extra sample delay. */
int original_chicago_voice_control(original_chicago_voice *s,const original_chicago_voice_controls *c,const original_chicago_voice_tables *t,float bpm,unsigned init_kick,unsigned *action);
int original_chicago_voice_resume_restart(original_chicago_voice *s,const original_chicago_voice_controls *c,const original_chicago_voice_tables *t,const original_chicago_voice_note *note,float bpm);
/* Native pending submasks dispatch B200(full=1) once per selected bit in
 * increasing1,2,4,8 order. Every initializer resets filter/EG/stereo/MG/counter
 * but resets target/current pitch only for its one oscillator. The owner
 * supplies newly resolved global tuning/MG inputs at each dispatch, and sets
 * final_part only on the last call. No render/control call is allowed between
 * parts. Mask15 is the ordinary full pending restart, one final call. */
int original_chicago_voice_resume_part(original_chicago_voice *s,const original_chicago_voice_controls *c,const original_chicago_voice_tables *t,const original_chicago_voice_note *note,unsigned mask,unsigned final_part,float bpm);
/* Low render is used after an already-dispatched helper/restart. sample()
 * returns2 with action2 and untouched outputs when restart resolution is
 * required; otherwise returns1. It returns0 for invalid call preconditions. */
int original_chicago_voice_render_frame(original_chicago_voice *s,const original_chicago_voice_controls *c,float *left,float *right);
/* Internal serial block owner: admit stable resources once, then preserve
 * every original control tick. No external edits or restored-state writes
 * may interleave admitted calls. Checked public entries remain unchanged. */
int original_chicago_voice_block_ready(const original_chicago_voice *,const original_chicago_voice_controls *,const original_chicago_voice_tables *,float bpm);
int original_chicago_voice_control_admitted(original_chicago_voice *,const original_chicago_voice_controls *,const original_chicago_voice_tables *,float bpm,unsigned kick,unsigned *action);
int original_chicago_voice_sample(original_chicago_voice *s,const original_chicago_voice_controls *c,const original_chicago_voice_tables *t,float bpm,float *left,float *right,unsigned *action);
enum { ORIGINAL_CHICAGO_REQUEST_FULL=1, ORIGINAL_CHICAGO_REQUEST_PARTIAL=2, ORIGINAL_CHICAGO_REQUEST_STAGE=3 };
/* A088 decision tests full then VCAstate only, never the enabled lane bit.
 * Classify BEFORE resolving a global tuned pitch/MG snapshot: staging must
 * not consume those producers. Returns0 for invalid API state. */
unsigned original_chicago_voice_request_kind(const original_chicago_voice *s,unsigned full);
/* Owner stages/merges the raw descriptor first using the old VCA state, then
 * calls this exact VCA5 write. It accepts disabled voices and does not enable
 * a lane, change a key flag, consume tuning, or enter a helper. */
int original_chicago_voice_stage_raw(original_chicago_voice *s);
typedef struct {float velocity_unipolar,velocity_bipolar,key_unipolar,key_bipolar;} original_chicago_voice_sources;
/* Pure source writes at A12c..A19c; caller can publish all four source buffers.
 * The selected renderer retains velocity_unipolar/key_bipolar in its state. */
int original_chicago_voice_source_values(const original_chicago_voice_note *note,original_chicago_voice_sources *out);
/* Exact immediate A088/B200 contract. Full requires VCA6 but permits enabled1
 * and resets only masked targets/current/activation plus full common state.
 * Partial permits disabled/state6/state5, writes raw note/velocity/flag/coord,
 * preserves current pitches and MG phases, and enters the forced helper.
 * action2 is exposed before pending initialization/outer tail and outputs no
 * audio; owner resolves latest pending descriptor then uses resume APIs.
 * Partial ignores note.last_pitch and note.mg, because native partial does.
 * These are additional APIs; existing narrow calls retain their contracts. */
int original_chicago_voice_immediate_raw(original_chicago_voice *s,const original_chicago_voice_controls *c,const original_chicago_voice_tables *t,const original_chicago_voice_note *note,unsigned full,unsigned mask,float bpm,unsigned *action);
#endif
