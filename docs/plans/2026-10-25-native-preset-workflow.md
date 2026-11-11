# Native presets, pattern sounds and step overrides — design only

Request: LX-REQ-033. Related: LX-REQ-032 (alternate engine), LX-REQ-001–004
(project lifecycle), LX-REQ-008/009 (notes/probability), LX-REQ-035 (cutoff ownership).
Date: 2026-10-25. Status: proposed, unimplemented; independent design review attached locally.

## Recommendation and smallest decision

Build a **303-only native preset browser and pattern sound assignment first**.
Offer four small original sounds after listening review, plus capture of the current
303 sound. Preserve PRESETS as the pattern selector. Make per-step sound selection
a separate second increment: same engine, ordinary NOTE attacks only. Existing
parameter locks already solve per-step variation of a few controls and remain the
best choice for that job. Preset assignment is useful when a whole timbre needs to
travel with a bass line; it must not replace parameter locks.

The smallest owner decision is whether the bounded first version is acceptable:
**two existing 303 parts, eight distinct project snapshots shared across 32 patterns,
with a later maximum of 16 explicit step overrides per project**. Eight is a proposed
capacity, not a measured musical requirement. If rejected, choose a larger capacity
before storage work; do not silently evict sounds. This decision does not authorize
firmware integration. A stable accepted source and a recovered resource budget are
implementation prerequisites. KORG import and an alternate engine are independent
questions and do not block an original native bank.

## Inspected evidence and boundaries

Read-only local source: `662ce39e43096fea1be159247ad46465034a0b28`, root tree
`fef05652b73d8e78ba25ab755e93b4531abdd5cc`, isolated clone of task-78.
This is PR189's inspected cutoff repair snapshot, not a claim that PR189 or PR188
is accepted NIGHTLY. No edits to either active branch. The live PR188 metadata read
returned draft head `4baf1160f69d8bbc55ec36ca2dae22c22856f3f4`, based on
`08d3164a265123cb4c84e5104642fd93257d63d7`; its reported native budgets below
belong to that candidate, not to this docs branch. Rebase the design onto the
integrator's chosen accepted source before implementation.

The authoritative request file was read through the GitHub connector:
`coordination/requests/requests.json` on `coordination/merge-queue`, blob
`a554aaa3d03f5325183a67ff576286439832c9f6`, generated 2026-10-25T10:29:45Z.
LX-REQ-033 explicitly requests design/compatibility/rights assessment; LX-REQ-032
reports research only, engine not implemented, supported reference-host permission
recovery unfinished. LX-REQ-035 reports 96 production cases for cutoff ownership,
with native/hardware still pending. LX-REQ-004 remains combined-validation pending.
No tracker edit or claim of finishing those requests is made here.

Prior task-72 `miami-feasibility.txt` and `layout_probe.txt` were read. They inspect
PR168 `65a0588ed636e8db6b35bdea4991e8990f476ff2`: two 303s, raw projects,
engine-untagged motion, and XIP overflow of 108 bytes. Current Miami is not an
embedded implementation. The older overrun is not the newer PR188 budget.
No vendor binaries, preset banks, parameters, samples or private reference results
are included. No native feature build, GUI, device, flash, merge or publication.

## What exists already

| Source at the inspected SHA | Implemented behavior and consequence |
| --- | --- |
| `firmware/src/app/engine.h:22–34,74–90` | `sound_t.v[7][12][16]` stores project-wide pot values; T_303 has voices A/B. Defaults and apply-sound exist. This is saved sound state, not a named preset bank or per-pattern bass sound. |
| `firmware/src/app/ui.c:2926–2937`; `docs/MANUAL.md:122,437–439` | PRESETS cues the current part's pattern on its screens, all parts elsewhere. Reassigning it to sounds would break navigation. |
| `firmware/src/app/ui.c:924–945,1929–2020` | EDIT already has named 303 pages and hold-step + knob p-locks; SAVE + knob clears locks. Existing step preview/cancel owns its original part. |
| `firmware/src/seq/motion.h:28–35`; `motion.c:18–109,137–150` | 160 shared motion lanes, 64 values each; 0xFF means base, ramps interpolate adjacent values. Live recording/hold can win over automation. This is already per-step parameter variation. |
| `firmware/src/app/engine.c:438–458` | Motion runs in the step callback before notes; `mot_base()` reads the project sound. All restoration needs to resolve the new base consistently. |
| `firmware/src/seq/pattern.h:27–56,80–100` | A bass step has note, gate/accent/slide flags and velocity. Patterns contain two bass parts; no bass preset/engine field. BREAK already has per-pattern settings/slots; do not duplicate or generalize it accidentally. |
| `firmware/src/app/project.c:21–33,280–348,535–583,593–718` | Live and staged projects, flash stream3 with stream1/2 migrations, stage validation and raw export1. Exact sizes/versions matter. |
| `firmware/src/app/ui.c:1207–1253,1375–1465,4681–4746` | Part copy includes lanes; undo uses byte differences and project_stage; project replacement uses exclusive hold/flush/commit. Reuse these semantics. |

