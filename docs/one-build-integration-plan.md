# One-build integration plan

**Status: PLAN ONLY — no source integration, acceptance, installation or shipment claim.**

## Frozen inputs and sole destination

- Candidate base: `094bb1a55c159aefb0e2bfa39790a1b4b74cee40`; coordination: `4a35a6cdaa0f112db6b3f577542138afd170c20d`.
- Destination is **wip/one-build only**. No main/nightly merge, release, automatic GitHub Actions, device operation, project migration or publication. Chicago and Miami only; original rev13 contract; preserve all current producers/reviewers and exhausted or refused budgets.
- Inventory covers 36 open PRs and 815 branches from complete API pagination, plus verified subsequent PR712/branch and PR713 on an existing branch: **38 PRs, 816 known branches**. This is a timestamped snapshot, not a claim that every legacy ancestry or live owner was verified.
- Existing PR699 is the proposed planning-registry consumer only after its owner accepts the scoped handoff. PR706 remains coverage evidence; PR703 remains reference source. Do not create a duplicate reference PR or copy its files. Historical PR bases in the inventory are provenance, never proposed destinations.
- `docs/one-build-integration-inventory.json` contains one disposition row per observed branch/PR, exact head/base where available, ownership uncertainty, dependencies and existing acceptance limitations. `include` means **proposed reuse**, not merge-ready; `superseded` means direct route/subset only, not permission to delete unique source; legacy unverified branches remain HELD.

## Dependency order and completion gates

This is a conditional topological order, not blind cherry-pick instructions. A held predecessor blocks only dependent composition. Disjoint preparation may proceed under its existing owner. Immediately before composition re-read remote pins, claims, changed file overlap and exact independent acceptance; changed closures invalidate only affected proof. Root coordinates; only the existing sole integrator can perform later authorized composition.

### S00 — after intake
PRs: #699, #706, #703. Freeze inventory and accepted scope; reuse 703 references without copying; preserve 706 missing IDs and 699 registry.
Result consumer: existing sole integrator/ledger owner, then the named source owner or Build/independent QA. Completion requires immutable source/test receipt and first substantive input-specific consumer action; a pointer alone is pending consumption.

### S01 — after S00
PRs: #708. Existing guards owner closes or bounds 57 historical failures; original-only rev13/no migration composition; exact independent delta acceptance.
Result consumer: existing sole integrator/ledger owner, then the named source owner or Build/independent QA. Completion requires immutable source/test receipt and first substantive input-specific consumer action; a pointer alone is pending consumption.

### S02 — after S01
PRs: #702, #711. Keep actual stack 702 then 711; compose build switches against 708, preserve pad 8–15 notes and releases; independent combined tests.
Result consumer: existing sole integrator/ledger owner, then the named source owner or Build/independent QA. Completion requires immutable source/test receipt and first substantive input-specific consumer action; a pointer alone is pending consumption.

### S03 — after S01
PRs: #710. Apply focused Linux project-stage test portability after guard composition; run current test, not historical 30320 checks.
Result consumer: existing sole integrator/ledger owner, then the named source owner or Build/independent QA. Completion requires immutable source/test receipt and first substantive input-specific consumer action; a pointer alone is pending consumption.

### S04 — after S01, S02
PRs: #709, #705. Existing SMC/transport owners compose two-peer first boot and queue subset. Union tests; no replacement by one-peer 700; resolve FX/page-lock and fresh negative paths.
Result consumer: existing sole integrator/ledger owner, then the named source owner or Build/independent QA. Completion requires immutable source/test receipt and first substantive input-specific consumer action; a pointer alone is pending consumption.

### S05 — after S01, S02, S04
PRs: #712, #701, #692, #691, #707. Existing engine/UI and optimizer owners select newer Miami donor explicitly and adapt 712; do not merge 701 wholesale. Compare Chicago stack and 707 closure; measured/golden proof remains separate.
Result consumer: existing sole integrator/ledger owner, then the named source owner or Build/independent QA. Completion requires immutable source/test receipt and first substantive input-specific consumer action; a pointer alone is pending consumption.

### S06 — after S04, S05
PRs: #688, #696, #689, #694. Existing owners deliver only accepted missing controls/settings/FX/BREAK subsets. Resolve 689 branch/head mismatch; 696 executed-input receipt failure. Preserve remaining-features assignment.
Result consumer: existing sole integrator/ledger owner, then the named source owner or Build/independent QA. Completion requires immutable source/test receipt and first substantive input-specific consumer action; a pointer alone is pending consumption.

### S07 — after S00
PRs: #685, #690. Disjoint offline editor/reference preparation may proceed; website owner must resolve prepared-sample race and rev13 exact roundtrip before any shared protocol composition.
Result consumer: existing sole integrator/ledger owner, then the named source owner or Build/independent QA. Completion requires immutable source/test receipt and first substantive input-specific consumer action; a pointer alone is pending consumption.

### S08 — after S06, S07
PRs: existing accepted request only. Reconcile all 15 MISSING IDs and additional request facets; explicitly carry deferred and blocked items. MBOOT PLAN_ONLY HOLD; never claim complete feature delivery.
Result consumer: existing sole integrator/ledger owner, then the named source owner or Build/independent QA. Completion requires immutable source/test receipt and first substantive input-specific consumer action; a pointer alone is pending consumption.

