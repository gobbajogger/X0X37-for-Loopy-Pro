/* Independently derived original Chicago control/clock projection. */
#ifndef ORIGINAL_CHICAGO_ARP_CONTROL_R3_H
#define ORIGINAL_CHICAGO_ARP_CONTROL_R3_H
#include <stdint.h>
/* Original15d0/17d0 arrays have128 uint32 velocity entries EACH. This is a
 * logical owned state, not a binary-compatible layout of the original object.
 * Selector scan/octave/direction/random state belongs to the caller and is not
 * reset by the1032byte aa90 array/count clear. */
typedef struct {
 uint32_t physical[128], latched[128];
 uint32_t physical_count, latched_count;
 uint32_t committed_enable, effective_latch;
 int32_t current_note;
} chicago_arp_control;
typedef struct { uint32_t enable, latch; uint8_t damper_byte; } chicago_arp_control_input;
typedef struct {
 /* aa90 first phase: return/remove active physical-held allocator records.
  * Called after committed_enable=0, before arrays/counts are cleared. */
 void (*retire_held_records)(void *);
 /* Original85c4 handoff. Disable emits every key0..127 in ascending order.
  * Latch-off can emit current GENERATED note, including values above127. */
 void (*note_off)(void *,int32_t note);
 /* aa90 remainder after all128 note_off calls: retire active voice records,
  * apply its VCA-state4 pass and perform aef0. Allocator internals are caller
  * owned. No guessed voice/held priority is implemented in this module. */
 void (*retire_voices_and_finish)(void *);
 /* Original raw current-note velocity lookup AFTER physical->latched copy.
  * Address was15d0+current_note*4.0..127 readphysical,128..255 readlatched;
  * other values require the caller's complete original-object bounds proof.
  * This callback must not clamp, reinterpret or substitute the selected key. */
 uint32_t (*current_velocity)(void *,int32_t current_note);
} chicago_arp_control_callbacks;
/* Block preparation54d94. Inputs enable/latch are converted Q2 values0/1.
 * Callbacks may observe state and must preserve their stated original ordering.
 * No control-input events, selector, MIDI delivery or allocator are supplied. */
void chicago_arp_control_prepare(chicago_arp_control *,const chicago_arp_control_input *,
                                const chicago_arp_control_callbacks *,void *);
typedef struct { float accumulator, threshold; uint32_t key_reset; } chicago_arp_clock;
typedef struct { float bpm; uint32_t tempo_sync; } chicago_arp_clock_input;
typedef struct {
 void (*step)(void *); /* Original8ff4 handoff; caller handles enable/selector. */
 void (*tempo_sync_notify)(void *); /* OriginalMG2 waveform/phase update. */
} chicago_arp_clock_callbacks;
typedef struct {
 /* Native108 low value with Hold0 calls98f4 thena954, in this order. */
 void (*cleanup_held)(void *);
 void (*cleanup_voices)(void *);
} chicago_arp_damper_callbacks;
/* Immediate native108 setter/internal event4 projection. Value is canonical
 * integer0..127, not normalized. Writes f5/inputdamper_byte=0below64 else127;
 * low+Hold0 runs both cleanup callbacks, then ALWAYS runscontrol_prepare.
 * This executes even if the previous f5 byte equals the new byte. Apply at the
 * resolved event point; it must not wait for a690 voice tick or nextblock.
 * This API supplies no physical CC mapping or allocator priority. */
void chicago_arp_damper_apply(chicago_arp_control *,chicago_arp_control_input *,
 uint32_t canonical_value,uint32_t committed_hold,const chicago_arp_damper_callbacks *,
 const chicago_arp_control_callbacks *,void *);
/* Recalculate threshold at block entry using ordered float32 products:
 * ((Fs*2.5)*convertedBase53)*convertedTimes54. Does not reset residual/flag. */
void chicago_arp_clock_prepare(chicago_arp_clock *,float fs,float base,float times);
/* Physical first-key notification. Caller supplies the original physical held
 * allocator count AFTER insertion, not an ARP-latched count. */
void chicago_arp_clock_key(chicago_arp_clock *,uint32_t key_sync,uint32_t physical_held_count);
/* Native51 is MG2KeySync; native52 is MG2TempoSync. */
/* Call after this frame's ordered queued input events and before voice/audio
 * preparation. Keyreset clearsresidual+flag and callsstep; then adds currentBPM.
 * A threshold crossing subtracts ONCE. Only when tempo_sync==1 does it then
 * notify followed by step; tempo_sync0 produces NEITHER callback on crossing.
 * The separate keyreset step still occurs regardless of tempo_sync.
 * Inputs are read after callbacks at their original use points. The caller must
 * supply strict float32 compilation (-ffp-contract=off; no fast-math). */
void chicago_arp_clock_tick(chicago_arp_clock *,const chicago_arp_clock_input *,
                           const chicago_arp_clock_callbacks *,void *);
#endif
