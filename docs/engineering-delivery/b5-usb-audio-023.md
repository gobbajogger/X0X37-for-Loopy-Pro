# B5: USB audio reader progress

Base: `build/0.0.23` at `bb1907494b8d1b52d52ac220dfeb9a0663ea3ee2`.
Source and regression handoff only. The owner explicitly prohibited all builds
and test execution in Codex; the Orchestrator runs them on the box.

## Existing implementation and reason

`6b4ca63fb7d43b9c2f653cf2f2c5503ee41e2b6e` is already an ancestor of this
base. No port is required and no part was dropped. The unchanged USB source
matches blob `fcd961f909c7d405e5a32a25ed1aa950669b8b4f` from the
[independent QA finding on #779](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/779#issuecomment-6541904178).

The mixer consumes ring frames through a local reader index but previously
published that index only at the end of a 256-frame block. Nested TIMER5 receive
could therefore reject incoming samples even after the mixer had freed space.
This change publishes each consumed slot after copying its sample locally,
with the existing compiler memory barrier before publication. It addresses a
possible playback dropout under that interleaving; an iPhone dropout has not
been reproduced in this session.

No descriptor, sample rate, identity string, buffer capacity, engine DSP or
MASTER/tap ordering changes. No new production static storage. Native RAM and
CPU effects remain unmeasured. Protected regions, bootloader, `.github`, Actions,
FM-1 targets, Miami ADSR, and feature rows 73/120 are outside this change.

## Regression and box handoff

`uac-out-irq` is registered in `tests/run_tests.sh`. It executes production
`uac_out_rx` and `uac_out_mix` with a USB register model. The runner instruments
only a temporary source copy after each mixed stereo frame, creating a
deterministic TIMER5 schedule without adding production test hooks.

The six packets `[44, 44, 44, 44, 45, 44]` interrupt one 256-frame render at an
initial fill of 760/1024. Assertions require all 265 received frames, zero false
overrun, unchanged mixed samples, and 769 remaining frames in order. The test
also covers physical/unsigned index wrap, genuine full and partial-full rings,
an empty poll, and underrun. Existing tests and expectations remain intact.

Run these commands on the box from the complete candidate checkout. These are
instructions, not results from this session.

```sh
# RED: candidate regression against the original production USB source.
b5_base_src=$(mktemp -d)
git show bb1907494b8d1b52d52ac220dfeb9a0663ea3ee2:firmware/src/usb.c > "$b5_base_src/usb.c"
SRC="$b5_base_src" sh tests/host/uac_out_irq_test.sh
# Expect a nonzero exit reporting false overrun / a lost incoming frame.
rm -rf "$b5_base_src"

# GREEN and existing USB coverage, then the complete host suite.
X0X_TEST_JOBS=1 sh tests/run_tests.sh uac-out-irq uac uac-out usb-audio-descriptors usb-product-names
sh tests/run_tests.sh

# Verified SDK V1.2.1 and JieLi toolchain paths must already be configured.
X0X_DEV_PADCAL=0 MODEL=elite ./build.sh --board smk37
X0X_DEV_PADCAL=0 MODEL=pro ./build.sh --board smk37
```

Record baseline and candidate static RAM free for both models. The brief's
baseline is 2352 B; any reduction needs a written reason. Also inspect target
audio-loop code/timing because reader publication now occurs per consumed frame.
Preserve all failures/skips. The integration owner retains qualification/merge.

| Check | This session |
| --- | --- |
| New regression, baseline RED and candidate GREEN | NOT_RUN |
| Existing USB tests and complete host suite | NOT_RUN |
| SDK V1.2.1 Elite / Pro, `X0X_DEV_PADCAL=0` | NOT_RUN |
| Static RAM free, Elite / Pro | NOT_MEASURED |
| iPhone enumeration, capture, playback, reconnect and 10+ minute soak | NOT_RUN |

Owner-operated iPhone checks must distinguish enumeration, sample-rate routing,
silence and drift/dropouts on the same installed bytes. Host simulation does not
establish device acceptance. No installation, flash, publication or merge is
performed by this source handoff. Visual: NO; mockup paths: N/A.

## Source review

Compound Engineering correctness, testing, project-standards, performance and
adversarial reviewers found no actionable source defects. All five reviews were
source inspections; no reviewer ran builds or tests. The adversarial pass used
an in-process reviewer because no different-provider CLI was available. This
review does not establish runtime qualification or merge readiness.
