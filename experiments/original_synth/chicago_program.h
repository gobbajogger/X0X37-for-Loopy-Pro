/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef ORIGINAL_CHICAGO_PROGRAM_H
#define ORIGINAL_CHICAGO_PROGRAM_H
#include <stdint.h>
/* Engine-owned binary32 snapshot, not a project serialization format. The84
 * shared core fields and4 fixed FX fields live once in immutable metadata.
 * Keep legacy byte programs with their existing backend/interpretation. */
typedef struct {
 float macro[22],target[29];
 uint32_t revision;
} original_chicago_program;
typedef struct {
 float arp_base_note,arp_times,master_squared,bend_bipolar;
 uint8_t fx[2][4]; /* original type/switch/Edit1/Edit2 integers */
 uint8_t arp_range,arp_mode,arp_enable,damper,vca_eg_mode;
 uint8_t arp_tempo_sync,arp_key_sync,arp_latch;
} original_chicago_routing;
/* Initialized snapshots must be owned sequentially by the event executor.
 * Render/held-note users retain an immutable copy. No heap or global scratch.
 * Public edits accept finite normalized[0,1] without a UI-byte round trip. */
int original_chicago_program_init(original_chicago_program *p,unsigned factory);
int original_chicago_program_set(original_chicago_program *p,unsigned parameter,float normalized);
/* Core0..108 and FX1/FX2 109..116 only. The original five mapped IDs above120
 * are no-ops in this CMP4 route and deliberately have no invented state. */
int original_chicago_program_native(const original_chicago_program *p,unsigned native_id,float *normalized);
const char *original_chicago_program_name(unsigned factory);
/* Compile once per control snapshot, before rendering. This produces exact
 * original outer values, not an ARP scheduler or a complete voice/FX graph.
 * master_squared is native96 squared once; downstream must not square again.
 * Native105 is the per-tick bend source; do not sum a second raw MIDI path. */
int original_chicago_program_routing(const original_chicago_program *p,original_chicago_routing *out);
#endif
