# Recorder endpoints: bounded stream 6 unit

This unit starts from firmware `d633833ac369f8466bc2563791614206070e87d1`.
It adapts the recorder endpoint and accepted-note generation algorithms from
`6496003d8b6d49de8a4f2502267b45dab0a3787a`; it does not replace the current
source tree, command-queue overflow handling, BLE paths, Chicago arp or DSP.

## Data contract

`bstep_t` remains three bytes. The upper flags nibble is the release position:
zero retains authored legacy gate behavior; 1 through 15 locate release at that
fraction of the current playback slot. A captured long note is one NOTE followed
by TIE cells. Incoming overlap places outgoing SLIDE on the actual source cell.
Recorded release has 1/15-slot resolution, plus observed audio-block/input batching
resolution; it is not a promise of raw USB or physical timestamp precision.

Stream 6 identifies this current, endpoint-only raw layout. Relevant local
recorder/schema/handoff/nightly refs were checked before allocation; none assigned
stream 6. Current main's stream 3 and historical frozen streams 4/5 are distinct.

- Stream 6 accepts the endpoint nibble on NOTE/TIE, never REST.
- Streams 2/3 retain their existing migration and require zero endpoint bits.
  The existing format-1 conversion remains supported under that same restriction.
- Ambiguous frozen stream 4, unsupported combined stream 5 and the reserved
  `MOT_SNAPSHOT` target 0xfe are refused, not stripped or guessed.
- Unsupported stored data blocks settings, sound and pattern writes, including
  boot-default fallback followed by autosave. Validated explicit staged reset or
  import is the recovery boundary. The UI states that loading/saving is blocked.
  Retain the original data and use its compatible firmware or raw object backup
  before an intentional reset; a RAM-default export is not that original data.
- Portable raw imports still require the exact current header and stage validation
  before adoption. Per-voice historical engine identity is not inferred here.

## One accepted lifecycle

Keyboard record presses carry audio and recording metadata in one critical queue
entry. Parameter floods cannot separate its sound from its recorder admission.
The accepted physical source, note and context generation determine legato;
reopened DSP gates do not. Source-owned release, REC-off, clear, selection, stop,
backend boundaries and disconnect finalize or invalidate the same capture.
Older or foreign releases cannot end a newer owner. Raw MIDI source 3 is refused;
it is reserved for the internal keyboard command path.

## Explicit remaining limit

PINGPONG and RANDOM can revisit a cell before the onset cell repeats. A single
cell endpoint cannot distinguish which visit should end the note. The actual
pre-guard regression released at 5,880/22,050 samples instead of the requested
28,000. This unit therefore refuses live record admission in those directions,
retains audition/transport, changes no pattern byte and displays
`REC NEEDS FWD/REV`. Exact per-occurrence recording remains **OPEN** and requires
a separate storage/design decision. This is not full recorder completion.
FWD/REV capture is periodic: holds beyond one complete traversal saturate one
traversal while preserving the onset; they do not invent an extra loop counter.

Full engine identity, whole-sound locks, combined stream-5 snapshot migration and
bass engine/preset UI remain separate integration obligations. No acoustic
percentage threshold, native fit, hardware pass, flash or publication follows
from host tests.

## Validation entrypoints

The normal test runner registers `rec-held-duration`, `rec-endpoint-lifecycle`,
`rec-endpoint-storage`, `rec-nonforward-guard` and `engine-cq-rec-overflow`.
The first exercises actual keybed REC, production engine/sequence replay, MIDI
release and nonzero PCM under internal and BLE-batched clocks. The reused
lifecycle suite drives actual MIDI queues and production sequencer replay for
release bins, legato, retrigger, route ownership, note zero, jitter, wrap, reverse,
REC-off/clear, undo/copy/save/load and canceled-gate reopening. The storage suite
uses real compression/NOR/object staging with synthetic bytes and observes zero
writes on refusal. The guard suite verifies no partial mutation and retained
normal audition/transport in the explicitly unavailable directions.

Independent review and exact Elite/Pro/controller native validation remain
required before qualification or integration.
