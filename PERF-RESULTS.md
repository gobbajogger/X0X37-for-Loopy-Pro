# SMK 37 Pro performance results

Target: 12–20% idle load with bit-exact sound. N/R means not run or not read.
Baseline values below were supplied by the owner, not measured by this lane.

Latest run (2026-10-30): `c287cfe4` installed successfully. Device identity,
controller readback, and slot preservation passed. PERF screen readings are
pending; no idle-load improvement or hardware CLOCK result is claimed.

| Commit / sample | Idle LOAD % | PEAK % | DROPOUTS | 303A % | 303B % | MASTER % | FX % | 909 % | CLOCK | Result / comparison |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Owner baseline | 59 | 62 | 2465 | 24.4 | 23.2 | 6.8 | 2.0 | 0.8 | ? | Reference |
| `f9ffd12e8c254015d61e89dab647b8466b67b0fd` | N/R | N/R | N/R | N/R | N/R | N/R | N/R | N/R | N/R | INSTALL BLOCKED; deltas and bit-exactness N/R |
| `c287cfe47c141f524a8b4a11a7deea6ddb9bbf8e` (`f9ffd12e` + CLOCK) | N/R | N/R | N/R | N/R | N/R | N/R | N/R | N/R | N/R | INSTALL PASS; slot checks PASS; PERF screen pending; baseline deltas N/R |

2026-10-29: the whole device loop stopped on sandbox access denial. The requested
`python3 tools/fm1_install.py --info` exited 10: `REFUSED: cannot open the hardware
lock /Users/mac/.x0x37-hardware.lock: Operation not permitted`. The lock was not
relocated or bypassed. Device connection and normal firmware identity could not
be verified. No install, slot backup, slot comparison, PERF capture or restore ran.
No protected flash writes were attempted, and nothing was pushed.

The queued head was already checked out on `codex/perf-test`. An explicit
checkout attempt failed with `index.lock: Operation not permitted`.
The ten active private inputs passed their pinned byte-count and SHA-256 checks.
The Pro build with `X0X_ORIGINAL_SYNTH=1 X0X_DEV_PADCAL=1` exited 1 before compiling:
`build.sh: Docker is not running`. No firmware package or firmware CRC can be
reported. Logs, the input receipt and evidence CRC32 values are in
`out/f9ffd12e/` (`STATUS.txt`, `build.log`, `info-before.txt`,
`original-inputs-receipt.json`, `clock-host-test.txt`, `clock-mmio-check.txt`,
`CRCS.txt`). `out/` is excluded locally and must remain uncommitted. The raw PERF
file and before/after archives are absent because those steps did not run.
The queue was re-read after the blocked candidate and still contained only
`f9ffd12e`; no additional candidate was processed.

## Separate CLOCK change — committed in the follow-up

The patch described below is now committed as
`c287cfe47c141f524a8b4a11a7deea6ddb9bbf8e`. The staging refusal and proposed
file list below describe the earlier sandboxed attempt.

Proposed commit: `Enable the PERF cycle counter before probing`.

Reason and purpose: the startup probe did not enable the core 0 performance
counters before reading TL_CKCNT. Follow the local AC79 SDK's `cpu_effic_init`
(`cpu/wl82/debug.c`) by setting DBG_CON bits 0–2, preserving all other bits.
Restore the saved register if the counter remains stopped, and keep the shared
enable bits set when the STALLS display is turned off. This lets PERF measure
CLOCK when the hardware counter responds and preserves continuous profiling
units. Audio sample arithmetic is unchanged; bit-exact sound is not yet verified.

Commit only these files, separately from this results log and queue:

- `firmware/hal/fm1_perf.h`
- `firmware/src/app/plat_fm1.c`
- `tests/host/fm1_perf_counters_test.py`

Validation: `python3 tests/host/fm1_perf_counters_test.py` PASS (16 modeled cases:
enable before sampling, preservation of unrelated debug bits, nonmoving-counter
fallback, timer/cycle wraparound and STALLS continuity). `tools/build.py`
`mmio_check()` PASS under Python 3.12. `git diff --check` PASS. Target build and
hardware verification remain NOT RUN. A host model does not verify the hardware
counter enable semantics or clock accuracy.

