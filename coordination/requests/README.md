# Request progress

This index preserves **all 30 initial requests** alongside the existing [139-row PR queue](../merge-queue/README.md). A new request never silently replaces an earlier one. The [ledger](requests.json) is authoritative; this compact view reflects the supplied 2026-10-25 09:02 UTC handoff, not live execution.

**Source-ready is not delivered.** Source, tests, builds, deployed bytes and hardware proof have separate fields. Role ownership does not establish active execution. `unassigned` is an explicit ownership gap.

Current focus: SMC active-lane/context-label refinement continues on PR168; Up/Down banks and strip8 mode switching remain design-only pending owner mapping preview. Physical Pause binding awaits live labelled capture. Website PR18 fidelity correction continues. Scheduler prompt repair is underway, with completion/readback pending. Compiler-runtime attribution is covered; three optimized-libc notice mappings remain unresolved.

## Requests

| ID | Request | State | Owner / next step |
|---|---|---|---|
| [LX-REQ-001](requests.json) | Reset project without touching imported samples | RECONCILIATION REQUIRED | unassigned. Reconcile current reset and unavailable historical commits; assign lifecycle and sample-preservation QA. |
| [LX-REQ-002](requests.json) | Reliable explicit Save Project | QA AND RECONCILIATION REQUIRED | unassigned. Assign combined project/NOR/cache/save/power-loss QA for PR127 on current root; resolve collisions. |
| [LX-REQ-003](requests.json) | Explicit Load Project and safe imported-project loading | QA AND RECONCILIATION REQUIRED | unassigned. Reconcile existing UI/serialization; assign staged-load, legacy migration, rollback and unsaved-edit tests. |
| [LX-REQ-004](requests.json) | Portable full-project export and import | RECONCILIATION REQUIRED | unassigned. Locate existing transfer implementation and historical evidence before duplicating; define project-only versus embedded samples. |
| [LX-REQ-005](requests.json) | Readable SMC mixer layout and deliberate controls | SOURCE AND ELITE BUILD REPORTED UI REFINEMENT ACTIVE | SMC layout and control implementation owner. Finish active-lane highlight and actual context-aware button labels, including filter S/R rather than assuming Solo/Record; independent review and exact-root qualification follow. |
| [LX-REQ-006](requests.json) | Physical Pause/context-lock mapping and LED feedback | ACTIVE CAPTURE MAPPING UNVERIFIED | Replacement-control capture owner with SMC implementation owner. Obtain live labelled capture to identify actual Pause/control messages; preserve STOP and mapped actions; bind/verify only after identification. |
| [LX-REQ-007](requests.json) | Multiple independent SMC controllers | IMPLEMENTATION REQUIRED | unassigned. Assign transport/controller implementer for per-slot queues, generations, Solo release, pickup and feedback on current accepted source. |
| [LX-REQ-008](requests.json) | Reset/load/transport motion lifecycle safety | COMBINED QA REQUIRED | Motion/sequencer producer and independent QA. Pin one current combined root; reconcile motion/clip fixes and run boundary negatives. |
| [LX-REQ-009](requests.json) | Preserve probability/TIE and RANDOM behavior | COMBINED QA REQUIRED | Sequencer producer and independent QA. Combine engine/sequencer skipped-MIDI, orphan-TIE and RANDOM tests; adapt bass_on callbacks and swing parity. |
| [LX-REQ-010](requests.json) | Preserve unique recording/DSP fixes in older WIP | RESIDUAL WORK RECONCILIATION REQUIRED | unassigned. Extract unique recording, swing/index, signed-shift and limiter deltas onto current source, including PR65/140 parity. |
| [LX-REQ-011](requests.json) | Durable sample backup/restore and same-device safety | BLOCKED SOURCE WRITE AND QA | Data Safety and Installer source owners. Resolve historical source-write denials through authorized route; integrate prepared repairs and run independent combined backup/export/reselect/readback/same-unit/OTA negatives. |
| [LX-REQ-012](requests.json) | Unified clear installer and sample-management page | ACTIVE CORRECTION NOT DEPLOYED | Installer illustration implementation owner. Complete faithful square 240x240 vector corrections in website PR18; preserve scripts/controls and historical SMC-preview label; review then single publisher. |
| [LX-REQ-013](requests.json) | Public BT-inclusive NIGHTLY delivery | THREE LIBC NOTICE MAPPINGS REMAIN | Controller notice/provenance completion owner. Resolve three optimized-libc notice mappings; verify exact-package corresponding-source/notices; hand off publication under action-specific authority. |
| [LX-REQ-014](requests.json) | Accurate public catalog/history and retained installable builds | LATEST QA SOURCE PENDING INTEGRATION | Catalog producer and website QA. Use website PR13 1fe8a5b and latest scoped QA; reconcile publication/history and stale circular prerequisite. |
| [LX-REQ-015](requests.json) | Recover residual installer UX and publisher safety changes | RESIDUAL WORK RECONCILIATION REQUIRED | unassigned. Map unique browser/accessibility/reconnect/catalog/safe-publication semantics onto accepted installer; retain production guards. |
| [LX-REQ-016](requests.json) | Exact combined-root build, independent QA and integration | COMBINED CANDIDATE NOT ESTABLISHED | Sole firmware integrator with Build and independent QA. Select bounded current candidate; consume producer proof; run remaining combined tests/build closure; record source-to-package-to-delivery receipts. |
| [LX-REQ-017](requests.json) | Fail-closed host checks and generators | FIX AND QA REQUIRED | Build source owner and independent QA. Retain unique PR37/51 safeguards; reconcile PR156–158 stack; serial/parallel failure and manifest/compatibility negatives. |
| [LX-REQ-018](requests.json) | Honest qualified artifact handoff and safe destinations | FIX AND QA REQUIRED | Build and publisher source owners. Remove dev-validator-only qualification implication and arbitrary-remote firmware push; reconcile product identity and cache invalidation. |
| [LX-REQ-019](requests.json) | Canonical workflow, plugin and skill coherence | TRIAL ACCEPTED DEFAULT ADOPTION PENDING | Sole workflow integrator. Reconcile stacked draft PR155 with intended protected target; synchronize authorized adapters/checkpoints and latest authority. |
| [LX-REQ-020](requests.json) | Productive schedulers and reduced request-to-install lead time | PROMPT REPAIR UNDERWAY READBACK PENDING | Scheduler maintenance coordinator. Complete authorized scoped prompt repair and verify supported readback; exact-head queue, one integrator, bounded handoffs, obsolete waits removed. |
| [LX-REQ-021](requests.json) | Preserve unique quickcheck and regression assets | RESIDUAL WORK RECONCILIATION REQUIRED | unassigned. Retain PR137 planner/test/docs absent from PR155; reconcile Projects dispatch, PR26 executable regressions and PR95 runner/docs. |
| [LX-REQ-022](requests.json) | Stable clock and transport behavior | COMBINED QA REQUIRED | Clock and transport producers with independent QA. Choose one combined root; consume R39/PR122 evidence; test interleavings, overload and lifecycle cutoffs. |
| [LX-REQ-023](requests.json) | Performance improvements with preserved behavior | SCOPED EVIDENCE NOT FULL OUTCOME | Performance producer and independent QA. Consume 44 paired comparisons; identify whole-renderer measurement/integration gap; avoid broad speedup claim. |
| [LX-REQ-024](requests.json) | Production main/picker promotion | BLOCKED REVIEW HARDWARE AUTHORIZATION | Sole integrator and production publisher. Obtain required eligible nonauthor latest-push approval, missing Pro/per-model hardware evidence and production action authority. |
| [LX-REQ-025](requests.json) | Historical evidence archives and deliberate cleanup | SOURCE READY OR RECONCILE | Sole integrator. Refresh docs PR22/25 heads/gates; reconcile PR27/62 conflicts/ancestry; retain limitations. |
| [LX-REQ-026](requests.json) | Retain accepted knob and already-integrated fixes | REUSE EVIDENCE NOT NEW IMPLEMENTATION | Sole integrator. Map baseline successors and retain proof; reopen only changed/invalidated scope. |
| [LX-REQ-027](requests.json) | Durable request/progress tracking and parallel continuity | TRACKER IMPLEMENTATION ACTIVE | Sole request tracker writer. Persist reviewed ledger/index and validator; independent public-safety review; verify remote branch delivery; continue evidence updates through designated writer. |
| [LX-REQ-028](requests.json) | Recover missing historical feature provenance | MISSING SOURCE PROVENANCE | unassigned. Locate reported 7537a46 and 66fb5c7d5ceec356513bf54b7d7b7ff499526909 via legitimate handoffs; compare current source before reimplementation. |
| [LX-REQ-029](requests.json) | Account for preserved branches outside PR queue | UNCLASSIFIED BRANCH INVENTORY | unassigned. Reconcile diverged residuals with accepted content/reviews; prioritize project/loader, data safety and performance; retain ancestral evidence. |
| [LX-REQ-030](requests.json) | Explore Up/Down knob banks and eighth-strip mode controls | DESIGN ONLY OWNER REVIEW REQUIRED | Active SMC UI refinement and design owner. After narrow UI refinement, inventory Up/Down and strip8 mute/Solo/arm/mode use; deliver mapping/conflict preview for owner acceptance before implementation. |

