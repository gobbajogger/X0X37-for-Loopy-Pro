# Bounded cold-code compiler-size discriminator

**Only 16 B was recovered. No additional 4 KiB sector is available from the
measured change.** The full-feature Elite application was built from baseline
`816e0d3987bcb69be972cff2cd4df4469969f196` in isolated scratch with one change:
compile `seq/pattern.c` with `-Oz` instead of `-O2`. No BLE removal, inert API,
feature omission, sample/pool change or hot-path flag change was made. This is
an unsuccessful fit discriminator, not a firmware recommendation.

## Eligible scope

`tools/build.py` already compiles the monolithic `firmware/src/x0x.c` at `-Oz`.
That unit includes UI, project/configuration, storage, editor, panel, USB,
Bluetooth and SMC routing. Its leading source comment still says `-Os`; the
actual compile invocation is authoritative. Several UI/clip/chord helpers also
already use `minsize`/`noinline`. Engine initialization/reset helpers already
have `X0X_COLD` (`minsize`).

The separate `O2_UNITS` are DSP, sequencer, TB3PO, motion and engine. Bass303 is
already separately selected for `-Oz`. Those units contain audio/sequencer hot
paths and were not retuned. In particular `tb3po_mutate` is called by
`sequencer.c:auto_mutate` during step firing, so the whole TB3PO unit is not a
cold UI-only unit. No broad no-inline/LTO refactor across mixed hot/cold code was
attempted.

One eligible separate cold unit exists: `seq/pattern.c`, containing only
`pattern_init`. It sets pattern defaults and is called from project defaults,
project loading and UI pattern initialization. Its baseline target code is only
208 B; even removing it entirely could not recover a 4 KiB sector.

## Actual full-feature target results

| Measurement | Baseline | Pattern-only `-Oz` |
|---|---:|---:|
| App image | 548480 B | 548464 B |
| App code slack | 4192 B | 4208 B |
| Main RAM | 97972 B | 97972 B |
| Pool | 335740 B | 335740 B |
| Pattern function code | 208 B | 168 B plus 12 B rodata |

The final linked image—not just function-symbol size—saves 16 B. Alignment and
constant placement prevent simply counting the 40-byte function reduction as
net savings. The build's model, memory, register-access and other scoped checks
pass unchanged. Loader output remains 7280 B. No controller rebuild was needed
or performed for this app compiler probe. Its generated package was not
installed, distributed or recommended.

Measured application SHA-256:
`e518d1db623083b0a0058e625028aaddebce147275dfda8d0f3b2ee039094280`.
Pinned JieLi clang SHA-256:
`c45189ee624de0430deb0db02e6dce171fb5538e7fe323e3964dc8ac6236240f`.

The parent's separate fully resolved vendor-host study reports 108708 B, before
segmented packaging admission. The optimistic 106496-byte controller capacity
already leaves a 2212-byte deficit before further manifest/adapters. This 16-byte
app reduction does not resolve that deficit or create a sector. It is not
permission to change another feature or hot-path optimization.

## Reproduce without editing the integration checkout

Set `REPO`, `SCRATCH`, `JIELI_TOOLCHAIN`, `AC79_SDK`, and `SDK_LOCAL` to verified
local paths. `SCRATCH` must be a new private directory outside the repository.
The SDK/toolchain stay private; the committed patch contains only the one
compiler-selection change.

```sh
mkdir "$SCRATCH"
git -C "$REPO" archive 816e0d3987bcb69be972cff2cd4df4469969f196 |
  tar -x -C "$SCRATCH"
(cd "$SCRATCH" &&
  git apply --check "$REPO/tools/studies/smc_cold_flags/pattern-oz.patch" &&
  git apply "$REPO/tools/studies/smc_cold_flags/pattern-oz.patch" &&
  X0X_DEV_PADCAL=1 CC=cc CXX=c++ python3 tools/build.py --board smk37 --model elite)
```

The baseline reference is the runtime owner's already freshly verified build
of the exact same source and inputs, not an independently repeated build in this
helper lane. The changed full-feature app build is fresh. Scratch log was
`smc-cold-size-study/pattern-oz-build.log`; no proprietary binary is committed.

## Functional check and limits

The host comparison compiles the same initializer separately with `-O2` and
`-Oz`, changing only its symbol name in the latter, then compares every byte of
all initialized pattern fields for 4096 seed pairs. It passes with Debian GCC
14.2.0. Reproduce from the repository root, writing temporary outputs outside it:

```sh
cc -O2 -std=c11 -Ifirmware/src/seq -c firmware/src/seq/pattern.c -o /tmp/pattern-o2.o
cc -Oz -std=c11 -Ifirmware/src/seq -Dpattern_init=pattern_init_size_probe \
  -c firmware/src/seq/pattern.c -o /tmp/pattern-oz.o
cc -O2 -std=c11 -Wall -Wextra -Werror -Ifirmware/src/seq \
  tests/host/pattern_size_equivalence_test.c firmware/src/seq/tb3po.c \
  /tmp/pattern-o2.o /tmp/pattern-oz.o -lm -o /tmp/pattern-equivalence
/tmp/pattern-equivalence
```

This host comparison is not target execution, timing evidence or hardware
validation. Because there is no useful sector gain, no production compiler
change is proposed and no broader optimization search is justified by this
bounded task. Parent remains the sole runtime/layout owner.
