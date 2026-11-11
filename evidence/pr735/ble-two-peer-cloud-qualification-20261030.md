# PR #735 — cloud scoped verification and qualification limits

Candidate branch: `chat/enable-two-peer-ble-central`
Source/verification HEAD at reporting: `ed195ab9c12053a8e07d2c21d00355fc0c62fbe0`
PR base branch: `integration/all-in-one`
Date: 2026-10-30 UTC.
All values below distinguish **executable checks** from source assertions and full-suite status. No hardware access is claimed.

## Verified evidence on the checked source closure

1. **Controller heap allocation replay — PASS (1 native executable, exit 0).**
   - `tests/host/ble_controller_heap_test.c` reconstructed from the GitHub API; verified Git blob `91c1690ee4661604ce5ed3122e3e17754567162a`, exact source.
   - `ble/ctl/src/ctl_heap.c` from supplied source cache; verified exact current Git blob `da63514fd9f42af90834e8e00b164fc0d085c0b6`.
   - Candidate `ble/ctl/ctl.ld` has `CTL_HEAP_SIZE=0x2980` (10624 bytes); candidate `config.c` and `tools/locked_settings.json` specify 2 links, 3 TX ACL credits, multilink 1. The linker size and credit values were passed explicitly as compiler defines to the replay.
   - Command: `gcc -std=c11 -fno-pie -no-pie -O1 -g -Wall -Wextra -Werror -DTEST_CTL_HEAP=10624 -DTEST_CTL_CREDITS=3 tests/host/ble_controller_heap_test.c -o test && ./test`
   - Output: `controller heap: A first-peer PASS; old B first-peer FAIL; both partial fixes FAIL reserve; corrected two-peer lifecycle x100 with 512-byte transient reserve PASS`
   - Binary SHA-256: `7fb5b3353feaf1941d5a035dd196294e0729d436c163fd79f2672046af5fb51e`.
   - Platform: Linux x86_64, GCC `14.2.0`. This is a bounded host replay, **not** target heap measurement, BLE RF proof or SDK image build.

2. **Second SMC outbound routing — PASS (1 source-slice C executable, exit 0).**
   - Copied the verified `plat_ble_smc_send_slot` production C function text into a deterministic stub harness, compiled with `gcc -std=c99 -O2 -Wall -Wextra -Werror`.
   - Verified slot 0 and slot 1 independently route MIDI to different central contexts, slot >=2 returns without IRQ lock, and five role/transport/off gates preserve both queues.
   - Output: `SMC outbound slot routing: two distinct peers, bounds and five off gates PASS`.
   - Binary SHA-256: `8590e618089a75f4f540f4fa741893b1673e86f8580c919da5fc2eb5dcfed2fe`.
   - This source-slice harness is **not** a full firmware build. A repository-integrated extraction/compile test was added at `tests/host/ble_smc_outbound_slot_test.py`, but that script itself is not yet executed against a complete exact checkout.

3. **GitHub API content/locked-default checks — PASS (17/17 assertions).**
   - Checked pinned content at `4fe0c02779ee1583b4da84f8cf728bf7fe9be8ae`. Subsequent commits to this report only added regression tests and runner registration; underlying firmware input blobs are unchanged.
   - 17 assertions covered controller config vs locked settings, 2 link count, 3 shared ACL credits, multi-link switch, default `BLEC_TWO_PEER`, fallback compilation, pair binding/routing, slot 1 MIDI/status path, host runner registrations and the corrected config parser.
   - Git input blobs: central `791cf4070b60bc0591ec9b6ed48c6966faa8d7c0`, central header `34aee7218483ad5bcf0bf6f68a8dc109fce53768`, app `efe09be348e4d45cb2b28ad9c2768ff1c38c681a`, controller config `e36dd8e4872ea75616634f4bdf9baa5bc7d5a3c5`, locked settings `6a20ff28505afa7ec41f53be23b971ccd42ca13f`.
   - These are source assertions, not host executable passes.

4. **Owner-supplied SDK/build-input cache — PASS (11/11 pinned file checks).**
   - Materialized `x0x37-private-build-inputs-20261021.zip` from the Project Library. Ran `python3 kit.py verify`: `Kit checksums and all 11 pinned SDK inputs verified.`
   - Owner-supplied JieLi compiler extracted and returned `clang version 4.0.1`.
   - The kit itself explicitly says it is **not a full SDK checkout**. The mounted complete SDK ZIP here is **V1.2.0**; no full V1.2.1 source checkout has yet been established. Do not substitute V1.2.0 for V1.2.1.

## Requested full qualification (accurate NOT_RUN)

| Requested operation | Result | Actual pass/fail count | Specific limit |
| --- | --- | --- | --- |
| Full `X0X_TEST_JOBS=8 sh tests/run_tests.sh` on exact candidate | **NOT_RUN** | **0 pass / 0 fail; no full-suite execution** | Entire current GitHub 4,000+ file tree cannot be materialized into this cloud runtime through the connected GitHub API; the local bootstrap tar is a separate historical SHA. |
| Elite firmware, SDK V1.2.1 | **NOT_RUN** | **0 builds attempted / 0 successful** | Exact candidate root and complete V1.2.1 SDK checkout unavailable in active execution filesystem. |
| Pro firmware, SDK V1.2.1 | **NOT_RUN** | **0 builds attempted / 0 successful** | Same exact-source and SDK closure limit. |
| Two physical SMC connections / RF robustness | **NOT_RUN** | **0 device trials** | No connected hardware. |

No `.github/` edits, GitHub Actions or zipball downloads, target flashing, binaries, or protected flash operations. PR stays draft until full automated and device acceptance is real.
