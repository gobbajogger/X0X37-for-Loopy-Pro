# SMC feedback and incomplete layout investigation

## Result

The source contains a reproducible navigation feedback defect. Five shortcuts
have LED conditions that do not describe their existing actions. The correction
is committed and checked as an Elite development build. The separate report of
SMC controls failing to affect x0x remains physically unverified.

- **Build:** source repair `513574131879ff1f26a2f9ca44a2d11a68de3afc`, Elite,
  development configuration with `X0X_DEV_PADCAL=1`.
- **Replaces:** incorrect navigation LED conditions in integration base
  `cb3c0502ae405af0a76325727d92ba1667d2982d`. This is a source repair handoff,
  not a completed integration or installation.
- **Checked against decisions:** the owner's three-mode fader/button mapping
  and subsequent knob request in [USER_PREFERENCES.md](USER_PREFERENCES.md),
  section “SMC mapping correction”; existing shortcut actions in
  [SMC_MIXER.md](SMC_MIXER.md); scoped development checks and sole integration
  ownership in the current project skills.

## Owner evidence and revised diagnosis

The owner first requested investigation of “real reasons why SMC not reaching
x0x”, then clarified:

> they are following start stop and looks like they still follow old feedback instead of new layout we locked

The observation that the lights actively follow Start/Stop changes the diagnosis.
`smc_frame()` sends feedback only while the application link is up and the central
role is enabled. An application link stuck down cannot explain fresh outgoing
feedback during the reported failure. The previously found reconnect race is a
real, separately repaired defect, but it is not a sufficient explanation of this
observation.

Working outgoing feedback does not establish that incoming SMC notifications
reach the application. Subscription and feedback use separate ATT operations.
The retained physical trace is explicitly idle and contains no attempted control
event. Its zero notification count cannot establish where an attempted event was
lost. This investigation does not turn that idle trace into an input failure test.

“Layout we locked” is treated as the agreed layout. It is not treated as a request
to invent a new persistent or latched operating mode.

## Confirmed feedback defect

The button action table and the LED code had diverged. Several reserved buttons
still shared a generic per-strip FX/MIX condition. This table describes the
main-map navigation shortcuts; drum SELECT keeps its separate track mapping.

| Control | Existing action | Incorrect feedback before repair | Repaired indicator |
| --- | --- | --- | --- |
| R8 | HOME | Also lit on MIX | HOME only |
| S6 | MIX, retaining its page | Lit on FX and off on MIX | Any MIX page |
| S8 | Selected part's sound | Lit on HOME/MIX and off on PART | Any PART sound page |
| SELECT 6 | Reverb | Also lit on delay | FX page 0 only |
| SELECT 8 | Master | Also lit on HOME and unrelated MIX pages | MIX page 3 only |

SELECT 7 already correctly identifies delay, FX page 1. S8 enters PART at page
0, but the PART view contains all sound pages for the selected part. Its LED
therefore stays on while navigating within that sound workspace.

The original focused probe found seven mismatches in 30 cases. The completed
regression tests more state combinations: the exact integrated baseline compiles
and fails 83 of 892 expectations. Those 83 failures are repeated manifestations
of the five shortcut defects, not 83 independent bugs. The repaired source passes
all 892 expectations.

Production changes are confined to `smc_led()` in
`firmware/src/app/smc_mixer.c`. A byte comparison confirms all other production
text in that file matches the integration base. The patch preserves drum MUTE
and SELECT precedence, existing Record/Solo behavior, input decoding, pickup,
transport handling and controller firmware.

## The agreed button layout is still incomplete

The preserved owner decision explicitly asks for three modes for faders and
buttons, followed by “Map knobs too.” The implementation provides main-mix and
individual 909/808 destinations for faders, knobs, MUTE and SELECT. Record/Solo
still use whole-part behavior on a drum MIX page. For example, R2/S2 address the
808 part rather than the second sound in the visible 909 bank.

The earliest retained deferral is an implementation report:

`evidence/mapping-repair/drum-mutes-notes.txt`

> R/S per-track policy is deferred. R2/S2 retain their whole-part behavior.

Later reports and tests repeat the deferral. The reviewed material contains no
direct owner approval for narrowing the three-mode button requirement this way.
Therefore the whole agreed button layout must not be described as completed or
the deferral as owner-approved. This feedback repair preserves existing R/S
actions; it does not invent missing per-drum record/solo semantics.

The current three-mode selection is page-dependent: individual drum routing
requires a visible 909 MIX or 808 MIX page, no overlay, and clip mode off. This is
verified implementation behavior. Its presence alone does not establish that the
owner requested an independent persistent mode.

## Why an old-looking layout does not prove an old binary