Scoped search of firmware, maintained manual and behavior found no native named
sound bank/load-save browser. SMC's 'preset' means its controller protocol; storage's
comment about Felucca presets names an unused flash area; neither is a sound bank.
Felucca's instrument preset protocol is explicitly not answered (`editor.c:8`).
No account-wide absence claim is implied.

## Proposed schema and use of existing fields

Keep T_303, its 16 control indices, voice IDs, mixer, MIDI channels and `sound_t`
unchanged. Do not hide new state in unused sound cells: imports currently preserve
them as opaque (`project.c:592–595`). Do not use note, velocity, gate/accent/slide
bits or generator seed as a sound ID. Do not append a parameter target casually:
T_PROB is NTARGETS=7, while command type8 is C_DRUM (`engine.h:22–24`,
`engine.c:133–145`). Keep probability separate from preset handles.

Proposed logical namespace: engine 1 = existing native303; engine 0 = invalid/none;
schema1 = the exact existing 16-pot descriptor order/ranges. Any future original
wobble engine gets a different engine ID and its own schema. IDs never depend on
UI order, names or a KORG program number. A registry allocates stable 32-bit preset
IDs; 0 means a custom captured snapshot, and other IDs are never reused for changed
content. Renaming/reordering a bank does not change identity. A source preset ID
is provenance/label lookup; **embedded parameter bytes are authoritative** after
assignment. An update to a factory bank cannot change an old project.

Explicit little-endian wire record (24 bytes for schema1):

```
engine_id:u16 | parameter_schema:u16 | preset_id:u32 | pots[16]:u8
```

Firmware must not deserialize this by casting bytes to a C struct. Registry/schema
descriptors supply count, units, ranges, enumeration and setters; validate all 16
values before publication to audio. A future schema with a different count uses a
new length-delimited record version, never truncation or zero-padding by guesswork.

For minimal RAM, propose two compact handles in `pattern_t.rsv[0]` and `[1]`, one
per bass part: 0 means **Project sound / inherit legacy**, 1–8 selects an embedded
snapshot. `[2]` stays reserved and must be zero in the new format. This avoids 64
new default-handle bytes. These bytes have no current sound semantics, but old
stream2 migration copies them: **all old stream1/2/3 and raw export1 migrations
must explicitly zero them**, even if nonzero garbage was previously tolerated.
Copy/clear must address the correct part's handle separately from `bpart_t`.

Append one preset sidecar after `arr`, keeping old offsets intact:

```
header8: version:u16, slot_count:u8, override_count:u8, reserved:u32
slots[8]: 24-byte snapshot records                     # 192 bytes
step_override[16]: {pattern:u8, bass_part:u8, step:u8, slot:u8} # 64 bytes
```

Total is **264 bytes per project**, no names or pointers. First increment may omit
the 64-byte override table (200 bytes), with its own version/capacity contract.
Unallocated records/unused bytes are canonical zero; override entries are sorted
and unique by `(pattern,bass_part,step)`, slot1–8 only. No entry means inherit the
pattern default, including handle0. Empty/unknown/mismatched slots are invalid.
Eight snapshot slots and 16 overrides are project-wide caps, not per-pattern.

