/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef ORIGINAL_MIAMI_GEOMETRY_H
#define ORIGINAL_MIAMI_GEOMETRY_H
#include "chicago_allocator.h"
/* Explicit original committed568..578 geometry plus caller-owned3428,
 *3228velocity map and2e28chord pairs. No allocation, new project encoding,
 * constructor defaults, voice pool resize or normalized-parameter conversion. */
typedef struct {
 original_allocator_geometry value;
 uint32_t *by_key;
 original_allocator_chord *pairs;
 uint32_t chord_count;
} original_miami_geometry;
enum {ORIGINAL_MIAMI_GEOMETRY_READY=1,ORIGINAL_MIAMI_GEOMETRY_CONFIGURE,ORIGINAL_MIAMI_GEOMETRY_CHORD,ORIGINAL_MIAMI_GEOMETRY_MODE,ORIGINAL_MIAMI_GEOMETRY_COMPLETE,ORIGINAL_MIAMI_GEOMETRY_RESTORE};
typedef struct {uint32_t stage,completed_resets,learned_chord;int status;} original_miami_geometry_progress;
/* Called AFTER exact D5E0 list reset, at B0738's direct numeric reset point.
 * Must not perform D444/all-notes-off, clear key arrays, enter an audio/helper
 * clock, replace programs or reenter this serial owner. The existing concrete
 * callback is original_miami_note_context_geometry_reset. Resources remain
 * owned/immutable; callback0 records a partial failure,never false rollback. */
typedef int (*original_miami_geometry_reset)(void *context);
/* Actual BFD0: empty active head leaves data/geometry untouched. Otherwise
 * last duplicate wins (includingmarked andzero values),ascendingnonzero map,
 *unusedpairs(-128,0),then B1FF0(force1),D5E0,B0738 using OLD committed mode.
 * Raw converted88/89 inputs are clamped by the original geometry arithmetic.
 * Buffers have128cells each and do not overlap each other or owner resources. */
int original_miami_geometry_learn(original_miami_geometry *,original_chicago_allocator *,int32_t requested_polyphony,int32_t requested_unison,original_miami_geometry_reset,void *,original_miami_geometry_progress *);
/* Owned B1AF4..B1BF4 prepare subsequence: compare RAW requested88/89 against
 * committed geometry BEFORE clamping; ifchanged configure/listreset/core.
 * Then B2114 commits requested58mode. Transition3->0 learns only if held
 * records still exist at that point; learning forces one reset underOLD3,
 * then mode0 commits geometry and resets AGAIN. If the first geometry phase
 * already reset the held list, learning is skipped. No coalesced resets.
 * Same mode/controls are a no-op and do not certify a whole restored state;clamped129 vs committed128 resets again.
 * Existing shared caller invokes this before B0C34 ARP maintenance; it owns
 * preceding waveform/parameter work and later event/MG/voice scheduling.
 * No full mode0 sound renderer, input transport or shared-file adoption is
 * admitted by this value/lifecycle path. Geometry capacity must fit ALREADY
 * owned storage; failure never clamps polyphony to available resources.
 * Input/list/resource preflight failures leave state/progress unchanged.
 * Callback failure may follow real prior effects;progress records its stage.
 */
int original_miami_geometry_prepare(original_miami_geometry *,original_chicago_allocator *,int32_t requested_polyphony,int32_t requested_unison,unsigned requested_mode,original_miami_geometry_reset,void *,original_miami_geometry_progress *);
/* Actual C184 caller: nonnull128-byte native chord velocity map. Every byte
 * is sign-extended,including0x80..ff;nonzero bytes create ascendingpairs.
 * Always B1FF0(force1) underCURRENT committedmode,evenfor an all-zero map.
 * NULL returns0 without mutation/reset. This is a decoded engine interface,
 * not a guessed project-file format or automatic legacy dispatch. Input must
 * remain disjoint from destination/state until consumed. Shared restore7C030
 * and the other original wrapper consumers retain their surrounding ordering.
 */
int original_miami_geometry_restore_map(original_miami_geometry *,original_chicago_allocator *,const uint8_t velocities[128],int32_t requested_polyphony,int32_t requested_unison,original_miami_geometry_reset,void *,original_miami_geometry_progress *);
#endif
