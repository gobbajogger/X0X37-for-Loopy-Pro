/* SPDX-License-Identifier: GPL-3.0-only */
/* X0X audio engine: the five parts, the mixer, the send FX and the master, driven by
 * the sequencer. engine_render() runs in the audio ISR; everything else is called
 * from the main loop and reaches the ISR through the command queue. A project
 * replacement first holds the ISR, then rebuilds the engines from the main loop.
 *
 * Parameters are addressed as (target, voice, index) -> pot value, the same triple
 * the UI pages, the project file and the command queue use:
 *   T_909 / T_808   voice = track 0..10, or DR_KIT / D8_KIT (= 11) for the kit
 *   T_303           voice = 0 / 1 (303 A / B)
 *   T_BRK           voice 0: breaks.h params (the per-pattern ones live in the pattern)
 *   T_FX            voice 0: fxbus.h params (reverb, delay, master drive / comp)
 *   T_MIX           voice = part 0..4: Level, Rev send, Dly send, Pan, channel cutoff
 *                   (drums: sends on top of the per-voice sends; cutoff is on the bus,
 *                   before the master filter)
 *   T_MST           voice 0: master filter + limiter
 */
#pragma once
#include <stdint.h>
#include "../dsp/x0x_param.h"
#include "../seq/sequencer.h"
#include "../seq/motion.h"

enum { T_909, T_808, T_303, T_BRK, T_FX, T_MIX, T_MST, NTARGETS };
#define T_PROB NTARGETS                /* a lane's pseudo-target: a step's chance, 5..100 % (not in sound_t);
                                        * voice = part << 4 | drum track (0 on the 303s and the break) */
#define T_VEL (T_PROB + 1)             /* drum step velocity 1..127 (not in sound_t); same voice packing as T_PROB.
                                        * MOT_NONE / base 0 = legacy accent / accent_q7 law. 303s keep bstep.velocity. */
enum { PART_909, PART_808, PART_303A, PART_303B, PART_BRK, NPARTS };
/* T_MIX, per part: level, reverb send, delay send, pan (64 = centre),
 * channel cutoff (127 = open, before the master filter). */
enum { MX_LEVEL, MX_REV, MX_DLY, MX_PAN, MX_CUT, MX_NPARAMS };
#define NVOICES_MAX 12                 /* a drum machine: 11 tracks + the kit */
#define NPARAMS_MAX 32                 /* fixed sound cell, including original factory/edit metadata */

/* The replacement profile owns a new project/patch format. It does not admit
 * legacy synth identities or their converters. */
#undef X0X_BASS_PRESET_PREVIEW
#define X0X_BASS_PRESET_PREVIEW 1
#undef X0X_BASS_TAGGED_SOUND
#define X0X_BASS_TAGGED_SOUND 1
#undef X0X_BASS_NOTE_SOUND
#define X0X_BASS_NOTE_SOUND 0
/* Separate optional storage features for the legacy build profile. */
#ifndef X0X_BASS_PRESET_PREVIEW
#define X0X_BASS_PRESET_PREVIEW 0
#endif
#ifndef X0X_BASS_TAGGED_SOUND
#define X0X_BASS_TAGGED_SOUND 0
#endif
#ifndef X0X_BASS_NOTE_SOUND
#define X0X_BASS_NOTE_SOUND 0
#endif
#if X0X_BASS_NOTE_SOUND && !X0X_BASS_TAGGED_SOUND
#error Note sounds require tagged bass storage
#endif
#if X0X_BASS_TAGGED_SOUND && !X0X_BASS_PRESET_PREVIEW
#error Tagged bass unit requires the reviewed runtime preset unit
#endif
#if X0X_BASS_TAGGED_SOUND
#include "bass_identity.h"
#endif
#if X0X_BASS_PRESET_PREVIEW
#include "factory_banks.h"
const x0x_param_t *engine_bass_bank_param(int bank, int param);
int engine_bass_bank(int voice);
int engine_bass_bank_available(int voice, int bank);
int engine_bass_preset_index(int voice);
int engine_bass_preset_edited(int voice);
/* 1 ready, -1 original Chicago FX storage has not been admitted, -2 DSP fault.
 * This is not an idle flag; ready original owners render every audio frame. */
int engine_bass_status(int voice);
int engine_bass_requested_bank(int voice);
int engine_bass_requested_preset(int voice);
/* 0 applied; 1 next audio boundary; 2 held/release tail; 3 wheel; 4 sound locks; 5 old input; 6 publication. */
int engine_bass_preset_pending(int voice);
void engine_bass_cancel_preset(int voice);
/* Main-loop edit/undo snapshots defer preset application without muting audio. */
void engine_bass_edit_hold(int hold);
int engine_bass_preview_changed(void);
void engine_bass_wheel_owners(uint8_t mask);
#endif