Git status was checked before staging these exact three source/test files.
`git add -- <three files above>` failed because the sandbox denied creation of
`.git/worktrees/perf-test/index.lock`. No commit was made and no private input or
binary was staged. No Git sandbox workaround was attempted.

## Full-access follow-up, 2026-10-30

Queue entry: `f9ffd12e8c254015d61e89dab647b8466b67b0fd`. Built the existing
`codex/perf-test` HEAD, `c287cfe47c141f524a8b4a11a7deea6ddb9bbf8e`, with
the separate CLOCK commit on top. The purpose is to measure the queued
sound-preserving optimizations on the Pro and make CLOCK readable without
changing audio sample arithmetic.

The Pro development build uses `X0X_ORIGINAL_SYNTH=1 X0X_DEV_PADCAL=1`.
All ten active external headers passed their pinned hashes. A scoped Linux
`tools/dev_validate.py run` completed with `DEV CHECKED`, including the 16
CLOCK host-model cases, firmware and controller builds, package integrity,
and memory budgets. App image: 413872 B; code free: 138800 B; RAM free:
872 B; pool free: 15184 B. The earlier Mac attempt used Python 3.9, then
encountered a Docker toolchain include-path mismatch. The successful run
uses the repository's Linux validation worker with consistent mounted paths.
No firmware source was changed to resolve those environment issues.

Evidence is ignored under `out/c287cfe4/`, including
`dev-checked/record.json`, build/check/controller logs, the external-input
receipt, `CRCS.txt`, package identity, and the three slot archives.
Final artifact CRC32 values:

- `x0x-smk37.fwsc`: `fad270f0`, 651414 B (validated package).
- `install-wire.fwsc`: `c699a0bc`, 651414 B (the same package composed with
  the selected controller, matching the installer's composition).
- `x0x.bin`: `5d00d916`, 413872 B.
- `ota.bin`: `a1d7ad4b`, 7298 B (RAM update loader).
- `ctl.img`: `9c07ae67`, 65492 B.

Validated package SHA-256:
`3e635723d1e4affc3d6db8fa8d2d7e742939f26b397c9be331fdf8e02933ea7e`.
Controller SHA-256:
`956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a`.

`fm1_install.py --info` found `SMK-37 Pro_900 [running]`, port `X0X FM-1`,
before installation. The independent `before.x0xslots` backup and saved
`before.txt` passed. Both slots were empty, with capacities 81920 and
122880 B. Their flash offsets and capacities exactly matched the package;
no slot migration was needed.

One foreground install ran with `--yes --backup install.x0xslots`, the exact
source commit, scoped DEV record, and explicit validated controller. The
installer exited **0**. The complete 65536 B controller block read back with
CRC32 `4a400899`, matching the expected controller plus erased padding.
The post-install `--info` again reported normal `SMK-37 Pro_900 [running]`.
This legacy identity contains no per-commit hash; the source and artifact
association comes from the exact validation/install evidence.

The independent `after.x0xslots` backup passed. All three archives passed
their own CRC checks and matched slot metadata and image bytes, excluding
archive timestamps. The appended per-slot size/CRC fields in `before.txt`
and `after.txt` match. Both slots remain empty: image sizes are 0 B and
payload CRCs are N/A. See `slot-comparison.txt` and `slot-map-check.txt`.
No manual restore was attempted.

`python3 tools/ble_autotest.py --diag-only` exited 0; its unmodified output
is `perf-raw.txt`. It reports Bluetooth OFF and no new crash, but contains
no PERF LOAD, PEAK, audio DROPOUTS, part percentages, or CPU CLOCK fields.
Those values and all baseline deltas therefore remain **N/R**. The lane
requested a transcription of the idle PERF screen and pauses further
installs until the readings are available. Hardware CLOCK behavior, the
12–20% idle target, and hardware sound equivalence remain unverified.
The unchanged engine/DSP sources retain the inherited host PCM/state proof
documented in `MIAMI-OPT.md`; this run does not claim a new sound comparison.

`QUEUE.md` was re-read after the install and still contained only `f9ffd12e`.
No second install was started. No bootloader, BTIF, USRTRIM, efuse, or key_mac
write was attempted. The RAM loader's write windows exclude the bootloader
and protected flash. No reference flash-head file was available, so the
installer explicitly logged that its reference-head comparison was not run.
All private headers, binaries, backups, and `out/` remain uncommitted.
Nothing was pushed.
