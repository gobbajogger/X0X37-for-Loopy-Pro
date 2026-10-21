# SMC normal connection investigation — 21 October 2026

## Result and corrected scope

The cause of the owner's historical repeated connection attempts is **not established**. The owner clarified that the latest candidate has not been tested. No new physical failure of that candidate is asserted.

The handle-zero repair in `6eaf01f9d4c5325b1cc66e45e7033179b87affef` fixes a reproduced stop/cancel defect. Its triggering sequence includes stop intent. The report of ordinary automatic attempts does not establish that sequence. Keep the repair and its regression evidence separate from this unresolved connection diagnosis.

This investigation compares the last verified installed source `a5cb24dd08b40b80e802e03ff0d9c848315a9751` with the earlier working-source reference `53d5654c9bbaec10b64c55fb60d0e7d857ba8156`, and reviews the current transport at `6eaf01f`. It changes no production firmware or connection policy.

## What the retained records establish

- `evidence/latest-install/install-record.json` identifies the installed Elite development package and successful application/controller verification. Its physical SMC input result is NOT TESTED. Later builds do not prove a later installation.
- `docs/smc-device-investigation-2026-10-20.md` records controls working on the earlier source without a transport change, and a later CONNECTED → CONNECTING → SCANNING transition whose cause was not captured. The earlier source is not a retained, precisely verified last-good physical package.
- `evidence/session-audit/audit.json` contains a real, older connected snapshot: MIDI value handle 114, CCCD 115, last ATT opcode `0x13`, and rejected header `50 20 10 00 0c 00 05 00`. That header describes connection handle `0x50`, an ACL payload of 16 bytes, and a 12-byte L2CAP signaling payload on CID 5. The signaling command and its parameters were not retained.
- The raw Mac capture contains 1,271 exact MIDI notification payloads. Its SHA-256 is `a92f72f54c6aba7004e2c3a29759fa38818bb72a976486fb272b38c61d9d245a`. These are notification payloads, not the keyboard's failed connection handshake. Existing replay evidence is reused; replay cannot reconstruct missing connection results.

There is no retained event sequence for the reported unsuccessful attempts. Idle zero notification counters do not establish receive loss. A historical successful CCCD write does not identify where a later attempt stopped.

## Old and installed source comparison

The following files have identical Git blobs in the earlier and installed source:

| Path | Git blob |
| --- | --- |
| `firmware/src/app/project.c` | `7874245f74113cc0c7caac3beeae849d66780809` |
| `firmware/src/ble/ble_glue.h` | `d30343e367b0c4ad08b2a1a59c8e976b3d8c2bb2` |
| `firmware/app.ld` | `a7c859343fc5f1bf218cf5330dbd3553239a92ee` |
| `firmware/hal/fm1_cpu1.h` | `a3724275dc18a7642f16c56920b2be2dfa848aca` |

The controller image SHA-256 remains `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a`.

Source review found the same settings bits, boot order, controller initialization, mailbox setup, core handoff, and timer service cadence. The ordinary scan/create commands and service, characteristic, descriptor, and subscription requests are unchanged. This excludes changes to those recorded bytes and operations; it does not establish a physical timing measurement or rule out all runtime faults.

## Findings that do not yet establish the reported cause

### Signaling responses changed

The earlier central ignores CID 5 signaling. The installed central queues replies and explicitly refuses Connection Parameter Update Requests. This is a real normal-session behavior change, and the older snapshot confirms signaling traffic existed.

However, only the eight-byte header survives. It does not prove the received command was a parameter request, when it arrived, or how this SMC responds to refusal. Bluetooth Core, Volume 3 Part A, sections 4.20–4.21 permits refusal with result `0x0001`. Therefore, the refusal alone does not establish a protocol violation that forces a disconnect.

The new queue prioritizes replies over ATT and feedback. Source review found no deterministic local stall from one normal request when ACL credits return. Failure requires a concrete condition such as five queued requests, a reply still unsent after three seconds, or exhaustion of the original five-second discovery deadline. None is shown in the retained device evidence.

Reference: [Bluetooth Core L2CAP specification](https://www.bluetooth.com/wp-content/uploads/Files/Specification/HTML/Core-62/out/en/host/logical-link-control-and-adaptation-protocol-specification.html), sections 4.20–4.21. The synthetic request used in `evidence/session-stage3/replay.c` is test input, not recovered peer traffic.

### Existing normal-path weaknesses

Both the earlier and installed source can fail on an unsolicited ATT MTU request during discovery. Both can move back to scanning on automatic Cancel Complete before a racing successful connection event is handled. Both have startup/scan commands that are not retried after transport refusal. These are source-level weaknesses, but their presence in both versions does not explain the reported change by itself. No retained failed-attempt event identifies one of them as the trigger.

The short-fragment repair, later input-queue repair, foreground link-publication repair, and handle-zero repair retain their separate evidence. None is relabeled as a verified cure for this historical connection failure.

## Concrete next observation

`tools/smc_connection_watch.py` collects successive existing ED_DIAG snapshots through USB MIDI. It is intended for the native host that can access the keyboard. It does not need a new firmware build or any fader, knob, or button movement.

From the checked repository, the command is:

```sh
python3 -B tools/smc_connection_watch.py --seconds 45 --interval 0.25 --output smc-connection-watch.jsonl
```

The watch records UTC time, actual elapsed time, raw decoded diagnostic fields, current connection state/step, and missing replies. The requested interval is best effort; USB port-open and reply time add overhead. It issues only read-only ED_DIAG commands. It does not reset, toggle Bluetooth, clear records, pair with the SMC, or operate its controls.

Interpretation must follow the observed sequence:

| Observation | Boundary to investigate |
| --- | --- |
| SCANNING without a transition | Advertising reception or target selection |
| CONNECTING/CREATE followed by another scan | Link establishment result |
| CONNECTING/CANCEL, then a failure or rescan | Cancellation completion and event order |
| DISCOVERING at SERVICE, CHAR, or CCCD | The corresponding GATT transaction |
| DISCOVERING/SUBSCRIBE with ATT pending | Notification subscription response |
| CONNECTED followed by another attempt | Connection loss after setup |

These are investigation boundaries, not automatic diagnoses. Fast transitions can fall between snapshots. Last ATT/rejection fields can survive a reconnect and must not be treated as belonging to the current attempt without supporting changes. Existing DIAG does not retain the HCI connection status, disconnect reason, or complete peer signaling payload. If the watch cannot distinguish the failure, obtain only the missing boundary evidence rather than guessing another transport repair.

## Validation and ownership

Validation is limited to the host collection tool and its existing decoder. Firmware source, package inputs, controller image, and device storage are unchanged, so no firmware rebuild is warranted. The exact host checks and hashes are recorded in `evidence/smc-normal-connect-20261021/validation.json`.

The current session has no native Mac/USB access to the keyboard. No live watch, physical connection test, installation, or remote push has occurred here. **Root cause UNKNOWN; latest firmware hardware result NOT TESTED.**

Firmware Main Builder remains the single receiving integration/device owner. The remaining action is a read-only watch during a normal connection attempt on the native host, then an evidence-based repair if its sequence identifies a defect.
