/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef ORIGINAL_CHICAGO_ALLOCATOR_H
#define ORIGINAL_CHICAGO_ALLOCATOR_H
#include <stdint.h>
#include "chicago_arp_control.h"
#define ORIGINAL_ALLOCATOR_NONE ((int16_t)-1)
#define ORIGINAL_ALLOCATOR_KEYS 128u
/* Index-linked logical records, not the original object ABI. The original
 * physical pool has128 records and the configured voice pool has1..128.
 * Array index is the immutable original voice ID. No pointer is persisted. */
/* Physical8358 key0..127, canonical velocity0..127, marked0/1 and128 IDs
 * fit exactly; -1 is the original empty-note/nil sentinel. This lossless
 * representation does not compact the generated signed32 voice note. */
typedef struct {int8_t note;uint8_t velocity,marked;int8_t prev,next;} original_held_record;
typedef struct {int32_t note,velocity;uint32_t marked,unison_flag;float coordinate;int16_t prev,next;} original_voice_record;
typedef struct {int16_t head,tail;uint16_t count;} original_record_list;
typedef struct {
 original_held_record *held;original_voice_record *voice;
 chicago_arp_control *keys;
 original_record_list held_active,held_free,voice_active,voice_free;
 uint32_t storage_voices;
 /* Converted/committed original runtime values; a serial owner updates them.
  * Capacity574 is a preferred-ID bound, with actual lists built by aef0.
  * No hypothetical physical input range or new polyphony mode is inferred. */
 uint32_t capacity,unison_count,hold,legato,mode;
 uint32_t pedal_byte,mg1_key_sync,mg2_key_sync,mg1_key_reset,mg2_key_reset;
 uint32_t arp_mode,arp_range;
 int32_t scan_index,previous_index,octave;
 uint32_t direction;
 int32_t last_note,gate_duration,gate_remaining;
} original_chicago_allocator;
typedef struct {
 /* Read-only original memory observations, without user callbacks/mutation. */
 unsigned (*enabled)(void *,unsigned voice_id);
 unsigned (*vca_state)(void *,unsigned voice_id);
 /* -1 preserves the corresponding original field. Release writeskey0,
 * VCF3/VCA3; forced steal writeskey0,VCA4 while preservingVCF and laneenabled.
 * These writes do not immediately retire the voice or clear its active bit. */
 void (*set_voice_state)(void *,unsigned voice_id,int key,int vcf,int vca);
 /* Actual A088 owner: includes full/partial numeric init, pending state5,
  * sources, global tuned target, ordered MG copies and extra helper entries.
  * Input record is captured before callback; last_note is native15b4.
  * The callback is synchronous/total and owns actual voice resources. */
 void (*note)(void *,unsigned voice_id,const original_voice_record *,unsigned full,unsigned oscillator_mask,int32_t last_note);
} original_allocator_callbacks;
/* Binds caller storage and explicit shared keys. Does not guess a native cold
 * constructor or clear existing physical/latched arrays. Controls must already
 * have been recovered/restored. aef0_reset reproduces the observed list reset:
 * free configuredvoices +128held records, notes-1,flags1,current/last60,gate0.
 * It preserves voice DSP state/active bits, key arrays/counts and ARP flags. */
int original_chicago_allocator_bind(original_chicago_allocator *,chicago_arp_control *,
 original_held_record held[128],original_voice_record *voice,unsigned storage_voices,
 unsigned capacity,unsigned unison_count,unsigned mode,unsigned hold,unsigned legato);
/* After binding and before key_event the owner sets pedal_byte, both KeySync
 * controls and converted arp_mode/range from the same original snapshot.
 * mg1/2_key_reset and selector scan/octave/direction are shared logical runtime
 * outputs; caller binds them to its MG/selector state without inventing defaults.
 * Bind's zero-filled adapter fields are not a native startup snapshot. */
void original_chicago_allocator_reset_lists(original_chicago_allocator *);
/* 9dac: prefer minimum disabled ID below capacity fromfree; otherwise freehead.
 * If free is empty, steal activehead into free and retry. Caller valid lists
 * guarantee at leastone record. Does not add a capacity clamp to nativefallback. */
unsigned original_chicago_allocator_acquire(original_chicago_allocator *,const original_allocator_callbacks *,void *);
/* A818: return active record byVOICE ID, not note; if enabled/non6, key0/VCA4
 * while preservingVCF. Control's normal retirement has already state6/disabled
 * so no state4write. Foundreturns1, absent0, with exact tail append order.
 * State5 action2 restart does NOT call this: its installedactive record stays
 * owned through the direct57e2c/B200 pending continuation. */
int original_chicago_allocator_return_voice(original_chicago_allocator *,unsigned voice_id,const original_allocator_callbacks *,void *);
/* 8cd0 unison group. Raw signednote is retained: generatedARP notes above127
 * must not be clamped. Velocity is original canonical event value0..127. */