/* Review-only runtime and separately guarded tagged-storage units; defaults OFF. */
#ifndef X0X_BASS_PRESET_PREVIEW
#define X0X_BASS_PRESET_PREVIEW 0
#endif
#ifndef X0X_BASS_TAGGED_SOUND
#define X0X_BASS_TAGGED_SOUND 0
#endif
#ifndef X0X_BASS_NOTE_SOUND
#define X0X_BASS_NOTE_SOUND 0
#endif
#if X0X_BASS_NOTE_SOUND && !X0X_BASS_TAGGED_SOUND
#error Note sounds require tagged bass storage
#endif
#if X0X_BASS_TAGGED_SOUND && !X0X_BASS_PRESET_PREVIEW
#error Tagged bass unit requires the reviewed runtime preset unit
#endif
#if X0X_BASS_TAGGED_SOUND
#include "bass_identity.h"
#endif
#if X0X_BASS_PRESET_PREVIEW
#include "factory_banks.h"
const x0x_param_t *engine_bass_bank_param(int bank, int param);
int engine_bass_bank(int voice);
int engine_bass_bank_available(int voice, int bank);
int engine_bass_preset_index(int voice);
int engine_bass_preset_edited(int voice);
int engine_bass_requested_bank(int voice);
int engine_bass_requested_preset(int voice);
/* 0 applied; 1 next audio boundary; 2 held/release tail; 3 wheel; 4 sound locks; 5 old input; 6 publication. */
int engine_bass_preset_pending(int voice);
void engine_bass_cancel_preset(int voice);
/* Main-loop edit/undo snapshots defer preset application without muting audio. */
void engine_bass_edit_hold(int hold);
int engine_bass_preview_changed(void);
void engine_bass_wheel_owners(uint8_t mask);
#endif

/* the sound of a project: every pot of every engine, mirrored here by the main loop */
typedef struct {
    uint8_t v[NTARGETS][NVOICES_MAX][NPARAMS_MAX];
#if X0X_BASS_TAGGED_SOUND
    bass_sound_tag_t bass[NBASS]; /* explicit LE bytes; never native ABI encoding */
#endif
} sound_t;
#if X0X_BASS_TAGGED_SOUND
int engine_bass_sound_bank(const sound_t *sound,int voice);
/* Compact undo candidate: no project-sized automatic scratch. */
typedef struct { uint8_t cell[NBASS][NPARAMS_MAX]; bass_sound_tag_t tag[NBASS]; } bass_base_state_t;
int engine_bass_base_ready(unsigned mask,const bass_base_state_t *target);
void engine_bass_base_restore(unsigned mask); /* caller holds audio; ready was 0 */
uint32_t engine_bass_choice_serial(void);

int engine_bass_sound_validate(const sound_t *sound);
int engine_bass_sound_adopt(sound_t *sound,int bank0,int bank1);
void engine_bass_sound_refresh(sound_t *sound,int voice);
#if X0X_BASS_NOTE_SOUND
#include "bass_note_sound.h"
int engine_bass_note_validate(const lane_t *lane);
int engine_bass_note_capture(lane_t *lanes,int pat,int voice,int step,const sound_t *sound);
#endif
#endif
#if X0X_BASS_PRESET_PREVIEW
int engine_bass_select_preset(int voice, int bank, int preset, sound_t *sound);
#endif

extern seq_t seq;                      /* the sequencer (its pattern pointer is the project's) */
/* render stages (the PERF screen): 909, 808, 303 A, 303 B, BREAK, FX, MASTER, SEQ */
enum { PR_909, PR_808, PR_303A, PR_303B, PR_BRK, PR_FX, PR_MST, PR_SEQ, ENG_PROF_N };
extern volatile uint32_t eng_prof[ENG_PROF_N], eng_prof_frames;
void engine_motion_enable(int on);                          /* off: lanes do not play (the PERF test) */
void engine_smc_solo(uint8_t mask);                         /* runtime SMC solo mask; 0 = all parts audible */
void engine_smc_solo_targets(uint8_t parts, uint16_t kit909, uint16_t kit808); /* union of parts and drum tracks */
void engine_fx_mute(uint8_t mask);                          /* runtime output gates, never edits intended pots or motion */
uint8_t engine_fx_muted(void);
uint8_t engine_tape_active(void); /* actual tape input/wet activity, presentation only */
void engine_smc_busfx(uint8_t down, const uint8_t effect[8]); /* SMC momentary buses; effect may be null */
void engine_motion_reset(void);                             /* lanes replaced (undo): knobs to their own values */
/* overload guard (engine.c): the platform reports each block's load after rendering it */
void engine_load(uint32_t pct, uint32_t frames);           /* ISR, after engine_render */
void engine_guard_enable(int on);                           /* off: full quality always (the PERF test) */
int engine_guard_active(void);
extern volatile uint32_t eng_guard_count;                   /* times it engaged (PERF screen) */
extern volatile uint8_t eng_step[NTRACKS];   /* playheads for the UI */
extern volatile uint16_t eng_peak[NPARTS];   /* part meters, Q15, decaying (UI) */
extern volatile uint8_t eng_hit[NKIT][NDRUM]; /* each drum track's last hit, 255 = full velocity (the UI lets it fall) */
#define SCOPE_N 128u                   /* power-of-two ring; 128 frees 256 B pool vs 256 */
extern int16_t eng_scope[SCOPE_N];     /* the last output samples, for the screen */
extern volatile uint32_t eng_scope_w;

