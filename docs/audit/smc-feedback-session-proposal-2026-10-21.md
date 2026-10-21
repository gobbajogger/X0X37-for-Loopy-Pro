# Source-only SMC feedback-session proposal

Build: isolated `lane/btr-feedback-session-audit-20261021`, based on `75c0872`.
Replaces: no active kit, package, receipt, physical HEAD, or installed firmware.
Checked against decisions: authorized cloud source repair and no-hardware checking;
application/controller ABI changes require both target builds and independent review.

## Defect and repair

Old application-to-controller feedback had no link identity. An old ring frame or
BUSY retry could survive a disconnect and be sent to the next peer. Draining one
frame on disconnect did not fix backlog or a delayed producer. This is an LED/fader
feedback defect, not evidence of the original missing-input cause.

The proposal uses explicit, incompatible contracts:

- The segmented manifest remains layout ABI 4, but its contract is `0x534D4303`.
  Both the app admission helper and a rebuilt symbolic loader use that contract.
- The copied-message boot/mailbox ABI is **5**, not 4. `xsm_boot_t` grows from
  96 to 100 bytes, appending named `feedback_epoch` at offset 96. Reserved bytes
  are not repurposed. A base ABI check precedes extension access.
- `LINK_SESSION` (`0x75`) contains little-endian epoch, up, reason (6 bytes).
  `MIDI_SESSION` (`0x76`) contains little-endian epoch and 1–3 MIDI bytes.
  Legacy untagged link/feedback frames are rejected in this profile.
- The controller allocates one nonzero epoch per accepted new connection, not
  per repeated UP publication. It preserves the counter across controller-only
  restart, uses `UINT32_MAX` once, and then refuses further sessions rather than
  wrapping. Only coordinated app startup, with the controller stopped and both
  rings cleared, initializes the counter to zero.
- Pending/ring feedback must match both the shared current counter and the
  profile's owned connection epoch. The app captures identity before constructing
  feedback in `smc_frame`, not merely inside the eventual send function. Main-loop
  feedback state must acknowledge that epoch; the old untagged vendor send API
  fails closed. Epoch acknowledgment also prevents the older link-generation
  counter's wrap from reusing stale feedback state.
- Radio desired state still takes precedence over a pending retry or queued ON.
  At epoch exhaustion, disconnecting an unadmitted zero-epoch link cannot erase
  the previous valid link's pending DOWN under ring pressure.

The raw-HCI ABI-2 runtime, musical mappings, sample quality, RF settings, storage
layout, and memory limits are unchanged. The larger feedback frame reduces the
1023-byte ring's maximum full 3-byte-message backlog from 170 to 102 frames;
backpressure remains explicit, with no silent overwrite.

## Verified checks

`sh tests/ble/run_ctl_vendor_scheduler.sh` covers the actual controller profile
and bridge, plus extracted unchanged app service/sender/producer bodies with
native HAL stubs. Ten integrated modes cover queued frames, BUSY retry, normal
drain, deep backlog, delayed producer, duplicate completion, DOWN pressure,
UP-success/notification-full retry, controller-only reset, and exhaustion pressure.
Shared checks also cover handle zero, wrong handles, fresh 1–3-byte feedback,
legacy/invalid frames, authoritative OFF, full app reset, and counter saturation.
Address/undefined-sanitizer checks also pass; leak detection is not claimed
(the executor cannot run LeakSanitizer under ptrace). Protected-page boot tests reject an old ABI or short extension before accessing
the new field. These are ordinary source callbacks, not proprietary HCI replay.

The independent reviewer reproduced an additional pre-entry producer race: a
reconnect between MIDI construction and sender entry could retag the old message.
The repaired actual producer passes that counterexample; its durable regression
is `tests/ble/check_feedback_producer.py`.

Additional passing checks:

- Actual full vendor parser → copied SMC queue → UI dispatch, including epoch
  acknowledgment when the older generation aliases:
  `X0X_HOST_DEFS='-DX0X_SMC_VENDOR_TEST=1' sh tests/host/smc_vendor_dispatch_test.sh`.
- `python3 tests/host/smc_feedback_test.py`: 1838 default-profile expectations.
- `python3 tests/host/smc_link_test.py`: 10 existing link/reset/wrap cases.
- `python3 tests/ble/check_smc_profile.py`: selector and model refusals.
- Native segmented-manifest tests and actual target-ELF fixture validation:
  bad old contract is rejected before any segment read; the fixture sealer rejects
  old raw controller ABI 4 rather than relabeling it as the new contract.
- Historical bundle unit tests still pass. Its contract-2 constructor rejects
  the new contract-3 fixtures. Existing retained ELF/source/region pins are intact.

## Target sizing and reproducibility

The application uses the existing non-installable `prepare.py`,
`prepare_admission.py`, and `prepare_bridge.py` export recipe, with Elite DEV
configuration and the verified native toolchain/SDK inputs. The controller uses
`tools/studies/smc_reference/link_runtime.py` with:

```
--controller-roles central --avctp-extra-commands 0
--placement data-only-92k-ramload --high-cpu-bss --cpu-init-pool
--no-classic-key-sniff --no-classic-test-modes
```

Both sides were built against an exact `75c0872` baseline as well as the proposal.
Input/source hashes and final sizes are in the adjacent JSON evidence record.
Private logs and non-installable target files are under
`/workspace/scratch/61c037da3f34/feedback-session-validation/`.
No SDK, toolchain, target binary, or raw capture is committed.

The final app is 521,732 bytes versus 521,428 baseline: **+304 bytes**, with
2,268 bytes below the 524,000-byte profile boundary. Main data+BSS is 98,116
versus 98,100 bytes: **+16 bytes**, with 188 bytes remaining. Pool use is unchanged
at 335,484 / 344,064 bytes. The final controller primary is 93,984 versus 93,744 bytes: **+240 bytes**,
with 224 bytes remaining. Low RAM stays 26,352 bytes; high CPU BSS grows from
6,452 to 6,460 bytes. This primary margin is particularly small and must be
rechecked after integration. Independent source-only review is clear after the
producer-entry and exhaustion-pressure repairs.

## Deliberately not admitted

This is not an installable or package-ready candidate. The historical bundle
constructor, retained-controller pins, active kit, receipts, and selected physical
HEAD were not updated. No paired-loader/package build, install, radio operation,
live heap/stack test, or hardware control test is claimed. The app target is an
unsealed study image, and controller output is a non-installable link/fixture.

The receiving integration owner must combine this source proposal with the
separate security lane, independently review the combined contracts and all
package/receipt consumers, rebuild both sides and the paired loader, and satisfy
all existing sizing and release gates before considering a package. These two
isolated target builds do not establish the combined candidate's fit.
