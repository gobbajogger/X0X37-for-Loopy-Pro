# SMC input investigation and queue repair

## Result

A reproducible input-loss regression is repaired in local commit
`6fc256598651a71009518bc1c8ea92c564baa8be` on
`lane/mb-smc-input-20261021`.

The repair passes the affected host checks and the Elite development validator.
It is not installed. The older report of all SMC controls remaining silent while
LED feedback works is still unresolved; the available evidence does not prove
that this queue regression causes that symptom.

## Repository and latest commits

The accessible local source was
`/workspace/scratch/3c13c8e69cb3/x0x37`, clean on
`lane/mb-projects-20261021` at
`6e458c9d51be010f1573e08bda1dfadb985f71d9`.

The isolated repair repository is
`/workspace/scratch/0abc2a1b0f2c/x0x37-smc`. The source repository remains clean
and unchanged at its original revision. No shared integration branch was modified.

Relevant recent commits:

| Commit | Relevance |
| --- | --- |
| `d96e5c108c542db7829087c08702e99a098a4643` | Added project-reset/load runtime actions and introduced the queue lifetime regression. |
| `30b6b17` | Added project-transaction guards for queued updates. It does not repair the SMC slot lifetime. |
| `bce13aa` | Preserved the save window before performance playback. It does not change SMC transport. |
| `6e458c9` | Latest available local tip; records project-management validation. |

The local history is shallow at `a5cb24d`. GitHub's default branch was separately
observed at `f8420eb3749c03e4ba2e24a0fb6ba7b97f627f97`; the local project-feature
branch contains later development work. That remote check did not replace the
requested local audit.

## Exact defect

In `smc_poll()`, `d96e5c1` moved each consumer's read-index advance before the
queued event was read. Advancing the index was necessary because an action can
reset the project and discard the remaining queue. However, it also gave the
interrupt producer permission to reuse the current slot too early.

When a ring is full:

1. The consumer advances its read index.
2. An interrupt writes the next incoming event into the newly released slot.
3. The consumer reads that slot and dispatches the replacement event.
4. The original event is lost. The replacement can be dispatched again at the tail.

This affects the direct Bluetooth SMC queue, the USB/peripheral-Bluetooth User
queue, and both MMC producer queues.

The repair copies the complete event to local storage before publishing the
read-index advance. Compiler memory barriers retain that ordering. Dispatch
still follows publication, so a reset can discard the remaining events without
the consumer restoring an obsolete read index afterward.

No MIDI mapping, preset, pickup rule, radio policy, controller source, queue
capacity, sample, model guard or memory limit changed.

## Verification

| Check | Result and scope |
| --- | --- |
| Exact-base negative control | All 24 cases fail on `6e458c9` at the intended event-overwrite assertion. |
| Repaired queue regression | All 24 cases pass with AddressSanitizer and UndefinedBehaviorSanitizer. |
| `project-runtime` | Project replacement discards stale SMC input; lock, abort, solo and pickup behavior pass. |
| `smc-reachability` | Complete navigation, control edits, pickup/frame, confirmation/help and clip paths pass. |
| `smc-performance-abort` | DAW, direct User MMC, host User notes and both host MMC sources retain abort/restoration behavior. |
| Independent source/test review | No blocking findings. The reviewer did not repeat the test suite. |
| Elite development validator | `DEV CHECKED` for exact commit `6fc256598651a71009518bc1c8ea92c564baa8be`. |
| Exact record/package verification | Passed against that revision and the generated package. |

The queue regression uses production queue declarations, producers, discard
helper and consumer. It injects the ISR immediately after publication, which is
a valid interrupt schedule. It checks all four producer routes at initial
counters 0, 240 and 255, both with and without reset discard. The incoming BLE
replacement has three bytes while the displaced original has six.

The first sanitizer attempt failed because LeakSanitizer cannot inspect
`/proc` in this sandbox. Its log is retained. The successful retry keeps ASan and
UBSan enabled with `ASAN_OPTIONS=detect_leaks=0`; no leak-check PASS is claimed.

Completed host results were reused for target validation only after checking
339 source inputs, the host compiler identity and seven retained log hashes.
The strengthened final queue test has its own successful sanitizer result and
exact-base negative control. The production repair did not change afterward.

### Elite development artifact

| Resource | Used | Limit | Free |
| --- | ---: | ---: | ---: |
| Application code | 544,272 B | 552,672 B | 8,400 B |
| Static RAM | 97,780 B | 98,304 B | 524 B |
| Pool | 335,740 B | 344,064 B | 8,324 B |

Configuration: SMK-37 Elite, development, `X0X_DEV_PADCAL=1`.
Package SHA-256:
`8b7c590f63b22b9b61610e11ab3d42fba271e6261417d2cb2ea8793b86c1e9af`.

The unchanged Bluetooth controller was not rebuilt or selected as a new
artifact. No Pro-specific input changed, so this shared-source check used Elite
under the current project policy. No release build or full gate ran.

## Original complete-silence report

The latest retained installation record concerns `a5cb24d`, before the new
project-reset queue regression. It explicitly leaves physical SMC input
unverified and the original failure unresolved. Therefore this repair cannot
be claimed as the established cause of that earlier failure.

The earlier physical trace was idle: connected, subscription response received,
no decoded MIDI events, no mailbox drops. It contains no actual control event
that can be traced through a failing boundary. The preserved raw BLE capture
and simulated HCI replay exercise decoding but do not establish current
physical reception. The existing transport source correctly connects its MIDI
callback to the SMC queue; no further transport mutation was justified by the
available records.

The supplied references `01a15b4d-a8bb-7232-95d5-d7ba37b574c6` and
`01a15b53-81fa-7f90-9452-231d6f6b1f35` could not be retrieved through this
session. No Mac-local execution or keyboard connection is exposed here.

## Integration handoff

Receiving owner: the existing sole firmware integration owner.

Use the checked source commit above as the repair input. Review it against the
actual current Mac-local revision and the two supplied threads before any
integration. Preserve other lanes and any newer SMC work. Reuse matching checks;
repeat only checks affected by changed inputs. A merge onto a different
production tree needs matching development evidence before installation.

Continue the original-silence investigation in the Mac-local session with its
existing post-install evidence. Do not request another manual control capture
or infer receive failure from idle zero counters. No speculative handle,
preset, channel or radio change is part of this repair.

Current authorization covers this requested diagnosis and source repair with
scoped development checks. Preserve the current integration and installation
ownership and any applicable existing permissions. No external push,
publication, release, device write or competing integration was performed here.

Read the current task, workflow, device-bug, check, integrate and writing skills
at the receiving repository's `skills/x0x37-*/SKILL.md` entry points. Initialize
the install skill only if a separately available and authorized device operation
is performed. The receiving completion condition is an integrated checked
repair and an evidence-based disposition of the original input failure; host
replay alone is not a physical-device PASS.

Evidence: [host result](../../evidence/smc-input-20261021/HOST_RESULT.txt),
[host identities](../../evidence/smc-input-20261021/host-evidence.json), and
[development result](../../evidence/smc-input-20261021/elite/record.json).
