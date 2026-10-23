# SMC Transport — BLE HCI ACL first-packet PB flags (2026-10-23)

**Scoped native source + host verification; actual target builds and devices NOT RUN.**

## Exact source and defect

Stacked on **draft PR #89 HEAD `ca777a056e7de0075aac30927a4f6b158d55c465`**, tree `83bfff30c28ce641884c5d1d9b13330ced3aaa79`. Source C Git blob `692c213e950f2d119cb8f42105f695cef658a68b`, BLE header `488977fab5e6300e8e2f7ad96cb834fe62c490f1`. Both native files matched these exact Git blobs prior to changes. Regression includes unchanged existing PR #89 SysEx safety test blob `23d06e1c40e28ad6eb5bf653b4f511e23ecfa9d6`.

`acl_in()` accepted PB=2 as the only new ACL L2CAP PDU. HCI PB=0 is **first non-automatically flushable** and PB=3 is **complete automatically flushable**. Valid ATT Write Command BLE-MIDI Note Off was discarded under both. Valid fragmented PB=0 starting L2CAP was likewise rejected. For contrast, source `firmware/src/ble/ble_midi_central.c` accepts PB 0/2/3 as starts. The changed peripheral source accepts PB=0/2 starts, and PB=3 only when the ACL payload contains the **complete declared L2CAP PDU**; PB=1 continues a prior valid start. Malformed lengths/CIDs and old-link packets retain rejection.

## Real native repro and fix evidence

Test `tests/host/ble_acl_start_flags_test.c`, which links entire source `firmware/src/ble/ble_midi_host.c` and its exact header rather than extracting functions. Baseline against exact unchanged PR #89 C: GCC compiler **exit 0**, native regression **exit 1**, **8/60 failed checks** (0/2/3 cases included). Baseline execution-log SHA256 `e8f93b5d4d60ca37ed0888653625ae7178eacd9afe5e1aace1f068ffe8756934`.

Minimal patch C proposed Git blob `74bfdaafd3a1329b996258d3a1314618c8b07fd0`, new test Git blob `56f9091bfccb805c3ca15017130574f76f0ac148`. GCC and Clang, each plain/UBSan/ASan+UBSan, executed both the new ACL packet regression and the original PR #89 SysEx/Note Off regression: **12 compile+run pairs, all exit 0**. New ACL checks: **80/80 passed for every configuration**. See committed `docs/scheduler-handoffs/smc-ble-acl-start-flags-20261023-native.log` and JSON manifest for exact arguments, executions and log hashes. No target device, live HCI peripheral, hardware flash, firmware package, actual Elite/Pro target build or static memory budget was tested.

## Consumer pipeline — no circular build gating

**Hourly Build now:** consume exact new producer commit SHA (in discoverable draft PR) and compiled-input Git blob `74bfdaafd3a1329b996258d3a1314618c8b07fd0`. Attempt pinned native Elite and Pro firmware + matching controller as **BUILD_ONLY**, without waiting for QA, merge, Pages, firmware hosting or device evidence. The new patch is stacked on PR #89 to retain its earlier SysEx preflight guard, so build from the exact stacked branch input (or matching combined tree), never from unrelated `nightly`.

**Independent QA concurrently:** re-fetch exact remote source/test blobs, rerun host tests against complete source checkout and packet-boundary negatives, verify no duplicate post-reconnect controls. **Sole Safe Merge later:** reconcile this shared BLE C file with **PR #50** (`4aa93f62b5f08eb5df9663747ada84fef590a014`, mailbox/credit backpressure) before integrating through protected firmware `nightly`. No other specialist branch, site, publisher, sequencer or data-safety file is modified. No installable claim until full model/controller/CRC/backup/QA/license/HTTP gates; physical owner test is production-only.
