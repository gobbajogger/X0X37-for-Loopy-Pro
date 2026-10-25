# Live 303 gate duration and legato capture

This repair starts from installed source `892bf60d912eca93597dcf03be25a9756540c949`.
The old live recorder wrote only a NOTE on press and discarded release. Manual
NOTE/TIE tests (including PR183) did not exercise or fix that path. This change
records real press/release edges through the production engine queue and uses
NOTE/TIE data plus a terminal endpoint. It does not reinitialize a voice.

## Timing and representability

Onsets retain the current nearest-step record mode. Releases intentionally use a
finer endpoint grid: nearest **1/15 of the actual swung time slot**, observed by
the same sequencer clock. This is an explicit extension to record policy, not
sub-step precision for the snapped onset. At120BPM straight16ths the endpoint bin
is8.333ms and rounding error is at most4.167ms plus observation delay; onset error
can still be half a step. Captured duration therefore includes onset snap error.
A release is not shifted by that onset snap. A press released before its snapped
onset becomes a minimum1/15-step event, so very short gestures have this additional
minimum-duration exception. Exact boundary releases attach endpoint15 to the
previous cell, rather than creating an extra1/15-step tail. The last slot duration
is frozen before changing pattern/rate so a new destination cannot rescale it.

MIDI does not carry a raw event timestamp through `plat_midi_in`. Edges are observed
in FIFO order at `engine_render` drain time. Normal native DMA halves are512frames
(11.61ms at44.1kHz), rendered in256frame chunks; this is observation granularity,
not a measured hardware latency guarantee. Queue backlog adds delay (64MIDI
packets drained per render call). Tests inject at named audio sample times and
exercise that production queue. Clock/transport packets ahead of a note are
consumed before its recording position is chosen, including a same-drain burst.
External clock jitter remains real; there is no claim of timing finer than the
received clock and endpoint grid. Physical keys add UI polling uncertainty: the
input is a sampled key bitmap, simultaneous scan changes have key-index order,
and original within-scan ordering cannot be reconstructed.

A held note writes TIEs along the actual sequencer traversal, including reverse
and wrapping from the last cell through cell zero. Continuations have endpoint0;
only the terminal cell has a finite endpoint. Once a hold revisits its source
cell, recording saturates at one traversal and preserves the originating NOTE.
Live audio continues until its real off. The periodic pattern has no loop-count
field and cannot encode a finite hold longer than one complete traversal; replay
repeats the saturated one-traversal event. Random/ping-pong revisits similarly
saturate when they revisit the source: their direction semantics remain existing
sequencer semantics, not a promise that a stochastic later pass repeats the
capture's timing. No step capacity or reserve was removed.

## One accepted live transition

The engine owns active pitch, input route and outstanding same-pitch press count.
It classifies an incoming transition once: a slide requires an actually open DSP
gate, the same source route, a held press and a different pitch. The recorder
consumes `SEQ_REC_LEGATO`; it does not infer a second independent overlap state.
It retains only the projection identity (pattern, source cell, current/previous
cell, slot and observed slot start). Manual LFO outgoing-slide remains a separate
explicit authored flag. Slide is written on the preceding recorded source cell,
including a TIE immediately before the destination, rather than on the new note.

Last press owns this monophonic line. Releasing an older different pitch or another
input route cannot end the newest note. Releasing the newest note stops it; older
still-held notes do not resume. Repeated same-pitch Note Ons are attacks, not
slides, and FIFO matching same-pitch offs count down outstanding presses. Counts
saturate at255; more than255 unmatched same-pitch presses are outside this bounded
input representation. MIDI pitch zero is a valid pitch. Sustain pedal(CC64) is
not supported by this engine and is not interpreted as an extended held note.

REC off, selecting another part/pattern, undo/copy/clear and STOP finalize the
original capture and release its voice. Outstanding physical membership remains
until its real off: a new press after context release attacks because the actual
gate is closed; an old same-pitch off cannot cut the new press prematurely.
Context gate-off is consumed before a queued successor press. Normal release
also flushes tracked MIDI output. Active recording reuses existing `fire_bass`
MIDI bookkeeping with audio skipped, preserving upcoming-step physical REC echo.
Outside recording, input MIDI is not newly echoed. Keyboard recorder commands
share the audio command queue; an epoch invalidates pending recorder edges before
bulk edits. The UI consumes an audio-side dirty latch. Brief project hold covers
recorder finalization, not bulk copying, and never resets DSP state.

## Storage and integration boundary

The upper nibble of the existing three-byte `bstep_t.flags` encodes endpoint1..15;
zero preserves all legacy half-step/TIE/SLIDE behavior. RAM layout, undo baseline,
pattern assignments and copy operations keep their original sizes. Stream4 names
the new semantics. Stream3 loads unchanged; stream2 expands note/flags to the
velocity-bearing layout as before, and format1 migration remains existing code.
Portable format3 imports remain accepted. Older firmware does not understand
stream4; downgrade requires an explicit conversion and must not be called a
lossless round trip. Manual gate replacement clears a stale hidden endpoint.

This isolated lane does not import Miami or sound-lock code. Sound-lock recovery
has independently proposed a different stream4 (`lane0xfe`,16parameters); these
are **conflicting revisions**, not compatible changes. A designated combined
integrator must allocate a new revision and migrate both before combining.
Future sound-lock integration must use accepted transition order through
`s_step`/`s_bon`/`s_boff` and its `synth_note`/`synth_off` dispatch: TIE step locks
before no-retrigger continuation; accepted onset with derived slide; actual
release without resetting voice; snapshot values retained through tail until idle
or next accepted note. Live MIDI currently calls bass303 directly in this source
and must be deliberately wired to those hooks by that integrator. Do not blindly
cherry-pick this lane into a separate engine writer's scope.

## Reproducible checks and limits

`tests/host/rec_live_test.c` injects timestamped Note On/Off messages into `min_q`,
runs actual `engine_render`/`midi_in`, asserts stored NOTE/TIE/endpoint data, and
replays it through production sequencer gate/slide callbacks. It also exercises
physical queued record events. The tests cover held audio beyond multiple old
half-step cutoffs; endpoint/data and one-onset replay; overlap versus detached and
same-pitch attacks; older/foreign-route offs; pitch0; wrap; reverse; >loop
saturation; swing; boundary±1sample; externalF8 jitter and same-drain clock bursts;
part selection and RECoff; STOP; same-pitch old-off debt after cancellation; queued
successor press after clear; real undo/redo, UI copy, pending-keyboard epoch,
stream4 save/load and actual legacy3 load. `project_303_stream_test` tests real
format2 conversion and legacy flags/velocities. `project_stage_test` exercises
current import semantic rejection with endpoint bits accepted but gate3 rejected.

Independent PCM review and exact native app budget evidence belong to the final
coordinator receipt, not to a host test count. The provisional808cold-init
attribute changes native helper inlining/callgraphs; host state/PCM equality does
not prove native CPU timing. No hardware flash, target stack highwater, device
recording latency or production promotion is claimed by these tests.