### S09 — after S08
PRs: #698. Sole integrator selects complete immutable combined source on wip/one-build, records source/dependency/test closure; independent review only affected changes. No merge operation authorized by this document.
Result consumer: existing sole integrator/ledger owner, then the named source owner or Build/independent QA. Completion requires immutable source/test receipt and first substantive input-specific consumer action; a pointer alone is pending consumption.

### S10 — after S09
PRs: existing accepted request only. Existing native_build/QA consume selected closure and provide exact current Mac build/install/performance commands only when source supported; unrun model/controller/Mac/golden/performance/hardware remain NOT_RUN. No publication or device operation.
Result consumer: existing sole integrator/ledger owner, then the named source owner or Build/independent QA. Completion requires immutable source/test receipt and first substantive input-specific consumer action; a pointer alone is pending consumption.

## Every MISSING PR706 requirement remains explicit

| ID | Requirement | Existing owner | Next action / unmet criterion |
|---|---|---|---|
| 41 | Complete mixer effects page | Existing mixer-UX owner | Caption/source reuse only; agreed full mappings, FX/lock end-to-end path and preview remain gap Reuse: 688@7b2d1726f3515b319d5edcab34529007d33ab3a8; smc_fx_map and caption regression. |
| 50 | Portable project export/import | Existing website parser + one-build owners | Full firmware-produced rev13 export/parse/stage roundtrip; CRC/size/interruption negatives missing Reuse: 690@9fe416a39506ad8d408fa5bf84603b696fd005a4 partial web rev13 admission;699 parser receipts. |
| 54 | Project format compatibility | Existing website parser + guards owner | Reject legacy on original profile; no migration. Partial admission still accepts3/4; exact13/13 layout required Reuse: same50;708@72477c0 excludes original migration helper. |
| 73 | Recovery bootloader | Existing maintenance/data-safety owner; exact current assignment required | Owner now requests PLANONLY m-bootloader default/detect/install-if-absent, full backup, verified image/CRC/recovery. No implementation, probe, flash, force orprotectedbytes; unchanged BTIF/USRTRIM/efuse/key_mac Reuse: Historical lane/mb-mboot-* branches enumerated;694 priorhold. |
| 97 | iPhone-focused editor | Existing offline-editor owner | Richportrait/landscape UI and actual phone/browser validation missing Reuse: 685@f776aa6192d9a95777df87e788f318c4340413f0. |
| 98 | Note selection/group operations | Existing offline-editor owner | Multi-note marquee/atomicgroupclipboard absent; do notduplicate existingparttools Reuse: 685 whole-partclipboard+undo exists. |
| 101 | Scale/snap/ghost/touch keyboard | Existing offline-editor owner | Root/scale snap, readonlyghost synth, touchkeyboard absent Reuse: 685 navigation/pan/zoom exists. |
| 104 | Bidirectional device editor protocol | Existing offline-editor + transport owners after scopedhandoff | Versioned snapshots/atomictransfer/capability/CRC/negativecontract absent; no deviceactions Reuse: 685 deliberatelyoffline;SMC transport source not editorprotocol. |
| 105 | Usable TouchOSC project | Existing offline-editor/TouchOSC owner via explicitscope | Finishedruntime + transporthandoff absent; noinventedbinarydelivery Reuse: 685 explicitly no usable bidirectional.tosc/Lua. |
| 111 | Complete updated NIGHTLY | Existing soleonebuildintegrator thenBuild/QA | Onebuildcomposition incomplete; this task STOP at wip/one-build source plan, no nightly/mainmerge orpublication Reuse: 698@094bb1a55c159aefb0e2bfa39790a1b4b74cee40 + currentcandidatePRs. |
| 112 | Actual Elite/Pro packages | Existing native_build | Exact combinedsource/SDKV1.2.1/controllerclosure and realmodeloutputs missing; SDKV1.2.0 notsubstitute Reuse: Historical635/677 nativeproof; current cloud UI22,949checks not firmware. |
| 113 | Existing installer delivery | Existing installer/build/QA/publisher gates separately | Currentmatchingpackages+catalog/hash/readback+installerproof missing;publication separate notauthorizedbyplan Reuse: 690 installerworkspace source; historicalinstallerbranches. |
| 119 | Community song library | Existing remaining-features owner | Deferred; noimplementation/newadmission Reuse: 694 explicitlydeferred. |
| 120 | Owner-voting request inbox | Existing intake/ledger owner | Automaticvoteinbox/reviewflow notdelivered; heldscope; registrydocdoesnotgrantimplementation Reuse: 699M04; coordination/feature-request-intake-20261025 existingreference. |
| 122 | Cursor/review contribution audit | Existing audit/QA/ledger owner | Thisinventory improvesmap but815branchancestries/currentclaimsnotfullyverified; endtoendcontribution/reviewlineage gap; refusedbudgetsremain Reuse: 699M04 + thisfullbranch/PR inventory + prior reviewerreceipts. |

## Newer and detailed owner requests

The supporting inventory carries all 34 SMC facets and 23 supplementary non-SMC rows from the independently reviewed feature register, without repeating private quotations or PR703 files. These are not additive counts: the register preserves 122 baseline requests plus three explicit SMC omissions and one simulator request (126 distinct top-level requests); it records 53 additional non-additive facets and five inactive historical records. Coverage is bounded to accessible reviewed history, not all account history.

