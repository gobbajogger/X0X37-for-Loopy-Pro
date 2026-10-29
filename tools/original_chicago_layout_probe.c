/* SPDX-License-Identifier: GPL-3.0-only */
/* Host JSON or target object constants. The target probe is NOT linked into
 * the application and allocates no runtime engine state. */
#include "chicago_note_context.h"
#include "chicago_control_owner.h"
#include "chicago_fx2_bank.h"
#include "engine_events.h"
#include "miami_voice_snapshot.h"
#include "chicago_voice_snapshot.h"
#include "miami_startup.h"
#include "program_envelope.h"
#include "miami_geometry.h"
#include <stddef.h>
struct original_align_original_chicago_program{char before;original_chicago_program value;};
struct original_align_original_chicago_voice_controls{char before;original_chicago_voice_controls value;};
struct original_align_original_chicago_voice{char before;original_chicago_voice value;};
struct original_align_original_chicago_voice_tables{char before;original_chicago_voice_tables value;};
struct original_align_original_chicago_allocator{char before;original_chicago_allocator value;};
struct original_align_original_held_record{char before;original_held_record value;};
struct original_align_original_voice_record{char before;original_voice_record value;};
struct original_align_original_pending_note{char before;original_pending_note value;};
struct original_align_chicago_arp_control{char before;chicago_arp_control value;};
struct original_align_original_chicago_selector{char before;original_chicago_selector value;};
struct original_align_original_chicago_scheduler{char before;original_chicago_scheduler value;};
struct original_align_original_chicago_global_mg{char before;original_chicago_global_mg value;};
struct original_align_original_chicago_selected_global_stage{char before;original_chicago_selected_global_stage value;};
struct original_align_original_chicago_voice_owner{char before;original_chicago_voice_owner value;};
struct original_align_original_chicago_note_context{char before;original_chicago_note_context value;};
struct original_align_original_chicago_extra_sources{char before;original_chicago_extra_sources value;};
struct original_align_original_chicago_engine_output{char before;original_chicago_engine_output value;};
struct original_align_original_chicago_fx2_bank{char before;original_chicago_fx2_bank value;};
struct original_align_original_chicago_control_owner{char before;original_chicago_control_owner value;};
struct original_align_original_miami_program{char before;original_miami_program value;};
struct original_align_original_miami_voice_controls{char before;original_miami_voice_controls value;};
struct original_align_original_miami_selected_voice{char before;original_miami_selected_voice value;};
struct original_align_original_miami_voice_tables{char before;original_miami_voice_tables value;};
struct original_align_original_miami_voice_owner{char before;original_miami_voice_owner value;};
struct original_align_original_miami_note_context{char before;original_miami_note_context value;};
struct original_align_original_miami_extra_sources{char before;original_miami_extra_sources value;};
struct original_align_original_miami_voice_snapshot{char before;original_miami_voice_snapshot value;};
struct original_align_original_chicago_voice_snapshot{char before;original_chicago_voice_snapshot value;};
struct original_align_original_miami_control_owner{char before;original_miami_control_owner value;};
struct original_align_original_miami_output{char before;original_miami_output value;};
struct original_align_original_miami_fx_workspace{char before;original_miami_fx_workspace value;};
struct original_align_original_engine_event{char before;original_engine_event value;};
struct original_align_original_engine_events{char before;original_engine_events value;};
struct original_align_original_miami_startup_state{char before;original_miami_startup_state value;};
struct original_align_original_miami_startup_owner{char before;original_miami_startup_owner value;};
struct original_align_original_miami_geometry{char before;original_miami_geometry value;};
struct original_align_original_miami_geometry_progress{char before;original_miami_geometry_progress value;};
#ifdef ORIGINAL_CHICAGO_LAYOUT_OBJECT
#define LAYOUT(T) const uint32_t original_layout_##T[2]={sizeof(T),offsetof(struct original_align_##T,value)}
LAYOUT(original_chicago_program);LAYOUT(original_chicago_voice_controls);LAYOUT(original_chicago_voice);LAYOUT(original_chicago_voice_tables);LAYOUT(original_chicago_allocator);LAYOUT(original_held_record);LAYOUT(original_voice_record);LAYOUT(original_pending_note);LAYOUT(chicago_arp_control);LAYOUT(original_chicago_selector);LAYOUT(original_chicago_scheduler);LAYOUT(original_chicago_global_mg);LAYOUT(original_chicago_selected_global_stage);LAYOUT(original_chicago_voice_owner);LAYOUT(original_chicago_note_context);LAYOUT(original_chicago_extra_sources);LAYOUT(original_chicago_engine_output);LAYOUT(original_chicago_fx2_bank);LAYOUT(original_chicago_control_owner);
LAYOUT(original_miami_program);LAYOUT(original_miami_voice_controls);LAYOUT(original_miami_selected_voice);LAYOUT(original_miami_voice_tables);LAYOUT(original_miami_voice_owner);LAYOUT(original_miami_note_context);LAYOUT(original_miami_extra_sources);LAYOUT(original_miami_voice_snapshot);LAYOUT(original_miami_control_owner);LAYOUT(original_miami_output);LAYOUT(original_miami_fx_workspace);LAYOUT(original_engine_event);LAYOUT(original_engine_events);LAYOUT(original_miami_startup_state);LAYOUT(original_miami_startup_owner);
LAYOUT(original_chicago_voice_snapshot);LAYOUT(original_miami_geometry);LAYOUT(original_miami_geometry_progress);
const uint32_t original_layout_envelope_bytes[4]={ORIGINAL_MIAMI_PROGRAM_ENVELOPE_BYTES,ORIGINAL_CHICAGO_PROGRAM_ENVELOPE_BYTES,ORIGINAL_MIAMI_VOICE_SNAPSHOT_BYTES,ORIGINAL_CHICAGO_VOICE_SNAPSHOT_BYTES};
#else
#include <stdio.h>
#define ROW(T) printf("\"" #T "\":{\"bytes\":%zu,\"alignment\":%zu},",sizeof(T),offsetof(struct original_align_##T,value))
int main(void){putchar('{');ROW(original_chicago_program);ROW(original_chicago_voice_controls);ROW(original_chicago_voice);ROW(original_chicago_voice_tables);ROW(original_chicago_allocator);ROW(original_held_record);ROW(original_voice_record);ROW(original_pending_note);ROW(chicago_arp_control);ROW(original_chicago_selector);ROW(original_chicago_scheduler);ROW(original_chicago_global_mg);ROW(original_chicago_selected_global_stage);ROW(original_chicago_voice_owner);ROW(original_chicago_note_context);ROW(original_chicago_extra_sources);ROW(original_chicago_engine_output);ROW(original_chicago_fx2_bank);ROW(original_chicago_control_owner);ROW(original_miami_program);ROW(original_miami_voice_controls);ROW(original_miami_selected_voice);ROW(original_miami_voice_tables);ROW(original_miami_voice_owner);ROW(original_miami_note_context);ROW(original_miami_extra_sources);ROW(original_miami_voice_snapshot);ROW(original_chicago_voice_snapshot);ROW(original_miami_control_owner);ROW(original_miami_output);ROW(original_miami_fx_workspace);ROW(original_engine_event);ROW(original_engine_events);ROW(original_miami_startup_state);ROW(original_miami_startup_owner);ROW(original_miami_geometry);ROW(original_miami_geometry_progress);puts("\"host_only\":true}");return 0;}

#endif
