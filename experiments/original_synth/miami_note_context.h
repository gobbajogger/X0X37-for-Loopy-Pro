/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef ORIGINAL_MIAMI_NOTE_CONTEXT_H
#define ORIGINAL_MIAMI_NOTE_CONTEXT_H
#include "miami_voice_owner.h"
#include "chicago_global_mg.h"
#include "chicago_scheduler.h"
typedef struct {
 original_miami_voice_owner *owner;
 original_chicago_global_mg *global;
 const original_chicago_global_mg_input *global_input;
 chicago_arp_clock *clock1,*clock2;
 uint8_t key_sync[2];
 int status;
 uint32_t helper_entries,head_seeds;
} original_miami_note_context;
/* Actual selected30/18 context: scale0/tonic0,44100Hz,both tempo-sync1,
 * waveform0..3,ARPdisabled. Values come from corresponding original snapshot;
 * binding preserves all global/voice/noise/clock words and counts. This does
 * not admit a changed secondary scale, hidden ARP enable or another host rate.
 * Shared global-MG numeric code is qualified against Miami itself (12288
 * original clock/notify/tail state rows), not inferred from Chicago's name. */
int original_miami_note_context_bind(original_miami_note_context *,original_miami_voice_owner *,original_chicago_global_mg *,const original_chicago_global_mg_input *,chicago_arp_clock *,chicago_arp_clock *,unsigned scale,unsigned tonic,unsigned key_sync1,unsigned key_sync2);
/* Explicit clocked context: same44100/Equal/tonic0/waveform0..3/both-sync1
 * numeric producers, with ARPoff/on and range1. KeySync0/1 is preserved.
 * Canonical arrays/counts, current/last0..127 and note modes0..3 are required.
 * Caller keeps range1, tempo flags and canonical ingress throughout this
 * serial lifetime; scheduler clock_input.tempo_sync must also be1. This does
 * not activate an ARP or change its state. Modes0/2 do not spawn at canonical
 * key/selector entry in the original; do not fabricate chord transposition.
 * Wider generated notes, tuning, free-sync and waveform4 remain separate. */
int original_miami_note_context_bind_clocked(original_miami_note_context *,original_miami_voice_owner *,original_chicago_global_mg *,const original_chicago_global_mg_input *,chicago_arp_clock *,chicago_arp_clock *,unsigned scale,unsigned tonic,unsigned key_sync1,unsigned key_sync2);
original_miami_note_producers original_miami_note_context_callbacks(original_miami_note_context *);
/* Supply these callbacks together with original_miami_voice_owner_callbacks
 * to the shared scheduler; context is the same Miami voice_owner. */
original_chicago_scheduler_callbacks original_miami_note_context_scheduler_callbacks(void);
/* B0A80/D444/B0738 owned engine projection: exact shared all-notes-off/list
 * phase, then reset each owned voice and pending fields, then global MG reset.
 * Preserve phase, velocity/source arrays, clocks' residuals/thresholds and
 * master/program/resources. The existing input owner separately clears its
 * native MIDI/controller/queue object (D444's unrepresented fd..468 region).
 * This is not the whole VST Reset; startup_reset orchestrates reset/restore/
 * warmup through its full owner. Diagnostic helper counts stay cumulative. */
int original_miami_note_context_core_reset(original_miami_note_context *);
/* Direct B0738 owned reset, after the geometry owner has performed D5E0.
 * Unlike D444/core_reset, this does NOT all-notes-off or clear key arrays.
 * Preserve clocks residual/threshold, phase, separate normalized source arrays,
 * master and program. Clears raw note/velocity/pending/EG/filter/MG projection.
 * Signature matches miami_geometry's checked reset callback. Resources must
 * be already admitted and immutable through this serial cold operation. */
int original_miami_note_context_geometry_reset(void *context);
#endif