- Explicit new SMC gaps: Bluetooth ON/OFF without reboot; automatic persistence of that choice outside Save Project/autosave; HOME/main SMC fader 8 master DJ/filter with independent mute. These must not be inferred complete from first-boot PR709.
- Owner-reported FX/page-lock failures remain MUST_FIX until real end-to-end evidence; current unconditional lock bypass is not permission to restore a historical gate blindly. Preserve two-peer names/count, target capture/release, pickup, transport/record, disconnect/overflow and foreign-sender negatives.
- UI retains six Settings sections/28 refs, clip-safe entry and confirmations, current typography/case, semantic filter/drive glyphs, live-signal TAPE freeze/resume, agreed part display order, held-note preset continuity and selected presets. Source selection must resolve newer Miami, not blindly copy 701.
- Faithful interactive SMK-37 Elite/SMC-Mixer/Loopy Pro simulator remains an undelivered request, not firmware proof or a new assignment. Deferred community library/inbox remain held. PR662 animation stays paused; no restoration of superseded boot video or occupancy-based TAPE.

## Local source preservation, not integration

81 absent-remote source-blob candidates are indexed with hashes and safety-scan state: 77 are text-scan-clean and four exact-worktree recoveries still require scanning. None is automatically selected for publication. Eligibility is not permission to publish every candidate: root must finish exact per-file filtering and independent review, preserve provenance/licenses, and verify actual remote absence immediately before delivery. Original historical commits must not be pushed wholesale: their trees contain excluded files. Only sanitized text projections are eligible; exclude private headers including state_codec_private.h, credentials, raw captures, private unit identities, and SDK/toolchain/media/binary payloads. Already delivered PR703/current-base blobs are reused, never copied as another archive.

## Complete open-PR disposition matrix

