# SMC connection publication repair

## Result

Source commit `a797db8395bb53a0731ba3add82e22028299356d` fixes a reproducible
condition where Bluetooth remains connected but the application permanently
discards SMC controls as disconnected. The repair passed the scoped host checks
and the Elite development validator. It has not been installed or verified on
the physical device.

The latest local input was `ffa7db975523d0ce8242295eae08f4fd5bf93e44` in
`/workspace/scratch/0abc2a1b0f2c/x0x37-smc`. Work is isolated in
`/workspace/scratch/6a6a00c36eaf/x0x37-smc` on
`lane/mb-smc-receive-20261021`. The original checkout and shared integration
branches were not changed.

## History and exact breaking point

| Commit | Finding |
| --- | --- |
| `d96e5c108c542db7829087c08702e99a098a4643` | Introduced the recent queue-slot lifetime regression documented in the previous audit. |
| `6fc256598651a71009518bc1c8ea92c564baa8be` | Already repaired that queue regression before this task. |
| `ffa7db975523d0ce8242295eae08f4fd5bf93e44` | Latest local tip inspected. Still contains the separate link-notification race repaired here. |
| `5b32093cb4c0c884f24b6e09ea493c02fd689f5b` | Initial SMC mapping/ingress commit already contains the unsafe link publication and acknowledgment. |

The older source was retrieved through the connected GitHub repository because
the local history is shallow. The initial
[SMC source](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/5b32093cb4c0c884f24b6e09ea493c02fd689f5b/firmware/src/app/smc_mixer.c)
has blob SHA `638a17c038e574e5ce87174602950abe49c8a160`. The same defect is also
present at the previously working revision
`53d5654c9bbaec10b64c55fb60d0e7d857ba8156`. This is a latent race, not evidence
that the latest project-management commits introduced this second defect.

The failure is in the handoff from `smc_ble_link()` to `smc_poll()`:

1. The main loop reads an outstanding disconnect from `smc_link_pending`.
2. TIMER5 interrupts and publishes a reconnect to the same variable.
3. The main loop resumes, clears the variable, and applies its older disconnect.
4. `ble_central_link_notify()` already recorded the reconnect. It publishes only
   changes, so subsequent connected notifications do not repair the lost edge.
5. The transport remains connected while `smc.up` remains zero. Later fader,
   knob, and button events are consumed and discarded with reason code 2.

The reverse interleaving can leave the application incorrectly connected after
transport disconnection. Both directions reproduce at two interrupt points in
the production poller.

## Repair

`firmware/src/app/smc_mixer.c` now uses a publication word written only by the
interrupt producer and a separate acknowledgment written only by the main loop.
The publication includes a generation and the final link state. The consumer
acknowledges only its snapshot and never clears a newer interrupt publication.
The next poll therefore sees a link change that interrupted the current poll.

The generation preserves reconnect handling when down/up events coalesce to
the same final state. Unsigned wrap, including a zero-valued disconnect
publication, is supported. Project reset preserves both publication and
acknowledgment. Existing coalescing behavior remains: this does not promise
delivery of every intermediate link edge.

The change adds no IRQ masking or API changes. It preserves the previous queue
repair, mapping, pickup, radio, controller, model, sample, and loader behavior.

## Verification

| Check | Result and scope |
| --- | --- |
| Exact latest-source negative control | Four of eight cases fail on `ffa7db9` at the intended stale-link assertions; the four ordinary cases pass. Reconnect failures report `transport=1 application=0 dispatched=0 reason=2`. |
| New `smc-link` regression | All ten cases pass with ASan and fail-fast UBSan: both interruption sites and directions, stable pickup, coalesced edges, project reset, solo cleanup, and both wrap states. |
| Existing `smc-queue` regression | All 24 ISR interleaving cases pass with ASan and fail-fast UBSan. |
| Existing `smc-reachability` | All five real-UI routes pass navigation, editing, pickup/frame, confirmation/help, and clip checks. This script does not apply the optional sanitizer flags. |
| Standard `project-runtime` | Reset, load, cancel, invalid project, transfer, save, sample preservation, queue discard, lock/abort/solo/pickup checks pass. |
| Independent source/test review | Approved, with no blocking findings. The reviewer did not repeat tests or builds. |
| Elite development validation | `DEV CHECKED` for exact source commit `a797db8395bb53a0731ba3add82e22028299356d`. Record/package verification also passes. |

