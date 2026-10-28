# LX-REQ-039: MBOOT through NIGHTLY — proposed protocol contract

Status: **DESIGN / SYNTHETIC MODEL ONLY**. This continuation does not implement
or enable a writable transaction, new wire commands, acquisition, firmware/UI
changes, a catalog entry, or device operations. The owner outcome remains normal
NIGHTLY installation; PR640's merged offline tool is a dependency, not that outcome.

## Ownership, exact inputs and bounded outcome

Retained source owner: `implement_mboot_source_adoption`; independent reviewer:
`review_mboot_adoption_source`; sole integration/intake owner: root. Website
producers retain all website files. Existing site delivery denials remain binding.

`nightly-source-closure.json` records the inspected Git blob closure at firmware
`21ce25dac3fef015dfd95f6b3b7371272c3fc5bd` and site
`909bb37c5afffd91fbb251d0dbd0a54293e413de`. PR640's Sol round 3, 17 C and 16
caller tests and pinned MBOOT build remain accepted for unchanged inputs; they
are not rerun or relabeled as a NIGHTLY transaction. This stage stops at an
independently reviewed contract, executable test-only refusal model and concrete
next implementation dependency. LX-REQ-039 stays OPEN until actual end-to-end
NIGHTLY delivery and separately authorized device acceptance exist.

## Actual caller/protocol closure

| Existing path | What it establishes | What it does not establish |
|---|---|---|
| Site `run` → `installKeepingSlots` → `Updater.install` | Package admission, sample-slot backup/migration when at risk, model-family checks and app→loader→app exchange | Complete raw unit snapshot, stable physical identity, MBOOT adoption |
| Site resume → `Updater.resume` / `finishApplication` | Loader model and later application product marker; single-port guard | Same physical unit across reconnect, full flash/head digest, transaction provenance; source explicitly warns identity is not exact-byte proof |
| Firmware `ota_stage` | Both flash.bin and ota.bin required; only known official/Felucca loaders; official path compares every byte of the 16 KiB head | A supported boot-installer-only/adoption protocol; do not weaken these checks |
| Felucca `ldr_session` | Checks package decrypts with unit configuration key; app-area writes; per-sector comparison/readback and update-record cleanup | Boot-slot writes; a key is not a unique unit identifier; ordinary app resume is not boot-erase recovery |
| Editor SMP_READ, project commands, CTL_READ | Scoped samples/projects and decrypted 64 KiB controller block | Raw full 1 MiB flash or retained-head attestation |
| Editor ED_BOOT_INFO/BEGIN/WRITE/READ/END/ABORT | Custom startup **picture/text** in boot-screen data sectors | Bootloader information or MBOOT update. Never repurpose these commands by name |
| PR640 `candidate` / `prepare.py` | In-memory transform of pinned complete snapshot; configuration at record 64; exact model/key/geometry/controller and preserved app/OTA bytes | Physical snapshot provenance/currentness, transport, writer, reconnect attestation, crash-consistent boot commit |

