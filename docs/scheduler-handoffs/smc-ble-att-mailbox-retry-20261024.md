# SMC Transport: ATT response mailbox retry (2026-10-24)

**Stage:** SOURCE COMPLETE / PRODUCER SCOPED NATIVE HOST CHECKED / INDEPENDENT QA PENDING / TARGET BUILD PENDING / DEVICE NOT TESTED / NOT INSTALLABLE.

## Consumed exact source and dependencies
- Git source repository `gobbajogger/X0X37-for-Loopy-Pro`, own PR #50 base and original HEAD `4aa93f62b5f08eb5df9663747ada84fef590a014`, source blob `f4415750533c0e43f1a6c45dcd3a54dac1af6605`, header blob `488977fab5e6300e8e2f7ad96cb834fe62c490f1`, original full root tree `c599c141d52db54798ae95b4fa530b7616fffebe`.
- Later stacked transport PR #89 `ca777a056e7de0075aac30927a4f6b158d55c465` and PR #94 `9083dc35a5e270930323b44dde7bdc5916f826f1`; #94 full root `447582f9bf505c2d448010b7ab23dd36e1ed7a33`, firmware subtree `9f6861806861251e3dde3b2a929f1b6634d1862c`.
- BEFORE this change, producer Build evidence for #94 claims real Elite/Pro/ctl BUILD_ONLY with hashes in PR #94 issue comment `6367845844`. It DOES NOT cover #50 or these ATT changes; no target build is claimed on new code.
- Source patch belongs to SMC transport; no flash/slot/website/clock changes.

## Problem reproduced
PR #50 correctly retained queued outgoing Note Off/SysEx and HCI command state when the controller mailbox rejected a packet, but synchronous ATT request replies (MTU negotiation, CCCD Write Response) still ignored mailbox rejection. When the mailbox became free, the lost reply was never retried, potentially hanging BLE client discovery/CCCD enabling even though MIDI notifications were otherwise functional. This is host-simulated, **not an attributable owner-device incident**.

## Narrow correction
Preserve at most one response-sized (BLEH_MTU) ATT reply in `bleh_t` when ACL credits or the mailbox prevent enqueue; retry it at the start of normal connected polling **before notification gating**, clear the queued reply on link reset, and refuse to overwrite it with a second request reply. Preserve #50 queued MIDI, ACL-credit accounting and HCI command retry, plus #89 SysEx bounds and #94 PB0/PB3 Note Off handling when independently combined by sole Safe Merge. Adds 1 bounded ATT state buffer (not yet target budget-checked).

## Actual native producer checks
Executed Linux x86_64, GCC and Clang, plain and `-fsanitize=address,undefined` (where stated), using historical checked BLE ABI/hdr and Git SHA-matched PR #94 affected C, **plus exactly re-applied #50 diff**. The combined-tree source was a local overlay, NOT a complete exact remote Git root.
- Before fix: `cc/clang -std=c99 -O1 -g -Wall -Wextra -Werror -I... tests/host/ble_att_response_retry_test.c firmware/src/ble/ble_midi_host.c ...` 4 compile exits 0, 4 expected run exits 1; 30 checks with 11 failures per run.
- After fix: same four compiler/sanitizer combinations compile exit 0, execute exit 0; 31/31 passed per run.
- Committed smaller executable regression `tests/host/ble_att_response_retry_test.c` against identical combined C: 4 fail-before runs (15 checks, 5 failures; exit 1 each), 4 pass-after runs (16 checks, 0 failures; exit 0 each), compiling actual BLE source.
- Combined mailbox/reconnect/SysEx/ACL suites (GCC/Clang plain/ASan+UBSan) after fix: mailbox 40/40 PASS, inbound PB flags 80/80 PASS, SysEx bounds PASS; all 12 compile/execution pairs exit 0.
- Negative cases: mailbox full, no controller ACL credit, ATT MTU response before CCCD enables notifications, repeated polls/no duplicates, CCCD Write Response, overlapping request, queued inbound MIDI Note Off, disconnect drops prior-link reply.
- Detailed native exit/status logs retained privately, not interpreted as full integrated QA, firmware build or hardware PASS.
- Unrun: complete exact remote PR50 root native test suite; combined #50+#89+#94 full source checkout; full pinned source model-target Elite/Pro/ctl build and budgets; independent QA; integrated OTA/backup/install negatives; license/distribution and immutable external HTTP verification.

## Consumers
**Hourly Build** may immediately compile this draft source BUILD_ONLY on its exact remote head, even if QA pending. **Independent QA** owns full exact remote source and negative retest. **Sole firmware Safe Merge** owns review/merge of #50 and conflict-free combination with #89/#94 in isolated integration PR targeting firmware `nightly`. No claim that any model is installable or device-proven.