The new test compiles the actual production link notifier, producer, queue,
project-reset helpers, and poller together. UI/platform stubs inject an
interrupt after the poller's link snapshot. It checks the exact control bytes
at the dispatch boundary; it is not a Bluetooth hardware test. The test is
registered in `tests/run_tests.sh` as `smc-link`.

The optional sanitized `project-runtime` run did fail at the unchanged
`firmware/src/dsp/fastmath.h:57`: a left shift of negative `-1` in `fm_exp2_nf`.
The header is byte-identical to the baseline, and a minimal compile of that
baseline header reproduces the failure under UBSan. The failure and its
baseline reproduction are retained; no sanitizer PASS is claimed for that
project run. The supported standard project-runtime configuration passes.
This unrelated DSP code was not changed.

Successful ASan runs use `ASAN_OPTIONS=detect_leaks=0` because LeakSanitizer
cannot inspect the necessary process state in this sandbox. No leak-check
PASS is claimed.

The target validator reused completed host results after checking all 431
source inputs, the compiler identity, and nine retained log hashes. Commands,
environment, hashes, and failure disposition are recorded in
[host-evidence.json](../../evidence/smc-link-20261021/host-evidence.json).

### Elite development build

Configuration: SMK-37 Elite, development, `X0X_DEV_PADCAL=1`.

| Resource | Used | Limit | Free |
| --- | ---: | ---: | ---: |
| Application code | 544,352 B | 552,672 B | 8,320 B |
| Static RAM | 97,796 B | 98,304 B | 508 B |
| Pool | 335,740 B | 344,064 B | 8,324 B |

The required margins pass. Compared with the preceding queue-fixed build,
application code grows by 80 B and static RAM by 16 B; pool usage is unchanged.
Package SHA-256:
`4234c1813775046b0b4250a50c056f9406893f547972f5edd3fc40c1b693903d`.

The unchanged controller was not selected as a new artifact. This shared-source
change used the project-required Elite development scope. The retained
[development record](../../evidence/smc-link-20261021/elite/record.json),
[build log](../../evidence/smc-link-20261021/elite/build.log), and
[host reuse log](../../evidence/smc-link-20261021/elite/check-0.log) refer to the
checked source commit above. A later evidence-only commit does not claim a new
target build.

## Physical-input boundary

This repair establishes a source-level persistent input blocker. It does not
establish the cause of every historical report of silent SMC controls. The same
`smc.up` gate also stops new `smc_frame()` LED feedback. Previously lit LEDs can
remain lit, but a session with demonstrably continuing LED changes needs a
different explanation.

The retained idle device trace contains no control event that establishes a
failed receive boundary. Existing capture replays exercise decoding and
dispatch; they are not proof of current physical reception. No Mac-local
session or physical device is exposed here. The earlier full-silence report
therefore remains unverified on hardware. No new manual capture is requested.

## Integration handoff

Receiving owner: the existing sole firmware integration owner.

Use source commit `a797db8395bb53a0731ba3add82e22028299356d` as the repair input
and preserve the preceding queue repair. Compare it with the actual current
integration revision and any newer SMC changes before applying it. Reuse checks
whose inputs match; repeat only checks invalidated by integration. Development
evidence must match the resulting production inputs before an authorized
installation.

Continue any physical-symptom disposition in the already established Mac-local
session using its existing evidence. Do not infer input loss from idle zero
counters, require another hands-on control capture, or call this host repair a
device PASS. The source, scoped tests, and checked development evidence are
ready for the existing integration owner. No shared branch, external push, or
device write was performed here.
