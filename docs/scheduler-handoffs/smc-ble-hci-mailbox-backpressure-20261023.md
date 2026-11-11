# BLE HCI mailbox backpressure — SMC Transport run 3

**2026-10-23 · SOURCE / FOCUSED HOST CHECKED · DEVICE NOT TESTED · BUILD PENDING**

## Scope and provenance

- Production base `main@c9718ae833eb64a5191c350922593df8a36017c9`; source file `firmware/src/ble/ble_midi_host.c` original Git blob `ece7f0cb3c2ba878dc4a351fcb864fcb2da5b8f7`.
- Source change Git blob `f4415750533c0e43f1a6c45dcd3a54dac1af6605`; new regression `tests/ble_mailbox_backpressure_test.c` blob `c24c65e4e651b2b12958fb37d7bcab7b76ed0199`.
- Code was edited and tested in the owner's local Linux x86-64 execution environment with a historical full-tree source projection. The *actual compiled BLE-MIDI host, host header, real HCI mailbox ABI, and preexisting BLE test source* each independently matched the current production Git blob before editing: `ece7f0cb`, `488977fa`, `8716949b`, `768d194b`. This **does not prove the entire Git tree** was reconstructed at current `main`; exact remote branch independent retest remains due.
- Related docs-only reproduction: optimization-watch PR #22 (`docs/audit/optimization-watch-2026-10-23-ble-mailbox-backpressure.md`). PR #18 changes `ble.c`/SMC central but not `ble_midi_host.c`. PRs #40 and #43 concern different files; no worker-owned source edit was overwritten.

## Reproduced failure, minimal fix

The real 1024-byte HCI mailbox can reject a frame while BLE controller ACL credits remain. Original peripheral `l2cap_send()` ignored send failure and decremented credits; `bleh_poll()` advanced the queued MIDI read index before the send, losing Note Off permanently. SysEx progressed past an unsent fragment; a failed startup HCI command incorrectly entered `cmd_wait=1` without any command queued.

The fix checks the actual `send` callback result. It commits the MIDI read index, SysEx progress, ACL credit decrement, pending HCI command, advertising desired state and parameter-update status **only after HCI mailbox acceptance**. Rejected startup/control commands retry on later normal polls without blocking the audio path. A disconnect still drops obsolete queued MIDI rather than replaying it into a new link. No BLE public struct fields, sample/slot handling, SMC control mappings or unrelated lanes changed.

## Executed native checks (all referenced source below: touched BLE blobs match live main)

| Check | Actual result |
| --- | --- |
| New regression on ORIGINAL `ble_midi_host.c`, GCC UBSan | **Expected fail:** 16 assertions, exit 1 (`1828 checks, 16 failures`) |
| New regression on PATCHED source, GCC ASan+UBSan | **PASS:** `1834 checks, 0 failures`, exit 0 |
| Same patched regression, Clang ASan+UBSan | **PASS:** `1834 checks, 0 failures`, exit 0 |
| Existing `tests/ble_test.c` native BLE and mailbox suite | **PASS:** `5153 checks, 0 failed`, exit 0 |
| `sh tests/host/ble_midi_receive_test.sh` | PASS, exit 0 (HCI fragments, LE signal, reconnect, stop) |
| `python3 tests/host/ble_capture_replay_test.py` | PASS, exit 0 (1271 recorded payloads plus resegmentation) |
| `python3 tests/ble_central_diag_test.py` | PASS, exit 0 |
| GCC and Clang `-std=c99 -O2 -Wall -Wextra -Werror` compile of patched BLE host | PASS, exit 0 |

Regression compile/run command (working tree root):

```sh
cc -std=c99 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer -Ifirmware/hal -Ifirmware/src/ble tests/ble_mailbox_backpressure_test.c firmware/src/ble/ble_midi_host.c -o /tmp/ble_mailbox_backpressure_test
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 /tmp/ble_mailbox_backpressure_test
```

The new regression fills the *real* mailbox with 255 one-byte frames leaving three free bytes, then checks retained Note Off, SysEx, startup Reset, advertising, connection parameter update, disconnect command, credits, no duplicates and no stale Note Off after disconnection. These are forced host conditions; device occurrence/performance impact is unknown.

## Stage and consumers

1. **Independent QA:** check complete exact remote branch tree and host/source model, rerun new test alongside SMC/central/clock regressions, examine event/reconnect interaction. Note that preexisting immediate ATT response paths may still drop control replies upon a mailbox reject; this focused fix only guarantees queued MIDI/SysEx and stateful command/update retries.
2. **Hourly Build:** obtain the exact integrated source then run pinned JieLi/AC79 V1.2.1 and four DSP references on Elite and Pro DEV, matching `ctl.img`, RAM/flash/pool/model/CRC/SHA256, installer backup/readback gates. **No target build or package qualification claimed here.**
3. **Safe Merge:** sole isolated integrator; review compatible PR #18/40/43 and subsequent findings without promoting automatically.
4. **User:** physical keyboard testing is their independent later stage; **not required to commit this source fix**.

No `main` merge, `gh-pages` publication, firmware binary, vendor input, physical flash or hardware PASS was performed.

X0X37-Role: LoopyX0X37 SMC Transport  
X0X37-Stage: FOCUSED HOST CHECKED / EXACT TREE QA PENDING / BUILD PENDING / DEVICE NOT TESTED  
X0X37-Consumer: Independent QA, then Hourly Build and Safe Merge  
X0X37-Evidence: tests/ble_mailbox_backpressure_test.c  
X0X37-Next: Rerun on exact branch SHA, then dev-build and isolated integration
