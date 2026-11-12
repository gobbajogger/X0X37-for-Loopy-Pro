# Combined sequencer and synth reliability candidate

This candidate repairs silent probability steps, orphan TIEs, RANDOM gate
lookahead, musical-time swing and logical bars, plus filter ADSR sustain during
live Decay changes. Existing NOTE/TIE controls and sound paths stay in place.
No held-key duration recording or new duration control is implemented.

## Inputs and source disposition

Owner: isolated combined-source integrator. Parent coordinator retains protected
integration and downstream publication ownership. Request continuity:
LX-REQ-008, LX-REQ-009, LX-REQ-023 (scoped evidence, not full request completion).
Contract pin: e2482a5039527195e346925c751ac3e3a333ada9;
contract SHA256 2dd8889e0843b4bbd9686cb42d3936921d7e23a4dde688bfea9107e1b8a556b3.

Accepted NIGHTLY read from GitHub and cloned at
`08d3164a265123cb4c84e5104642fd93257d63d7`.
Runtime and tests candidate: `1072a2ead7171c2e1c7f1e9471318698894716e1`;
complete root `80551c5ae03da14c599df0d54c30c64982fd9c40`;
firmware tree `f000d4d5fcbe5ad3d1cdae8ff16699a990272ef1`.
This handoff is a later Markdown-only addition. Exact model records use the
runner's source fingerprint, which excludes Markdown; it must remain identical.

| PR | Exact inspected open head | Applicable included repair |
| --- | --- | --- |
| 139 | `55acb2fd442bd86f372e72e06727d7f27a4f5474` | R1 MIDI acceptance, R2 rejected-state cleanup, R3 orphan TIE silence |
| 140 | `4b67024e62bfd1b4d5789827334f6bea4f9b4e00` | R1 logical bars, R2 temporal RANDOM swing, R3 external/internal phase handoff |
| 159 | `28fbcf78b8930da6f1535268501cdb7c0a66b6b3` | Copy-RNG selected TIE preview |
| 182 | `e2856159236de7b8e6587f05f139436ac72967a5` | Patch 650d632d plus sustain evidence |
| 183 | `f9d1480a4db20af513ab5f7504a3ad56d3a03692` | Production-linked duration/migration tests and existing-control documentation |

All five unique patch series remain absent from the inspected NIGHTLY. They were
applied as individual commits, without merging historical branches. This combined
candidate is a proposed replacement integration vehicle; original PRs are not
closed or declared merged. Earlier PR139 R1/R2-only and PR140 R1/R2-only candidates
are superseded for this combination. PR168 size recovery is separate.

Runtime differences are confined to engine.c, sequencer.c/.h and bass303.c.
No SMC, website, UI mapping, controller, persistent-format or SDK changes.

## Integration decisions

PR139 changes bass_on from void to an int acceptance result. Updated all five
remaining affected fixtures (PR159, three PR140 tests and clk_lock) to return 1,
without incompatible casts. PR183's generic callback already supports either ABI.
Registered-selector conflicts retained every selector.

The first combined registered sequencer run failed its old expectation that an
orphan TIE after REST triggers a note. BAS-03 and reviewed PR139 R3 require silence.
The corrected fixture preserves previous slide/sustain assertions and verifies
that the next ordinary NOTE alone attacks at step 6 without a slide. Independent
review compiled and passed the full sequencer fixture; removing only the orphan
TIE guard compiled but deliberately failed the new assertion. No runtime patch
was added to satisfy the obsolete expectation.

## Executed combined validation

Linux image: `sha256:0cffe038efa0f50556de3fb444de6386729d495f63703c8c3b40fdfd0482d4b1`.
No Actions execution, device operation, network-policy change or binary publishing.

Registered affected selectors on exact 1072a2ea: sequencer, bass-prob-midi,
seq-tb3po-bars, seq-random-swing, seq-random-clock-handoff, seq-long-notes,
seq-pingpong-resize, clk-lock, seq-random-tie-preview, bass303_lite,
bass303-sustain and project-303-stream: all 12 PASS. The optional TB-3PO reference
comparison was absent and skipped; this is not a full suite/reference PASS.

Independent read-only reviewer compiled all affected sequencer fixtures on Apple
Clang 21: bars 878/878, swing 1222/1222, clock handoff 1472/1472, selected TIE 34/34,
long notes 545/545, acceptance/MIDI and clock-lock passed. Eight sustain render
controls each produced 0/88,200 differing samples. Prior PR182 paired-audio
preservation evidence is reused only for unchanged bass303 DSP bytes; no new
whole-synth performance or device-audio claim.

