# Miami span observation: timer-free fixture

The observation copy preserves the unchanged PR #830 workload's call order,
frames, complete backend state and PCM for factory 0 and factory index 15.
This prepares attribution for the existing performance owner. It establishes no
performance improvement and changes no player behavior.

| Per program | Observed result |
| --- | ---: |
| Output frames, 128 blocks of 256 | 32,768 |
| Rendered leaf spans | 1,152 |
| Active-voice kernel calls | 2,304 |
| Voice-frame work, both voices combined | 65,536 |
| Every 64th span eligible, starting at ordinal 64 | 18 |
| Eligible active-voice calls, both voices included | 36 |
| Complete backend state images compared | 128 × 3,840 bytes |
| PCM bytes compared | 262,144 |

Both programs use the existing note 60/velocity 100, 120 BPM, 44.1 kHz held-note
fixture. Each voice receives 1,152 calls and 18 eligible calls. Eligibility resets
for each program; the ordinal continues across its 128 output blocks. No branch
uses eligibility to skip, subdivide, reorder or otherwise change a DSP call.
The span length histogram has 512 one-frame spans, 350 63-frame spans and 290
other spans. Full histograms are in [results.json](results.json).

Eligible calls cover lengths 1, 24, 37, 46, 59 and 63. Deterministic one-in-64
selection omits other observed lengths and can align with the control schedule.
It is a reproducible eligibility rule, not a representative statistical sample.
Do not extrapolate its future timing to all spans or presets.

## Executed validation

```sh
python3 tests/host/run_miami_span_observation.py --out /tmp/miami-span-validation > /tmp/miami-span-validation.stdout 2> /tmp/miami-span-validation.stderr
```

Exit **0** with GCC `cc (Debian 14.2.0-19) 14.2.0`, `-O2`, software FMA,
`-fno-fast-math` and `-ffp-contract=off`. The runner retains every command,
compiler dependency hash and program result. Use a fresh output directory.
See [commands.txt](commands.txt) and [verification.json](verification.json).

The new C harness includes the unchanged PR #830 C fixture. Each program runs
through the unchanged backend first, then the temporary observation backend.
It compares every call's block, voice and frame count with the baseline trace.
The original comparison dispatcher still compares separately compiled kernels'
return status, filter/gain/phase state and stereo samples on every call. The
counter histograms and eligibility counts are independently checked against the
baseline call trace. Eligibility boundaries 0, 1, 63, 64, 65, 127, 128 and
UINT32_MAX are asserted.

The unchanged fixture reinitializes the same static backend object for both runs
in the same process. Its address and complete initial state must match. All
backend bytes, including pointers and padding, are then compared after each
block. This is same-process state equality, not a portable serialized snapshot.
The tests also compare the complete PCM streams and PR #830's stored hashes:

- Factory 0: `64fd6099abc260f6f857cfc2daa815b8ddf514e8c93777e486c9b37190dc11ad`.
- Factory 15: `4cac17b6b13ccfe5c8733a614e95c4c361e24420533b6faf4bd4c9514fd0b727`.

Before adding the hooks, characterization passed those baseline/state checks
for factory 0 and failed the missing-counter assertion with binary exit **31**.
An initial compile setup error selected an external prototype for duplicate
renaming; it was corrected before characterization. The final false-observation
negative changes only the reported voice span from `n` to `n+1`; it fails with
exit **31**, `OBSERVATION_MISMATCH observation agrees with actual kernel call`.
The production kernel remains unchanged in that negative.

The runner authenticated 176 available PR #830 input hashes and reused its
zero-XMOD result and phase-sign mutant evidence. Its three generated temporary
inputs were unavailable; they are not claimed reauthenticated. The reported
996,430-case arithmetic receipt remains reused as supplied, not rerun or newly
authenticated. The new execution hashes 181 consumed input files.

## Source, ownership and remaining input limit

