# VCA decay skip recovery

Base: `a06e20189141ff5fb8766e2c6152be460f4356c3` (`wip/one-build`).
Lane: `lane/recover-vca-decay-skip-20261106`.
Recovery source: `handoff/local-codex-recovery-20261106`, tree
`2abf039d503e77c0c15ee71188a471e019fc3bb7`,
`docs/handoff/local-codex-recovery/LOST-WORK.md` (lost ZL-B `e7b18d75`).

The recovered optimisation skips the decay coefficient, zero delta and division
when a state-1 VCA level equals clamped sustain. It keeps `(q + 1.0f) - 1.0f`
at equality. This rounding can change an arbitrary sustain such as `0.1f`;
returning its unrounded value would change envelope output. Below-sustain reset,
unequal decay, input validation, state and action behaviour remain unchanged.
Sustained notes avoid redundant control-tick work without added voice state.

## Native development builds, run here

Linux x86-64, JieLi clang 4.0.1, official `jieli-linux-toolchains-20260730.1`.
Downloaded archive SHA-256:
`a1545cddbc451a06bac43eb5cd96d831470c42a97c5c0b1f98879a20fc7647bd`.
SDK V1.2.1 commit `d179b4484759423312073f5fbb232501aa491047`;
all three package inputs and eight controller inputs match committed SHA-256 pins.
`X0X_DEV_PADCAL=1`; no release flag.

| Metric (bytes, each model) | Base Elite / Pro | Candidate Elite / Pro | Delta |
| --- | ---: | ---: | ---: |
| App image | 474840 | 474856 | +16 |
| VCA function text | 592 | 608 | +16 |
| App RAM | 95780 | 95780 | 0 |
| `.ram_text` | 2712 | 2712 | 0 |
| Pool usage | 335464 | 335464 | 0 |
| Package size | 651457 | 651457 | 0 |

App RAM limit: 98304 bytes; headroom: 2524 bytes.
Pool headroom: 8600 bytes (required >=8192).
App code headroom: 77816 bytes.
`.ram_text` remains 844 decoded instructions with no calls (builder check).
Controller: PASS, 61088 / 65536 bytes, RAM 26336 / 26368 bytes.
Cycle savings, device timing and use of the approximately 7256 cycles/sample
budget at 44.1 kHz / 320 MHz were not measured. Host results are not device proof.

Reproduce in this workspace with verified inputs:

```sh
export JIELI_TOOLCHAIN=/workspace/work/inputs/jieli-linux-toolchains-20260730.1
export AC79_SDK=/workspace/work/inputs/sdk-v1.2.1
export SDK_LOCAL="$AC79_SDK"
export X0X_GEN_CACHE=/workspace/work/gen-cache
export X0X_DEV_PADCAL=1
python3 tools/build.py --board smk37 --model elite
python3 tools/build.py --board smk37 --model pro
CTL_NATIVE=1 CTL_NO_CACHE=1 sh ble/ctl/build_ctl.sh
```

The exact base was built separately with the same compiler, SDK and flags.
All binaries remain local and are excluded from this delivery.

## Host proof

The new executable regression failed on the base because equal sustain still
called the coefficient helper. After the fix, it passes 186766 bit-exact
comparisons, including equal/adjacent levels, subnormals, signed zeros, decay
clamping, changing sustain, and representative sample rates and durations.
It asserts that the real coefficient helper is not called at equality.

Independent read-only review: no Critical, Important or Minor findings.
The reviewer also ran the comparisons under the four standard rounding modes
and checked sustain clamping and mixed signed zeros. Floating-point exception
flags are not preserved by omitted arithmetic; the audio contract does not
promise those flags.

Full-suite result: **PASS**, 333 selectors, 0 FAIL, 0 SKIP,
0 NOT_RUN, exit 0. Strict preflight: 16 PASS, 0 FAIL.
Final suite log SHA-256:
`dad3a0d6bc38e2cc1a6d1e48ab590fe61863aafc1a73bc463414b02561e7655a`.

Checked VCA source SHA-256:
`42eeb8e5c90eecdbaed9d13e3b70f7c2e7ed16f23cc2a8f23110b96a459535af`.

The first suite attempt exposed Node child-process `EPERM` under the restricted
sandbox and was interrupted. A Node spawn probe and the unchanged failing
installer selector pass with the supported network capability enabled. The
full run uses that capability; no test expectation was weakened.
The initial native attempt hit an unwritable default generation cache; the
explicit workspace cache above resolves it without a source change.

Reference inputs are the existing release pins: schwung-9W9 `10cbe5c`
(from synthfr), schwung-8W8 `94aa271` and schwung-tb3po `f49d81d`
(from michaelmenesescs). All sources came from those immutable archive refs.

```sh
export NINEW9_DIR=/workspace/work/inputs/schwung-9W9
export EIGHTW8_DIR=/workspace/work/inputs/schwung-8W8
export TB3PO_REF=/workspace/work/inputs/schwung-tb3po/src/dsp/tb3po.c
X0X_STRICT_PREFLIGHT=1 X0X_TEST_JOBS=4 sh tests/run_tests.sh
```

Source changes are limited to the VCA function, its new regression and suite
registration, plus this sanitised evidence. No protected branch, `.github`,
flash area, device, binary, token or raw-capture change is part of this delivery.
Integration and hardware acceptance remain with the existing owner.

## Artifact identities (metadata only)

- base elite app SHA-256: `47c8d02545a6d3138ece9dba260c946a1eb251968f7bf7849c397304e43735dd`.
- base pro app SHA-256: `d6f4a728ff649b2d5d4d69cf9c2f1c33d429924a184dfb05028fba405aad3992`.
- candidate elite app SHA-256: `dfdcb9ac89fa552e48b3fde56dd2d94d437bbbc333aa6b2ea342fdfa313bef75`.
- candidate pro app SHA-256: `aeeac4dab293afb989ad73f5c87173dbccc567d62982f4edf2127f9945d85f8d`.

Both models and both revisions have identical `.ram_text` SHA-256:
`52705d9683efd8ba671487161c9f1d73a5305bfe5a66d3dd0570b9432f33a00c`.
