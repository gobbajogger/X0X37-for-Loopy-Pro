# Native OTA transport repair packet — 2026-10-24

Continuation: [native macOS provider source](native-ota-coremidi-provider-20261024.md)
adds a read-only installer diagnostic and native pairing observation. The
original validation/source hashes below describe the earlier diagnostic stage;
they do not cover this later source revision. Destructive transport stays NO-GO.

**REPAIR HANDOFF — PRODUCTION TRANSPORT INCOMPLETE / NIGHTLY NO-GO.**
This supplementary lane delivers a bounded contract harness, read-only Linux
libusb observer and pinned API investigation. It enables investigation, not OTA.
No new physical identity provider, installable firmware or hardware PASS exists.

## Source and ownership

- Repository: `gobbajogger/X0X37-for-Loopy-Pro` (public GPL-3.0-only source).
- Own isolated lane: `lane/transport-contract-20261024-1236`.
- Consumed PR121 head: `2e8c2795ffab81d663890e2ed3b88f470da924de`, root
  `d5bf900e7a53cab51463304e99d2fe856fc3f703`. It includes PR114 R9
  `5b16ff81ea1dcb731b719f0505ae2349571481cf` and PR121 fixture reconciliation.
- Unchanged installer blob: `f1e2a91615b26b3ba84fe63ee9ae8dbfe5f74882`;
  unchanged firmware tree: `5c209609b770fd13949230aeb80956594fd6b741`.
- No `AGENTS.md` or `.agents/skills` files exist in the checked current tree;
  `/workspace/.agents` is empty. Read current `BUILDING.md`, `LICENSE`,
  `LICENSING.md` and `docs/rules/firmware-nightly-promotion.md`. Historical
  branches' missing rules are not represented as current checked files.
- Read live PR114/121/124 discussions and open PR list before edits. PR114
  remains the Data Safety producer. This lane does not modify its branch.
  Sole Independent QA and sole Safe Merge retain acceptance/integration.
  [PR124 restoration status](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/124)
  and browser admission task `01a16de4` retain their distinct scopes.
- Parent task coordinates downstream cloud tasks. No successor or duplicate
  producer, schedule, release, protected ref or publication was created here.

## Deliverables and their limits

`tests/host/ota_transport_boundary_test.py` imports the **whole real installer
module** from this checkout, replacing only Mido with an adversarial simulated
transport. It asserts B receives **zero** UPGRADE, package response or sample
erase messages. Current R9 fails this stricter contract. The diagnostic stays
separate from the existing runner; no xfail/skip transforms failures into PASS.
This is not a physical reproduction, nor evidence that native handles actually
retarget inside send. Real MidoBackend has no physical_id and ordinary install
still refuses before UPGRADE. Input-only token tests identify what must remain
untrusted if a future backend supplies a token without binding output.

The cases cover inputA/outputB under equal, normalized and sole-output names;
replacement during input/output open; disconnect before send; in-send permanent
and transient rerouting; queued B callback after the lookup returns to A; strong
captured-handle positive; fresh synthetic loader rename; absent native identity;
wrong device family and wrong destination loader. Existing root suites cover
full synthetic application-to-loader flows, supported Elite/Pro migrations,
foreign loader, interrupted firmware writes, backup and restoration. No session
protocol is introduced; replay/wrong-session acceptance remains a future gate.

`tools/ota_usb_observe.py` uses Linux libusb enumeration and optional explicit
BUS:ADDRESS open/close. It never claims an interface, detaches a driver, resets or
configures a device, reads string/serial values, transfers USB/MIDI payloads,
selects a sole device automatically, or opens a fallback. Descriptor serial
presence is only a boolean. `selected_open_object_checked` means only
`libusb_get_device(handle) == selected libusb_device` pointer consistency. It
does **not** prove kernel enumeration/open race safety, native endpoint pairing,
unit identity, disconnect behavior or continuity across enumeration. Phase and
bus/address/port path are annotations/locators, never trust anchors. The tool is
not connected to `Updater` and has no write API.

`tests/host/ota_usb_observe_test.py` covers descriptor ABI, explicit selection,
no automatic open, absent/ambiguous selection, no fallback on failed open,
reference lifetime, context cleanup, and handle close after success/mismatch.
These tests use a fake API. The actual Linux libusb enumeration was also run;
it reported an empty device list. `/dev/snd` and `/dev/bus/usb` are absent.

The [independent API audit](native-ota-transport-api-audit-20261024.md) records
primary-source URLs and hashes. Its important additional finding is that pinned
RtMidi ALSA output sends to subscribers, not exclusively to a saved destination.
CoreMIDI's saved endpoint is a different contract and supplies no reboot proof.

## Selected route and exact remaining fact

Use **one directly opened USB device handle for both MIDI directions**, with no
automatic reopening and permanent invalidation on disconnect, as the smallest
candidate for the same-open-handle property. Do not certify the existing
Mido/RtMidi name/index/subscriber adapter as that transport. API/source review
supports investigating libusb; hardware/platform observation is still absent.

Cross-reboot is a separate unsolved gate. App and loader descriptors have
`iSerialNumber=0`; handshake carries model/package, not unit/session identity.
JEDEC model ID and CRC-derived BLE calibration address are not trustworthy unit
IDs. Do not read/export protected trim/key_mac values or invent serials.

Before enabling writes the receiving Data Safety owner must supply either:

1. A documented, safely readable nonreusable unit identity on every **supported
   already-installed** app and loader, plus actual native input/output binding;
   or