The last retained verified installation is
`a5cb24dd08b40b80e802e03ff0d9c848315a9751`, recorded in
`evidence/latest-install/install-record.json`. It already contains the newer
fader/knob and drum MUTE/SELECT implementation. Its `smc_mixer.c` blob is
`fa885dad6c721f413da64ab78a2f7439b51d29d7`, matching the earlier validated mapping
candidate `4ff0cd4`.

The mapping, button and LED functions are identical between that recorded install
and integration base `cb3c050`. The latest integration checkout observed during
handoff preparation was `fd5487df6dd5d7a3ff0fbb561425f9cd5abcfa0c`; its changes
since `cb3c050` add receive-repair evidence and do not modify those functions.
Consequently, the queue/link repairs do not supply a different button or LED
layout. The misleading feedback is present in the source itself.

The retained install record is not a fresh readback of the currently running
application. Runtime identity `SMK-37 Elite_900` does not identify a source
commit. A stale current binary remains unproven.

## Verification

The isolated lane is `lane/mb-smc-feedback-20261021` at
`/workspace/scratch/9d5aa4f082c9/x0x37-feedback`.

| Check | Result | Scope |
| --- | --- | --- |
| Exact base negative control | Expected failure: 83/892 | Production LED/frame functions compiled from `cb3c050` with substituted surrounding state |
| Corrected feedback regression | 892/892 pass | Navigation positives/negatives, both kits/banks, unused slots, existing part/transport feedback, emitted MIDI and output guards |
| Existing `smc-filter` | Pass | Independent master mute and filter destination; fixture adds only the newly referenced `V_PART` identifier |
| Existing `smc-drum-mutes` | Pass | Full production UI/engine consumer for drum action and feedback behavior |
| Independent review | No blocking finding | Existing action destinations, page definitions, drum precedence and test boundaries; no duplicate test run |
| Elite development build | DEV CHECKED | Source commit, frozen inputs, package and required resource margins |
| Development record verification | Pass | Exact package, source identity and log hashes |
| Device input / installed correction | NOT TESTED / NOT INSTALLED | No device operation was performed here |

The scoped development runner used:

```sh
env CC=cc CXX=c++ \
  JIELI_TOOLCHAIN=/workspace/scratch/3c13c8e69cb3/deps/jieli-linux-toolchains-20260730.1 \
  AC79_SDK=/workspace/scratch/3c13c8e69cb3/deps/ac79-sdk \
  X0X_TEST_JOBS=1 X0X_HOST_DEFS=-D_POSIX_C_SOURCE=200809L \
  PYTHONDONTWRITEBYTECODE=1 \
  python3 tools/dev_validate.py run \
  --commit 513574131879ff1f26a2f9ca44a2d11a68de3afc --model elite \
  --plan /workspace/scratch/9d5aa4f082c9/feedback-dev-plan.json \
  --output /workspace/scratch/9d5aa4f082c9/feedback-elite-validation \
  --no-controller
```

The plan selects `smc-feedback`, `smc-filter` and `smc-drum-mutes`. Input and
controller code are unchanged by this repair. Controller rebuilding and duplicate
Pro validation were not part of this development scope.

Remaining margins: **8,288 B code, 508 B static RAM, 8,324 B pool**. The pool
minimum remains 8,192 B. Compared with the integration base, the correction uses
32 more bytes of code and leaves RAM/pool use unchanged.

Exact application package SHA-256:
`1468bf0045dfebbfd5d99527b39d1c547627752eb0e149f0284e57b0d756d6cc`.

Evidence locations:

- `/workspace/scratch/9d5aa4f082c9/feedback-validation/validation.json`, negative
  and positive logs, and exact baseline source.
- `/workspace/scratch/9d5aa4f082c9/feedback-elite-validation/record.json`,
  `check-0.log`, `build.log` and the checked development package.
- `/workspace/scratch/9d5aa4f082c9/feedback-dev-plan.json`.

## Handoff and remaining work

**One receiving owner:** the existing firmware integration lane
`lane/mb-integrated-smc-20261021`, at
`/workspace/scratch/efe94e5f6f65/x0x37-integrated`.

The integration owner can take source commit
`513574131879ff1f26a2f9ca44a2d11a68de3afc` from this isolated lane, review it
against the current integration head and carry its scoped evidence into the
development installation workflow. There is no competing installer from this
task. No external push, device write or cross-thread notification was performed.

Two items remain separate from the checked LED correction:

1. Complete the agreed per-drum button behavior after recovering or resolving the
   actual R/S semantics; do not treat implementation notes as owner approval.
2. Locate a failure-period incoming control event in existing runtime evidence or
   supported device access. The decisive boundary is whether it was transmitted,
   received, decoded/enqueued and applied. Fresh outgoing lights alone cannot
   answer that question. Do not request another generic movement/capture cycle or
   infer loss from idle zero counters.

This handoff makes the confirmed source defect reviewable without claiming that
the missing physical input has been cured.