void original_chicago_allocator_spawn(original_chicago_allocator *,int32_t note,int32_t velocity,const original_allocator_callbacks *,void *);
/* 8e7c: negative match releases every active record; other values matchrawnote.
 * Pedal/Hold mark when ARP isoff; ARPon releases independent of those bytes. */
void original_chicago_allocator_release(original_chicago_allocator *,int32_t match,const original_allocator_callbacks *,void *);
/* 8a04 physical record operation. Nonzero velocity obtains FIFO free record or
 * recycles activehead when full; zero removes/marks every matching key.
 * Nonzero velocity requires physicalkey0..127/canonicalvelocity1..127;
 * generated rawnotes use spawn and never enter this physical producer. */
void original_chicago_allocator_physical(original_chicago_allocator *,int32_t key,int32_t velocity);
void original_chicago_allocator_cleanup_held(original_chicago_allocator *);
void original_chicago_allocator_cleanup_voices(original_chicago_allocator *,const original_allocator_callbacks *,void *);
/* aa90 tail, after caller's arrayclear and128 ascending NoteOff callbacks:
 * release markedrecords, mark every enabled/non6 voiceVCA4, then aef0 lists.
 * Use this for the existing R3 retire_voices_and_finish callback. */
void original_chicago_allocator_finish_disable(original_chicago_allocator *,const original_allocator_callbacks *,void *);
/* Complete observed aa90 composition. Does not change committedARP/latch;
 * callers such as54d94 make their own preceding committedflag assignment. */
void original_chicago_allocator_all_notes_off(original_chicago_allocator *,const original_allocator_callbacks *,void *);
/* aa48/internal event7: zeroonlybothvelocityarrays/counts and128NoteOffs.
 * No markedprecleanup, state4pass or listreset; distinct from event5/aa90. */
void original_chicago_allocator_clear_keys(original_chicago_allocator *,const original_allocator_callbacks *,void *);
/* 85c4 rawNoteOff supports0..255 explicitly: original15d0+note*4 aliases the
 * adjacent latchedarray for128..255. Other raw object bounds remain caller
 * dependent; no127clamp is inserted. Returns0beforemutation outofcontract. */
int original_chicago_allocator_note_off(original_chicago_allocator *,int32_t note,const original_allocator_callbacks *,void *);
/* 8358 physical event. Physical key is0..127; generatednotes use spawn().
 * velocity0 first calls NoteOff then continues the original physical/update
 * path, including its observable count transitions. No MIDI encoding inferred. */
int original_chicago_allocator_key_event(original_chicago_allocator *,int32_t key,int32_t velocity,const original_allocator_callbacks *,void *);
/* Actual55710 internaltype0 route: canonicalvelocity0 calls85c4 directly;
 * it NEVER enters8358's velocity0 continuation. Timestamp/hostqueue owner is
 * external. This interface does not claim a physical MIDI transport mapping. */
int original_chicago_allocator_canonical_note(original_chicago_allocator *,int32_t key,int32_t velocity,const original_allocator_callbacks *,void *);
/* Exact pending field projection at A088 entry. The voice backend owns this
 * descriptor and writes VCA5 after staging. No helperentry runs here. */
typedef struct {int32_t note,velocity;uint32_t unison_flag;float coordinate;uint32_t mask;int32_t oscillator_note[4];} original_pending_note;
void original_chicago_allocator_stage_pending(original_pending_note *,const original_voice_record *,unsigned mask,unsigned previous_vca_state);
typedef struct {int32_t polyphony,unison,groups,capacity,simd_groups;} original_allocator_geometry;
/* Pure56150/55b88 geometry arithmetic, not the shared startup/reset owner.
 * Inputs are already converted88/89, committed58mode and3428chordcount.
 * The owner applies changed geometry, invokes aef0 then original54898 reset,
 * and retains the exact surrounding restore/warmup/event ordering. */
void original_chicago_allocator_geometry(original_allocator_geometry *,int32_t polyphony,int32_t unison,int32_t mode,int32_t chord_count);
typedef struct {int32_t note,velocity;} original_allocator_chord;
/* 99b4 data projection: lastrecord for duplicate physical key wins, including
 * marked records, then ascendingnonzero velocities. Fillunusedpairs(-128,0).
 * Caller provides two buffers and follows with56150(force1)/54898 exactly;
 * this function does not edit startup/persistence or invent that reset. */
/* Empty physicalactive list returns0withoutchanging buffers/count, like99b4.
 * Nonempty returns1 and sets count; caller only invokes reset for that case. */
int original_chicago_allocator_chord_snapshot(const original_chicago_allocator *,uint32_t by_key[128],original_allocator_chord pairs[128],unsigned *count);
/* Source/host only. No allocator activation, storage adoption or Sol approval
 * follows from this API. Three review rounds are already used; pendingreview. */
#endif
