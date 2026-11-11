# Longer bass notes: source investigation and bounded regression handoff

Stage: tests/docs delivered for review. No runtime or UI mapping change.

The musician needs bass notes that last longer than one step. Existing NOTE/TIE
gates already represent these notes. This change explains the working controls
and adds observable regression coverage so that integration preserves sustain,
retrigger, slide and release behavior. It does not introduce a second duration
format or change the timing of steps.

## Exact source and ownership

- Base NIGHTLY: `08d3164a265123cb4c84e5104642fd93257d63d7`.
- Production `main` observed: `c2ca0c1384390aa985b7df83b73dfe9bef40d7c9`.
- Unchanged compiler-input `firmware/` tree: `a8eb1d512a8917f84f26908da181d9b042dad282`.
- Production sequencer blob: `09d9cea9936de02a6e410f80321d1c6c4b400f11`.
- Isolated writer: `lane/seq-duration-tie-20261025`; no changes to other lanes.
- Contract: reviewed trial `e2482a5039527195e346925c751ac3e3a333ada9`,
  `docs/engineering-delivery/SKILL.md` SHA256
  `2dd8889e0843b4bbd9686cb42d3936921d7e23a4dde688bfea9107e1b8a556b3`.

## Current interaction, source trace and limits

Current controls, not a proposed UI:

```text
SEQ: hold step -> KNOB 2: REST / NOTE / TIE
Stopped REC: key -> NOTE; LFO + OCT -> TIE; ENV + OCT -> REST
Example: NOTE C2 | TIE | TIE | REST
         attack   hold  hold  silence
```

`app/ui.c:bass_write` stores the gate in the existing `bstep_t.flags` byte.
`bass_kbd` writes one onset on key-down in live REC; it does not write duration
from held-key time. `seq/sequencer.c:fire_bass` retains the previous sounding
pitch during TIE. It calls the local sink only for NOTE (or the existing orphan
TIE case discussed below); NOTE lookahead suppresses gate release before a TIE.
The final TIE uses the maintained half-step gate rule. Three occupied positions
therefore sound for 2.5 straight steps. RATE controls step spacing independently.
SLIDE on the preceding step connects to a new NOTE pitch. A same-pitch NOTE
still retriggers; a TIE does not.

Production `app/engine.c:s_bon` calls `bass303_note_on_velocity`, while `s_boff`
calls `bass303_note_off`; the sequencer also independently owns external MIDI
NoteOn/NoteOff and slide-overlap release. The new fixture links the actual
sequencer/pattern/TB-3PO and observes its local and MIDI sinks. It does not render
the complete synth audio path or claim to prove its envelopes.

No new explicit-length control is necessary for this existing representation.
If held-note recording is desired, the minimal *proposal*, not implemented or
approved UI, is an opt-in recording-length mode that writes continuation TIEs as
the held note crosses actual playback steps. Review its screen and interaction
before rollout. Decide quantization, occupied-step overwrite, loop limits,
direction changes and key release/transport ownership first. Do not change RATE,
repurpose KNOB 2 or reinterpret saved project bytes to add duration.

## Reuse existing fixes; do not duplicate their writers

- [PR139](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/139),
  `55acb2fd442bd86f372e72e06727d7f27a4f5474`: probability rejection and orphan
  TIE suppression. Its exact production-linked regression compiled on this
  NIGHTLY with its supported baseline macro and failed 17 assertions, exit 1;
  exact PR139 source compiled and passed, exit 0. This is the existing fix.
- [PR140](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/140),
  `4b67024e62bfd1b4d5789827334f6bea4f9b4e00`: logical bars and temporal RANDOM
  swing, including external-to-internal clock handoff. Preserve these independent
  timing corrections when integrating duration behavior.
- [PR159](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/159),
  `28fbcf78b8930da6f1535268501cdb7c0a66b6b3`: selected RANDOM next-step TIE.
  Its unchanged exact-source regression reproduced 27/34 on NIGHTLY, exit 1;
  exact PR159 source passed, exit 0. The seven failures include premature local
  and MIDI release and a false onset at the selected TIE.
