# Product continuation: bounded parallel handoff

Planning snapshot: 2026-10-28 03:32 UTC. This document preserves existing source,
review, integration and publication ownership. It records product work, not a new
role registry, permission policy, queue or scheduler. No request is completed by
this handoff, and it does not adopt additional request IDs into the canonical
ledger.

## Inputs and evidence limits

- Coordination input: `7237aafd4d341aa5b3430efc8188aa6177d74efb`.
- Firmware NIGHTLY: `8a00ecd20d455e5871c4561bc142d47b214df1a8`.
- Website main: `20f792cd7edfca28d75bc98e212a9ad24dd60e07`.
- Exact approved inputs and unchanged passing evidence remain reusable. Refresh
  the relevant current head/base before integration; inspect changed dependency
  closure rather than repeating all work because a timestamp or SHA changed.
- Source review, native checks, compiled packages, source integration, website
  publication, live served bytes and device acceptance remain separate results.
  Actual action-specific refusals and repository protections remain binding.

## 1. LX-REQ-011/012: reject false-empty sample slots before OTA

**Existing owners:** firmware installer producer, assigned independent Firmware
QA reviewer, then the sole firmware integrator. Website parity remains with its
separate source owner.

**Candidate:** [firmware PR666](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/666),
head `92be9b8907d88f13e87a1b3fd73e0bbb928b4fe1`, based on the firmware NIGHTLY
pin above. The changed module blob is
`96955871ba394a26abab4473ed8bc4a391ad7683`; the registered runner blob is
`f9759bd69dbe9431da55a339163063f511919a97`.

**Reason:** contradictory empty-slot metadata must not cause the installer to
skip sample backup and enter firmware writing. The narrow parser predicate
preserves genuine empty/occupied descriptors and existing truncated-record checks.

