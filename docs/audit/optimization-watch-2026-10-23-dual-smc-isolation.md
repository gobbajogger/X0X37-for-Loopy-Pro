# LoopyX0X37 optimization watch — dual-SMC input isolation

**Date:** 2026-10-23 · **Scope:** audit only, no firmware/installer changes.

**Current main:** [`c9718ae8`](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/commit/c9718ae833eb64a5191c350922593df8a36017c9).
**Examined proposal:** [PR #18](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/18) at [`d29f365d`](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/commit/d29f365df8b60436749b6712ca9d605d2f912708), not yet in main.

## Executive summary

An exact-source, sanitizer-enabled C experiment shows that the proposed second SMC shares a single 32-entry input queue with the first SMC. A burst of 31 CC events from one SMC can displace the other SMC's Solo release when no main-loop drain intervenes. This extends the single-SMC saturation finding already recorded in this PR; it is a cross-controller fairness test, **not** a field-reproduced fault.

| Priority | Observation / status | Consequence / owner |
| --- | --- | --- |
| P1 research | SOURCE CONFIRMED + FORCED HOST REPRO: both device slots use one `smc_q[32]` and one fullness counter. | A released Solo could remain active if its note-off is dropped. Dual-SMC/firmware owner: characterize burst reachability and restore reliable critical-release behavior. |
| Merge guard | SOURCE CONFIRMED + HYPOTHETICAL MERGE MODEL: PR #18 moves SMC queue acknowledgement to after dispatch while main uses a copy-and-ack-before-dispatch contract and project-reset drain. | A careless integration could replay events invalidated by project replacement. Sole integrator must retain current main's project guards and consumer order. |

## Evidence

- [PR #18 input queue and producer](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/d29f365df8b60436749b6712ca9d605d2f912708/firmware/src/app/smc_mixer.c#L7-L23) and [slot-tagged enqueue](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/d29f365df8b60436749b6712ca9d605d2f912708/firmware/src/app/smc_mixer.c#L48-L64) show a **shared** `smc_q` and a capacity test against `SMC_Q=32`, without per-slot reserves.
- [PR #18 dispatch](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/d29f365df8b60436749b6712ca9d605d2f912708/firmware/src/app/smc_mixer.c#L538-L598) routes each queued event to its slot; [Solo release handling](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/d29f365df8b60436749b6712ca9d605d2f912708/firmware/src/app/smc_mixer.c#L486-L491) needs the Note Off to reach dispatch.
- [PR #18 reachability checks](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/d29f365df8b60436749b6712ca9d605d2f912708/tests/host/smc_reachability_test.c#L209-L248) exercise an overflow and two SMC state records **separately**, not a one-SMC-flood/other-SMC-release interleaving.
- **Exact source identity:** PR #18's `smc_mixer.c` Git blob `2405976eb2842f4a26a0cb2b0c3dbe91027c21cb`; the extracted 472-character `smc_ble_midi_slot()` function has FNV-1a32 `32149eda` (checked before compilation).
- **Host runner:** Linux x86_64; GCC 14.2.0, `-fsanitize=address,undefined -fno-omit-frame-pointer -O1 -Wall -Wextra -Werror`.
- **Forced input:** slot 1 queues `90 08 7F` (Solo press); slot 0 queues 31 CC events; slot 1 then sends `80 08 00` (Solo release) without a consumer drain.
- **Observed:** 32 retained events (slot 0: 31, slot 1: 1), one dropped event, slot 1 Note On retained, slot 1 Note Off absent. Exit 0 with no sanitizer diagnostics. After simulated drain, a fresh release was accepted.
- **Limitation:** This tests the source's queue producer in isolation. It does not measure real SMC traffic rate, two-link radio behavior, consumer scheduling or hardware Solo persistence.

## Reproduction from a scratch clone

Use a scratch Git checkout with PR #18's commit reachable; do not change an active owner worktree. The following extracts the *exact* function before building the host harness. GitHub fetch was not available from this sandbox; the extracted, source-hash-checked function and C harness were executed here.

~~~sh
git fetch origin cursor/smc-pause-lock-10fa
git show d29f365df8b60436749b6712ca9d605d2f912708:firmware/src/app/smc_mixer.c > /tmp/x0x37-pr18-smc.c
python3 - <<'PY'
from pathlib import Path
import subprocess
import tempfile

source = Path("/tmp/x0x37-pr18-smc.c").read_text()
start = source.index("void smc_ble_midi_slot(")
end = source.index("\n}\n", start) + 2
function = source[start:end] + "\n"
hash32 = 2166136261
for ch in function:
    hash32 = ((hash32 ^ ord(ch)) * 16777619) & 0xffffffff
assert f"{hash32:08x}" == "32149eda", "Wrong input source revision"

code = """#include <stdint.h>
#include <stdio.h>
#define SMC_Q 32u
static struct {uint8_t n,slot,b[6];} smc_q[SMC_Q];
static volatile uint8_t smc_qw,smc_qr,smc_enqueued,smc_dropped;
""" + function + """
int main(void) {
  const uint8_t on[3]={0x90,8,127},off[3]={0x80,8,0};
  uint8_t cc[3]={0xb0,16,1};
  smc_ble_midi_slot(1,on,3);
  for(unsigned i=0;i<31;i++){cc[2]=(uint8_t)i;smc_ble_midi_slot(0,cc,3);}
  smc_ble_midi_slot(1,off,3);
  unsigned on_count=0,off_count=0,first_count=0;
  for(unsigned i=0;i<smc_qw;i++) {
    unsigned k=i%SMC_Q;
    first_count+=smc_q[k].slot==0;
    on_count+=smc_q[k].slot==1 && smc_q[k].b[0]==0x90 && smc_q[k].b[1]==8;
    off_count+=smc_q[k].slot==1 && smc_q[k].b[0]==0x80 && smc_q[k].b[1]==8;
  }
  printf("slot0=%u total=%u on=%u off=%u drops=%u\\n",
         first_count,smc_qw,on_count,off_count,smc_dropped);
  return !(first_count==31 && smc_qw==32 && on_count==1
           && off_count==0 && smc_dropped==1);
}
"""
with tempfile.TemporaryDirectory() as tmp:
    src = Path(tmp) / "smc_queue.c"
    exe = Path(tmp) / "smc_queue"
    src.write_text(code)
    subprocess.run([
        "gcc", "-std=c99", "-O1", "-g", "-Wall", "-Wextra", "-Werror",
        "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
        str(src), "-o", str(exe)
    ], check=True)
    subprocess.run([str(exe)], check=True)
print("PASS: forced cross-controller queue loss reproduced; NOT a device test")
PY
~~~

## Merge-specific project-reset risk (not a merged-build finding)

- [Current main's project-drop guard](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/c9718ae833eb64a5191c350922593df8a36017c9/firmware/src/app/smc_mixer.c#L77-L92) sets `smc_qr=smc_qw`; [main dispatch](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/c9718ae833eb64a5191c350922593df8a36017c9/firmware/src/app/smc_mixer.c#L581-L626) takes a local copy and releases the slot *before* processing a message.
- [PR #18 dispatch](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/d29f365df8b60436749b6712ca9d605d2f912708/firmware/src/app/smc_mixer.c#L569-L579) instead advances `smc_qr` *after* processing `smc_message()` and omits the newer project's busy/reset functions (its base diverges from main).
- A small bounded C model with `write=3`, an action that sets `read=write`, and a delayed `read=old_read+1` reports **2 invalidated entries visible again** versus **0** with main's ordering. The model is a conditional integration counterexample, not an execution of a merged firmware.

## Safe follow-up

1. **Dual-SMC owner / independent test reviewer:** Add an end-to-end regression that keeps slot 1 Solo engaged while slot 0 floods its controls, sends slot 1 Note Off during peak queue usage, and asserts eventual release without reconnect. Measure realistic bursts; choose a fix only after confirming the transport can reproduce the pressure.
2. **Sole firmware integrator:** Reconcile PR #18 with latest main's project load/reset, note-off semantics, per-drum Solo, busy guard and pre-dispatch queue acknowledgment. Do not copy PR #18's SMC polling loop wholesale. Exercise project replacement with queued dual-SMC events.
3. **Candidate validator:** After a separately authorized repair, run all affected BLE/HCI/SMC regressions, both Elite and Pro DEV builds and RAM/flash/pool guard checks (minimum 8192 bytes free pool), then the approved installer/model/sample-preservation checks. Only actual hardware can verify dual Bluetooth links and live Solo release.

## Disposition

**Research evidence checked; device behavior untested.** No CI pass, firmware target build, sample backup/write, merge or `gh-pages` publication is claimed for this audit. [PR #28](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/28) owns a separate DSP undefined-shift sanitizer repair; [PR #26](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/26) and [PR #27](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/27) own unrelated bugfinder/installer research. This report changes only independent Markdown evidence and does not authorize a code patch or bypass the firmware integrator.