SMK loader executable bounds are `[0x4000,0x9d000)` for the app and
`[0x9d000,0xc0000)` for update-record cleanup (not the generic top-of-file FM-1
comment's upper bound). Its resume relies on the existing bootloader still
starting a staged updater. That assumption cannot justify erasing that bootloader.

## Non-negotiable invariants

1. Separate intents: **ordinary app update**, **first MBOOT adoption**, **adoption
   verification/resume**. No catalog filename or UI toggle silently changes intent.
   Existing ordinary NIGHTLY behavior remains separate and is not globally blocked
   merely because new adoption capabilities are absent. Do not claim MBOOT was
   installed or preserved from model/product identity alone.
2. Unit binding is an explicitly established stable physical-unit identity, exact
   Pro/Elite model, protocol revision and fresh session challenge. Browser port
   name, product string, matching encryption key, a caller-supplied UUID or a local
   file hash is not physical identity. This is identity/freshness binding, not a
   claim of cryptographic attestation against malicious hardware.
3. A complete immutable snapshot is exactly 1,048,576 raw bytes under a proven
   exclusive maintenance/quiescence regime. Capture receipt binds the unit,
   session, byte count, full digest, complete range coverage and mutation epoch.
   No duplicate/gapped/out-of-order chunk completion or concurrent store changes
   may produce a COMPLETE receipt. Private bytes remain local; only sanitized
   hashes/capabilities belong in public evidence.
4. Bind intent, exact source/boot build, complete package/controller, original head,
   expected transformed head, full snapshot and preservation expectations to one
   transaction digest. Every reconnect is fresh: reread stable identity and durable
   transaction state; stale observations cannot authorize a transition. Package
   reselection, unit swap, reboot/session change or epoch/digest change invalidates
   precommit preparation. A hash does not replace current observation.
5. Adoption changes only boot record `[0x20,0x40)` and boot slot `[0xa0,0x38d0)`.
   Config/application/projects/samples/calibration/key-MAC are equal before reboot.
   PR640 refuses an occupied calibration-copy reservation `[0xff000,0xff23c)`;
   do not erase it or use it as a transaction journal. After first boot, upstream
   may initialize that reservation: accept only separately specified exact valid
   TRIMCOPY bytes derived from the retained calibration, never arbitrary changes.
   Restored calibration or any other unexpected change is an explicit unresolved
   result, not preservation PASS. This test-only model covers pre-reboot equality;
   post-boot TRIMCOPY validation is an unimplemented downstream acceptance gate.
6. Ordinary-update-after-adoption preserves the entire current 16 KiB head and
   uses only the known Felucca app-only path with unchanged range/key guards and a
   complete matching controller. Head adaptation is local package construction,
   not a relaxed head guard or a device write. PR640's first-adoption caller rejects
   occupied calibration guard space; it cannot be reused unchanged as a general
   already-adopted-unit preparation path. That future read-only caller must verify
   the installed exact MBOOT identity plus valid guard ownership without rewriting
   either. Unknown/stock boot state may use existing ordinary update behavior, but
   must not be represented as proven MBOOT preservation.
7. Cancellation before any possible boot write is clean and releases only the
   session-owned resources. Once a write might have started, cancellation, lost
   acknowledgment, disconnect, timeout and crash mean **OUTCOME_UNKNOWN**, never
   “nothing written”, automatic success, blind retry, or automatic ordinary resume.
8. Boot commit must have a reviewed, target-qualified interruption-safe primitive
   with persistent same-unit transaction status and exact readback. This is absent
   today. A host journal, double-buffered boot-screen record, ordinary app-loader
   retry, watchdog reset or “write header last” is not proof for a single boot slot.
   No free flash reservation, A/B slot, atomic NOR write or ROM fallback is assumed.
9. Completion needs fresh same-unit exact head/container readback, preservation
   readback, transaction status and later app/controller identities/readback. A
   product marker, disappearance/reappearance, progress100, installer timeout or
   lost finish acknowledgment cannot establish adoption or end-to-end success.

## Proposed state machine (requirements, not implemented wire protocol)

The following names are semantic states, not allocated command numbers. A future
capability response is authoritative only after its actual firmware implementation
and source/target proof; the all-capable fixture used in tests is explicitly fake.

| State / event | Required evidence | Next state / refusal |
|---|---|---|
| START ordinary | Existing normal installer admission | EXISTING_ORDINARY_PATH; no MBOOT claim |
| START adoption | Stable binding + consistent full-snapshot capability | SNAPSHOT_REQUIRED; absent capability → UNSUPPORTED_ADOPTION |
| Snapshot complete | Exact size/digest/range coverage, fixed epoch/session, retained private backup confirmed | SNAPSHOT_VERIFIED; partial/stale/cancel → refuse cleanly |
| Prepare | PR640 transform and exact model/key/config/layout/controller/package inputs; separate supported writer and interruption proof | PREPARED_MODEL_ONLY; missing write/status/readback capabilities → PROTOCOL_GAP, no writes |
| Revalidate | Fresh nonce, same unit/model/session/epoch/full digest/head, identical transaction/package pins | REVALIDATED; any change → STALE_INPUT / UNIT_CHANGED |
| Hypothetical begin | Target-supported reviewed commit primitive; same transaction acknowledged durably | VERIFY_REQUIRED; this model emits no command and no write |
| Timeout/disconnect/cancel after begin | No definitive transaction receipt | OUTCOME_UNKNOWN; neither cancel-success nor ordinary resume |
| Reconnect while unknown | Fresh same-unit binding and exact durable transaction ID/status/readback | NEEDS_RECONCILIATION; never directly COMPLETE or replay begin |
| Reconcile not-started | Fresh full original state and status prove no boot mutation | New snapshot/revalidation required; no reuse of old preparation |
| Reconcile committed | Exact expected head plus non-boot preservation proof | ADOPTION_READBACK_VERIFIED; still need post-boot guard/boot/app checks |
| Reconcile partial/foreign/missing status | Incomplete, conflicting or unknown facts | STOP_NO_AUTORETRY; preserve backup; no recovery/device action authorized |
| Already-adopted ordinary intent | Installed exact boot identity, proven valid calibration guard, full current head and original normal guards | FUTURE_PRESERVING_UPDATE; not PR640 first-adoption transform |
| App update completion | Fresh same-unit exact expected app/controller plus preserved head; post-boot guard check | END_TO_END_VERIFIED only after actual target acceptance, not this model |

## Protocol feasibility and sequence

Current protocol satisfies **none** of stable-unit session binding, consistent raw
full snapshot, externally verifiable head/full-flash digest, durable boot adoption
transaction status, or interruption-safe boot commit. There is no safe composition
of present commands that supplies them. Their absence is a protocol gap, not a
request to obtain a manual dump and then bypass the installer.

1. **Ready source dependency:** independently design a versioned **read-only**
   capability/unit-binding + complete snapshot/digest observation contract under
   maintenance/quiescence. Specify identity provenance, raw/encrypted address
   representation, bounds, epoch/challenge lifetime, privacy and exact responses.
   Validate feasibility against actual target primitives and memory/time budgets;
   no opcode is reserved here. Add only that independently reviewed read-only
   firmware/host implementation and mocked tests first. Actual device reads still
   require their separate authority. Existing data APIs are not silently broadened.
2. **Independent design blocker for writes:** establish supported boot commit,
   persistent status and interruption/readback guarantees. If the single-slot
   architecture cannot provide the required guarantees through the normal route,
   return a concrete unsupported result and reviewed alternatives to root. Do not
   implement an erase/write workaround or move into recovery research in this stage.
3. **Later integration:** pure host preparation/state handling, separately reviewed
   exact boot-payload admission (not arbitrary loaders), then controlled backend.
   Preserve ordinary OTA code until supported design and exact proof exist. No
   writable implementation is authorized by passing the present model tests.
4. **Website owner:** approved proposed UI → internal review → implementation →
   actual browser tests for intent, progress, cancel, reconnect, private backup
   retention and unknown outcomes. Never route adoption through the ordinary
   loader-resume button. No website source is changed by this continuation.
5. **Delivery/acceptance:** exact Elite/Pro app+matching-controller packages,
   current catalog policy, native/browser/integration checks and authorized
   same-unit interruption/preservation/boot acceptance. Source merge, publication,
   owner availability and device proof remain separate receipts.

## Executable proof and coverage limits

`tests/host/mboot_nightly_contract/test_contract.py` runs a pure model with synthetic
observations. It checks current-capability refusal; ordinary route separation;
partial/stale/swap/reselection/cancel refusals; occupied guard; preservation;
post-begin uncertainty; reconnect identity and durable status; and false completion.
`source-closure` checks bind these tests/design to the inspected caller files.
No model capability bit asserts a real firmware feature. No model state invokes
MIDI, serial, USB, flash, installers, subprocesses or file output. Existing PR640
preservation tests are reused, not replaced by boolean mock claims.

Focused later test matrix: wrong Pro/Elite, wrong key/controller, missing config,
banked app geometry, all head mismatch bytes (existing PR640 proof); changed unit
with identical product/key, repeated/replayed nonce, capture chunk gaps/overlap,
mid-capture mutation, staged-but-uncommitted cancellation, every boot-operation
interruption boundary, lost ACK after commit, stale host journal, partial/unknown
readback, invalid TRIMCOPY/valid user recalibration and successful same-unit retry.
Only the implemented model cases are PASS here; actual wire, interruption timing,
post-boot guard/readback and hardware cases are NOT_IMPLEMENTED / NOT_RUN.

### Reproduction for this bounded stage

```sh
python3 -B tests/host/mboot_nightly_contract/test_contract.py
python3 -B tests/host/mboot_nightly_contract/check_source_closure.py \
  --firmware . --website /path/to/site-repository-containing-the-pinned-commit
```

The source checker reads the pinned Git blobs and rejects changed protected
firmware callers in the candidate. It does not prove runtime capabilities.
A focused execution of existing `tests/ota_test.c` (SMK Elite selection) and
`tests/ldr_test.c` (SMK app-loader logic) at the pinned current source also passed
with the unchanged complete Elite PR639 fixture
`ba7703fe2226a883ffdda41526d36c58060e355fdfa96bdab232b844aafa9f55`.
These tests use simulated NOR and prove the current ordinary staging/app-head
preservation/key-refusal boundary only. They do not exercise MBOOT adoption,
a current native application build, physical power-loss or real flash.