void engine_init(pattern_t *patterns, song_t *song, lane_t *lanes, const sound_t *base);
void engine_render(int32_t *out_lr, uint32_t n);   /* ISR: n stereo frames, 24-bit in int32 */
/* Main loop project transaction. The render and main loop run on the same core; once held,
 * the ISR emits silence without reading project or sample memory.
 * Flush on the old channels before replacing the project, then engine_init + apply its sound.
 * Releasing an aborted transaction preserves the sequencer and queued parameter commands. */
void engine_project_hold(int hold);
void engine_project_release(void);              /* end a transaction; discard MIDI received while held */
int engine_project_notes(void);                 /* held or queued notes: cannot begin a transfer */
void engine_project_flush(void);                /* held only: end old output notes and transport */

/* parameter descriptors (UI) */
int engine_nvoices(int target);
int engine_nparams(int target, int voice);
const x0x_param_t *engine_param(int target, int voice, int i);
const char *engine_voice_name(int target, int voice);
int engine_bass_is_wobble(int voice); /* live WBL1 backend on 303 A/B voice */

/* main loop -> ISR */
void engine_set(int target, int voice, int i, int value);   /* queued; the sound_t mirror is the caller's */
void engine_set_base(int target, int voice, int i);         /* exact patch value, after a temporary edit */
#ifdef X0X_HOST
unsigned engine_original_keys(int voice, int latched);
uint32_t engine_original_value_bits(int voice, unsigned parameter);
#endif
void engine_out_loud(int on);          /* 1: the output at the codec's full scale (6 dB hotter than Felucca's) */
void engine_usb_full(int on);          /* 1: engine_render at full level, engine_volume the MASTER knob after */
void engine_volume(int32_t *out_lr, uint32_t n);   /* ISR, after engine_render (and the USB tap) */
void engine_apply_sound(const sound_t *s);                  /* queue every value (load, init) */
void engine_sound_defaults(sound_t *s);                     /* each engine's power-on values */
void engine_sound_centre_drum_pans(sound_t *s);             /* every drum voice's PAN (its last pot) to centre */
void engine_drum(int kit, int voice, float vel);            /* play now (keys) */
void engine_bass_on(int part, int note, int accent, int slide);
void engine_bass_on_velocity(int part, int note, int accent, int slide, int velocity);
void engine_bass_record(int part, int note, int velocity, int flags); /* accepted live REC press, audio-owned */
void engine_bass_off(int part);
void engine_bass_off_note(int part, int note); /* Chicago arp: release one held pitch; 0 note = all */
void engine_brk_bend(int value); /* transient -8192..8191, +/-12 semitones */
#ifdef X0X_HOST
int engine_brk_bend_value(void);
#endif
void engine_brk_live(int key, int down);
void engine_brk_edit_hold(int held);
uint32_t engine_brk_held(void); /* aggregate slice ownership, macros excluded */
int engine_brk_count(int bank); /* currently applied snapshot count; requested changes may wait for release */
void engine_brk_loops(void);                                /* (re)read the A / B loops of the current pattern */
void engine_brk_flush(void);                                /* publish a loop change that waited for the sounding voice */
void engine_brk_drop_range(const uint8_t *base, uint32_t nbytes);
int engine_brk_slice(void);                                 /* slice sounding now, -1 none (UI) */
void engine_brk_state(int *slice, int *bank, int *div, int *running);
int engine_brk_outline(int which, uint8_t *peaks, int n);   /* the loop's peaks for the screen (main loop) */
/* motion (seq/motion.h): lanes play into the engine, never into the sound_t mirror */
int engine_motion_part(int target, int voice);              /* the part a parameter's lane follows */
int engine_motion_value(int lane);                          /* what the lane plays now, -1 none */
void engine_motion_rec(int lane);                           /* record a pass from the next step */
void engine_motion_hold(int lane);                          /* the knob wins for a pass */
void engine_motion_clear(int lane);                         /* free a motion lane (undo / bulk edit) */
float engine_gr_db(void);                                   /* master compressor gain reduction (UI meter) */
int engine_master_get(int i);                             /* read-only applied master pot */
void engine_master_format(int i, char *buf);                /* a master pot in its unit */
int engine_brk_nslots(void);                                /* built-in loops + user slots */
const char *const *engine_brk_slot_names(void);

/* Physical recording edges use the existing audio command queue. */

/* Physical eligible Free Play only; velocity zero releases the captured token.
 * state arp=-1 means serial-exhausted admission: fail closed until cold boot. */
void engine_live_key_state(int voice,uint32_t *generation,int *arp,uint32_t *context);
void engine_live_key_cancel(int voice);
int engine_live_key(int voice,uint32_t generation,int token,int pitch,int velocity,int accent,int slide);

#ifdef X0X_HOST
#endif