**Current action:** independent Sol review completed with APPROVE, round 1/3,
for the exact candidate. Native Node and affected negatives passed; the package
subcheck was explicitly skipped. [Exact review and consumer handoff](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/666#issuecomment-6459943375).
Integration is not yet claimed. The producer's earlier simulated-MIDI checks
remain distinct from the independent review.

**Acceptance:** corrupt named, used-size and truncated empty descriptors fail
before installer writes; genuine controls retain their flow; link cleanup and
the existing relevant test union remain correct. Record exact source, commands,
exit statuses and review findings. The sole integrator then qualifies any normal
NIGHTLY source merge against current inputs and protections.

This changes firmware-repository browser source, not compiled firmware inputs.
Do not request an unrelated firmware rebuild. Do not overwrite the website's
newer importer with this repository's copy. Loader-unit continuity, atomic
snapshot work, real browser/device tests and website delivery are separate scope.

## 2. LX-REQ-014: qualify Catalog R48 on the actual current website main

**Existing owners:** assigned independent Website QA, then sole Website Main
Merger and sole Website Publisher. Return only demonstrated source discrepancies
to the existing Catalog producer; do not appoint another producer.

**Candidate:** [website PR83](https://github.com/gobbajogger/x0x37-for-Loopy-Pro_nightly/pull/83),
head `5e575a614f27bb9dd40173fa355895f97d6193dd`, tree
`bace9b33574bdccac0be9b7348665a6baa1be559`. Current-main composition target:
`17e8b0bdb11f1cd3c225d5e9d663cfea4ca6c687`, based on the website main pin above.
[The 01:55 review](https://github.com/gobbajogger/x0x37-for-Loopy-Pro_nightly/pull/83#issuecomment-6458558451)
covers the virtual composition/history seals, not full native/browser acceptance.

**Current action:** the assigned QA consumer materialized the actual composed
checkout and verified all 547 file bytes/modes and Git tree `17e8b0b...`.
Generated parity, catalog schema/provenance and immutable history checks passed;
final independent acceptance and integration are separate pending steps.
The earlier `132cbd...` composition is historical.

**Reusable evidence:** repaired history negatives, 281/281 edge cases and 12/12
page cases, plus unchanged npm-chain/build/parity receipts. Do not call the
monolithic repaired-head npm command rerun when only its applicable components
were executed. Preserve the historical failed result at the older source.

**Acceptance:** actual composed checkout identity, retained history/test union,
explicit passed/failed/unrun coverage and independent acceptance. A documentation
delta does not require unrelated firmware work. After acceptance, the sole merger
handles draft/readiness and normal protected source integration; the publisher
separately qualifies any permitted website update and live-byte readback.
Source-only history rows must not become selectable firmware by metadata alone.

## 3. Existing UI and intro continuation

**Existing owners:** the retained UI/animation owner and independent visual
reviewer. This continues accepted work; it creates no new canonical request ID
or second UI writer.

**Input:** reviewed source `e2f881b0b0d5fbde8b4e352ab03c8ae2adc46058`, tree
`0dcdd4dc5abca2506b361090140ce84fb141ed38`, existing approved soundtrack,
storyboard and actual exported UI activity. The current continuation has consumed
the real 192-frame activity export and native semantic content; a scoped proposal
review passed. Final rendered-video acceptance is still pending.

**Next action:** complete coherent semantic 3D content, truthful step/GR/history/
BREAK/piano-roll activity and smooth meaningful motion joins. Reuse approved
visual decisions and actual telemetry rather than decorative invented activity.
The sequenced piano roll is not an external-MIDI monitor.

**Acceptance:** short actual-content/motion proof, retained independent review,
then complete render/mux with the approved audio and verified owner-available
delivery. Proposal approval is not final render acceptance. Firmware intro
integration, public source delivery and hardware validation remain separate.

## Parallel dependency graph and next consumers

The three units above are independent: browser-source QA, website composition
qualification and 3D continuation do not share a source writer. Their checks may
run concurrently. Serialize only overlapping writes and target-ref integrations.

```text
PR666 exact QA → producer correction if needed → sole firmware source integration
PR83 actual composition QA → sole website source integration → qualified publication
Actual UI activity → scoped visual proof/review → final render and usable delivery
```

For each transition, retain the same request/stage, exact inputs, result and next
owner. Verify the consumer's first substantive action; a comment, enabled flag or
dispatched task is not consumption. An already authorized next stage does not
wait for an owner acknowledgment. A real gate blocks only its dependent action.

## Remaining owner queue index

This index preserves existing canonical requests; it does not reassign them or
restart completed stages.

- **LX-REQ-001–004, 008, 034, 037:** lifecycle and Project Workspace owners close
  remaining save/load/reset/round-trip/interruption proof. Retain PR651 strict
  evidence and its specific unresolved integration restriction. Approved local
  repairs are not new implementation work.
- **LX-REQ-005–007, 009, 022, 030, 035:** current SMC/sequencer/UI owners retain
  accepted behavior and qualify missing interactions only. Actual hardware proof
  is separate from host/controller/source evidence.
- **LX-REQ-013, 014, 016, 018:** current selector, Build, QA, Catalog and delivery
  owners consume exact qualified package identity. The current e2 private package
  has compiled-input equivalence to `2c8b1572b013180725d7302e9337e7376bbf97b7`;
  reuse unchanged valid outputs rather than rebuilding. Private availability is
  not public hosting or hardware acceptance; `_900` is not build number 900.
- **LX-REQ-010, 015, 017, 021, 026, 028, 029:** existing source/tooling owners
  disposition unique residuals against current successors. No blind old-stack
  merge, bulk branch deletion or repetition of completed generator repairs.
- **LX-REQ-023, 032, 033:** performance and sound owners close actual measurement,
  assignment and compatibility gaps. Helper timing is not measured device gain.
- **LX-REQ-019, 020, 025, 027, 031, 036:** existing maintenance/intake owners retain
  completed bounded work and verify real changed-request/result consumption.
  Do not replace product execution with repeated fleet or documentation audits.
- **LX-REQ-024:** production retains separate exact-model hardware, review and
  action-authority requirements. **LX-REQ-038 remains owner-deferred.**

Any accepted work not yet represented by a canonical public request row remains
with its existing owner and continuity record. This document neither discards it
nor silently creates or marks a canonical row complete.
