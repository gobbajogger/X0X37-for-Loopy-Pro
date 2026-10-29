# Orchestrator lane — owner-approved split

State: PLANNED; no implementation or tests claimed by this document.
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch: `wip/orchestrator` (to be created after plan report and document review).
Base: `wip/fmb-guards-slots@4775ef9123c2e31d1ed9bbe39c3b77ece9c03bd8`, verified by producer preparation as the existing Chicago/Miami rev13 source. Preserve its ancestry and owner; never modify the base branch.
PR: not created. Producer and independent QA are separate native subagents; root retains coordination.

## Scope and ownership

The owner explicitly reassigned roughly half of the catchall list. This lane owns the following 22 outcomes; ownership does not mean existing behavior is missing or accepted. Inspect current source and existing proof before changing behavior.

- **4. Physical pitch bend and damper controls.** Wheel/MIDI bend and damper remain independent performance inputs.
- **7. Pattern-bound sound and engine.** Pattern recall restores the associated engine, sound and locks.
- **8. Whole-sound locks on individual notes.** Each note can retain an unambiguous complete sound state without unsupported morph claims.
- **11. Notes longer than one step.** NOTE, TIE and REST preserve duration, accent and slide; TIE never creates a missing note.
- **12. Record actual held-note duration.** Recorded note release and playback/MIDI duration agree across step boundaries.
- **13. Musically correct probability, including drum steps.** Skipped events produce no Note On; drum-step probability has a real selectable control.
- **14. RANDOM preview and playback agreement.** Preview and playback agree; swing follows musical time rather than random note index.
- **15. Preserve cutoff locks during navigation.** Navigation does not overwrite active cutoff locks with base values.
- **17. Safe motion and recording lifecycle.** Transport, load and pattern changes leave no stale recording ownership or held notes.
- **18. Consistent internal and external clocks.** Clock-source changes preserve phase and reject stale transport events.
- **21. Tempo-synced Miami divisions and pad selection.** Tempo divisions and pad selection coexist safely with held notes.
- **22. Recorded legato becomes slide.** Legato recording produces slide while preserving gate and release timing.
- **43. Silent linked SMC context changes.** Remote context synchronization alone does not spuriously display a mixer overlay.
- **47. Explicit reliable Save Project.** Save preserves all supported project state with explicit persistence status.
- **48. Explicit safe Load Project.** Invalid loads preserve the current project and sample references.
- **49. Separate Clear Song and Reset Project.** Song clearing and project reset remain distinct and never erase imported samples.
- **50. Complete portable project export/import.** Supported project state round-trips with integrity and sample-identity checks.
- **52. Atomic operations across multiple clients.** One client cannot cancel or overwrite another client’s transaction.
- **54. Explicit project-format compatibility policy.** Document supported rev13 inputs and reject unsupported legacy inputs without migration.
- **63. Safe navigation wrap.** Eligible navigation wraps; numeric limits and destructive actions do not inherit wrapping.
- **110. Bounded everyday-usability acceptance.** Exercise a bounded existing musician workflow and fix evidenced interaction gaps.
- **114. Keep delivery readiness states distinct.** Report source, test, build, publication and device readiness independently.

Remaining-features retains these 25 items: 23, 30, 38, 57, 58, 59, 60, 61, 62, 64, 65, 66, 69, 73, 74, 76, 83, 86, 88, 112, 113, 115, 118, 119, 121.
Started work #23, #115 and #121 remains with remaining-features. Recheck any newer started work before overlapping edits. A missing remote commit or PR does not prove that an item has no local implementation.

All named lanes remain owned. Explicit skips 29, 55, 116, 117, 120, 122 remain skipped; deferred community item119 remains deferred. Retained hardware/publication items are not newly authorized. Never cherry-pick broad sibling PR stacks as if independent.

## Existing behavior versus gaps

