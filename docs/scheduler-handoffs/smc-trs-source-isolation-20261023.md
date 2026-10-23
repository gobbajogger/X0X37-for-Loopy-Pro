# SMC host TRS/unknown source isolation — 2026-10-23

**Producer:** LoopyX0X37 SMC Transport. **Next consumers:** Independent QA, Hourly Build, then sole Safe Merge owner. **Stage:** FOCUSED HOST CHECKED / EXACT CURRENT TREE QA PENDING / DEV BUILD PENDING / DEVICE NOT TESTED. This is development source, NOT an installable candidate.

## Identity and existing ownership

- Production base: `main@c9718ae833eb64a5191c350922593df8a36017c9`.
- Producer branch: `lane/smc-host-trs-source-guard-20261023-1721`; **refresh live remote HEAD** before using this file.
- Original `firmware/src/app/engine.c` blob `555480d9c81ca8b088842bfa6bac93e85865d49a`; proposed modified blob `fd11e81d77a0aec1afb395a2ea5b070483d87ec4`. Exact main `firmware/src/app/plat.h` source IDs map 0=USB, 1=BLE peripheral, 2=TRS, 3=unassigned.
- `smc_protocol.h` original blob `03a642226aae01ffb9bc29888e0f5802d72ced26`, admits opt-in User controls only on MIDI channel 16.
- Source scope: `firmware/src/app/engine.c` MIDI receive routing, and `tests/host/smc_sender_isolation_test.py`. Other active SMC popup/lock branches and PR #40 (host malformed data-byte guard) retain their own ownership; no edits to `smc_mixer.c`, central BLE, sequencer, USB parser, storage, installer, publishing or firmware packages.

## Root cause and minimal fix

In `midi_in(uint32_t pkt)`, the existing `smc_midi_host(pkt)` call has **no MIDI source guard**, even though User-mode SMC host controls are supported only via USB and Bluetooth peripheral. As a result an attached TRS MIDI instrument transmitting channel 16 CC 40–47 (or the note IDs for SMC Solo/Select) can be consumed by `smc_midi_host`, causing a mixer action and suppressing ordinary instrument routing.

The patch gates the call with `src < 2`, preserving existing USB and BLE opt-in acceptance and bypassing User SMC for TRS (source 2) and unassigned source 3. Existing clock-source lock and channel routing are otherwise unchanged.

## Executed native Linux evidence

Test `tests/host/smc_sender_isolation_test.py` extracts the **actual exact-source SMC routing block** from `firmware/src/app/engine.c` and compiles it using the exact production `smc_host_accept` helper from `firmware/src/app/smc_protocol.h`; 9 assertions exercise USB, BLE, TRS, unknown-source, Solo, non-User-channel and disabled-User cases.

- **Before fix:** `python3 -B tests/host/smc_sender_isolation_test.py` — **nonzero exit (SIGABRT)** as expected at TRS CC40: SMC consumed a source-2 packet; original behavior did not satisfy the regression.
- **Patched GCC:** `python3 -B tests/host/smc_sender_isolation_test.py` — **exit 0, 9/9**.
- **Patched Clang/UBSan:** `CC=clang SAN_FLAGS='-fsanitize=undefined -fno-sanitize-recover=all' python3 -B tests/host/smc_sender_isolation_test.py` — **exit 0, 9/9**.
- **Patched GCC/ASan+UBSan:** `SAN_FLAGS='-fsanitize=address,undefined -fno-sanitize-recover=all' python3 -B tests/host/smc_sender_isolation_test.py` — **exit 0, 9/9**.
- Python syntax check `python3 -m py_compile tests/host/smc_sender_isolation_test.py` — **exit 0**.
- Native runner Linux x86_64, GCC and Clang installed. Tests were executed on an isolated reconstruction of the **exact relevant remote source functions**, not a complete byte-for-byte checkout of the repository. Current remote source/blob and Git branch diffs were reread after pushes.

**Unrun:** whole exact Git tree native host suites, independent QA/second pass, actual pinned SDK/toolchain/DSP Elite/Pro DEV/ctl.img builds, memory/flash/pool/model/CRC/package/backup gates, actual device. Historical CI is NOT execution evidence. Do NOT call this a full-firmware PASS.

## Receiving instructions

1. **Independent QA:** fetch the live branch HEAD, compare to main and PR #40, rerun this regression from a complete **exact Git tree** with a trusted native runner, plus `smc_queue`, `smc_filter`, engine MIDI routing and negative clock/Note-Off tests where relevant. Ensure SMC real User messages on BLE/USB still work; TRS instrument channel-16 messages must reach ordinary MIDI handling.
2. **Hourly Build:** only after appropriate host review and combined-tree selection run actual pinned V1.2.1 SDK + JieLi and four DSP references Elite/Pro DEV builds, matching controller image, model/format/budget/CRC/SHA, and guarded sample backup/readback/installer checks.
3. **Safe Merge:** sole owner of isolated integration. Check conflicts and live branch ownership, then test exact integrated tree; no unreviewed main merge.
4. **Owner physical testing:** separate stage after individually approved software-safe distributable NIGHTLY DEV test package; NOT a requirement for source code Git delivery.

No firmware artifact, `main` or `gh-pages` update; no release or hardware flashing.