| PR | Head | Current base | Disposition | Owner and unmet gate |
|---|---|---|---|---|
| #711 | `1eb56ce34841bd6574b41ea4db7fbbfb53e4a5ca` | `fix/freeplay-unused-pads` / `c95b6785776ecf391efde571b6dbbcf8b7ce9948` | INCLUDE | Existing RATE-pad producer; After #702; collides #708 in all three changed paths; Producer scoped tests; independent exact delta acceptance pending |
| #710 | `7383a6d27d968cb9b7bd1e3133cf3ce66c2cc383` | `wip/one-build` / `094bb1a55c159aefb0e2bfa39790a1b4b74cee40` | INCLUDE | Existing host project-stage producer; After #708 test composition; Historical30320 checks only; exact-current acceptance pending |
| #709 | `7668e48efc2fc831987f1e2f35d9bd7412ce4bf7` | `wip/one-build` / `094bb1a55c159aefb0e2bfa39790a1b4b74cee40` | INCLUDE | Existing SMC producer; After #708 project.c guards; retain two-peer stack; Producer4x28 boot checks and6xBLE; independent acceptance pending |
| #708 | `72477c0d08ff3e76426537bafe8ef72194223891` | `wip/one-build` / `094bb1a55c159aefb0e2bfa39790a1b4b74cee40` | HELD | Existing guards-slots producer; Compose before709/711; preserve UI/SMC/storage owners; 57 previous failures open; full host/native NOT_RUN |
| #707 | `58136bc26bfa255be799b967e9abe044b064635b` | `wip/one-build` / `094bb1a55c159aefb0e2bfa39790a1b4b74cee40` | INCLUDE | Existing Miami optimizer; Reconcile691/701 source selection; no duplicate DSP graft; Producer equality/scoped tests; full exact composition pending |
| #706 | `07bb15e3deab9942531712100b7358e6750f6e5d` | `wip/one-build` / `094bb1a55c159aefb0e2bfa39790a1b4b74cee40` | INCLUDE | Existing coverage documentation owner; Reuse for coverage status; update from this inventory; 122row validation; not firmware acceptance |
| #705 | `5a75197afa47a5ee47f0fbe9cc5d84e75055ec65` | `wip/one-build` / `094bb1a55c159aefb0e2bfa39790a1b4b74cee40` | INCLUDE | Existing MIDI queue producer; Prefer focused current original-runtime delta over695/677; Scoped producer tests;101 historical695 failures not cleared |
| #704 | `9256d1ff3f33f87c11e7a0b2ad72c1f1fb09c376` | `wip/one-build` / `094bb1a55c159aefb0e2bfa39790a1b4b74cee40` | HELD | Existing host-validation producer; Remove automatic GHA workflow before inclusion; Workflow not permitted; no executed job steps |
| #703 | `64575f86375e7daa9a939de25e6eb2f5047e0665` | `wip/one-build` / `094bb1a55c159aefb0e2bfa39790a1b4b74cee40` | INCLUDE | Existing recovery archive owner / root; Reuse reference archive only; no wholesale active tree replacement; Independent archive accepted76files; no runtime acceptance |
| #702 | `c95b6785776ecf391efde571b6dbbcf8b7ce9948` | `wip/one-build` / `094bb1a55c159aefb0e2bfa39790a1b4b74cee40` | INCLUDE | Existing Free Play producer; First focused integration; parent of711; Independent QA SCOPED PASS c95b6785; full build NOT_RUN |
| #701 | `58ccaf0229884bce5618bd91e81ed34bb3339b58` | `wip/ui-theme` / `a0279524decd54d2840474883fefa5882b15fb4d` | HELD | Existing DSP/UI producer; Stack on wip/ui-theme; reconcile691/692/696/707; Combined current source acceptance pending |
| #700 | `0a8ee065c7b417853bac1015200dd9989e4c8e93` | `dnbox-base-smc-20261029` / `77b1ba80102d9d8c9e2dfcebb439c4703fc51cba` | HELD | Existing SMC A-port producer; Do not replace two-peer;709 already ports focused boot idea; Complete codec header dependency unresolved |
| #699 | `445b91b60b6d97eb56ab4c673aaf56c2186ee967` | `wip/one-build` / `1f27d2fb0415db62d54fadb3137658cf4e5314ef` | INCLUDE | Existing missing-features documentation producer; Reuse122IDregistry; integration plan belongs existing doc flow; Source-only inventory; stale statements require updates |
| #698 | `094bb1a55c159aefb0e2bfa39790a1b4b74cee40` | `nightly` / `66073c68dd80d51a7053432c2c161a8b720a1341` | EXCLUDE | Existing sole one-build integrator; Destination branch, not PR to merge into itself or nightly; Current source WIP not full owner delivery |
| #697 | `ff0f35c02163543ced4f79666787f2166ac327b4` | `nightly` / `66073c68dd80d51a7053432c2c161a8b720a1341` | HELD | Existing author-policy producer; Keep policy scope separate from integration source; Independent QA publisher destination/binary enforcement NEEDS CHANGES |
| #696 | `c670b6ed60465b590c5dd289c06b5e8c2b11d90b` | `nightly` / `66073c68dd80d51a7053432c2c161a8b720a1341` | HELD | Existing chat/ui-theme producer; Preserve current UI; only reviewed distinct glyph/test delta; Dirty executed-source evidence finding NEEDS CHANGES |
| #695 | `6ea8e572f0dffcf8e2c828afe69c71ef0541c3af` | `nightly` / `66073c68dd80d51a7053432c2c161a8b720a1341` | HELD | Existing conflict producer; 705 replaces MIDI queue subset ONLY; other scope held; ONLY queue subset superseded, remaining scope explicitly HELD; 208PASS101FAIL old Mac unresolved; not fully superseded |
| #694 | `530046e176a0d1c74c0c8cddf9db3f346e026ec0` | `nightly` / `66073c68dd80d51a7053432c2c161a8b720a1341` | HELD | Existing remaining-features producer; Preserve25assigned IDs and current source; not wholesale merge; Receiptplan substitution QA onolderhead unresolved until exactnewclosure accepted |
| #693 | `4f039829352d7dc805ebd2865b073d34f397cb6e` | `nightly` / `66073c68dd80d51a7053432c2c161a8b720a1341` | HELD | Existing Orchestrator source owner; 364pathdiff broad; select distinct current owned correction only; Exact combined acceptance unverified |
| #692 | `f055d79fdc8813a495cf82fb46e6295f909330df` | `wip/chicago-opt` / `dd735533eaceae5856f5b657fe617667e7cc5b6e` | INCLUDE | Existing Chicago optimizer; Stack wip/chicago-opt; compare current onebuild engine closure; Independent focused host accepted; golden/native/perf pending |
| #691 | `b8fc366a1f017890151230912beeb4e5c9e9fcf1` | `wip/miami-opt2` / `c305351b73b8821ded0b59277648cb0191f10a2f` | HELD | Existing Miami optimizer; Stack wip/miami-opt2; reconcile707 and701; Old GitHub approval not latest head approval |
| #690 | `9fe416a39506ad8d408fa5bf84603b696fd005a4` | `nightly` / `66073c68dd80d51a7053432c2c161a8b720a1341` | HELD | Existing website producer; Partialrev13 parser reuse for50/54; preserve browser ownership; Preparedsample queuedA/newB receipt race NEEDS CHANGES |
| #689 | `163191c3b679b5204db7a1808b6b2b34a99cb972` | `nightly` / `66073c68dd80d51a7053432c2c161a8b720a1341` | HELD | Existing BREAK producer; PRhead163191 !=branch953aa; resolve actual source before integration; 953aa GCCportability scopedPASS; overall incomplete |
| #688 | `7b2d1726f3515b319d5edcab34529007d33ab3a8` | `nightly` / `66073c68dd80d51a7053432c2c161a8b720a1341` | HELD | Existing mixer-UX producer; 41fullFX missing; preserve source for controls/lock; Caption scoped tests not completeFX acceptance |
| #687 | `7840c0394967f0e731d905e5832fb6cf7c2127d7` | `nightly` / `66073c68dd80d51a7053432c2c161a8b720a1341` | HELD | Existing SMC-port producer; Reuse handlezero; broad270pathcandidate not wholesale; Rev13 parser + original MIDI recording P1 remain |
| #685 | `f776aa6192d9a95777df87e788f318c4340413f0` | `wip/one-build` / `094bb1a55c159aefb0e2bfa39790a1b4b74cee40` | INCLUDE | Existing offline-editor producer; 22selfcontained web/offline-sequencer paths; no deviceprotocol; Independent storage scopedPASS; full browser pending |
| #684 | `9d1cf9fb017889967f0c5f4b57727eab0d15a185` | `nightly` / `66073c68dd80d51a7053432c2c161a8b720a1341` | SUPERSEDED | Existing Chicago/Miami baseline producer; Onebuild current base replaces wholeoldreleaseUI baseline; reuse provenance; Not complete latest UI/feature acceptance |
| #679 | `f8567f0f4d2c6031e5ea0dad7bf85d81959e4729` | `lane/mb-smc-composed-20261027` / `c647f98d81318ffb4084611683a8231ea0609b6d` | HELD | Existing SMC transport owner; Historical stack lane/mb-smc-composed; reconcile current twopeer; No independent latest combined qualification |
| #677 | `2d555bee09021d8d71b59a5903eb1c93dd74f8e2` | `nightly` / `7e4bbfd95da1d9ad86c80541b9bceae48231d006` | HELD | Existing firmware-bug repair owner; 705 focused queue subset,695 conflict reference; preserve otherdistinctrepair; ONLY queue subset superseded, remaining scope explicitly HELD; Historical exact DEV proof not current onebuild proof |
| #651 | `c1f3f776b13ad8f4f56bcbdb8a8061924191b93d` | `lane/opt-reviewed-composition-20261027` / `5f6121cb1f50aedcf72dc93fc33ff8e6c26ab134` | HELD | Existing lifecycle proof owner; Stack638; historical hostproof reference only; Refused Codex budget retained; no retry |
| #650 | `a51b030d98e1400f697ba1fa7b5056a62f7399fc` | `lane/opt-reviewed-composition-20261027` / `5f6121cb1f50aedcf72dc93fc33ff8e6c26ab134` | HELD | Existing main-UI producer; Stack638; overlap696/703/new UI; Historical pi32size finding; no blind renderer replacement |
| #638 | `5f6121cb1f50aedcf72dc93fc33ff8e6c26ab134` | `nightly` / `ba4c2bc7535c266379d2ccc8cbfca85c4733654a` | HELD | Existing historical integrator; Historical combinedroot; onebuildcurrentdestination; Distinct subtree/buildclosure, not interchangeable |
| #637 | `f25f2fd47106992e4dbfdf6211b9299f6eb8c757` | `nightly` / `16a794ecd8811c09f058defe8ec7c5571d6096e8` | SUPERSEDED | Existing BREAK owner; Reuse historical proof through689/currentBREAK source; Superseded for directintegration only; no deletion |
| #636 | `92108efd743fb9fbadd665c361ee48e22b0e86e0` | `nightly` / `16a794ecd8811c09f058defe8ec7c5571d6096e8` | SUPERSEDED | Existing NightUI owner; ReuseprovenUI through696/703; Historical source not latestpreviewacceptance |
| #635 | `0e2b6f32d28d3001e84a7810a91f67aafce03a21` | `nightly` / `16a794ecd8811c09f058defe8ec7c5571d6096e8` | HELD | Existing DataSafety owner; Preserve atomicOTA invariants; selectedsourceportonly if absent; Historical scopednativeaccepted; currentclosure unknown |
| #212 | `9aab1f1deb8da75a0ae3dcae59c5841787f50cd1` | `nightly` / `08d3164a265123cb4c84e5104642fd93257d63d7` | HELD | Existing experimental engine owner; No experimental migration or wholebranchmerge; DistinctunadoptedLXSE; legacy assumptions unsafe forrev13 |
| #712 | `24a36371fe7779bdca6b72b1bc2aee9d58502ec4` | `wip/one-build` / `094bb1a55c159aefb0e2bfa39790a1b4b74cee40` | HELD | Existing #701 engine/UI producer; Adapts #701@58ccaf0229884bce5618bd91e81ed34bb3339b58,51paths; newer Miami691/707 selection unresolved; do not duplicate701; No reviews; compiled/render/golden/performance/native NOT_RUN. Existing701 scopedQA reusable only unchanged closure |