Deduplicate exact `(engine,schema,preset_id,pots)` records. Editing a native sound
makes a custom snapshot (preset_id0), with display name `Custom 01` etc.; arbitrary
user naming is deferred rather than consuming unbudgeted RAM. A custom name table
can later be optional wire metadata if preservation/UI memory are designed.
Never mutate a shared snapshot in place. Capture/edit uses copy-on-write and changes
only the selected assignment. If no free/unreferenced slot or override exists,
show SOUND MEMORY FULL / STEP SOUND MEMORY FULL and make no change.
Slot reuse/compaction, table bytes and every changed reference belong to the same
undo gesture. Undo restores the old table before audio resolves restored handles.
Do not keep raw slot pointers in ISR/previews; copy validated pots and use generation
tokens. Reclamation must consider every live default and override, including entries
outside the current shortened pattern length. Avoid routine global compaction.

Bound-pattern knob edits must not silently change global `proj.sound`: stage the
gesture in a prepared16-pot editing buffer, publish an audible transient through
the same generation mailbox, and commit a custom COW snapshot for this assignment
as one explicit undo transaction when the gesture completes. Preflight a spare or
unreferenced slot before the first change; FULL means no audible or persistent
edit. Repeated turns in the same gesture use that reservation, not a new slot per
tick. If shared, only this pattern detaches; keep other assignments unchanged.
Inherit mode retains today's project-wide editing behavior and is labeled Project
sound. A step-selected sound edit must visibly state STEP scope before changing its
snapshot; ordinary p-lock editing remains the existing hold-step knob gesture.
Reserve the editing buffer and transient copy in the native memory budget. Snapshot
records are immutable while shared/published; serialized COW/GC commits must explicitly
close prior undo gestures and atomically commit table+refs, not rely on400ms quiet.

## Playback precedence and restoration

For a newly accepted ordinary NOTE, resolve in this order:

1. Pattern default: embedded snapshot, or legacy project sound when handle0.
2. Optional same-engine step snapshot overrides the entire default for this note.
3. Existing matching-engine parameter locks/motion override individual controls.
4. Existing live hold/record gesture wins for its current documented duration.

Probability/condition stays independent. Evaluate its lane and note acceptance
before committing a candidate step sound; a rejected NOTE must not latch a preset
or revive an orphan TIE. It is insufficient to load a snapshot in `s_bon()` after
`motion_step()` has already applied locks to the old base. Define one event-plan
phase: resolve candidate/default and probability; decide accepted gate/legato;
commit eligible base; apply matching locks/holds against that base; emit note/MIDI.
Preserve probability/MIDI acceptance fixes on the eventual accepted baseline.

One `effective_base(part,engine,schema,param)` must serve motion's base/to_base,
lock preview/release/clear, motion-forget, UI value display, undo, stop, cue, song,
copy, reset and import. Today several directly read `proj.sound`; changing only
playback leaves stale restoration bugs. Changing base invalidates motion's cached
`applied` value even when the numeric lock value happens to be unchanged. Release
of a different pattern's lane must not restore an older base over current locks.
Navigation itself must issue no sound writes: preserve LX-REQ-035's repair.

Overrides are **note scoped, not latched**. A later accepted ordinary NOTE without
an override resolves the pattern default again. REST/rejected note/TIE do not
activate a step override. TIE holds the current timbre. An existing legato SLIDE
retains the current timbre across its continuation; a pending default/step sound
waits for the next ordinary nonlegato attack. Show `WAITING FOR NOTE` rather than
silently breaking the tie/glide. Stopped audition may explicitly preview an ordinary
attack. Preserve dormant override entries if their gate is changed to TIE/REST;
UI marks them inactive, allowing editing back without data loss. Shrink/expand,
RANDOM, REVERSE and PINGPONG resolve by actual visited step, never step+1 guesswork.

Stop ends audition/sequence note ownership, releases motion and restores the current
pattern default after the existing voice tail/declick policy; it never writes the
last step's snapshot to global project pots. Start, load and reset reconstruct from
validated persistent state, not whichever runtime preset sounded last. Pattern cues
use the destination part's pattern; held notes retain the old resolved snapshot
until the next eligible attack, even when the UI is displaying the new pattern.
UI shows stored assignment and active/pending sound separately. When manually
playing without transport, no step override applies; use the selected pattern base.