## Preservation and updates

- [79 preserved branch heads](requests.json) remain accounted for under LX-REQ-029: 34 ancestral, 45 divergent. Divergence is not abandonment or proof of unique residual work. No branch cleanup is authorized here.
- Existing queue JSON, CSV and review/state history are retained byte-for-byte at their previous paths. This request ledger supplements their PR-level evidence; it does not overwrite it.
- Append new stable IDs. Before revising status/scope/evidence, append old values and timestamps to that record's `history`. Retain unanswered blockers. A removed request needs a `tombstones` entry with `request_id`, nonempty `reason` and `superseded_by` IDs pointing to retained records. Keep the continuity registry append-only.
- Only `dependencies` are blocking prerequisites; `related_requests` are coordination links. Maintain evidence provenance and exact source/package identities; never promote unknown to done.
- Use public-safe technical summaries and role owners. Tracker maintenance grants no merge, publication, permission-change or device authority; historical source-write denials remain binding.

Run the small offline check from the repository root:

```sh
python3 coordination/requests/validate_requests.py
# For later updates, compare the last accepted tracker revision:
python3 coordination/requests/validate_requests.py --base-ref <previous-tracker-commit>
```

Alternatively use `--previous previous-requests.json`. The initial check uses the permanent registry; cross-revision checks also detect later-added records erased without supersession. This is a continuity check, not a firmware gate. No firmware builds or device tests were run for this documentation change.
