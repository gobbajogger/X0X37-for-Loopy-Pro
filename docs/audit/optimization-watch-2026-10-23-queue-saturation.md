# LoopyX0X37 optimization watch — queue-saturation audit (2026-10-23)

> **Audit result: REPRODUCED forced-saturation event loss; real-world incidence UNTESTED.**
> Documentation-only report. Neither firmware fixes nor device checks are claimed.

## Summary

Two distinct production input queues discard release events when intentionally filled to capacity, potentially leaving a bass note or mixer Solo held.
Source-extracted C reproductions completed successfully, but no evidence currently shows that a real performance reaches either saturation condition.

**Revision:** [`main` c9718ae](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/commit/c9718ae833eb64a5191c350922593df8a36017c9) · **Targets:** SMK-37 Elite/Pro · **Runner:** isolated Linux x86_64, GCC 14.2.0, Python 3.13.5.

| Priority | New evidence | Status | Risk / receiving owner |
| --- | --- | --- | --- |
| P1 | Full 1,024-entry audio command queue drops a subsequent bass Note Off. | SOURCE-PROVEN + HOST-REPRODUCED, workload feasibility UNTESTED | Possible sustained 303 note. Firmware audit owner to characterize reachability and add a regression. |
| P1 | Full 32-entry direct-SMC ingress ring drops a subsequent Solo-button release. | SOURCE-PROVEN + HOST-REPRODUCED, real BLE burst UNTESTED | Possible stuck 909/808 or part Solo until an independent clear. Coordinate with existing SMC PR owners. |
| P2 | Latest SMC PR workflow failure lacks step/log evidence. | CI FAILURE OBSERVED, test execution UNKNOWN | No CI PASS claim or performance comparison is justified. CI/integration owner to investigate job logs or run tests on an independent runner. |

## 1. Audio command queue — lost note-off under forced saturation

