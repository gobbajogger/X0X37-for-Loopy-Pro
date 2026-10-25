# Project persistence reconciliation, 2026-10-25

Scope: requests LX-REQ-001 through LX-REQ-004 and provenance request
LX-REQ-028 in tracker commit `ce8bfd4a40179590485ecf35e6bb0be2d652e602`.
Inspected NIGHTLY source: `58e2c5205a02836df678083eaa8db9994b6192e8`.
This is source reconciliation and a bounded legacy-load repair, not delivery of
new reset/save/load/export systems or device qualification. Issues
[#2](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/issues/2),
[#3](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/issues/3) and
[#4](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/issues/4) were read in full.
Their original source audits are not implementation receipts.

## Existing functionality

| Request | Current source and observable contract | Remaining qualification |
| --- | --- | --- |
| Reset | `ui_project_reset`, `project_reset_stage` and `ui_project_end` reset all project content and runtime. The menu confirms `RESET PROJECT? / SAMPLES ARE KEPT`. Device Bluetooth/SMC roles and separate calibration/sample storage are retained. Reset restores autosave ON and follows ordinary quiet-save policy. | Combined motion lifecycle fixes and exact-candidate host/model/device evidence. Reset may overwrite the saved project after quiet autosave, as BEHAVIOR SET-07 explicitly documents. |
| Save | Menu Save queues during playing/active clip input; direct `ui_project_save` requests reject busy input. `project_save` preflights stream capacity before settings/sound writes. NOR storage uses A/B copies and verifies committed writes. | Existing PR127 addresses hash-cache collisions but its wider digest still does not establish byte equality. Multi-object saves remain non-atomic across power loss. |
| Load | GLO > LOAD SAVED is reachable, confirms unsaved edits and validates a staging project before runtime adoption. Failed staging keeps the live project, sequencer, samples and memory gauge. | The legacy arrangement rejection defect below; separate existing PAN/HP migration repairs PR123/128 and combined QA. |
| Export/import | `web/x0x_projects.{js,html}`, editor ops 64–74 and `docs/PROJECT_FILES.md` implement versioned full-project transfer. Portable files include original sample images and 512-byte headers; project-only imports require matching identities. Destructive sample changes require confirmation and downloaded recovery backup, readback and rollback/recovery hold. | Current-root protocol/runtime/companion evidence, later authorized physical interruption/recovery testing and separate website integration. Sample-only backup is not full-project export. |

These implementations must be retained, rather than duplicated. See maintained
BEHAVIOR SET-06 through SET-13 and `docs/PROJECT_FILES.md` for the actual contracts.
No sample write, calibration change, confirmation bypass or new format is part
of this repair.

## Recovered provenance

Commit `73d25b6d431aa5eaabaab54a0d640df2526fead4` (PR9, in the inspected
NIGHTLY ancestry) explicitly records publication of the integrated project
reset/save/load/export/import tree from original local commit
`7537a46ab49419920428299db8f3321d01e6fd16`, tree
`152ca944e9cd3d0913906a1d3fa9d8d2826ea639`. Its message explains that the
original 52 local commit objects were not recreated by consolidated publication.
That is a concrete successor/provenance lead, not independent verification that
the original unavailable object or every historical artifact matches today.
The inspected NIGHTLY `project.c` and `web/x0x_projects.js` are byte-identical to
their PR9 successor versions (SHA-256 respectively
`1c19cea86a74416dd36a255edabbce2192b956890646492853806f0556be8307` and
`80e22fdc151d5f5c5660f993bb0d33c5e940d9aacf72b3cd2f8214146d235f2e`).

The retained `docs/audit/project-management-dev-validation.md` describes the
implemented features and historical host/Elite/Pro development evidence at
`68e1117262aba958f96b6eb65cb3bfb1b3cc94ce`. Its source object and referenced
artifact bytes were not available in this checkout; its results are historical
reported evidence, not fresh qualification. The second historical reference
`66fb5c7d5ceec356513bf54b7d7b7ff499526909` remains unresolved. Neither missing
object proves a missing feature. Personal-context retrieval was unavailable.

## Bounded missing behavior: legacy arrangement refusal

The legacy pattern loader called `load_arrange1`, then silently substituted an
empty arrangement on every failure without marking the project invalid. A
readable truncated/wrong-format song or motion object, or an incomplete pair,
could therefore make explicit Load succeed and replace current edits while
discarding saved song/motion content.

The repair distinguishes the historical both-unavailable arrangement case from
an invalid or incomplete pair. Both members are inspected. Present malformed
objects and incomplete pairs reject staged Load; the startup loader still has
its safe defaults fallback and reports failure. Valid legacy song/motion data
still widen to 64 steps. Both unavailable retain pre-arrangement compatibility.

The storage API returns the same negative result for absent and unreadable
objects. Consequently two unreadable arrangement objects remain indistinguishable
from the historical absent pair; this bounded repair does not claim to detect
that case. A stronger distinction would require a separately reviewed storage
API/compatibility decision.

## Verification for this repair

Native Apple Clang 21.0.0 on macOS arm64 compiled the complete checkout's
production project/storage/codec code. The new persisted legacy fixtures failed
on the original implementation for each member's missing, truncated and
wrong-format cases, then passed with the repair. Valid arrangement widening and
both-unavailable compatibility passed. Every staged case checks unchanged live
project, sequencer, memory gauge and flash bytes; the storage harness forbids
sample/calibration writes.

A separate read-only reviewer accepted the exact source/test diff and
independently passed project-stage with ASan+UBSan (leak detection disabled,
undefined behavior stops execution). Runtime reset/load/cancel/rejection/save and
SMC state, project protocol confirmations/recovery, memory-full save preflight,
format-2 stream widening, and companion core/page host checks passed.
The unmodified runtime runner initially failed to compile macOS SDK headers;
`X0X_HOST_DEFS=-D_DARWIN_C_SOURCE` resolved that platform configuration. The
preflight and format-2 runners omit `X0X_HOST`, so their default commands failed
on the target `.pool` section; their native commands passed with `-DX0X_HOST`.
These are disclosed host invocation adaptations, not product source changes.

Exact commands, source identities, result/log digests and limits are in
`docs/scheduler-handoffs/project-persistence-reconciliation-20261025.json`.
No target builds, GUI tests, physical flash/power interruption tests, publication
or source merges were performed. Passing scoped host tests does not close the
full feature requests or certify a current installable package.

## Integration handoff

The sole firmware integrator should reconcile this narrow project.c hunk with
PR123 (PAN migration order), PR128 (legacy HP conversion), and PR127 (save cache)
on one chosen current candidate, retaining all independent evidence and rerunning
the combined project storage/staging/runtime/protocol and affected motion suites.
Build the changed candidate for Elite and Pro with pinned controller inputs and
budgets before later qualification. Source merges, main edits, hardware use and
binary publication are outside this task.

The designated tracker writer should update the existing ledger with this
source reconciliation and exact PR/head/test receipt; preserve previous values in
history. Keep device/deployed/build evidence separate and leave issues open until
their full acceptance criteria are met. This task does not compete with that
writer or the SMC/website owners.
