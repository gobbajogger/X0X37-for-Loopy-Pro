# Retry transient failure when retiring an owned SMC link

## Defect and scope

At source `4cf8c2f`, failed ATT initialization, service search, CCCD submission,
or missing CCCD requests a disconnect through `retire_connection`. If that
command returns SDK BUSY (-100) or BUFFER_FULL (-97), ownership is retired but
the command is never retried unless the user separately selects Bluetooth OFF.
Input, feedback and security correctly remain blocked, while the still-owned
connection prevents scanning. This is a reproducible adapter recovery defect,
not a demonstrated cause of any historical keyboard failure.

The regression fails on the unchanged source at the first expected scheduler
retry. The repair schedules only those two transient statuses at least 20 ms
apart, with at most one disconnect command per poll. An accepted command is
not repeated; only an actual matching link-down event clears ownership and
permits a new scan. OFF does not enable scanning. A later ON does not cancel
cleanup of the retiring connection.

Retirement remains active before calling the vendor, so late input, feedback
and JustWorks requests cannot revive that connection. Unrelated peer cleanup
cannot change the current handle or its retry deadline. Terminal SDK errors
remain explicitly failed and blocked; they are not silently retried or
reported as successful disconnect. This patch does not recover every possible
stack error or add a timeout when an accepted command never produces link-down.

## Fresh checks

- The expanded `tests/ble/ctl_vendor_profile_test.c` fails on the unchanged
  `4cf8c2f` source and passes on the repair with `-Wall -Wextra -Werror`.
- Four setup failures times two transient statuses cover ATT initialization,
  search submission, CCCD submission, and missing descriptor. They include
  paced retries, 32-bit time wrap, OFF, OFF then ON, repeated OFF after accepted
  disconnect, late/unrelated events, terminal refusal, and a new connection.
- The profile passes AddressSanitizer and UndefinedBehaviorSanitizer with
  `ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1`. LeakSanitizer was
  not run because this executor uses tracing.
- `sh tests/ble/run_ctl_vendor_scheduler.sh` passes all scheduler, event,
  profile, high-CPU BSS and CPU-init scope checks. `git diff --check` passes.
- The receiving root independently reviewed the source and compiled the
  expanded regression; its unchanged-source negative control also failed.

Run the native profile alone from the checkout root:

```sh
cc -std=c11 -O1 -g -Wall -Wextra -Werror \
  tests/ble/ctl_vendor_profile_test.c -o /tmp/ctl_vendor_profile_test
/tmp/ctl_vendor_profile_test
```

## Fresh controller-only target link

The diagnostic link uses the exact retained owned-security recipe: central
role, AVCTP extra commands 0, `data-only-92k-ramload`, high-CPU BSS, CPU-init pool,
no Classic key/sniff and no Classic test modes. Tool, library, RF and recipe
hashes match the retained build. The only changed compiled source is
`ctl_vendor_profile.c`. The toolchain is the existing pinned JieLi clang 4.0.1.

| Region | Baseline | Repair | Capacity |
|---|---:|---:|---:|
| Primary flash | 93,604 | 93,744 | 94,208 |
| Read-only flash | 7,272 | 7,272 | 8,192 |
| RAM-load flash | 3,524 | 3,524 | 4,096 |
| Low controller RAM | 26,352 | 26,352 | 26,368 |
| High-CPU RAM | 6,448 | 6,452 | 7,168 |

All sizes are bytes. The link resolves every import. Total loaded bytes rise
from 104,400 to 104,540. Low RAM still has only 16 bytes of static margin; this
does not establish runtime stack or heap high-water. The build remains a
non-installable diagnostic with a manifest placeholder, not a DEV package.

## Delivery limits

No application rebuild, package repinning, installer change, hardware access,
proprietary HCI replay, sample change or DSP change was performed. The pinned
8145957 kit and its pending installer transaction remain unchanged. Any later
integration must rebuild and repin the exact combined controller and rerun
affected package admission; this patch does not authorize reuse of the old
controller identity. Independent CCCD-completion work is not included here.

Private local evidence is under `overnight-x0x-triage/`: the baseline/fixed/
sanitized profile logs, `vendor-scheduler.log`, `retirement-target.log`, and
`retirement-target/result.json`. Proprietary objects, libraries and ELF are
not committed or distributed with this source audit.
