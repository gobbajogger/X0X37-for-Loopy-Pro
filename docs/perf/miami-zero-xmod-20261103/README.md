# Miami zero-XMOD: no applicability in these two audible workloads

Both default factory 0 and factory index 15 use nonzero XMOD throughout this
focused held-note render. A zero-XMOD specialization would cover **0 of 131,072
oscillator-pair samples per program** here. Do not advance this idea as a speedup
for either measured program. No timing benchmark or optimization was performed.

| Program | XMOD binary32 bits | Pair samples | Nonzero carrier-gain pairs | Zero-XMOD pairs | Nonzero output frames |
| --- | --- | ---: | ---: | ---: | ---: |
| Default, factory 0 | `3dc8b978` | 131,072 | 131,068 | 0 | 32,757 |
| Factory index 15 | `3d482186` | 131,072 | 65,534 | 0 | 32,728 |

The XMOD values are approximately 0.0980100 and 0.0488601. "Nonzero carrier"
counts the even/master oscillator's current gain at the original gain-read
point, before its ramp advances. It does not assert perceptual audibility.
The backend output has nonzero PCM, which guards against an idle-only result.
Each program rendered 32,768 frames: note 60, velocity 100, 120 BPM, 44.1 kHz,
128 calls of 256 frames, with two real UNISON lanes. Factory indices are zero
based. This does not cover other notes, macro edits, releases or all presets.

## Executed evidence

Run on the managed Linux execution environment with GCC
`cc (Debian 14.2.0-19) 14.2.0`:

```sh
python3 tests/host/run_miami_zero_xmod_assessment.py --out /tmp/miami-zero-xmod-20261103 > /tmp/miami-zero-xmod-20261103.stdout.json
```

Exit **0**. The runner compiles the pinned kernel independently twice, routes
the actual backend's span calls through a comparison fixture, then compiles
temporary mutant and counter variants. All actual compiler/linker/test commands
are retained in [commands.txt](commands.txt). No external SDK is needed.

| Check | Default | Factory index 15 |
| --- | --- | --- |
| Baseline versus separately compiled baseline | PASS, 2,304 calls | PASS, 2,304 calls |
| Phase-sign negative mutant | Expected FAIL, exit 23, first call | Expected FAIL, exit 23, first call |
| Counter copy versus baseline: status, filter/gain/phase state and stereo samples | PASS, 2,304 calls | PASS, 2,304 calls |
| Complete backend PCM, baseline versus counter copy | PASS, 262,144 bytes identical | PASS, 262,144 bytes identical |

The mutant changes only `phase[i+1]-=(uint32_t)step_i` to addition in a generated
temporary source copy. The phase witnesses were respectively
`d4644800` versus `2b9bb800` and `54277a10` versus `abd885f0`, at call 1,
61 frames. These are witnessed failures, not inferred mutation coverage.

PCM SHA-256 (interleaved little-endian binary32 backend output words):

- Factory 0: `64fd6099abc260f6f857cfc2daa815b8ddf514e8c93777e486c9b37190dc11ad`.
- Factory 15: `4cac17b6b13ccfe5c8733a614e95c4c361e24420533b6faf4bd4c9514fd0b727`.

Baseline/counter source differs only by an observational callback in the pair
loop. No zero-XMOD fast path is implemented or qualified by this assessment.
The two original fixtures were not accessible through this thread's filesystem,
GitHub refs or personal-context reader; the new fixture is explicitly the one
linked above. The prior **996,430-case phase arithmetic evidence is reused as
supplied, NOT_RERUN**. Its original command/artifact has not been independently
authenticated here. Do not substitute this kernel fixture for that arithmetic
receipt or claim the old test was rerun.

Full results, exits, and compiler-discovered source/header SHA-256 identities
are in [results.json](results.json). Existing private table inputs are consumed
locally; this change publishes no table/header bytes, PCM or executable binaries.
Full host suite, sanitizer suite, target builds, timing and device checks are
NOT_RUN in this bounded lane.

## Source and ownership

- Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
- Firmware: `8be16486e33f65b31c76710e1d825b6aa701c2c9`.
- Tree: `f952ab4455723e4a1a68fdb3a8eea22d2da59710`.
- Kernel: `experiments/original_synth/miami_ms20.c`, blob
  `f74384b3f5e8850339fe4d42309b36fc10dd6b3b`.
- Existing base branch at that exact pin: `build/0.0.24.5`.
- Policy: `nightly@66073c68dd80d51a7053432c2c161a8b720a1341`.
- Coordination: `coordination/merge-queue@4a35a6cdaa0f112db6b3f577542138afd170c20d`,
  LX-REQ-023 related to LX-REQ-016.
- Live producer PR [#814](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/814):
  OPEN, `opt/miami-coef-cache-0.0.24@dcc9833f24b20ec2baf47beb131f937e32d95e11`.
  Its producer branch was not changed. Its existing independent reviewer remains
  `independent_qa/6adafbbf54088191a86715d49e7dc6b0`.

The parent Work delegation released assessment/focused tests only. The new lane
owns these test/evidence paths; it does not own #814 implementation, integration
or its independent qualification. Artifact review here is distinct from #814
firmware QA. Native Git's preconfigured author does not match the connected
account, so delivery must use the connector's default author without changing
local identity configuration.

## Handoff to the existing producer

Consume this exact result as a negative applicability finding for default/15.
Keep the zero-XMOD shortcut out of any claimed improvement for these workloads.
If considering it for another accepted workload, first identify actual audible
zero-XMOD use through this counter fixture, then reconcile the existing arithmetic
receipt and obtain the existing reviewer's candidate-specific checks before any
optimization or benchmark. Those later actions are outside this assignment.

PR #814 source/native/package and full-suite gates remain with its existing
producer and reviewer. This report does not close LX-REQ-023 or qualify #814.
The existing producer's first scoped consumption is pending; a posted Git
pointer alone does not prove it happened.

No additional operational monitoring required: production source is unchanged.