- Existing or historically implemented, requiring exact-current inclusion/acceptance rather than automatic rewrites: 4, 7, 11, 12, 14, 15, 17, 18, 47, 48, 49, 50.
- Partial or concrete unresolved behavior to inspect: 8 (whole note sound), 13 (drum-step probability selection), 21 (extended divisions/pad selection), 22 (legato recording), 43 (silent SMC-linked context), 52 (multi-client transaction ownership), 63 (navigation outside existing matrix wrap).
- 54 is an explicit compatibility disposition and proof task, not authorization for legacy migration.
- 110 is a bounded usability acceptance task; 114 is truthful readiness reporting, not a new audio feature.
These classifications come from the inventory, not a new correctness review.

## First bounded unit

Deliver this split document from the verified rev13 wip base, after independent documentation review. Do not use older `nightly@66073c68` as if it were rev13: that source uses stream6/7 and sound10/11.

For #54, consume the existing exact-base validator evidence rather than launch duplicate tests. The base lane documentation reports real rev13 reload and rejection of sound0–12/stream1–12 without writes, with 24,334 project-stage checks. This is producer-reported evidence until its exact source/input/log closure is consumed; it does not constitute a fresh independent PASS. Preserve the existing host-repair owner's 57-fixture repair. Do not create `project_revision13_contract_test.c` or another review round merely to repeat that work. No browser parser or draft/storage repair is transferred from PR687/685.

The first real missing-feature discovery unit is #13, drum-step probability selection. Inspect actual `firmware/src/seq/sequencer.c`, public pattern structs/serialization, existing probability tests and UI mappings on the verified base. Establish whether per-step drum probability is absent, inaccessible, or already implemented. Trace the real playback/MIDI decision and ensure skipped events send no Note On and cannot revive absent notes through TIE.

Produce a minimal path-level implementation plan and report it before edits. If a gap exists, one producer owns only those expressly identified files; a different subagent reviews the frozen exact candidate. Preserve rev13 serialization and Chicago/Miami bit-exact output. No copied-model tests, invented fixtures or speculative code to fill the lane. If the complete feature exists, record the source/evidence and select the next actual assigned gap rather than rewrite it.

Do not edit `web/x0x_projects.js`, editor draft/storage scripts, engine private headers, the existing host fixture repair, or other named-lane source in this unit. Shared UI/SMC/clock files need a narrow nonoverlapping path/scope check before assignment; this split does not take a named lane's active repair. A change requiring an incompatible format or forbidden dependency is held, not migrated.

## Execution and proof

For each unit: source-grounded plan → report plan → one producer → actual host proof → frozen exact-input independent QA → reviewed normal push → PR toward nightly → exact Mac commands → state report. A failed check stays FAIL, unavailable check NOT_RUN. Reuse unchanged evidence and preserve review budgets. Never merge main or nightly or enable auto-merge.

Host entrypoints inspected at the historical nightly pin (refresh against the selected wip base before execution): `sh tests/host/run_project_stage.sh` and `sh tests/host/run_project_protocol.sh`. The first compiles real project implementation with host DSP/runtime dependencies; the second stubs project validation and is not proof of validator correctness. Run normal and supported sanitizer configurations, with exact compiler/exit/log receipts. New harness commands are provisional until implemented and executed.

Mac qualification must use SDK1.2.1 and the exact candidate. Derive build/install/performance commands from that candidate's maintained scripts; do not guess install or device commands. Unrun native/golden/CPU/install/hardware checks remain NOT_RUN. No measured improvement without a measurement.

## Boundaries

Chat only, at most six native subagents for the entire chain. Default Git author. Only Chicago/Miami in the 303/wobble slots, bit-exact behavior; rev13 only, no project migration. No private #684 headers, `state_codec_private.h`, SDK/toolchain or other binaries. No bootloader, BTIF0xC0000–0xC0FFF, USRTRIM0xC1000, efuse, key_mac, `fm1_rescue.py` or `--force` operations. No hardware operations, publication, merges or access changes. Preserve actual denials.

Every report: state, branch, HEAD, PR, owner-only blocker and next step. Source/host/QA/native/package/publication/device states remain separate. No owner reply gate for routine authorized continuation.