2. A specified session continuity protocol and a separately proved **legacy
   bootstrap**. A new app's future session guard says nothing about the initial
   old app. A session-personalized staged loader is an investigation option,
   not compatibility evidence: it needs lawful validated package personalization,
   target builds, safe initial native handle, mandatory nonce checks and final
   app/restore continuity. No legacy fallback may send destructively to an
   unbound endpoint. Resume and power-loss policy must be explicit.

An unavailable physical fact prevents safe production enablement in this stage.
No name polling, sole-port trust or post-send check can manufacture that fact.
Legacy normal install and unauthenticated loader resume remain refused.

## Required next observations and commands

On an authorized Linux host, **without flashing**, run:

```sh
python3 -B tools/ota_usb_observe.py --phase application
python3 -B tools/ota_usb_observe.py --device BUS:ADDRESS --phase application
```

Substitute the observed explicit locator; do not script a first/sole match.
The observer's output alone cannot qualify OTA. On separately authorized
application/loader states, collect analogous descriptor observations. Do not
ask this read-only tool to trigger that transition.

The actual transport implementation must then demonstrate on the supported
platform/version that both directions belong to the same opened USB device and
that the original handle cannot deliver harmless marker traffic to B after
unplug/replacement, including before/during open and send. Use simulation or
instrumented captures; destructive firmware/sample traffic and actual flashing
are not authorized by this packet. Record positive A delivery, B zero-delivery,
disconnect errors, permanent invalidation, callback epoch and no implicit reopen.
Keep cross-reboot/session and Elite/Pro identity tests separate.

Run the strict gate against the actual repaired module:

```sh
python3 -B tests/host/ota_transport_boundary_test.py
python3 -O -B tests/host/ota_transport_boundary_test.py
```

It must eventually pass the B-zero invariant without deleting scenarios or
simply weakening their outputs. A different backend may require a separately
reviewed adapter-specific harness; this Mido adversarial contract does not prove
a libusb implementation's behavior. Rerun the complete-checkout normal and -O
installer/slot/controller matrix and PR121 fixtures. If firmware changes are
needed, build real pinned Elite/Pro DEV app+loader and matching controller,
record actual CRC/SHA/budgets; host mocks are not installable firmware.

## Consumer packet

**Data Safety PR114 owner:** independently inspect these diagnostic source/tests
and the API audit, then implement one native transport plus genuine continuity
design on your owned lane. Preserve all CRC/model/backup/readback guards and
explicitly classify supported vs refused legacy/resume cases. Do not cherry-pick
unrelated PR124/browser changes or overwrite their owners.

**Sole Independent QA:** consume the exact delivered commit/tree and independently
rerun strict B-zero tests, callback/open/send/disconnect cases and complete-root
normal/-O suite. Review the production backend, not just synthetic physical_id.
[Existing QA findings](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/114#pullrequestreview-5638209425)
remain relevant. No QA self-approval is claimed here.

**Sole Safe Merge PR103:** do not integrate this as an enabled/qualified transport.
Consume a repaired exact combined tree only after independent acceptance.
Unchanged compiler work can continue BUILD_ONLY separately. This stage does not
qualify any package, controller distribution, NIGHTLY installer, production or
physical hardware test.

Public committed evidence contains source, synthetic diagnostics and commands
only. No private artifacts, vendor source/binaries, controller/firmware bytes,
credentials or user sample archives were created or uploaded. Private main
Library artifact IDs/hashes are therefore not applicable to this stage.

## Executed validation receipt

Tested source commit: `8e366b6c388e45a71074c133861cb6a003615fab`;
root tree: `8a25b824a7ef9c5a1220c0c6ea609d43f7774e98`.
This section and text logs are an evidence-only follow-up to that tested source.
The [machine-readable receipt](../transport-evidence-20261024/receipt.json)
contains every argv, exit, log byte count and SHA256. It records Linux x86_64,
Python 3.12.14, Node 24.19.0 and installed libusb 1.0.28 (the separately audited
upstream source is v1.0.29).

- `X0X_TEST_JOBS=1 bash tests/run_tests.sh` with the receipt's 23 explicit
  installer/data-safety selectors: **exit 0, all 23 selected suites passed**.
- Every selected suite's Python script run directly with `python3 -O -B`:
  **23 commands, all exit 0**. This includes PR121 fixture reconciliation,
  Elite/Pro model paths, foreign loaders, slot integrity, interrupted
  backup/write/restore, finish ACK and controller readback/publication guards.
- New observer tests normally and with `-O`: **9 tests each, exit 0**.
- New strict boundary tests normally and with `-O`: **15 tests each, 9 assertion
  failures, exit 1**. Six zero-foreign-message checks fail, one callback check
  fails, and two further foreign package-response/sample-erase checks fail.
  These are real failed contract assertions, not xfails or reported PASS.
- Actual native libusb enumeration: **exit 0, devices=[]**; explicitly selected
  absent `--device 1:2`: **exit 2**, structured refusal, no fallback.
- Python AST parsing and staged `git diff --check`: **exit 0**.
- SMK target package/JS parity is explicitly **SKIPPED** by the selected runner
  because built app/loader and pinned SDK are not present. No firmware source
  changed, so this observation/diagnostic stage does not require fresh target
  compilation. New Elite/Pro/controller artifacts, actual native hotplug traffic,
  cross-reboot proof, flashing and publication are **NOT RUN**.

Independent subagent source/API review found no blocking issue for delivering
this explicitly failing diagnostic and read-only tool. This is not the sole
Independent QA role's acceptance of production OTA.

RU: Этот этап добавляет проверяемые тесты границы транспорта и инструмент
наблюдения USB без записи. Рабочая OTA-установка и связь одного устройства после
перезагрузки не доказаны. Требуются отдельные доказательства транспорта и
безопасного начального перехода со старой прошивки; NIGHTLY остаётся NO-GO.