- Parallel [PR182](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/182)
  source patch `650d632dfdb4756e7e75b8f334b6ad27e4245862`, evidence head
  `e2856159236de7b8e6587f05f139436ac72967a5`, owns Decay/filter-envelope
  reopening after sustain. Its completion was supplied by the coordinator;
  this task has not independently qualified it. No overlapping DSP edit here.

Independent source review found no additional unambiguous unowned gate defect.
Potential live-key NoteOff ownership needs its own bounded reproduction before
changing the synth command queue. Do not silently equate missing duration
recording with a regression in the existing TIE representation.

## Executed evidence and scope

Registered `seq-long-notes`: 545/545 checks on both Apple Clang 21/macOS and
GCC 12.2/Linux. Links complete production sequencer, pattern and TB-3PO sources.
Cases cover both bass parts, 2/4/16/64-step chains, all four step rates, forward
and reverse, wrap, REST, repeated NOTE versus TIE, new-pitch SLIDE, final-half-step
release, tempo/rate edits, pattern cue, stop/start/continue, and incoming 24ppqn.
The new-rate case observes the actual next interval at sample 13965. RANDOM
failure behavior is covered by the existing PR159 regression, not marked passing
on NIGHTLY. Ping-pong live resize is covered by the existing registered test.

The new callback uses C11 `_Generic` to select the exact existing void callback
or PR139's int acceptance callback, with no incompatible pointer cast. Accepted
notes are the scope; PR139's separate test owns rejection cases.

Actual registered Linux command:

```sh
X0X_TEST_JOBS=1 sh tests/run_tests.sh seq-long-notes project-303-stream sequencer seq-pingpong-resize clk-lock
```

All five selectors passed. The migration test now verifies existing TIE and
TIE/SLIDE/ACCENT flags and pitches survive production format-2 conversion across
all 32 patterns. It is migration coverage, not full device project save/load.
The macOS migration compile failed before the added assertions because `.pool`
uses an ELF section attribute; the Linux test passed. No target-code workaround
was introduced for this host limitation.

Linux image identity:
`sha256:0cffe038efa0f50556de3fb444de6386729d495f63703c8c3b40fdfd0482d4b1`.
Containers ran without network or hardware. Apple Clang ASan+UBSan and Linux
UBSan passed 545/545. The first Linux GCC ASan+UBSan process emitted repeated
`DEADLYSIGNAL` and segfaulted without a useful trace. A minimal sanitized program
passed; the diagnostic rerun of the actual test passed 545/545 with sanitizer
initialization/interceptor warnings and no reported memory violation. Cause of
the initial failure is unresolved; do not call Linux ASan consistently clean.

Independent reviewer executed the registered final selector: 545/545, exit 0.
Reviewed blobs: test `c9c9698bc3e7148fa244c24d32cfb3a4dfd4153a`, runner
`736a5b0b2237ebab4a53efa4333cefb117b72098`, migration
`1912a35315e39da1239d4ce97b93511374ed5871`, manual
`7d644120a73d10c80759d3b98a983829673c7ab7`.
Before the final two rate assertions, independent temporary source mutations
compiled but failed: TIE retrigger 274/543, removed TIE sustain 209/543, removed
slide 541/543. These are deliberate negative controls, not source fixes.

## Next bounded integration

The existing firmware integrator owns combining PR139/140/159/182 and this
tests/docs lane on one actual committed tree. Preserve existing independent QA
and native-build ownership. Rerun the registered tests on that tree and resolve
the known PR159 void-callback versus PR139 int-callback test mismatch. The new
long-note test accepts either exact ABI, but does not repair PR159's test.

No app compiler input changed in this lane, so native Elite/Pro budgets are
unchanged by construction; target builds were not executed here. Changed runtime
combinations require their own exact-input Elite/Pro builds and independent
audio/gate/clock/probability/data-safety checks. Complete-root QA, full project
save/load, UI screenshots, model firmware/controller builds, hardware, merges
and binary publication are NOT_RUN. This evidence is bounded tests/docs
acceptance; it is not install qualification or production promotion.