- [Source insertion logic](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/c9718ae833eb64a5191c350922593df8a36017c9/firmware/src/app/engine.c#L130-L146) uses `CQ = 1024` and unconditionally drops a new command when full. The enqueue function is shared by [parameter edits, bass note-on, bass note-off and break commands](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/c9718ae833eb64a5191c350922593df8a36017c9/firmware/src/app/engine.c#L230-L361).
- [Audio render drain](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/c9718ae833eb64a5191c350922593df8a36017c9/firmware/src/app/engine.c#L643-L668) consumes the retained commands, so the queued note-on is applied without the attempted dropped note-off in this constructed case.
- **Exact fixture:** enqueue `C_BON` for bass 0; then 1,023 `C_PARAM_T0` writes without a drain; then enqueue `C_BOFF` for bass 0.
- **Observed:** `ring=1024 on=1 off=0 attempted_off_dropped=YES`; source-extracted C harness returned exit code 0 when this loss condition was detected.
- **Limitation:** Reproduction deliberately stalls the consumer until the ring is full. It does **not** establish that a normal UI/project workload can enqueue 1,024 commands between audio drains. Do not call this a field-reproduced stuck note.

## 2. SMC ingress ring — dropped Solo release

- [SMC BLE input](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/c9718ae833eb64a5191c350922593df8a36017c9/firmware/src/app/smc_mixer.c#L7-L29) keeps a 32-message ring and drop counter. The [enqueue operation](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/c9718ae833eb64a5191c350922593df8a36017c9/firmware/src/app/smc_mixer.c#L94-L106) rejects **all** message kinds while full.
- A press on SMC Solo note 8 can set a held solo, and its [MIDI note-off release handler](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/c9718ae833eb64a5191c350922593df8a36017c9/firmware/src/app/smc_mixer.c#L529-L579) is the corresponding clear; reconnect or project transitions provide some independent clears. [Main-loop dispatch](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/c9718ae833eb64a5191c350922593df8a36017c9/firmware/src/app/smc_mixer.c#L581-L654) cannot apply an event that the ISR did not retain.
- **Exact fixture:** enqueue a note-on `[0x90, 8, 127]`, 31 CC messages, then note-off `[0x80, 8, 0]` before polling.
- **Observed:** `smc_ring=32 press=1 release=0 dropped=1 attempted_release_lost=YES`; isolated C harness returned exit code 0 for detecting loss.
- **Limitation:** The SMC file differs between historical snapshot and current main, but the 379-byte extracted `smc_ble_midi()` function was byte-compared using FNV-1a32 `63d3acd2` and is identical. This is not an end-to-end SMC Solo, Bluetooth or physical controller test.
- **Current ownership:** [draft PR #18](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/18) owns dual-SMC changes, and [draft PR #19](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/19) owns popup transition repairs. Do not patch either branch from this audit lane.

## 3. Reproduce the two queue-only cases

The following command, from the repository root, compiles the **actual production queue functions extracted from the checked-out C files** into a temporary host executable. Use only the pinned `main` source above when comparing with this report; it tests forced-overflow semantics, not an actual controller or DSP timing.

```sh
python3 - <<'PY'
from pathlib import Path
import subprocess, tempfile
e = Path('firmware/src/app/engine.c').read_text()
s = Path('firmware/src/app/smc_mixer.c').read_text()
ring = e[e.index('enum { C_PARAM_T0 = 0, C_DRUM'):e.index('/* ------------------------------------------------------------ params --- */')]
rx = s[s.index('void smc_ble_midi('):s.index('void smc_ble_link(')]
c = '#include <stdint.h>\n#include <stdio.h>\n' + ring + '''
#define SMC_Q 32u
typedef struct { uint8_t n, b[6]; } smc_event_t;
static smc_event_t smc_q[SMC_Q];
static volatile uint8_t smc_qw, smc_qr, smc_dropped, smc_enqueued;
''' + rx + '''
int main(void) {
    cq_put(C_BON, 0, 60, 400);
    for (unsigned i=0; i<CQ-1u; i++) cq_put(C_PARAM_T0, 0, 0, i);
    cq_put(C_BOFF, 0, 0, 0);
    unsigned bon=0, boff=0;
    for (unsigned i=0; i<cq_w; i++) {
        bon += (cq[i%CQ]>>28)==C_BON;
        boff += (cq[i%CQ]>>28)==C_BOFF;
    }
    if (cq_w!=CQ || bon!=1 || boff!=0) return 1;
    uint8_t on[3]={0x90, 8, 127}, off[3]={0x80, 8, 0}, cc[3]={0xB0, 16, 0};
    smc_ble_midi(on, 3);
    for (unsigned i=0; i<SMC_Q-1u; i++) {
        cc[2]=(uint8_t)(i+1u); smc_ble_midi(cc, 3);
    }
    smc_ble_midi(off, 3);
    unsigned presses=0, releases=0;
    for (unsigned i=0; i<smc_qw; i++) {
        const uint8_t *p=smc_q[i%SMC_Q].b;
        presses += p[0]==0x90 && p[1]==8;
        releases += p[0]==0x80 && p[1]==8;
    }
    if (smc_qw!=SMC_Q || presses!=1 || releases!=0 || smc_dropped!=1) return 2;
    puts("REPRODUCED: full audio CQ drops bass OFF; full SMC queue drops Solo release");
    return 0;
}
'''
with tempfile.TemporaryDirectory() as d:
    src=Path(d)/'queue_stress.c'; src.write_text(c)
    exe=Path(d)/'queue_stress'
    subprocess.run(['gcc','-std=c99','-O2','-Wall','-Wextra','-Werror',str(src),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
PY
```

## 4. Scoped verification and source provenance

**Offline runner warning:** The user-supplied bootstrap checkout is a synthetic historical snapshot, commit `f58d62f0b622b9c5792bda81793e0031d8be8747` (source tree `152ca944e9cd3d0913906a1d3fa9d8d2826ea639`), **not** the current `main` checkout. GitHub is not resolvable from this sandbox. Tests below use only selected production/test/helper blobs confirmed identical to live `main`; they do not validate the entire present repository.

| Actual check / command | Observed result | Identity / limitation |
| --- | --- | --- |
| `gcc -std=c99 -O2 -Wall -Wextra -Werror` on source-extracted queue harness; run executable | REPRODUCED both loss paths; exit 0 | `engine.c` Git blob `555480d9c81ca8b088842bfa6bac93e85865d49a` equals main; SMC producer slice exact but complete `smc_mixer.c` differs. |
| `X0X_TEST_JOBS=1 CC=gcc sh tests/run_tests.sh fastmath sequencer clk-lock smc-protocol` | 4 selected checks PASS, command exit 0 | Selected tests and input source blobs matched main; runner script itself is older, not a whole-current-main suite. |
| `gcc -std=c99 -O2 -ffp-contract=off -w -Ifirmware/src/dsp -Ifirmware/src -o /tmp/x0x_bass303_lite tests/host/bass303_lite_test.c firmware/src/dsp/bass303.c -lm && /tmp/x0x_bass303_lite` | bass303 lite PASS, exit 0 | Input source and test blobs match main. This is not a sound-quality listening check. |
| `python3 tests/host/smc_queue_test.py` | 24 full-queue ISR interleavings PASS | Historical full SMC implementation; does not test full-queue *dropping release* and cannot certify current-main entire SMC path. |
| `python3 tests/x0x_slots_test.py` | PASS: synthetic backup/restore, CRC, 2- and 3-slot cases | Test and relevant helper blobs match main; no device writes. |
| `python3 tests/install_slots_test.py` | PASS: synthetic downgrade, backup-before-write, interrupted restore and fail-closed cases | Test, mock and installer blobs match main; no physical install. |

**CI:** [SMC run 39820154010](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/actions/runs/39820154010) reports `failure` for one job but exposes zero step records, and fetching its job logs returned HTTP 404; test execution and cause remain **UNKNOWN**. [Main Pages run 39816499153](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/actions/runs/39816499153) likewise reports build failure with no job steps; a distinct [gh-pages deployment 39822941019](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/actions/runs/39822941019) reports completed jobs and success, which is not a firmware test.

## 5. Required follow-up before changing firmware

1. **Firmware audit/integration owner:** Determine realistic maximum audio command-ring occupancy under project loads, automation bursts and rapid UI/SMC edits. Add a dedicated controlled full-ring regression that demonstrates the note-off safety requirement without dropping sequencer events.
2. **SMC owner (coordinate with #18 and #19):** Capture live incoming burst size/rate and exact mode, replay 32+ events with held Solo and note-off, and prove that link, project and mode changes always terminate stuck states. A fix must not reorder Note On/Off, silently change mixer page ownership, or weaken producer/consumer synchronization.
3. **Optimization owner:** Measure actual worst/p95/p99 audio half time, XRUN counter and BLE latency before selecting coalescing, reserved capacity or alternative critical-release state publication; test Pro and Elite budgets and 44.1-kHz DSP reference fidelity for a future source change.
4. **Installer/data-safety owner:** Preserve the verified synthetic slot safety contracts; run separate actual package/model/backup/readback checks if a firmware patch or install candidate is produced. Do not infer physical PASS from these mocked tests.
5. **CI owner:** Retrieve real job logs or execute the host suite on an identifiable runner. Treat no-step failures, prior `startup_failure` and unavailable logs as **not verified**, never as test PASS.

**Unresolved:** exact real-world saturation probability, audio deadline behavior under queue floods, SMC BLE delivery/dual-link performance, clip/LED device timing, present target flash/RAM/pool margins, physical slot preservation and full physical usability. No target builds, installer preview, code changes, release, `gh-pages` publication or hardware writes were performed for this documentation audit.