303 locks keep their exact current meaning. A future engine needs an engine tag in
lane identity; `lane_t.rsv` can hold a compact engine tag without growing its 72-byte
layout only after versioned migration. Initialize it in motion_alloc (currently not
initialized) and copy it in copy_part (currently only values are copied). Include
tag in find/record/hold/ramp/release/validation/undo/UI. Old lanes explicitly tag303
when T_303, not 'whatever engine is active'. Inactive-engine lanes remain saved and
must never restore 303 values into another engine. Full schema identity resolution
must not be guessed from one byte. Initial 303-only feature need not change lanes.

Engine changes are deliberately excluded from step overrides in the first version.
Unknown engine/schema rejects import before live mutation with an explicit error;
no fallback that relabels303 controls as Miami. Old firmware is expected to reject
new stream/export versions, not partially load them. Keep pre-upgrade backups.

## Atomicity, note safety and de-clicking design

Prepare, range-check and copy each 16-pot request outside audio. Use a dedicated
generation/acknowledgment mailbox or equivalent bounded immutable event packet;
ordinary `engine_set` floods can drop commands (`engine.c:138–145`) and cannot
atomically load a preset or guarantee note-off. The commit is one audio-owned
operation at an eligible attack, before its locks and note-on. No flash/file access,
allocation, decompression or preset searching in the ISR. Slot/pattern/part and
project generation accompany the request; late commands and preview releases act
only on their original owner. Coalesce uncommitted browse requests; never lose
final note-off or report APPLIED before ACK. Cancel pending state on reset/import.

For a same-engine fresh attack, terminate prior owned gate and outgoing MIDI note
before a new note-on if required by existing ownership; apply the complete matching
snapshot and matching locks, then trigger. Do not globally send All Notes Off or
kill another part/source. Preserve keybed/MIDI/preview ownership and current queues'
existing protections; this design does not declare those unfinished fixes complete.

A single DSP state per part avoids dual-render transitions. Proposed de-click
prototype: retain the previous output sample, then interpolate from that boundary
sample to newly rendered audio over a bounded 64-frame gain bridge (~1.45ms at
44.1kHz). It needs only bounded per-part sample/ramp state, no full audio buffer and
no allocation. The first sample matches the prior output; measure transient slope,
DC and musical attack effects. This is a testable proposal, not proof of click-free
audio. Resetting filter/envelope state under an open gate without a ramp is forbidden.
Continuous parameter-only edits keep existing smoothing; discrete waveform/drive/
envelope-mode changes use the same transition. Do not restart envelope modes on
every unchanged pot application. A later engine switch uses a separately tested
fade/end-owned-notes/reset/new-bank/fade lifecycle and refreshed descriptors; never
reuse303 setters for a different engine. Per-step cross-engine switching stays out.

## Musician-facing proposal and review preview

**Unimplemented interaction storyboard — no control reassignment is approved here.**
Keep existing TONE, VOICE, FILTER ADSR, SEQ, hold-step knob locks and all encoder roles.
Add one explicit SOUND action at the end of EDIT's existing SEL list, opening a
dedicated browser. Proposed screen content:

```
303A · P03     SOUND                         [proposed]
Stored: Project sound       Active: Project sound
> Original / Round Saw      303 · schema1
  Original / Hollow Pulse
  Original / Short Pluck
  Original / Driven Line
  Capture current base
Scope: Pattern P03 / 303A      Preview: OFF
```

SELECT moves rows as it already does in a list; SEL opens/selects an explicit row;
HOME backs out/cancels. Use explicit PREVIEW and ASSIGN TO PATTERN actions, no knob
push assumption (encoders are relative and have no push). PRESETS continues to cue
patterns even in this context; a cue/part/page change cancels browser preview and
its original note before leaving. Browser highlights alone never assign or sound.

Stopped PREVIEW uses a short bounded note, REC off, and never writes steps or
motion. Match a documented audition pitch/velocity and conservative level; show
which pitch is sounding. Release timeout, HOME, transport start, part/pattern change,
disconnect and project transaction all end the original preview and restore the
effective base plus any held lock state. First version disables audio preview while
playing or a live source holds notes; browsing remains available. ASSIGN while
playing queues until an eligible attack and shows PENDING. Do not silently save:
existing dirty/autosave/explicit SAVE policy applies.

