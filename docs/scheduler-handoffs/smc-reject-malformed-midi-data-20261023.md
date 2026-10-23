# SMC host User-mode MIDI payload guard — 2026-10-23

**Owner/producer:** LoopyX0X37 SMC Transport. **Next consumer:** Independent QA, then Hourly Build and sole Safe Merge owner. **Status:** focused host regression passed on checked matching function source; independent exact-current-tree review and DEV builds remain **PENDING**. **DEVICE NOT TESTED**; not an installable firmware candidate.

## Source and isolation

- Production repository: `gobbajogger/X0X37-for-Loopy-Pro`.
- Source base: production `main@c9718ae833eb64a5191c350922593df8a36017c9`. Owned delivery branch: `lane/smc-reject-invalid-midi-data-20261023`; its real HEAD must be refreshed, not inferred from this file.
- Scope: `firmware/src/app/smc_mixer.c` `smc_midi_host()` and `tests/host/smc_queue_test.py` only. No transport mapping, queue capacity, SMC BLE central logic, status catalogs or firmware bytes changed.
- Original **current-main** blobs independently fetched before editing: `smc_mixer.c b89e5c12062a3dc3459eeb660d205f607273aab6`, `smc_queue_test.py e03ba06dbc985935298264ca321f90ab726fde92`, `smc_protocol.h 03a642226aae01ffb9bc29888e0f5802d72ced26`. The affected `smc_midi_host()` body in GitHub main was checked against the local extracted production function.

## Defect and patch

In opt-in channel-16 SMC User mode, `smc_midi_host()` masked the MIDI data1 high bit **before checking admission**, then copied raw bytes. A malformed byte `0x88` was admitted as note `0x08` (Solo 1); `0x90` was admitted as CC `0x10`. Invalid data2 bytes were also admitted. These could cause accidental SMC actions after later masking in `smc_message()`.

The repair rejects host SMC candidates when either of the two required MIDI data bytes has bit 7 set. Only then does it run the unchanged opt-in/channel/preset admission function and enqueue the original valid seven-bit payload. Legitimate CC, Note On and Note Off values are unaffected. This affects only USB/peripheral-BLE SMC User-mode host admission, not the direct BLE central queue or actual source attribution when Loopy and SMC share an identical forwarded channel.

## Executed native evidence

Native execution environment: Linux x86_64, GCC, Python 3 and UBSan. The offline bootstrap source snapshot is **historical** `7537a46ab49419920428299db8f3321d01e6fd16`, not the current Git `main` tree. The tested function and original test `smc_queue_test.py` were verified against exact live main source; the *whole* historical SMC file and surrounding project are not identical to main.

1. Baseline negative-control, after adding the four invalid-data assertions but **before** fixing source: `python3 -B tests/host/smc_queue_test.py` returned **exit 1**, 6 of 24 host-queue interleaving cases failed as expected at the first invalid high-bit data1 CC.
2. Patched scoped test: `python3 -B tests/host/smc_queue_test.py` returned **exit 0**, **24/24** cases. The test additionally asserts disabled User mode still refuses valid SMC packets without advancing the queue.
3. UBSan: `SAN_FLAGS='-fsanitize=undefined -fno-sanitize-recover=all' python3 -B tests/host/smc_queue_test.py` returned **exit 0**, 24/24.
4. Protocol helpers: `gcc -std=c99 -O2 -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=all ... tests/host/smc_protocol_test.c`, then running the binary, **exit 0**.
5. `python3 -B tests/host/smc_filter_test.py` **exit 0**.
6. `sh tests/host/smc_reachability_test.sh` **exit 0**, full simulated SMC routes 0–4.
7. `sh tests/host/ble_midi_receive_test.sh` **exit 0**, HCI receive/fragments/signaling/reconnect/stop cases.
8. `python3 -B tests/host/smc_link_test.py` **exit 0**, ten link lifecycle cases.

All commands ran in local Linux. No GitHub Actions job was used. Historical source-relative tests support the narrow guard, **not** an independent exact-current-tree or Elite/Pro package PASS.

## Required receiving stages

- **Independent QA:** reread actual remote branch HEAD and diff, build a checkout matching its exact Git tree, and repeat the targeted negative-control and native affected regression suite. Check that invalid data2 and note-on/note-off packets cannot enter the SMC User queue while valid channel-16 opt-in controls remain accepted. Look for malformed-event handling in the ordinary instrument MIDI path.
- **Hourly Build:** if this is selected for integration, validate against exact combined source under pinned JieLi toolchain, V1.2.1 SDK, four pinned DSP refs, and actual Elite/Pro DEV builds, `ctl.img`, flash/RAM/pool/model/CRC/SHA256 and backup/readback safeguards. Do not use the historical local snapshot to claim an exact-current-target build.
- **Safe Merge:** sole owner of isolated integration. Confirm no other active producer owns conflicting `smc_mixer.c` changes, especially popup/pause-lock branches. Merge only after independent review on correct combined source.
- **User device check:** user alone installs/tests an individually approved safe nightly DEV candidate later; no keyboard or physical PASS is required for this source commit.

No production `main` or `gh-pages` change; no Pages publication, firmware binary, SDK, DSP, samples, or stable-release promotion.