Firmware `8be16486e33f65b31c76710e1d825b6aa701c2c9` has tree
`f952ab4455723e4a1a68fdb3a8eea22d2da59710`. The backend blob is
`2efde1794c8454b7af0f9c527f49abd03a28a783`; the kernel blob is
`f74384b3f5e8850339fe4d42309b36fc10dd6b3b`. The runner verifies both committed
and working-file blobs and requires unchanged firmware, experiments and build
recipe paths. Only temporary generated backend copies get observation hooks.

PR #830 at `e1c6b02bf6660142af3ae575a3b159cd051a011a` supplies the unchanged
fixture, runner and prior evidence. Its README says the older original fixtures
were unavailable. The separately named stage-3 fixture has not been identified
or recovered here: workspace/GitHub-path searches and Library searches did not
resolve it, and personal context is unavailable. This work reuses PR #830's
accessible fixture; it does not claim the missing stage-3 bytes were reused.
The parent or existing analyst owns reconciliation of that input identity.

Policy: `nightly@66073c68dd80d51a7053432c2c161a8b720a1341`.
Coordination: `4a35a6cdaa0f112db6b3f577542138afd170c20d`, LX-REQ-023 linked
to LX-REQ-016. Live #814 remains open at
`opt/miami-coef-cache-0.0.24@dcc9833f24b20ec2baf47beb131f937e32d95e11`.
Its producer and `independent_qa/6adafbbf54088191a86715d49e7dc6b0` reviewer
retain ownership. No shared ledger, source branch or review assignment changed.
Connected GitHub identity `gobbajogger` has repository admin permission; native
Git's preconfigured author differs, so delivery uses the connector default
without changing local Git identity.

This lane owns only the new test/evidence files. One independent manual artifact
review is the bounded budget carried from PR #830. Full CE multi-reviewer,
cross-model, full host/sanitizer suites, native builds and device checks are
**NOT_RUN** and are not qualified by this artifact review.

## Proposed boundaries for the existing performance owner

These are proposals only. No clock code, timed command or reservation is added.
Before a benchmark, the existing owner must obtain a separate **actual global
slot reservation**, record its real identity and validity, pin the accepted
fixture/source and reconcile the stage-3 identity above. This task reserves no
slot and must not trigger a benchmark.

| Future measurement | Proposed start | Proposed end | Interpretation |
| --- | --- | --- | --- |
| Whole output block | Immediately before the original public `original_miami_backend_render_words` call, after workload/input preparation | Immediately after return, before validation or PCM I/O | Includes backend preparation, scheduler/control work, synthesis and FX; use unchanged 256-frame blocks |
| Leaf span | After recursive splitting in `render_span`, before accumulator clearing | After output-word conversion, immediately before the successful return | Includes clearing, ascending active-voice sum and output conversion; excludes enclosing scheduler/control work |
| Active-voice kernel | Immediately before `original_miami_voice_render_add` for an eligible leaf span and voice | Immediately after that call returns, before `completed(s, result)` validation | Includes kernel entry checks and DSP work; excludes the fixture's reference call, state comparisons and telemetry writes |

Keep whole-block and nested measurements separate; nested costs cannot be
added as independent totals. Any future timed version must retain call order
and PCM/state checks in a separate untimed validation pass. Reporting and file
I/O belong outside the prospective measured regions. The production owner,
existing reviewer and global-slot owner decide the accepted timing protocol;
this disposable fixture does not assume their authority.

Return this exact artifact to the existing performance producer for consumption.
The parent handoff is the supported receiver route in this environment; no
direct Work-thread reader/creator is exposed. Receiver consumption stays pending
until an actual scoped action is observed. The producer's #814 implementation,
full-suite and combined acceptance gates stay open under their existing owners.

No benchmark, hardware, Safari, Actions, production edit, merge, force push,
firmware build or publication was performed. No additional operational monitoring
required: production source is unchanged.