Per-step increment adds a SOUND action for a captured selected step in the EDIT
list, with scope shown before assignment: `STEP 05 · 303A · P03`; capture step/part/
pattern before opening the list, since overlay closes held-step handling. No new
hold-step + SELECT/ALGORITHM gesture (already chance/velocity). Show `INHERIT
PATTERN`, explicit override and `CLEAR STEP SOUND`; display a small S marker distinct
from a parameter-lock mark. Gate TIE/REST and SLIDE continuation show INACTIVE / NEXT
ATTACK with reason. HOME cancels without toggling the step on key release. Keep
SAVE+knob lock removal and HOME+REC/PLAY undo/redo unchanged. Review this storyboard
with the owner before finalizing any UI implementation.

Original bank candidate names above describe musical jobs only. Design/listen to
four303 pot sets independently, using existing native ranges; characterize level,
accent, ties, drive and worst-case extremes. No generated sound values are included
in this design and no sound quality is claimed. Bank code would be four *16=64 pot
bytes plus IDs/names/indexes in XIP; real linked delta is larger and unknown.

## Persistence, undo and transfer migration

Use a new flash stream4 for preset sidecar/default semantics; retain OBJ_SOUND and
its existing sound revisions. A new encoder must deliberately serialize fields,
including legacy payload fields as needed; do not continue adding `sizeof(project_t)`
as a wire specification. Preserve the measured-before-write capacity test and CRC
checks across all stream objects. The four-object stream's capacity does not grow.
High-entropy snapshots/overrides may exceed compressed capacity even when a default
project fits. No write to settings/sound/stream on a MEMORY FULL preflight failure.
Retain current torn-stream rejection/default behavior; this proposal does not claim
current multi-object saves are fully atomic across power loss. Never borrow sample,
controller, calibration or rollback flash.

Read flash1/2/3 with frozen old layouts and CRC/length checks first; apply existing
sound migrations/velocity defaults, canonicalize new handle bytes to0 and create
an empty sidecar. This preserves old global-sound behavior exactly. Never give old
patterns different independently copied sounds by default. Validate all references,
engine/schema IDs, unique override keys, counts, flags, ranges and capacities before
audio sees them. A malformed import preserves live project, samples and undo.

New portable export2 should use header version, total length, CRC and typed,
length-delimited chunks for settings/sound/pattern/arrangement/presets. Required
unknown chunks or engine/schema reject; optional chunks may be skipped only by an
explicit preservation contract. The snapshot carries engine/schema and pots so it
survives bank deletion or library reorder. No plugin assets in native export.
Support legacy export1 using frozen layouts and its original stream/sound tags;
the current reader accepts only exact versions/sizeof and cannot already do this.
Legacy-format2 project support must be tested for export as well as flash; do not
assume the existing flash decoder is an export decoder.

`editor.c:152,188–252` currently indexes raw project_stage bytes and enforces one
PROJECT_FILE_SIZE. Add negotiated supported versions and bounded encoded read/write
streaming; validate CRC/length before commit. Update `web/x0x_projects.js` inner
payload validation/outer archive metadata and firmware/editor transfer tests, without
touching website PR19. Old clients must fail clearly, never truncate. Keep samples'
companion manifest/restoration semantics. Implement this work with the lifecycle
owner after the accepted baseline is pinned, not in this docs stage.

Include appended sidecar in UNDO_BYTES and update undo callbacks/labels: sound-base
and handle/slot changes trigger a bounded recomputation/ACK, not just engine_set for
raw sound offsets. New fields must not all be mislabeled MOTION by the current
`k>=lane_offset` test. One assignment/capture/delete/copy is one undo gesture.
Part-copy carries only that part's default and overrides, deduplicates/remaps slots
and preflights full lane/snapshot/override capacity before any destination edit.
Pattern-used/overwrite warnings must count default assignments and dormant step
overrides even when there are no notes (`ui.c:1180` currently checks musical steps).
Existing copy_part can partially copy before MOTION FULL; do not copy that failure
mode into the new feature. Clear notes keeps pattern sound but removes that part's
step overrides/locks; explicit Clear Sound resets assignment to inherit. Whole
pattern clear/reset behavior must be documented and testable. Shared snapshots
needed by other patterns survive. Project replacement resets history as currently;
cancel preserves it. Undo u16 offsets must stay below65536 and no ordinary preset
gesture should exceed the16128-byte history pool (`undo.c:10–20,100–103`).