Independent combined real-source matrix, optimized and Apple ASan+UBSan:
each 18,336 cases / 232,256 assertions / 0 failures. Both bass parts, lengths
3/4/8/16/64, seeds 1..512 with distinct selected neighbors, swing 0/100,
rejected sliding/accent NOTE to orphan TIE, held TIE, unused TIE to actual REST,
clean later NOTE, accent, RNG preservation/single commit and balanced MIDI Stop.
Negative controls compiled and failed as intended: removed orphan guard 12,224
failed assertions; removed selected lookahead 21,504. Matrix harness SHA256:
`9a1d412beb0c2c8773f636c59e64ea925711d2a6e1e1f6496aaa9e7166c339b1`.
This matrix injects acceptance at the actual sequencer sink.

An additional Linux whole-host probe drives real motion T_PROB -> engine prob_skip
-> sequencer/MIDI via engine_render. Both bass parts x FORWARD/RANDOM PASS:
chance 0 rejects, actual next orphan TIE remains silent on MIDI, and changing
chance to 100 allows a note after Stop/Start. DSP code executes, but audio samples
and private envelope state are not asserted; no acoustic silence claim.
Independent review accepted that bounded scope and its positive control.

Linux combined UBSan: six sequencer fixtures compiled and passed. Initial Linux
ASan+UBSan: long notes, selected TIE, probability/MIDI and RANDOM swing passed;
logical bars and clock handoff compiled but each exited 139 with repeated
DEADLYSIGNAL and no useful trace. One independent diagnostic run per unchanged failed binary passed (878/878 and
1472/1472). A separately compiled minimal `int main(void){return 0;}` ASan+UBSan
program also failed with the identical exit-139 signal pattern in the same image.
This demonstrates sanitizer-runtime instability independent of project code,
without attributing the exact original crashes or declaring universal success.
PR183's earlier initial Linux GCC ASan crash also remains unexplained. Passing
reruns cannot establish a universal sanitizer pass.

No maintained firmware/spec ratchet implementation was found. Ratchet behavior
is unclaimed; clip note-repeat/ARP is outside this scope.

## Remaining gates and next owner

Whole-host `host-build` plus nine scenarios (blenotes, clkmove, extclock,
probcond, probreset, song, step303, tb3po, usbnotes): all PASS on the combined
source. This is affected-scenario coverage, not the complete repository suite.

Both actual Elite and Pro DEV builds passed `tools/dev_validate.py` and its
independent artifact/record verify command. Source fingerprint for both:
`b8680eb7dae65f937e9787413003d7ce98f8191776a450a051b34dab94c2863f`.
SDK/toolchain/image/runtime identities are captured in each private schema-2
record. No proprietary inputs or artifacts are committed.

| Model | App bytes | Package bytes | Code free | RAM free | Pool free |
| --- | ---: | ---: | ---: | ---: | ---: |
| Elite DEV | 550540 | 651396 | 2132 | 412 | 8324 |
| Pro DEV | 550540 | 651392 | 2132 | 412 | 8324 |

Pool minimum is 8192 bytes; the remaining margin above it is 132 bytes.
Model identities independently read back as SMK-37 Elite_900 and SMK-37 Pro_900.
Package header/list/file CRCs pass, as do controller header/CRC checks.
Both separately rebuilt controller outputs match: 65492 bytes, CRC32 77b7a496,
RAM 25824/26368 bytes. Package SHA256:

- Elite: `461275b369b9b4177124e7ec0515e7afd06804a66cc1d5851ab5f4c6a6aa2d8d`.
- Pro: `9b027ffb38e0b5f5f2b7b135d2671ac7df2d04bac05db908fd98ca1596789f93`.
- Matching controller: `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a`.

These are DEV CHECKED outputs, not installed or selectable NIGHTLY. The scoped
validator plan passes; it does not convert the separately retained sanitizer
crash history into a pass. Both records and binaries remain private to the
execution environment. No firmware was published.
 No full-suite,
reference-comparison, complete project save/load, backup/readback/restore/OTA,
package/model negatives, physical hardware or installation acceptance is claimed.
These remain distinct from scoped source review and development validation.

Parent coordinator / sole integrator should consume this candidate and its exact
records, resolve the remaining sanitizer disposition, verify fresh base/head and
required review/protections, then decide protected NIGHTLY integration. Owner
installation follows exact model/controller package verification and applicable
remaining gates. No production or picker promotion is authorized here.