| #713 | `97f98da92a3d60f69708f8d597489b0b044def6c` | `wip/one-build` / `094bb1a55c159aefb0e2bfa39790a1b4b74cee40` | HELD | Existing author-policy producer; claims successor to697, not independently accepted. Full test_publish_nightly.py NOT_RUN. No publication authority. |

The full 816-row branch matrix is machine-readable in the supporting inventory, including branches without open PRs; none is omitted because of an unknown owner.

## MBOOT default and install-if-absent — PLAN_ONLY HOLD

### Source-backed bounded contribution

Status: PLAN ONLY; no device operation, source implementation or external write.
Inspected canonical `wip/one-build` commit `094bb1a55c159aefb0e2bfa39790a1b4b74cee40`, tree `ce0b0b9555e31ddd8d8ac54f6da385052e3565af` (complete API tree, not truncated). Owner decision: MBOOT becomes default, with installation on units that lack it. This is a requested outcome, NOT implemented/installable functionality. Integrate into `wip/one-build` only under the existing source owners.

## Reuse, do not restart

Continue LX-REQ-039 and its existing adoption contract. It names source owner `implement_mboot_source_adoption`, reviewer `review_mboot_adoption_source` and root integration ownership; these are historical recorded claims to reconcile, not newly activated workers. Existing unchanged source evidence is reusable, but historical pass statements are not new runs.