## Resource assessment — estimates, no fit claim

Task-72 measured host layouts: settings40, sound1344, voice796, lane72, pattern656,
arrange12676, project35052, old export35084 bytes. The isolated layout probe here
must reconfirm them against the inspected headers. Native ABI/link map remains
authoritative. Two303 parts already cost1592 bytes of DSP state.

PR188's live metadata reports both app images550540, XIP spare2132, static RAM spare
412, physical pool spare8324. With the mandatory8192 reserve, usable pool margin is
**132 bytes**. These are reported existing build results, not a fresh build here,
and PR189's delta was not target-measured in this task. Earlier PR168 failed XIP+108;
do not assert that the active fixes have settled resource headroom.

| Proposed storage | Delta | Against PR188's reported margins, before new code/runtime |
| --- | ---: | --- |
| Pattern defaults in explicit versioned rsv bytes | 0 | Migration/copy/validation code still costs XIP |
| Phase1 sidecar header+8 snapshots | +200 live +200 staged | static212 left; pool68 below mandatory reserve |
| Phase2 adds16 sparse overrides | +64 live +64 staged | total264 each; static148 left; pool132 below mandatory reserve |
| Naive dense step handles | 32*2*64=4096 each | 8192 across live+stage; clearly unaffordable currently |
| Naive inline defaults, 24B per bass/pattern | 1536 each | 3072 across mirrors before steps/runtime |

Appending200/264 increases raw project/undo shadow to35252/35316; old raw export
lengths must not be silently updated as a compatibility strategy. The proposed
new wire length is determined by its encoder, not these sizeof numbers. Compact
snapshot slots save duplication but impose real capacity/error UX.

Budget at least two prepared16-byte snapshots, generation/ACK fields, active handle
and bounded de-click state; an illustrative **96 bytes total runtime** would leave
116 static bytes after phase1 or52 after phase2 if all placed in app RAM, while
pool would still fail by68/132. This96 is a design allowance, not compiled evidence;
measure globals, native padding, stack and transients. Per-preview backups and
cache/table storage must be included or removed. Do not shrink undo or8192 reserve
without explicit engineering review. No extra256-frame stereo buffer (2048 bytes).

CPU: at most16 descriptor/set operations at a committed sound change, sparse lookup
bounded16 entries, two parts, no allocation in audio. Setters recalculate coefficients;
cost at every dense accepted note must be measured, including fragments1..256 and
44.1kHz rendering. Worst-case DMA deadline11609us, guard thresholds85/92%, and other
drums/FX/core ownership remain as prior assessment. No target timing or click-free
claim follows from host schema tests. Native Elite/Pro links and controller closure,
RAM/pool/stack and deadline checks are later explicit gates.

## KORG stock presets: feasible host workflow versus unproven conversion

The supplied tracker says Miami/Chicago3.1.6 are installed and factory presets exist.
This task did not inspect binaries, decode bank formats, instantiate the plugins,
enumerate preset values or attempt the reference-host route whose permission recovery
is already separately owned. Installation does not establish permission to extract,
adapt or distribute preset content. No format decoder or redistribution license was
found/claimed. Existing private reference files stay local and out of this branch.

Official KORG Studio Guide5 page23 documents its program-name list and next-program
button; page25 documents assigning MIDI CCs to controls. It documents normal use,
not an embedded-bank export format or content redistribution grant:
https://www.korguser.net/gadget/manual/studioguide/Gadget_SG_E5.pdf

