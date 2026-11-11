# Disabled capture: caller pre-admission

LX-REQ-039 continuation on merged PR674 (`8f277ca86c88659f5bf7034aed33edaec0866371`).
Root confirmed no overlapping source writer for this isolated current-nightly
storage/OTA/editor/main-loop scope on 2026-10-28. Historical UI/Settings branches and
denied descendants are not used or modified. Acquisition remains UNSUPPORTED;
this is a source prerequisite, not enabled capture or normal NIGHTLY MBOOT installation.

## Actual changes

The earlier low-level gate could refuse flash while a caller had already changed
its staging buffer, deadline or logical operation state. This stage stops those
existing operations before that work:

- Production `x0x.c` wires explicit admission hooks into storage, OTA and editor.
  Standalone existing host modules retain their no-maintenance default hooks.
- `st_save`, `st_load` and `store_stage_lock` refuse before scratch/destination
  changes, lookup reads or lock ownership changes. A denied operation revokes the
  held observation but never unlocks another owner.
- `ota_session` returns -20 before deadline, staging, cleanup, UI feedback or reset.
  `ota_service` consumes a refused pending handshake rather than replaying it after
  release. Existing package/official-head/range validation is unchanged.
- Editor service checks the held state before project timeout, boot housekeeping,
  copying frames or dispatch. Existing-protocol frames received while held are
  consumed and the observation revoked; none is queued for later execution. Direct
  editor dispatch also refuses before building a reply or invoking a command.
  No success or invented BUSY payload is inserted into a command's unrelated format:
  old-protocol callers receive no operation reply and may time out. A future live
  UI/protocol must explicitly represent maintenance; this is not that implementation.
- Both normal and safe main loops poll maintenance immediately after watchdog feed
  and USB retry, before normal UI/input/storage/update processing. While held they
  use only the editor's rejection branch and clear/revoke explicit OTA/UBOOT/delayed
  reboot requests. Normal UI/housekeeping does not run. Idle held polling does not
  itself revoke an otherwise valid lease. With no lease, the helper returns without
  consuming a frame or changing those request flags; ordinary processing proceeds.

This is a **pre-admission backstop**, not a complete runtime maintenance mode. No
capture command is exposed, and there is no acquisition call in production. A future
reader must be serviced by a separately reviewed dispatcher before old-protocol
rejection; simply enabling acquisition now would not provide a functioning capture
path. Its admission must first reject/drain pre-existing project/sample/boot/OTA
operations and qualify quiescence, user-input lifecycle, controller/core1/SDK closure,
raw identity/session and timing. This patch does not claim to stop interrupts,
maintain active playback or prove such quiescence. Reset and power loss invalidate
sessions as before; no boot writer or recovery route is added.

## Fail-before / pass-after proof

```
python3 -B tests/host/mboot_maintenance/check.py --output /tmp/mboot-maintenance-proof
```

The actual old base storage/OTA sources compile against the same new pure gate and
harness, then fail the held `st_save` refusal assertion: old code changed state rather
than refusing. This expected negative is retained, not counted as a candidate failure.
Actual candidate storage/OTA passes normally and under ASan/UBSan: no scratch,
destination, stage ownership, deadline, flash, UI or commit side effects while held;
inactive save/load and ordinary OTA admission still work.

A separate sanitizer harness executes the exact editor pre-switch prefix, full editor
service and maintenance main-loop helper with synthetic frame counters. It verifies
no timeout/housekeeping/dispatch while held, discarded rather than queued requests,
idle-held validity and unchanged inactive dispatch. It does not execute every editor
command body. Source checks bind both main-loop admission points and production macro
wiring; no MMIO, device, raw unit capture or installed update runs.

Existing storage, exhaustive shared-scratch fault tests (162789 checks), complete
Elite fixture OTA staging/refusal tests and boot-screen record tests also pass.
The imported legacy OTA harness needs its existing warning allowances in the focused
strict harness; no unrelated warning cleanup or settings change is made. The initial
boot-screen smoke compile lacked its include directory; the corrected documented
`-Ifirmware/src` compile/run passed.

## Exact native scope and budgets

The new genuine Elite DEV app+ordinary-loader build uses the same pinned SDK and
qualified 20260730.1 compiler as the accepted PR674 base. That exact unchanged base
build proof is reused, not rebuilt merely for the merge. App 337188→337412 B (+224),
normal RAM 98056/98304 B unchanged (248 B free), pool 323000/344064 B unchanged
(21064 B free, 12872 B above the locked 8192 B minimum). Linker budgets, stack guard
bands and locked settings remain untouched; RAM-code bytes and ordinary-loader
bytes are unchanged. No Pro-specific branch or universal-model PASS is asserted.

```
JIELI_TOOLCHAIN=/path/to/qualified/toolchain AC79_SDK=/path/to/pinned/sdk \
  X0X_GEN_CACHE=0 X0X_DEV_PADCAL=1 python3 -B tools/build.py --board smk37 --model elite
```

Private generated package is app+loader build evidence only, not a complete matching
controller owner delivery or publication. No hardware, live transport, stable identity,
full capture memory/latency or interruption-safe boot-commit qualification is inferred.
