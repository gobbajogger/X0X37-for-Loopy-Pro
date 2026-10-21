/* SPDX-License-Identifier: GPL-3.0-only */
/* Explicit TEST-only app lifecycle gate. Zero initialization denies updates.
 * Safe recovery must opt in BEFORE controller entry; every controller attempt
 * permanently revokes permission for this boot, even if subsequently stopped.
 * Physical cold boot is still required: software cannot prove opaque RF state
 * survived no earlier warm reset. This is not an RF shutdown implementation. */
typedef struct { unsigned char recovery, attempted; } bp_update_gate;
static inline void bp_update_recovery(bp_update_gate *g) { g->recovery=1; }
static inline void bp_update_attempt(bp_update_gate *g) { g->attempted=1; }
static inline int bp_update_allowed(const bp_update_gate *g)
{ return g->recovery && !g->attempted; }