Apple documents AUv3 `factoryPresets` / `currentPreset`, bridged factory-preset support,
and a host-visible `parameterTree`. Its `fullStateForDocument` is a persistable plugin
state for a user document. AUv2 has `kAudioUnitProperty_FactoryPresets`. These APIs
provide a normal host route in principle; their existence does not prove this exact
KORG build exposes a complete list or every sound-relevant parameter:
https://developer.apple.com/documentation/audiotoolbox/auaudiounit/factorypresets
https://developer.apple.com/documentation/audiotoolbox/auaudiounit/parametertree
https://developer.apple.com/documentation/audiotoolbox/auaudiounit/fullstatefordocument
https://developer.apple.com/documentation/audiotoolbox/kaudiounitproperty_factorypresets
https://developer.apple.com/videos/play/wwdc2019/509/

**Conclusion:** normal host enumeration is a technically plausible personal-use
input path, but a lawful/native sound conversion path is **not yet established**.
Do not interpret opaque fullState/ClassInfo chunks or host preset files. A factory
name/number alone can identify a program for the same installed plugin, not rebuild
its sound on native303. Ordinary same-plugin session recall is a different outcome.

Next permitted evidence, after the existing hosting owner resolves supported access:
read only normal host-exposed component/version, whether factory-name/number list
exists, parameter IDs/names/units/ranges/enum metadata and whether documented readback
is complete. Record capability/completeness locally, not public factory values.
No automatic bulk extraction is authorized by this design. Assess the applicable
KORG EULA/content terms for personal conversion and consult KORG for explicit written
permission if unclear; public inclusion needs separate permission covering factory
parameters, banks/assets and names/attribution as applicable. Do not ask the owner to
approve proprietary decoding as a workaround.

Even with lawful readback, required mappings remain missing: exact parameter IDs
and schema/version, normalized versus physical curves/enum meanings, hidden effects/
macros, oscillator/filter/CRUSH behavior, envelope and note/velocity/glide rules,
stereo, gain and timing. Official KORG synth documentation describes Miami as a
monophonic wobble synth and Chicago as its own engine; neither is native303:
https://support.korguser.net/hc/en-us/articles/115001030803-Synth-gadgets
Matching labels or0–127 values is not acoustic equivalence. Miami's engine remains
unimplemented and owner-required offline audio comparison remains unmet. If an
approximation is later permitted, label it an approximation, keep source version and
mapping coverage, reject unmapped critical controls and never call it stock-equivalent.

## Minimal increments and acceptance

1. Agree capacities/storyboard; lifecycle and size owners provide one stable accepted
   source and budget. Preserve active PR168/188/189 and website PR19. No coordinator
   ledger writes from this worker; parent/designated writer owns LX-REQ-033 update.
2. Isolated storage/semantic implementation: stream/export migrations, effective-base
   resolver and303-only immutable snapshots. Independent persistence review before
   firmware/UI integration. Four original sounds need listening and level review.
3. Browser/capture/assign with stopped preview and no control remapping; native builds
   and independent combined tests. Pattern-only feature can ship separately only
   after the normal qualification gates; no publication authority is implied here.
4. Same-engine NOTE overrides after first increment is proven and resource budget
   retained. Alternate-engine selection and KORG compatibility remain independent.

Required meaningful tests: byte/semantic preservation of flash1/2/3 including nonzero
reserved bytes, old raw export1 layouts, malformed/new/unknown engine/schema/CRC/length,
duplicate/dangling handles and override capacity; full/high-entropy/torn save; aborted
import and companion sample hashes; copy/clear/undo/redo with slot reuse and shared
snapshots; pattern A→B→A, first note after stop/load, lock release/gap/ramp/base changes,
live holds/recording and navigation producing no parameter writes; probability rejection,
RANDOM actual step, REST/TIE/orphan/SLIDE; queue saturation and late generation events;
preview cancellation/transport/MIDI ownership; deterministic restored pots after bank
reorder/removal. Audio tests must include extreme waveform/drive/envelope transitions,
finite output, bounded boundary discontinuity and listening. Host schema checks below
prove only proposed resolution/resource arithmetic, not these production behaviors.

Independent reviewer findings incorporated: shared effective-base resolver; reserved
byte canonicalization; copy lane tag and per-part handles explicitly; undo invalidation
for appended sidecar; editor/client fixed-size migration; no PRESETS remap; preview
owns its original part; no borrowed unused sound cells. Remaining independent review
status and exact local evidence are in the handoff receipt.