Source citations below are immutable canonical GitHub links at the inspected commit:

- [Existing complete adoption protocol contract](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/094bb1a55c159aefb0e2bfa39790a1b4b74cee40/docs/mboot/nightly-adoption-contract.md).
- [Offline adoption and image preparation](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/094bb1a55c159aefb0e2bfa39790a1b4b74cee40/tools/mboot/README.md).
- [Private host backup adapter](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/094bb1a55c159aefb0e2bfa39790a1b4b74cee40/tools/mboot/CAPTURE_HOST.md).
- [Platform-disabled capture core](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/094bb1a55c159aefb0e2bfa39790a1b4b74cee40/tools/mboot/CAPTURE_CORE.md).
- [Adoption transform exact bounds](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/094bb1a55c159aefb0e2bfa39790a1b4b74cee40/tools/mboot/adoption.c#L26-L83).
- [Preparation pin/model/CRC/key/geometry checks](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/094bb1a55c159aefb0e2bfa39790a1b4b74cee40/tools/mboot/prepare.py).
- [Current ordinary installer](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/094bb1a55c159aefb0e2bfa39790a1b4b74cee40/tools/fm1_install.py).

## Ordered bounded stages and gates

1. **Resolve protected-region conflict before selecting payload.** Offline adoption preserves BTIF `0xC0000–0xC0FFF`, USRTRIM `0xC1000`, calibration, configuration and key/MAC allocations. But its README explicitly says pinned upstream first boot can restore calibration at `0xC1000`, and initialize `[0xFF000,0xFF23C)`. The owner's untouched-USRTRIM requirement therefore is NOT proven by preboot byte preservation. Require exact startup source proof and negative tests showing the selected payload never writes protected regions, including first boot and error paths. Do not silently authorize calibration restoration, change efuse/key_mac, or treat the reserved guard as a transaction journal. Hardware activation is HOLD until this conflict is resolved within owner scope.

2. **Detect present/absent/unknown, read-only, same unit.** Future target-qualified detection must bind physical unit identity, exact Pro/Elite model, protocol/version, fresh challenge and session/epoch. Validate exact installed head/container identity and guard ownership. A port/product name, matching key, `ED_BOOT_*` startup-picture commands, or supplied UUID is not bootloader detection. Present-and-qualified skips adoption and uses exact current 16 KiB head-preserving ordinary updates; proven absent enters backup/adoption; unknown/inconsistent refuses adoption. First-adoption `prepare.py` rejects an occupied guard and is not an already-adopted preparation path. Live detection is currently MISSING; do not supply a fabricated probe command.

3. **Full private backup before any later write.** Require exactly 1,048,576 raw flash bytes under actual global writer exclusion with complete ordered coverage, stable identity/session/model/epoch, independent target full/head SHA-256, fresh revalidation and verified durable local reread. Retain an owner-only backup and completion receipt; preserve projects, samples, controller, calibration and configuration. Existing slot backup is not full flash backup, and efuse must remain untouched rather than pretending flash backup includes it. Reuse capture core and POSIX private retention logic; live transport/target binding and writer barrier remain missing. Linux synthetic retention proof does not establish Mac compatibility. Backups/raw identities/private unit hashes remain private.

4. **Authentic model-qualified payload and preparation.** Existing upstream source pin is `jvitkauskas/m-bootloader@64b74d009e6d21fe85ca60754b7c01d438214804`, tree `105c8347f2b8a1f1f4e3993bec31735fde1e5828`; preserve LICENSE/NOTICE. Existing common SMK container requires separate exact Pro/Elite snapshot/package/model qualification: do not invent separate model image filenames. Verify trusted source/build provenance and compiler hashes, exact container SHA-256, header/file CRCs, identity marker, bounded slot and startup/RAM checks. SHA-256 proves consistency, not provenance or a signature; CRC is not authenticity. Frozen pins contain exactly model, snapshot_sha256, package_sha256, container_sha256, controller_sha256. Require complete matching controller, retained configuration at record64, same decoded key prefix, exact app geometry/banking, unchanged app/controller/OTA. Current transform allows only `[0x20,0x40)` and `[0xA0,0x38D0)` changes. Prepared outputs explicitly remain NOT_INSTALLABLE until downstream gates pass.

5. **Power/interruption and recovery design before a writer.** Stable power and a retained backup are prerequisites, not proof that a single-slot boot erase is recoverable. Require target-qualified atomic/recoverable commit primitive, persistent same-unit transaction status, bounded write ranges and exact readback at every interruption boundary. Current source explicitly lacks this primitive. Do not assume A/B boot slots, free journal sectors, ROM recovery, header-last safety or ordinary app-loader resume. After any possible write, lost ACK/cancel/disconnect/power loss is OUTCOME_UNKNOWN; never auto-retry. Reconnect requires fresh same-unit identity/status/readback: proven not-started requires fresh backup/revalidation; committed requires exact expected head and preservation proof; partial/foreign/unknown stops without automatic recovery. No supported recovery command can presently be supplied. Preserve backup and diagnostic state, leave transaction pending for a reviewed supported route. No fm1_rescue.py, no force option, no hardware probing/flashing now. There is no brick-prevention guarantee.

6. **Default integration and Mac acceptance later.** Existing installer/website owners implement explicit adoption intent, detection outcomes, private backup, progress, cancel and unknown-outcome UI only after reviewed protocol feasibility. Preserve ordinary OTA guards and independent app-only flow; the current optional head-reference guard is not adequate adoption admission. Integrate reviewed disjoint work into wip/one-build, not main/nightly. Actual candidate preview precedes install-workflow rollout. Owner Mac later verifies qualified model-specific build/controller/package, backup, startup, physical continuity, power-interruption and protected-region readback. All such checks remain NOT_RUN, and no exact install command is currently source-supported.

## Source-supported commands (not device installation)

Executed once during this research from the inspected local source:

```sh
python3 -B tools/mboot/capture_host.py
```

Exit0; output `{"live_capture": "UNSUPPORTED", "write_capability": false}`. This is PASS for truthful unsupported-capability reporting only, not a backup or device detection result.

Existing documented offline verification, after a legitimately acquired private backup exists:

```sh
python3 -B tools/mboot/capture_host.py --verify-backup /private/owner/backup
```

Safe synthetic cloud checks available in the pinned repository (NOT_RUN in this research; do not repeat already accepted unchanged proof merely for this plan):

```sh
python3 -B tests/host/mboot_capture_core/check.py --output /tmp/mboot-core-proof
python3 -B tests/host/mboot_capture_host/test_host.py
python3 -B tests/host/mboot_nightly_contract/test_contract.py
```

The local partial build tree lacks these test directories; complete API tree confirms they exist remotely. Fetch pinned public source when an actual uncovered proof is assigned. Capture-core command covers normal/ASan/UBSan and SHA differential synthetic cases; host tests cover private retention/refusal; contract tests model refusal and unknown outcomes, not physical power interruption.

Existing report-only offline candidate preparation, conditional on authentic reviewed private inputs and pinned upstream checkout:

```sh
python3 -B tools/mboot/prepare.py --snapshot /private/reviewed-unit.bin \
  --package /private/reviewed-complete.fwsc --container /private/uboot.boot \
  --pins /private/reviewed-pins.json --upstream /path/to/pinned/m-bootloader
```

NOT_RUN. This is not installation. Do not upload unit data, candidate heads/packages, private headers, proprietary toolchains or binaries. Native boot build command exists in tools/mboot/README.md but its alternative compiler qualification does not establish owner Mac/hardware compatibility; reuse exact existing build receipts rather than claiming a new run.

## Precise pending inputs/actions

PENDING source owner: qualified detection, target identity/session/raw-capture wire adapter and complete writer exclusion; existing core alone supplies none of these platform facts. PENDING payload owner/reviewer: resolve first-boot USRTRIM restoration conflict and authentic exact model-qualified payload manifest. PENDING protocol owner: supported interruption-safe boot commit/status/readback and recovery route; absence blocks device-stage installation, not documentation. PENDING owner Mac stage: complete authentic same-unit backup/model binding and physical validation after source gates. Existing source/QA claims must be reconciled by sole admission owner, not reassigned by this contribution. No live probe, readback, flash, install, recovery, Mac, golden, performance or hardware check ran.


## Validation and delivery boundary

`python3 tools/validate_one_build_integration_plan.py` checks exact snapshot coverage, unique rows, full pins, all 15 required IDs, additional facet presence, dependency order, sole target and MBOOT hold. `python3 tests/test_one_build_integration_plan.py` includes missing branch/PR/requirement and invalid-target negative cases. These validate documentation consistency only, not firmware correctness.

Independent document review precedes API delivery. Root records actual commit/branch/tree/files/PR readback and existing consumer acceptance separately. No local file, passed validator or draft PR establishes integration, installed functionality or model-qualified firmware.

## Bounded subsequent observations — 2026-10-30 05:24:35 UTC

The 38-PR/816-branch matrix above is the **historical enumeration snapshot**, retained unchanged for provenance. It is not a fresh current open-PR count. The following bounded observations supersede only the named rows' current state/pins; no broad inventory recheck, source change, new admission or acceptance occurred.

- **PR697 closed, unmerged at 05:21:40 UTC**, head `ff0f35c02163543ced4f79666787f2166ac327b4`, superseded by PR713 according to comment6515642405. Retain historical QA finding6509207960 and its budget; do not reopen, and do not treat closure as correction acceptance.
- **PR712 remains draft at `ba303d95c2ad913a6410eae1fa536a0c1c8336dc`**, based on `094bb1a55c159aefb0e2bfa39790a1b4b74cee40`. Production DSP/UI is unchanged from the historical `24a36371fe7779bdca6b72b1bc2aee9d58502ec4` row: intermediate `77be40162e72ea30f59610ac06b88c42e0d4ab98` added tests/evidence, and the latest delta is documentation only. Producer-reported second-compiler and extracted Free Play handler checks are scoped proof, **not independent QA acceptance**. Whole-checkout host/UI/golden/performance/native/hardware qualification remains NOT_RUN; newer Miami source-selection and combined-input gates remain HELD.
- **PR713 remains draft at `ca59579a772999802b347f65eb4ba42812a2a83c`**, on the same one-build base. Since the historical `97f98da92a3d60f69708f8d597489b0b044def6c` row, the helper now fails closed for `publish_valid_nightly(push=True)` while retaining local preview; the latest source delta adds a CLI negative test. Producer reports 10/10 scoped native cases and17/17 source assertions; the full original publisher regression remains NOT_RUN. Comment6515650883 is an existing-QA **recheck request**, not consumer acceptance. Preserve the existing producer/reviewer and all gates; no publication authority follows.

Evidence is the three immutable scoped observation receipts dated05:20:16,05:22:25 and05:24:35 UTC, identified by filename/hash in `subsequent_observations` in the supporting inventory. Only sanitized technical conclusions are included; no raw comment text, private inputs or owner quotations are reproduced.

### Delivery identity and consumer limitation

This package consists only of the existing plan, supporting inventory, validator and validator tests, with four-file SHA-256 identities in the local validation receipt. The existing PR699 planning-registry owner is a **proposed consumer**, not a verified accepted handoff; the existing sole one-build integrator remains the downstream consumer. No first substantive input-specific consumption has been established. Independent R1 ACCEPT covered the preceding documentation snapshot; this bounded addendum is for the same reviewer's affected-delta consumption, not a new review claim.

Actual API delivery remains root-owned and pending. The archive-blob write and prior inventory-commit controls remain denied pending approval; this documentation update neither retries nor routes around them. No remote branch, commit, PR delivery or source integration is claimed. Keep generated local cache files outside the four-file publication whitelist; no cleanup operation is part of this contribution.

## Owner follow-up — source-backed MUST_FIX and current delivery route

This bounded addendum supersedes the earlier *delivery-pending* statement only for the original feature inventory: root has delivered its66unchanged files to PR716, branch `wip/owner-requested-features`, head `d73eea51e7dbdf63dd1e64c475ecd5e2cbd72b92`, based on `094bb1a55c159aefb0e2bfa39790a1b4b74cee40`. This plan's reviewed documentation delta may continue on that existing root-owned PR; it is not delivered merely by this statement. PR699's separate owner/provenance is preserved, with no accepted handoff or permission to mutate its branch inferred. Root retains unrelated archive-write denials; no alternate-route publication is authorized here. Destination of eventual source composition remains **wip/one-build only**.

The subsequent scoped SMC read identifies one-build at `cb5423c4aad7d9fc0c3e0a532c2aac080f3cfd21`; the earlier094bb matrix remains historical. Refresh exact combined closure before source action rather than applying its old pins blindly.

### MUST_FIX: SMC-OWN-PAGE

No complete independent mixer-page implementation was found across199accessible SMC source copies/11distinct blobs and11,080scoped SMC source/test/patch files. This excludes inaccessible/packed history from completeness claims. Historical0e58 SMC blob `91e75789dc2430b506d811497c319fa45e966cd9` and currentcb542 SMC blob `0867629e4d2cb4b0e85228d312fa609487d797a2` already exist remotely; neither should be uploaded as a claimed repair.

Both exact-source synthetic probes compiled successfully but **failed the owner contract**: keyboard-only page changes redirected the mixer target100→200, and locked arrow blocker returned0. These are extracted-function reproductions, not full dispatch/device tests. Fader-button LED coupling was source inspection only.

The **existing SMC/mixer producer**, not a new writer, must retain independent mixer view/page/part/bank and captured target context; unlocked mixer navigation changes its own context, keyboard navigation must never redirect it, page lock must block mixer navigation and preserve captured targets, and unlock must not import unrelated keyboard state. Fader-button navigation/selection LEDs must use the same context as actions while mute/Solo/REC state remains accurate. Acceptance covers first/last pages, locked/unlocked forward/back, keyboard-only navigation in both states, lock/unlock, drum-bank/FX/master selection and two-peer context isolation. Preserve reconnect, queue ordering, NoteOff/held cleanup and pickup. Do not import whole historical SMC files over newer fixes.

This is a named **MUST_FIX**, not a source fix or acceptance claim. Add it to S04/S06's same-owner acceptance closure; S08 cannot close this request without exact behavior/LED proof and independent review. Native firmware/controller, Mac, golden/performance and hardware checks remain NOT_RUN.

### MBOOT without any USRTRIM write

See [the source-backed install-plan investigation](mbootloader-usrtrim-install-plan.md). No authentic compliant SMK variant/configuration was established in the bounded upstream/current/initial-version/fork check. Existing SMK startup can erase0xC1000–0xC1FFF and restore trim; choosing the FM-1 board to disable this is not a valid workaround. Cloud core and SMK boot tests ran normal+ASan/UBSan with precise private-fixture skips. Passing restoration tests demonstrate the conflict, not compliance. Keep **PLAN_ONLY HOLD** until a qualified existing variant or separately admitted reviewed source correction proves no protected-region writes across startup/error/update/wipe/install paths. No flash, install or recovery command is authorized or performed.

Publication whitelist for this continuation: the existing plan, inventory, validator and tests, plus `docs/mbootloader-usrtrim-install-plan.md`. No upstream source checkout, host executable, cache, private fixture or raw receipt is included. All five files require exact remote readback and existing consumer action before delivery can be claimed.
