/* SPDX-License-Identifier: GPL-3.0-only */
/* Compile probe only; this codec is deliberately absent from current project I/O. */
#include "app/synth_state.h"
int synth_probe_decode(const void *data, size_t n, synth_state_view_t *view)
{ return synth_state_decode(data,n,35052u,view); }
int synth_probe_migrate(const void *data, size_t n, synth_state_view_t *view)
{ return synth_state_migrate_legacy(data,n,35052u,view); }
int synth_probe_encode(void *data, size_t n, const synth_state_view_t *view, size_t *written)
{ return synth_state_encode(data,n,view,written); }
